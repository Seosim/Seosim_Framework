#include "pch.h"
#include "D3D12App.h"
#include "d3dUtil.h"
#include "Vertex.h"

D3D12App& D3D12App::Instance()
{
	static D3D12App d3d12App = {};
    return d3d12App;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return D3D12App::Instance().MessageProc(hwnd, msg, wParam, lParam);
}

bool D3D12App::InitWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mhAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, mWidth, mHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

bool D3D12App::InitDirect3D()
{
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	ID3D12Debug* debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

		RELEASE_COM(debugController);
	}
#endif

	HR(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mdxgiFactory)));

	HRESULT hardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&md3dDevice));

	// DX 12를 지원하는 하드웨어 디바이스를 생성할 수 없으면 WARP 디바이스를 생성합니다.
	if (FAILED(hardwareResult))
	{
		IDXGIAdapter1* dxgiAdapter;
		HR(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter)));

		HR(D3D12CreateDevice(dxgiAdapter, D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&md3dDevice)));
	}

	//펜스 생성
	HR(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

	//Rtv, Dsv, CbvSrvUav 크기 알아오기
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	HR(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

	CreateCommandObjects();
	CreateSwapChain();
	CreateRtrAndDsvDescriptorHeap();

	OnResize();

	HR(md3dCommandList->Reset(md3dCommandAllocator, nullptr));

	CreateCbvSrvUavDescriptorHeap();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShaderAndInputLayout();
	BuildBox();
	BuildPipelineStateObject();
	BuildObjects();
	BuildLight();
	BuildCamera();

	HR(md3dCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { md3dCommandList };
	md3dCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//Wait initialize Completed.
	FlushCommandQueue();

	return true;
}

void D3D12App::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	HR(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&md3dCommandQueue)));
	HR(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&md3dCommandAllocator)));
	HR(md3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, md3dCommandAllocator, nullptr, IID_PPV_ARGS(&md3dCommandList)));
	
	//커맨드 리스트는 항상 닫힌 상태로 있어야 함 Why? Reset을 호출하기 위해
	HR(md3dCommandList->Close());
}

void D3D12App::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mWidth;
	sd.BufferDesc.Height = mHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.OutputWindow = mhMainWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	HR(mdxgiFactory->CreateSwapChain(md3dCommandQueue, &sd, reinterpret_cast<IDXGISwapChain**>(&mSwapChain)));

	// 전체 화면 전환을 비활성화합니다.
	HR(mdxgiFactory->MakeWindowAssociation(mhMainWnd, DXGI_MWA_NO_ALT_ENTER));
}

void D3D12App::CreateRtrAndDsvDescriptorHeap()
{
	//Create RTV Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	
	HR(md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));

	//Create DSV Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	dsvHeapDesc.NumDescriptors = 1;

	HR(md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)));
}

void D3D12App::CreateCbvSrvUavDescriptorHeap()
{
	//Create CBV Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	cbvHeapDesc.NumDescriptors = 1;

	HR(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

void D3D12App::BuildConstantBuffers()
{
	constexpr int MAX_OBJECT_COUNT = 100000;
	mObjectCB = std::make_unique<UploadBuffer>(md3dDevice, MAX_OBJECT_COUNT, true, sizeof(ObjectConstants));

	UINT objCBByteSize = (sizeof(ObjectConstants) + 255) & ~255;

	//for (int i = 0; i < 3; ++i)
	//{
		//D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
		//int boxCBufIndex = 0;
		//cbAddress += boxCBufIndex * objCBByteSize;

		//D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		//cbvDesc.BufferLocation = cbAddress;
		//cbvDesc.SizeInBytes = (sizeof(ObjectConstants) + 255) & ~255;

		//auto handle = mCbvHeap->GetCPUDescriptorHandleForHeapStart();
		//handle.ptr += mCbvSrvUavDescriptorSize * boxCBufIndex;

		//md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
	//}
}

void D3D12App::BuildRootSignature()
{

	D3D12_DESCRIPTOR_RANGE descriptorRange[1];
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].RegisterSpace = 0;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	constexpr int ROOT_PARAMATER_COUNT = 4;
	D3D12_ROOT_PARAMETER rootParamater[ROOT_PARAMATER_COUNT];

	//Per Object
	rootParamater[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamater[0].Descriptor = {
		.ShaderRegister = 0,
		.RegisterSpace = 0
	};
	rootParamater[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//Per Material
	rootParamater[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamater[1].Descriptor = {
		.ShaderRegister = 1,
		.RegisterSpace = 0
	};
	rootParamater[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//Per Light
	rootParamater[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamater[2].Descriptor = {
		.ShaderRegister = 2,
		.RegisterSpace = 0
	};
	rootParamater[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//Per Camera
	rootParamater[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamater[3].Descriptor = {
		.ShaderRegister = 3,
		.RegisterSpace = 0
	};
	rootParamater[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//rootParamater[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//rootParamater[0].DescriptorTable.NumDescriptorRanges = 1;
	//rootParamater[0].DescriptorTable.pDescriptorRanges = &descriptorRange[0];
	//rootParamater[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.NumParameters = ROOT_PARAMATER_COUNT;
	rootSignatureDesc.pParameters = rootParamater;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;

	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	HR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob));
	HR(md3dDevice->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));

	RELEASE_COM(errorBlob);
	RELEASE_COM(signatureBlob);
}

void D3D12App::BuildShaderAndInputLayout()
{
	HRESULT hr = S_OK;

	mVertexBlob = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_1");
	mPixelBlob = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_1");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void D3D12App::BuildBox()
{
	//std::array<Vertex, 8> vertices =
	//{
	//	Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
	//	Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
	//	Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
	//	Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
	//	Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
	//	Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
	//	Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
	//	Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	//};

	//std::array<std::uint16_t, 36> indices =
	//{
	//	// front face
	//	0, 1, 2,
	//	0, 2, 3,

	//	// back face
	//	4, 6, 5,
	//	4, 7, 6,

	//	// left face
	//	4, 5, 1,
	//	4, 1, 0,

	//	// right face
	//	3, 2, 6,
	//	3, 6, 7,

	//	// top face
	//	1, 5, 6,
	//	1, 6, 2,

	//	// bottom face
	//	4, 0, 3,
	//	4, 3, 7
	//};

	//const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	//const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	//mBoxGeo = std::make_unique<MeshGeometry>();
	//mBoxGeo->Name = "boxGeo";

	//HR(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
	//CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	//HR(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	//CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	//mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice,
	//	md3dCommandList, vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);

	//mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice,
	//	md3dCommandList, indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

	//mBoxGeo->VertexByteStride = sizeof(Vertex);
	//mBoxGeo->VertexBufferByteSize = vbByteSize;
	//mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	//mBoxGeo->IndexBufferByteSize = ibByteSize;

	//SubmeshGeometry submesh;
	//submesh.IndexCount = (UINT)indices.size();
	//submesh.StartIndexLocation = 0;
	//submesh.BaseVertexLocation = 0;

	//mBoxGeo->DrawArgs["box"] = submesh;
}

void D3D12App::BuildLight()
{
	mpLight = new Light();
	mpLight->Initialize(md3dDevice);

	mpLight->SetDirection({ 0.5f, 0.5f, 0.5f });
	mpLight->SetColor({ 1.0f, 1.0f, 1.0f });
}

void D3D12App::BuildCamera()
{
	mpCamera = new Camera();
	mpCamera->Initialize(md3dDevice);

	mpCamera->SetPosition({ 0.5f, 0.5f, 0.5f });
}

void D3D12App::BuildPipelineStateObject()
{
	//D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	//ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	//psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	//psoDesc.pRootSignature = mRootSignature;
	//psoDesc.VS =
	//{
	//	reinterpret_cast<BYTE*>(mVertexBlob->GetBufferPointer()),
	//	mVertexBlob->GetBufferSize()
	//};
	//psoDesc.PS =
	//{
	//	reinterpret_cast<BYTE*>(mPixelBlob->GetBufferPointer()),
	//	mPixelBlob->GetBufferSize()
	//};
	//psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	//psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	//psoDesc.SampleMask = UINT_MAX;
	//psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//psoDesc.NumRenderTargets = 1;
	//psoDesc.RTVFormats[0] = mBackBufferFormat;
	//psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	//psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	//psoDesc.DSVFormat = mDepthStencilFormat;
	//HR(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void D3D12App::LoadHierarchyData(const std::string& filePath)
{
	std::ifstream loader{ filePath, std::ios::binary };

	if (not loader)
	{
		exit(-1);
	}

	LoadGameObjectData(loader);
}

void D3D12App::LoadGameObjectData(std::ifstream& loader, GameObject* parent)
{
	GameObject* gameObject = new GameObject();

	gameObject->AddComponent<CTransform>();
    CTransform& cTransform = gameObject->GetComponent<CTransform>();

	mGameObjects.push_back(gameObject);

	XMFLOAT3 position;
	loader.read(reinterpret_cast<char*>(&position), sizeof(XMFLOAT3));

	XMFLOAT4 rotation;
	loader.read(reinterpret_cast<char*>(&rotation), sizeof(XMFLOAT4));

	XMFLOAT3 scale;
	loader.read(reinterpret_cast<char*>(&scale), sizeof(XMFLOAT3));

	cTransform.SetParent(parent);
	cTransform.SetTransformData(position, rotation, scale);

	bool bHasMesh;
	loader.read(reinterpret_cast<char*>(&bHasMesh), sizeof(bool));

	if (bHasMesh)
	{
		int meshLength = 0;
		char meshName[64] = {};

		loader.read(reinterpret_cast<char*>(&meshLength), sizeof(int));
		loader.read(reinterpret_cast<char*>(meshName), meshLength);
		meshName[meshLength] = '\0';
		std::string meshPath = "Assets/Models/";
		meshPath += meshName;
		meshPath += ".bin";

		gameObject->AddComponent<Mesh>();
		Mesh& mesh = gameObject->GetComponent<Mesh>();

		mesh.LoadMeshData(md3dDevice, md3dCommandList, meshPath);
		//mMesh.LoadMeshData(pDevice, pCommandList, meshPath);
	}

	bool bHasMaterial;
	loader.read(reinterpret_cast<char*>(&bHasMaterial), sizeof(bool));

	if (bHasMaterial)
	{
		int materialLength = 0;
		char materialName[64] = {};

		loader.read(reinterpret_cast<char*>(&materialLength), sizeof(int));
		loader.read(reinterpret_cast<char*>(materialName), materialLength);
		materialName[materialLength] = '\0';
		std::string materialPath = "Assets/Materials/";
		materialPath += materialName;
		materialPath += ".bin";

		gameObject->AddComponent<Material>();
		Material& cMaterial = gameObject->GetComponent<Material>();

		cMaterial.LoadMaterialData(md3dDevice, mRootSignature, mCbvHeap, materialPath);
	} 

	int childCount;
	loader.read(reinterpret_cast<char*>(&childCount), sizeof(int));

	for (int i = 0; i < childCount; ++i)
	{
		LoadGameObjectData(loader, gameObject);
	}
}

void D3D12App::BuildObjects()
{
	LoadHierarchyData("Assets/Hierarchies/MaterialTest.bin");
}

void D3D12App::OnResize()
{
	ASSERT(md3dDevice);
	ASSERT(mSwapChain);
	ASSERT(md3dCommandAllocator);

	FlushCommandQueue();

	HR(md3dCommandList->Reset(md3dCommandAllocator, nullptr));

	for (int i = 0; i < SwapChainBufferCount; ++i)
	{
		if (mSwapChainBuffer[i])
		{
			mSwapChainBuffer[i]->Release();
			mSwapChainBuffer[i] = nullptr;
		}
	}

	if (mDepthStencilBuffer)
	{
		mDepthStencilBuffer->Release();
		mDepthStencilBuffer = nullptr;
	}

	HR(mSwapChain->ResizeBuffers(SwapChainBufferCount, mWidth, mHeight, mBackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i], nullptr, rtvHeapHandle);
		rtvHeapHandle.ptr += mRtvDescriptorSize;
	}

	//Create Depth/Stencil Buffer & View.
	D3D12_RESOURCE_DESC depthStencilDesc = {};
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mWidth;
	depthStencilDesc.Height = mHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear = {};
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	HR(md3dDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(&mDepthStencilBuffer)));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = mDepthStencilFormat;
	auto dsvHandle = DepthStencilView();
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer, &dsvDesc, dsvHandle);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = mDepthStencilBuffer;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	md3dCommandList->ResourceBarrier(1, &barrier);

	HR(md3dCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { md3dCommandList };
	md3dCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.Width = static_cast<float>(mWidth);
	mViewport.Height = static_cast<float>(mHeight);
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mWidth, mHeight };

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void D3D12App::FlushCommandQueue()
{
	mFenceValue++;

	HR(md3dCommandQueue->Signal(mFence, mFenceValue));

	auto val = mFence->GetCompletedValue();

	if (mFence->GetCompletedValue() < mFenceValue)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

		HR(mFence->SetEventOnCompletion(mFenceValue, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

ID3D12Resource* D3D12App::CurrentBackBuffer() const
{
	return mSwapChainBuffer[mCurrBackBuffer];
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12App::CurrentBackBufferView() const
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += mCurrBackBuffer * mRtvDescriptorSize;

	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12App::DepthStencilView() const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3D12App::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);

		std::wstring windowText = mMainWndCaption +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

		SetWindowText(mhMainWnd, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

int D3D12App::Update()
{
	MSG msg = { 0 };

	mTimer.Reset();

	while (msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation/game stuff.
		else
		{
			mTimer.Tick();

			if (!mAppPaused)
			{
				CalculateFrameStats();
				Draw(mTimer);
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

void D3D12App::UpdateBox()
{
	//// Convert Spherical to Cartesian coordinates.
	//float x = mRadius * sinf(mPhi) * cosf(mTheta);
	//float z = mRadius * sinf(mPhi) * sinf(mTheta);
	//float y = mRadius * cosf(mPhi);

	//// Build the view matrix.
	//XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	//XMVECTOR target = XMVectorZero();
	//XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	//XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	//XMStoreFloat4x4(&mView, view);

	//XMMATRIX world = XMLoadFloat4x4(&mWorld);
	//XMMATRIX proj = XMLoadFloat4x4(&mProj);
	//XMMATRIX worldViewProj = world * view * proj;

	//// Update the constant buffer with the latest worldViewProj matrix.
	//ObjectConstants objConstants;
	//XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	//mObjectCB->CopyData(0, objConstants);
}

void D3D12App::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathTool::Clamp(mPhi, 0.1f, MathTool::PI - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathTool::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

float D3D12App::GetAspectRatio() const
{
	return static_cast<float>(mWidth) / mHeight;
}

void D3D12App::Draw(const GameTimer& gameTimer)
{
	HR(md3dCommandAllocator->Reset());
	HR(md3dCommandList->Reset(md3dCommandAllocator, nullptr));

	md3dCommandList->RSSetViewports(1, &mViewport);
	md3dCommandList->RSSetScissorRects(1, &mScissorRect);

	//렌더타겟 상태 변환
	D3D12_RESOURCE_BARRIER barrier0 = {};
	barrier0.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier0.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier0.Transition.pResource = CurrentBackBuffer();
	barrier0.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier0.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier0.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	md3dCommandList->ResourceBarrier(1, &barrier0);

	auto rtvHandle = CurrentBackBufferView();
	auto dsvHandle = DepthStencilView();
	
	md3dCommandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::LightGreen, 0, nullptr);
	md3dCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	md3dCommandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);
	
	//항상 CBV 내용 변경 전 RootSignature Set 필요.
	md3dCommandList->SetGraphicsRootSignature(mRootSignature);

	//Update Constant Camera Buffer, Light Buffer
	mpCamera->Update(md3dCommandList);
	mpLight->Update(md3dCommandList);

	//Draw Object.
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap };
	md3dCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// Convert Spherical to Cartesian coordinates.
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	int index = 0;
	static float testVal = 0.0f;
	testVal += gameTimer.DeltaTime();
	for (GameObject* gameObject : mGameObjects)
	{
		//if(index == 0)
		//gameObject.GetTransform().RotateByWorldAxis(0, gameTimer.DeltaTime() * 33.0f, 0);
		//gameObject->GetTransform().Rotate(0, gameTimer.DeltaTime() * 33.0f, 0);

		CTransform& cTransform = gameObject->GetComponent<CTransform>();
		//cTransform.Rotate(0, 1, 0);
		//cTransform.RotateByWorldAxis(0, 1, 0);

		auto xmf4x4world = cTransform.GetWorldTransform();

		XMFLOAT4X4 test;
		XMStoreFloat4x4(&test, xmf4x4world);

		XMMATRIX world = xmf4x4world;
		XMMATRIX proj = XMLoadFloat4x4(&mProj);
		XMMATRIX worldViewProj = world * view * proj;

		// Update the constant buffer with the latest worldViewProj matrix.
		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
		mObjectCB->CopyData(index, objConstants);
		auto cbv = mCbvHeap->GetGPUDescriptorHandleForHeapStart();
		//md3dCommandList->SetGraphicsRootDescriptorTable(0, cbv);
		md3dCommandList->SetGraphicsRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress() + index++ * ((sizeof(ObjectConstants) + 255) & ~255));

		if (gameObject->HasComponent<Mesh>())
		{
			Mesh& mesh = gameObject->GetComponent<Mesh>();
			gameObject->Render(md3dDevice, md3dCommandList, mCbvHeap);
			
			//struct TestCBuffer {
			//	XMFLOAT4 Color;
			//};

			//TestCBuffer cBuffer =
			//{
			//	.Color = {sinf(testVal), sinf(testVal), sinf(testVal), 1.0f}
			//};

			//mat.UpdateConstantBuffer(cBuffer);
			Material& mat = gameObject->GetComponent<Material>();
			mat.SetConstantBufferView(md3dCommandList);
			mesh.Render(md3dCommandList, mCbvHeap, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		}
	}


	//md3dCommandList->SetPipelineState(mPSO);

	//auto vbv = mBoxGeo->VertexBufferView();
	//auto ibv = mBoxGeo->IndexBufferView();

	//md3dCommandList->IASetVertexBuffers(0, 1, &vbv);
	//md3dCommandList->IASetIndexBuffer(&ibv);
	//md3dCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//md3dCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	//md3dCommandList->DrawIndexedInstanced(mBoxGeo->DrawArgs["box"].IndexCount, 1, 0, 0, 0);

	//Draw가 끝난 후 사용한 렌더타겟을 Present로 상태 변환
	D3D12_RESOURCE_BARRIER barrier1 = {};
	barrier1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier1.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier1.Transition.pResource = CurrentBackBuffer();
	barrier1.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier1.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier1.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	md3dCommandList->ResourceBarrier(1, &barrier1);

	HR(md3dCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { md3dCommandList };
	md3dCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	HR(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
	FlushCommandQueue();
}

void D3D12App::RenderObject()
{
}

void D3D12App::Finalize()
{
	//mBoxGeo->Finalize();

	for (auto gameObject : mGameObjects)
	{
		delete gameObject;
	}

	delete mpCamera;
	delete mpLight;

	for (int i = 0; i < SwapChainBufferCount; ++i)
	{
		RELEASE_COM(mSwapChainBuffer[i]);
	}
	RELEASE_COM(mDepthStencilBuffer);

	RELEASE_COM(mFence);
	RELEASE_COM(md3dCommandQueue);
	RELEASE_COM(md3dCommandAllocator);
	RELEASE_COM(md3dCommandList);
	RELEASE_COM(mRtvHeap); 
	RELEASE_COM(mDsvHeap);
	RELEASE_COM(mCbvHeap);
	RELEASE_COM(mRootSignature);
	RELEASE_COM(mPSO);

	RELEASE_COM(mSwapChain);
	RELEASE_COM(md3dDevice); 

	RELEASE_COM(mdxgiFactory);
}

LRESULT D3D12App::MessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		Finalize();
		PostQuitMessage(0);
		break;
	case WM_SYSCOMMAND:
		if (wParam == SC_MAXIMIZE || wParam == SC_MINIMIZE) {
			return 0;
		}
		break;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_SIZE:
		// Save the new client area dimensions.
		mWidth = LOWORD(lParam);
		mHeight = HIWORD(lParam);
		if (md3dDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				// Restoring from minimized state?
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if (mResizing)
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
			break;
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
	//switch (msg)
	//{
	//	// WM_ACTIVATE is sent when the window is activated or deactivated.  
	//	// We pause the game when the window is deactivated and unpause it 
	//	// when it becomes active.  
	//case WM_ACTIVATE:
	//	if (LOWORD(wParam) == WA_INACTIVE)
	//	{
	//		mAppPaused = true;
	//		mTimer.Stop();
	//	}
	//	else
	//	{
	//		mAppPaused = false;
	//		mTimer.Start();
	//	}
	//	return 0;

	//	// WM_SIZE is sent when the user resizes the window.  
	
	//	}
	//	return 0;

	//	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	//case WM_ENTERSIZEMOVE:
	//	mAppPaused = true;
	//	mResizing = true;
	//	mTimer.Stop();
	//	return 0;

	//	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	//	// Here we reset everything based on the new window dimensions.
	//case WM_EXITSIZEMOVE:
	//	mAppPaused = false;
	//	mResizing = false;
	//	mTimer.Start();
	//	OnResize();
	//	return 0;

	//	// WM_DESTROY is sent when the window is being destroyed.
	//case WM_DESTROY:
	//	PostQuitMessage(0);
	//	return 0;

	//	// The WM_MENUCHAR message is sent when a menu is active and the user presses 
	//	// a key that does not correspond to any mnemonic or accelerator key. 
	//case WM_MENUCHAR:
	//	// Don't beep when we alt-enter.
	//	return MAKELRESULT(0, MNC_CLOSE);

	//	// Catch this message so to prevent the window from becoming too small.
	//case WM_GETMINMAXINFO:
	//	((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
	//	((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
	//	return 0;

	//case WM_LBUTTONDOWN:
	//case WM_MBUTTONDOWN:
	//case WM_RBUTTONDOWN:
	//	OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	//	return 0;
	//case WM_LBUTTONUP:
	//case WM_MBUTTONUP:
	//case WM_RBUTTONUP:
	//	OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	//	return 0;
	//case WM_MOUSEMOVE:
	//	OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	//	return 0;
	//case WM_KEYUP:
	//	if (wParam == VK_ESCAPE)
	//	{
	//		PostQuitMessage(0);
	//	}
	//	else if ((int)wParam == VK_F2)
	//		Set4xMsaaState(!m4xMsaaState);

	//	return 0;
	//}


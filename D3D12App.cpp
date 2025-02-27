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
	BuildSkybox();
	BuildObjects();
	BuildLight();
	BuildCamera();
	BuildResourceTexture();

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
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;//DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = 0;//DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
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
	rtvHeapDesc.NumDescriptors = (UINT)eRenderTargetType::COUNT;
	
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
	////Create CBV Descriptor Heap
	//D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
	//cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//cbvHeapDesc.NodeMask = 0;
	//cbvHeapDesc.NumDescriptors = 1;

	//HR(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));

	constexpr int MAX_SRV_COUNT = 10000;

	//Build SRV Heap.
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = MAX_SRV_COUNT;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HR(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvHeap)));
}

void D3D12App::BuildConstantBuffers()
{
	constexpr int MAX_OBJECT_COUNT = 100000;
	mObjectCB = std::make_unique<UploadBuffer>(md3dDevice, MAX_OBJECT_COUNT, true, sizeof(ObjectConstants));
}

void D3D12App::BuildRootSignature()
{
	D3D12_DESCRIPTOR_RANGE descriptorRange[2];

	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].RegisterSpace = 0;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descriptorRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[1].NumDescriptors = 1;
	descriptorRange[1].BaseShaderRegister = 1;
	descriptorRange[1].RegisterSpace = 0;
	descriptorRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	constexpr int ROOT_PARAMATER_COUNT = 6;
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

	//Texture
	rootParamater[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParamater[4].DescriptorTable.NumDescriptorRanges = 1;
	rootParamater[4].DescriptorTable.pDescriptorRanges = &descriptorRange[0];
	rootParamater[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	
	rootParamater[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParamater[5].DescriptorTable.NumDescriptorRanges = 1;
	rootParamater[5].DescriptorTable.pDescriptorRanges = &descriptorRange[1];
	rootParamater[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_STATIC_SAMPLER_DESC pd3dSamplerDescs[1];

	pd3dSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].MipLODBias = 0;
	pd3dSamplerDescs[0].MaxAnisotropy = 1;
	pd3dSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[0].MinLOD = 0;
	pd3dSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[0].ShaderRegister = 0;
	pd3dSamplerDescs[0].RegisterSpace = 0;
	pd3dSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.NumParameters = ROOT_PARAMATER_COUNT;
	rootSignatureDesc.pParameters = rootParamater;
	rootSignatureDesc.NumStaticSamplers = _countof(pd3dSamplerDescs);
	rootSignatureDesc.pStaticSamplers = pd3dSamplerDescs;

	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	HR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob));
	HR(md3dDevice->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));

	RELEASE_COM(errorBlob);
	RELEASE_COM(signatureBlob);
}

void D3D12App::BuildLight()
{
	mpLight = new Light();
	mpLight->Initialize(md3dDevice);

	mpLight->SetDirection({ -0.32f,  0.77f, 0.56f });
	mpLight->SetColor({ 1.0f, 0.9568627f, 0.8392157 });
}

void D3D12App::BuildCamera()
{
	mpCamera = new Camera();
	mpCamera->Initialize(md3dDevice);

	mpCamera->SetPosition({ 0.5f, 0.5f, 0.5f });
}

void D3D12App::BuildSkybox()
{
	GameObject* skybox = new GameObject();
	skybox->AddComponent<CTransform>();
	skybox->AddComponent<Material>();
	skybox->AddComponent<Mesh>();
	
	Material& material = skybox->GetComponent<Material>();
	Mesh& mesh = skybox->GetComponent<Mesh>();

	material.Initialize(md3dDevice, md3dCommandList, mRootSignature, mSrvHeap, NULL, Shader::eType::Skybox);
	mesh.LoadMeshData(md3dDevice, md3dCommandList, "./Assets/Models/cube.bin");

	mSkybox = skybox;
}

void D3D12App::BuildResourceTexture()
{
	mDepthTexture = new Texture();
	mDepthTexture->CreateSrvWithResource(md3dDevice, mSrvHeap, L"DepthMap", mDepthStencilBuffer, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);

	mNormalTexture = new Texture();
	mNormalTexture->CreateSrvWithResource(md3dDevice, mSrvHeap, L"NormalMap", mRenderTargets[(int)eRenderTargetType::CAMERA_NORMAL], DXGI_FORMAT_R8G8B8A8_UNORM);

	mMSAATexture = new Texture();
	mMSAATexture->CreateSrvWithResource(md3dDevice, mSrvHeap, L"MSAAMap", mRenderTargets[(int)eRenderTargetType::MSAA], DXGI_FORMAT_R8G8B8A8_UNORM);
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

		cMaterial.LoadMaterialData(md3dDevice,md3dCommandList, mRootSignature, mSrvHeap, materialPath);
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

	Shader::Command command = Shader::DefaultCommand();
	command.SampleCount = 1;
	command.DepthEnable = FALSE;
	mScreenShader = new Shader();
	mScreenShader->Initialize(md3dDevice, mRootSignature, "Screen", command);
}

void D3D12App::OnResize()
{
	ASSERT(md3dDevice);
	ASSERT(mSwapChain);
	ASSERT(md3dCommandAllocator);

	FlushCommandQueue();

	HR(md3dCommandList->Reset(md3dCommandAllocator, nullptr));

	for (int i = 0; i < (int)eRenderTargetType::COUNT; ++i)
	{
		if (mRenderTargets[i])
		{
			mRenderTargets[i]->Release();
			mRenderTargets[i] = nullptr;
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
		HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mRenderTargets[i])));
		md3dDevice->CreateRenderTargetView(mRenderTargets[i], nullptr, rtvHeapHandle);
		rtvHeapHandle.ptr += mRtvDescriptorSize;
	}

	// Create the render target view for msaa.
	{
		constexpr SIZE_T MSAA = SIZE_T(eRenderTargetType::MSAA);

		constexpr D3D12_HEAP_PROPERTIES HEAP_PROPERTIES =
		{
			.Type = D3D12_HEAP_TYPE_DEFAULT,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1
		};

		D3D12_RESOURCE_DESC RT_DESC =
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Width = (UINT)mWidth,
			.Height = (UINT)mHeight,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.SampleDesc = {.Count = MSAA_SAMPLING_COUNT, .Quality = 0 },
			.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
		};

		D3D12_CLEAR_VALUE CLEAR_VALUE =
		{
			.Format = RT_DESC.Format,
			.Color = { 0.0f, 0.0f, 0.0f, 0.0f }
		};

		HR(md3dDevice->CreateCommittedResource(&HEAP_PROPERTIES
			, D3D12_HEAP_FLAG_NONE
			, &RT_DESC
			, D3D12_RESOURCE_STATE_RESOLVE_SOURCE
			, &CLEAR_VALUE
			, IID_PPV_ARGS(&mRenderTargets[MSAA])));

		D3D12_RENDER_TARGET_VIEW_DESC RTV_DESC =
		{
			.Format = RT_DESC.Format,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS
		};

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += MSAA * mRtvDescriptorSize;
		md3dDevice->CreateRenderTargetView(mRenderTargets[MSAA], &RTV_DESC, rtvHandle);
		if (mMSAATexture)
			mMSAATexture->ChangeResource(md3dDevice, mSrvHeap, L"MSAAMap", mRenderTargets[(int)eRenderTargetType::MSAA], DXGI_FORMAT_R8G8B8A8_UNORM);
	}

	//Create CameraNormal Map
	{
		constexpr SIZE_T CAMERA_NORMALS = SIZE_T(eRenderTargetType::CAMERA_NORMAL);

		constexpr D3D12_HEAP_PROPERTIES HEAP_PROPERTIES =
		{
			.Type = D3D12_HEAP_TYPE_DEFAULT,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1
		};

		D3D12_RESOURCE_DESC RT_DESC =
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Width = (UINT)mWidth,
			.Height = (UINT)mHeight,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.SampleDesc = {.Count = MSAA_SAMPLING_COUNT, .Quality = 0 },
			.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
		};

		D3D12_CLEAR_VALUE CLEAR_VALUE =
		{
			.Format = RT_DESC.Format,
			.Color = { 0.0f, 0.0f, 0.0f, 0.0f }
		};

		HR(md3dDevice->CreateCommittedResource(&HEAP_PROPERTIES
			, D3D12_HEAP_FLAG_NONE
			, &RT_DESC
			, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			, &CLEAR_VALUE
			, IID_PPV_ARGS(&mRenderTargets[CAMERA_NORMALS])));

		D3D12_RENDER_TARGET_VIEW_DESC RTV_DESC =
		{
			.Format = RT_DESC.Format,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS
		};

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += CAMERA_NORMALS * mRtvDescriptorSize;
		md3dDevice->CreateRenderTargetView(mRenderTargets[CAMERA_NORMALS], &RTV_DESC, rtvHandle);
		if (mNormalTexture)
			mNormalTexture->ChangeResource(md3dDevice, mSrvHeap, L"NormalMap", mRenderTargets[(int)eRenderTargetType::CAMERA_NORMAL], DXGI_FORMAT_R8G8B8A8_UNORM);
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
	depthStencilDesc.SampleDesc.Count = MSAA_SAMPLING_COUNT;
	depthStencilDesc.SampleDesc.Quality = 0;
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
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	dsvDesc.Format = mDepthStencilFormat;
	auto dsvHandle = DepthStencilView();
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer, &dsvDesc, dsvHandle);
	if(mDepthTexture)
		mDepthTexture->ChangeResource(md3dDevice, mSrvHeap, L"DepthMap", mDepthStencilBuffer, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);

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
	return mRenderTargets[mCurrBackBuffer];
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


	//렌더타겟 상태 변환 (MSAA)
	{
		D3D12_RESOURCE_BARRIER barrier0 = {};
		barrier0.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier0.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier0.Transition.pResource = mRenderTargets[(int)eRenderTargetType::MSAA];
		barrier0.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier0.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier0.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		md3dCommandList->ResourceBarrier(1, &barrier0);
	}

	//렌더타겟 상태 변환 (CameraNormal)
	{
		D3D12_RESOURCE_BARRIER barrier0 = {};
		barrier0.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier0.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier0.Transition.pResource = mRenderTargets[(int)eRenderTargetType::CAMERA_NORMAL];
		barrier0.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier0.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier0.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		md3dCommandList->ResourceBarrier(1, &barrier0);
	}

	//SRV To DSV
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = mDepthTexture->GetResource();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_READ;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		md3dCommandList->ResourceBarrier(1, &barrier);
	}

	auto rtvHandle = CurrentBackBufferView();
	auto dsvHandle = DepthStencilView();

	D3D12_CPU_DESCRIPTOR_HANDLE MSAAHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	MSAAHandle.ptr += mRtvDescriptorSize * (int)eRenderTargetType::MSAA;

	D3D12_CPU_DESCRIPTOR_HANDLE cameraNormalHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	cameraNormalHandle.ptr += mRtvDescriptorSize * (int)eRenderTargetType::CAMERA_NORMAL;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvhandles[] = { MSAAHandle, cameraNormalHandle };
	FLOAT clearVal[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	
	md3dCommandList->ClearRenderTargetView(MSAAHandle, clearVal, 0, nullptr);
	md3dCommandList->ClearRenderTargetView(cameraNormalHandle, clearVal, 0, nullptr);
	md3dCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	md3dCommandList->OMSetRenderTargets(2, rtvhandles, false, &dsvHandle);
	
	//항상 CBV 내용 변경 전 RootSignature Set 필요.
	md3dCommandList->SetGraphicsRootSignature(mRootSignature);

	//D3D12_GPU_DESCRIPTOR_HANDLE texHandle = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
	//texHandle.ptr += mCbvSrvUavDescriptorSize;

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap };
	md3dCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	//md3dCommandList->SetGraphicsRootDescriptorTable(4, texHandle);

	//Update Constant Camera Buffer, Light Buffer
	mpCamera->Update(md3dCommandList);
	mpLight->Update(md3dCommandList);

	//Draw Object.

	// Convert Spherical to Cartesian coordinates.
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	// Build the view matrix.
	mpCamera->SetPosition({ x, y, z }); 
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	CTransform& skyboxTransform = mSkybox->GetComponent<CTransform>();

	skyboxTransform.SetPosition({ x, y, z });
	skyboxTransform.SetScale({ 10, 10, 10 });

	//Render Skybox
	{
		auto xmf4x4world = skyboxTransform.GetWorldTransform();
		XMMATRIX world = xmf4x4world;
		XMMATRIX proj = XMLoadFloat4x4(&mProj);
		XMMATRIX worldViewProj = world * view * proj;
		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
		mObjectCB->CopyData(0, objConstants);
		md3dCommandList->SetGraphicsRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress());

		Material& skyboxMat = mSkybox->GetComponent<Material>();
		Mesh& skyboxMesh = mSkybox->GetComponent<Mesh>();

		skyboxMat.SetConstantBufferView(md3dCommandList, mSrvHeap);
		skyboxMesh.Render(md3dCommandList);
	}

	int index = 1;
	for (GameObject* gameObject : mGameObjects)
	{
		CTransform& cTransform = gameObject->GetComponent<CTransform>();

		auto xmf4x4world = cTransform.GetWorldTransform();

		XMMATRIX world = xmf4x4world;
		XMMATRIX proj = XMLoadFloat4x4(&mProj);
		XMMATRIX worldViewProj = world * view * proj;

		mpCamera->SetMatrix(mView, mProj);

		// Update the constant buffer with the latest worldViewProj matrix.
		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
		mObjectCB->CopyData(index, objConstants);
		md3dCommandList->SetGraphicsRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress() + index++ * ((sizeof(ObjectConstants) + 255) & ~255));

		if (gameObject->HasComponent<Mesh>())
		{
			Mesh& mesh = gameObject->GetComponent<Mesh>();
			Material& mat = gameObject->GetComponent<Material>();

			mat.SetConstantBufferView(md3dCommandList, mSrvHeap);

			D3D12_GPU_DESCRIPTOR_HANDLE texHandle = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
			texHandle.ptr += 32 * (mNormalTexture->GetID());
			md3dCommandList->SetGraphicsRootDescriptorTable(5, texHandle);

			mesh.Render(md3dCommandList);
		}
	}

	{
		D3D12_RESOURCE_BARRIER barrier1 = {};
		barrier1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier1.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier1.Transition.pResource = mRenderTargets[(int)eRenderTargetType::CAMERA_NORMAL];
		barrier1.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier1.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier1.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		md3dCommandList->ResourceBarrier(1, &barrier1);
	}

	{
		D3D12_RESOURCE_BARRIER barrier0 = {};
		barrier0.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier0.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier0.Transition.pResource = mRenderTargets[(int)eRenderTargetType::MSAA];
		barrier0.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier0.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier0.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		md3dCommandList->ResourceBarrier(1, &barrier0);
	}


	//Render Screen
	{
		//렌더타겟 상태 변환
		{
			D3D12_RESOURCE_BARRIER barrier0 = {};
			barrier0.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier0.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier0.Transition.pResource = CurrentBackBuffer();
			barrier0.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier0.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier0.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			md3dCommandList->ResourceBarrier(1, &barrier0);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE rtvhandles[] = { rtvHandle };
		md3dCommandList->OMSetRenderTargets(1, rtvhandles, false, nullptr);
		md3dCommandList->ClearRenderTargetView(rtvHandle, clearVal, 0, nullptr);


		{
			D3D12_GPU_DESCRIPTOR_HANDLE texHandle = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
			texHandle.ptr += 32 * (mMSAATexture->GetID());
			md3dCommandList->SetGraphicsRootDescriptorTable(4, texHandle);
		}

		{
			D3D12_GPU_DESCRIPTOR_HANDLE texHandle = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
			texHandle.ptr += 32 * (mNormalTexture->GetID());
			md3dCommandList->SetGraphicsRootDescriptorTable(5, texHandle);
		}

		mScreenShader->SetPipelineState(md3dCommandList);
		md3dCommandList->DrawInstanced(6, 1, 0, 0);
	}


	//DSV To SRV
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = mDepthTexture->GetResource();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_READ;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		md3dCommandList->ResourceBarrier(1, &barrier);
	}

	//Draw가 끝난 후 사용한 렌더타겟을 Present로 상태 변환
	{
		D3D12_RESOURCE_BARRIER barrier1 = {};
		barrier1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier1.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier1.Transition.pResource = CurrentBackBuffer();
		barrier1.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier1.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barrier1.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		md3dCommandList->ResourceBarrier(1, &barrier1);
	}

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
	delete mSkybox;

	for (auto shaderPointer : Shader::ShaderList)
	{
		delete shaderPointer.second;
	}
	Shader::ShaderList.clear();

	delete mScreenShader;

	for (auto texturePointer : Texture::TextureList)
	{
		delete texturePointer.second;
	}
	Texture::TextureList.clear();

	//for (int i = 0; i < (int)eRenderTargetType::COUNT; ++i)
	for (int i = 0; i < SwapChainBufferCount; ++i)
	{
		RELEASE_COM(mRenderTargets[i]);
	}
	//RELEASE_COM(mDepthStencilBuffer);

	RELEASE_COM(mFence);
	RELEASE_COM(md3dCommandQueue);
	RELEASE_COM(md3dCommandAllocator);
	RELEASE_COM(md3dCommandList);
	RELEASE_COM(mRtvHeap); 
	RELEASE_COM(mDsvHeap);
	RELEASE_COM(mCbvHeap);
	RELEASE_COM(mSrvHeap);
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


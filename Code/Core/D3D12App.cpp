#include "pch.h"
#include "D3D12App.h"
#include "d3dUtil.h"
#include "Resource/Vertex.h"

D3D12App& D3D12App::Instance()
{
	static D3D12App d3d12App = {};
    return d3d12App;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Input::Instance().SetHWND(hwnd);
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
	Texture::mCbvSrvUavDescriptorSize = mCbvSrvUavDescriptorSize;


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
	BuildComputeRootSignature();
	BuildUAVTexture();
	BuildShadow();
	BuildSkybox();
	BuildObjects();
	BuildLight();
	BuildCamera();
	BuildResourceTexture();
	BuildComputeShader();


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
	dsvHeapDesc.NumDescriptors = 2;

	HR(md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)));
}

void D3D12App::CreateCbvSrvUavDescriptorHeap()
{
	constexpr int MAX_SRV_COUNT = 1000000;

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
	constexpr int MAX_TEXTURE_COUNT = (int)eDescriptorRange::COUNT;
	D3D12_DESCRIPTOR_RANGE descriptorRange[MAX_TEXTURE_COUNT];

	{
		constexpr int cubemapIndex = (int)eDescriptorRange::CUBE_MAP;

		descriptorRange[cubemapIndex].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[cubemapIndex].NumDescriptors = 1;
		descriptorRange[cubemapIndex].BaseShaderRegister = 0;
		descriptorRange[cubemapIndex].RegisterSpace = 0;
		descriptorRange[cubemapIndex].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}

	{
		constexpr int shadowmapIndex = (int)eDescriptorRange::SHADOW_MAP;

		descriptorRange[shadowmapIndex].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[shadowmapIndex].NumDescriptors = 1;
		descriptorRange[shadowmapIndex].BaseShaderRegister = 1;
		descriptorRange[shadowmapIndex].RegisterSpace = 0;
		descriptorRange[shadowmapIndex].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}

	{
		constexpr int texture2Index = (int)eDescriptorRange::TEXTURE0;

		descriptorRange[texture2Index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[texture2Index].NumDescriptors = 1;
		descriptorRange[texture2Index].BaseShaderRegister = 2;
		descriptorRange[texture2Index].RegisterSpace = 0;
		descriptorRange[texture2Index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}

	{
		constexpr int texture1Index = (int)eDescriptorRange::TEXTURE1;

		descriptorRange[texture1Index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[texture1Index].NumDescriptors = 1;
		descriptorRange[texture1Index].BaseShaderRegister = 3;
		descriptorRange[texture1Index].RegisterSpace = 0;
		descriptorRange[texture1Index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}

	{
		constexpr int texture2Index = (int)eDescriptorRange::TEXTURE2;

		descriptorRange[texture2Index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[texture2Index].NumDescriptors = 1;
		descriptorRange[texture2Index].BaseShaderRegister = 4;
		descriptorRange[texture2Index].RegisterSpace = 0;
		descriptorRange[texture2Index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}

	{
		constexpr int texture2Index = (int)eDescriptorRange::TEXTURE3;

		descriptorRange[texture2Index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[texture2Index].NumDescriptors = 1;
		descriptorRange[texture2Index].BaseShaderRegister = 5;
		descriptorRange[texture2Index].RegisterSpace = 0;
		descriptorRange[texture2Index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}

	constexpr int ROOT_PARAMATER_COUNT = (int)eRootParameter::COUNT;
	D3D12_ROOT_PARAMETER rootParamater[ROOT_PARAMATER_COUNT];

	//Per Object
	constexpr int objectIndex = (int)eRootParameter::OBJECT;
	rootParamater[objectIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamater[objectIndex].Descriptor = {
		.ShaderRegister = 0,
		.RegisterSpace = 0
	};
	rootParamater[objectIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//Per Material
	constexpr int materialIndex = (int)eRootParameter::MATERIAL;
	rootParamater[materialIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamater[materialIndex].Descriptor = {
		.ShaderRegister = 1,
		.RegisterSpace = 0
	};
	rootParamater[materialIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//Per Light
	constexpr int lightIndex = (int)eRootParameter::LIGHT;
	rootParamater[lightIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamater[lightIndex].Descriptor = {
		.ShaderRegister = 2,
		.RegisterSpace = 0
	};
	rootParamater[lightIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//Per Camera
	constexpr int cameraIndex = (int)eRootParameter::CAMERA;
	rootParamater[cameraIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamater[cameraIndex].Descriptor = {
		.ShaderRegister = 3,
		.RegisterSpace = 0
	};
	rootParamater[cameraIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//Per Shadow
	constexpr int shadowIndex = (int)eRootParameter::SHADOW;
	rootParamater[shadowIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamater[shadowIndex].Descriptor = {
		.ShaderRegister = 4,
		.RegisterSpace = 0
	};
	rootParamater[shadowIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//Texture
	{
		constexpr int cubeMapIndex = (int)eRootParameter::CUBE_MAP;
		constexpr int cubeMapRangeIndex = (int)eDescriptorRange::CUBE_MAP;
		rootParamater[cubeMapIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParamater[cubeMapIndex].DescriptorTable.NumDescriptorRanges = 1;
		rootParamater[cubeMapIndex].DescriptorTable.pDescriptorRanges = &descriptorRange[cubeMapRangeIndex];
		rootParamater[cubeMapIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	{
		constexpr int shadowMapIndex = (int)eRootParameter::SHADOW_TEXTURE;
		constexpr int shadowMapRangeIndex = (int)eDescriptorRange::SHADOW_MAP;
		rootParamater[shadowMapIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParamater[shadowMapIndex].DescriptorTable.NumDescriptorRanges = 1;
		rootParamater[shadowMapIndex].DescriptorTable.pDescriptorRanges = &descriptorRange[shadowMapRangeIndex];
		rootParamater[shadowMapIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	{
		constexpr int texture0Index = (int)eRootParameter::TEXTURE0;
		constexpr int texture0RangeIndex = (int)eDescriptorRange::TEXTURE0;
		rootParamater[texture0Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParamater[texture0Index].DescriptorTable.NumDescriptorRanges = 1;
		rootParamater[texture0Index].DescriptorTable.pDescriptorRanges = &descriptorRange[texture0RangeIndex];
		rootParamater[texture0Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	{
		constexpr int texture1Index = (int)eRootParameter::TEXTURE1;
		constexpr int texture1TextureRangeIndex = (int)eDescriptorRange::TEXTURE1;
		rootParamater[texture1Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParamater[texture1Index].DescriptorTable.NumDescriptorRanges = 1;
		rootParamater[texture1Index].DescriptorTable.pDescriptorRanges = &descriptorRange[texture1TextureRangeIndex];
		rootParamater[texture1Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	{
		constexpr int texture2Index = (int)eRootParameter::TEXTURE2;
		constexpr int texture2TextureRangeIndex = (int)eDescriptorRange::TEXTURE2;
		rootParamater[texture2Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParamater[texture2Index].DescriptorTable.NumDescriptorRanges = 1;
		rootParamater[texture2Index].DescriptorTable.pDescriptorRanges = &descriptorRange[texture2TextureRangeIndex];
		rootParamater[texture2Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	{
		constexpr int texture3Index = (int)eRootParameter::TEXTURE3;
		constexpr int texture3TextureRangeIndex = (int)eDescriptorRange::TEXTURE3;
		rootParamater[texture3Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParamater[texture3Index].DescriptorTable.NumDescriptorRanges = 1;
		rootParamater[texture3Index].DescriptorTable.pDescriptorRanges = &descriptorRange[texture3TextureRangeIndex];
		rootParamater[texture3Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	constexpr int SAMPLER_COUNT = 4;
	D3D12_STATIC_SAMPLER_DESC pd3dSamplerDescs[SAMPLER_COUNT];

	pd3dSamplerDescs[0].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
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

	pd3dSamplerDescs[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	pd3dSamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[1].MipLODBias = 0.0f;
	pd3dSamplerDescs[1].MaxAnisotropy = 16;
	pd3dSamplerDescs[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	pd3dSamplerDescs[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	pd3dSamplerDescs[1].MinLOD = 0;
	pd3dSamplerDescs[1].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[1].ShaderRegister = 1;
	pd3dSamplerDescs[1].RegisterSpace = 0;
	pd3dSamplerDescs[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dSamplerDescs[2].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	pd3dSamplerDescs[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[2].MipLODBias = 0.0f;
	pd3dSamplerDescs[2].MaxAnisotropy = 16;
	pd3dSamplerDescs[2].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	pd3dSamplerDescs[2].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	pd3dSamplerDescs[2].MinLOD = 0;
	pd3dSamplerDescs[2].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[2].ShaderRegister = 2;
	pd3dSamplerDescs[2].RegisterSpace = 0;
	pd3dSamplerDescs[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dSamplerDescs[3].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[3].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[3].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[3].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[3].MipLODBias = 0.0f;
	pd3dSamplerDescs[3].MaxAnisotropy = 0;
	pd3dSamplerDescs[3].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	pd3dSamplerDescs[3].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	pd3dSamplerDescs[3].MinLOD = 0;
	pd3dSamplerDescs[3].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[3].ShaderRegister = 3;
	pd3dSamplerDescs[3].RegisterSpace = 0;
	pd3dSamplerDescs[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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

void D3D12App::BuildComputeRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE srvTable0;
	srvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE srvTable1;
	srvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

	CD3DX12_DESCRIPTOR_RANGE srvTable2;
	srvTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);

	CD3DX12_DESCRIPTOR_RANGE srvTable3;
	srvTable3.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);

	CD3DX12_DESCRIPTOR_RANGE uavTable;
	uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);


	constexpr int PARAMETER_COUNT = (int)eComputeRootParamter::COUNT;
	CD3DX12_ROOT_PARAMETER slotRootParameter[PARAMETER_COUNT];

	slotRootParameter[(int)eComputeRootParamter::INPUT0].InitAsDescriptorTable(1, &srvTable0);
	slotRootParameter[(int)eComputeRootParamter::OUTPUT].InitAsDescriptorTable(1, &uavTable);
	slotRootParameter[(int)eComputeRootParamter::INPUT1].InitAsDescriptorTable(1, &srvTable1);
	slotRootParameter[(int)eComputeRootParamter::DOWN_SCALE_SIZE].InitAsConstants(1, 0);
	slotRootParameter[(int)eComputeRootParamter::CAMERA_N_TIMER].InitAsConstantBufferView(3, 0);
	slotRootParameter[(int)eComputeRootParamter::INPUT2].InitAsDescriptorTable(1, &srvTable2);
	slotRootParameter[(int)eComputeRootParamter::INPUT3].InitAsDescriptorTable(1, &srvTable3);


	CD3DX12_STATIC_SAMPLER_DESC linearSampler(
		0, 
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,  // 선형 필터링
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // Clamp 모드
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(PARAMETER_COUNT, slotRootParameter,
		1, &linearSampler,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ID3DBlob* serializedRootSig = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSig, &errorBlob);

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	HR(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mComputeRootSignature)));
}

void D3D12App::BuildLight()
{
	mpLight = new Light();
	mpLight->Initialize(md3dDevice);

	mpLight->SetDirection(mpShadow->GetShadowDirection());
	mpLight->SetColor({ 1.0f, 0.9568627f, 0.8392157f });
}

void D3D12App::BuildShadow()
{
	mpShadow = new Shadow();
	mpShadow->Initialize(md3dDevice, mRootSignature, mSrvHeap, mShadowResource);
}

void D3D12App::BuildCamera()
{
	mpCamera = new Camera();
	mpCamera->Initialize(md3dDevice);

	mpCamera->SetPosition({ 0.5f, 0.5f, 0.5f });
	mpCamera->SetScreenSize(mWidth, mHeight);

	mCamera = new GameObject;
	mCamera->AddComponent<CameraController>();
	mCamera->AddComponent<Transform>();
	mCamera->AddComponent<RigidBody>();
	mCamera->AddComponent<BoxCollider>();
	CameraController& cameraController = mCamera->GetComponent<CameraController>();
	Transform& transform = mCamera->GetComponent<Transform>();
	transform.SetPosition({ -10, 0, 0 });
	RigidBody& rigidBody = mCamera->GetComponent<RigidBody>();
	rigidBody.UseGravity = true;
	rigidBody.SetTransform(&transform);

	cameraController.Initialize(mpCamera, &transform);

	mCamera->AddComponent<PlayerController>();
	PlayerController& controller = mCamera->GetComponent<PlayerController>();
	controller.SetRigidBody(&rigidBody);

	BoxCollider& boxCollider = mCamera->GetComponent<BoxCollider>();
	boxCollider.Initialize(&transform, { 1.0f, 1.0f, 1.0f }, &rigidBody);
}

void D3D12App::BuildSkybox()
{
	GameObject* skybox = new GameObject();
	skybox->AddComponent<Transform>();
	skybox->AddComponent<MeshRenderer>();
	
	mSkyboxMat = new Material;
	Mesh* mesh = new Mesh;

	mSkyboxMat->Initialize(md3dDevice, md3dCommandList, mRootSignature, mSrvHeap, NULL, Shader::eType::Skybox);
	mesh->LoadMeshData(md3dDevice, md3dCommandList, "./Assets/Models/Cube.bin");

	MeshRenderer& meshRenderer = skybox->GetComponent<MeshRenderer>();
	meshRenderer.SetMesh(mesh);
	meshRenderer.AddMaterial(mSkyboxMat);

	mSkybox = skybox;
}

void D3D12App::BuildSSAO()
{
}

void D3D12App::BuildResourceTexture()
{
	mDepthTexture = new Texture();
	mDepthTexture->CreateSrvWithResource(md3dDevice, mSrvHeap, L"DepthMap", mResolveCameraDepth, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, false);

	mPositionTexture = new Texture();
	mPositionTexture->CreateSrvWithResource(md3dDevice, mSrvHeap, L"PositionMap", mResolvePosition, DXGI_FORMAT_R16G16B16A16_FLOAT, false);

	mNormalTexture = new Texture();
	mNormalTexture->CreateSrvWithResource(md3dDevice, mSrvHeap, L"NormalMap", mResolveCameraNormal, DXGI_FORMAT_R16G16B16A16_FLOAT, false);

	mMSAATexture = new Texture();
	mMSAATexture->CreateSrvWithResource(md3dDevice, mSrvHeap, L"MSAAMap", mRenderTargets[(int)eRenderTargetType::MSAA], DXGI_FORMAT_R16G16B16A16_FLOAT);

	mScreenTexture = new Texture();
	mScreenTexture->CreateSrvWithResource(md3dDevice, mSrvHeap, L"ScreenMap", mRenderTargets[(int)eRenderTargetType::SCREEN], DXGI_FORMAT_R16G16B16A16_FLOAT , false);

	mSSAOTexture = new Texture();
	mSSAOTexture->CreateSrvWithResource(md3dDevice, mSrvHeap, L"SSAOTexture", mRenderTargets[(int)eRenderTargetType::SSAO], DXGI_FORMAT_R16_FLOAT, false);

	mNoiseTexture = new Texture();
	mNoiseTexture->LoadTextureFromDDSFile(md3dDevice, md3dCommandList, L"./Assets/Textures/SsaoNoise.DDS", RESOURCE_TEXTURE2D, 0);
	mNoiseTexture->CreateSrv(md3dDevice, mSrvHeap, L"SSAONoise");
}

void D3D12App::BuildComputeShader()
{
	mPostProcessingShader = new ComputeShader();
	mPostProcessingShader->Initialize(md3dDevice, mComputeRootSignature, "PostProcessing");

	mBloomShader = new ComputeShader();
	mBloomShader->Initialize(md3dDevice, mComputeRootSignature, "Bloom");

	mDownSampleShader = new ComputeShader();
	mDownSampleShader->Initialize(md3dDevice, mComputeRootSignature, "DownScale");

	mUpSampleShader = new ComputeShader();
	mUpSampleShader->Initialize(md3dDevice, mComputeRootSignature, "UpScale");

	mVBlurShader = new ComputeShader();
	mVBlurShader->Initialize(md3dDevice, mComputeRootSignature, "VBlur");

	mHBlurShader = new ComputeShader();
	mHBlurShader->Initialize(md3dDevice, mComputeRootSignature, "HBlur");

	mSSAOHBlurShader = new ComputeShader();
	mSSAOHBlurShader->Initialize(md3dDevice, mComputeRootSignature, "HSsaoBlur");

	mSSAOVBlurShader = new ComputeShader();
	mSSAOVBlurShader->Initialize(md3dDevice, mComputeRootSignature, "VSsaoBlur");
}

void D3D12App::BuildUAVTexture()
{
	mPostProcessingTexture = new Texture();
	mPostProcessingTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"PostProcessedTexture", mWidth, mHeight);

	mBloomTexture = new Texture();
	mBloomTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"BloomTexture", mWidth, mHeight);

	mDownScaled4x4BloomTexture = new Texture();
	mDownScaled4x4BloomTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"DownScaled4x4BloomTexture", mWidth / 4, mHeight / 4);

	mDownScaled16x16BloomTexture = new Texture();
	mDownScaled16x16BloomTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"DownScaled16x16BloomTexture", mWidth / 4 / 4, mHeight / 4 / 4);

	mDownScaled64x64BloomTexture = new Texture();
	mDownScaled64x64BloomTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"DownScaled64x64BloomTexture", mWidth / 4 / 4 / 4, mHeight / 4 / 4 / 4);

	mBloomHBlurTexture = new Texture();
	mBloomHBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"BloomHBlurTexture", mWidth, mHeight);

	mBloomVBlurTexture = new Texture();
	mBloomVBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"BloomVBlurTexture", mWidth, mHeight);

	mBloom4x4HBlurTexture = new Texture();
	mBloom4x4HBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"Bloom4x4HBlurTexture", mWidth / 4, mHeight / 4);

	mBloom4x4VBlurTexture = new Texture();
	mBloom4x4VBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"Bloom4x4VBlurTexture", mWidth / 4, mHeight / 4);

	mBloom16x16HBlurTexture = new Texture();
	mBloom16x16HBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"Bloom16x16HBlurTexture", mWidth / 16, mHeight / 16);

	mBloom16x16VBlurTexture = new Texture();
	mBloom16x16VBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"Bloom16x16VBlurTexture", mWidth / 16, mHeight / 16);

	mBloom64x64HBlurTexture = new Texture();
	mBloom64x64HBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"Bloom64x64HBlurTexture", mWidth / 64, mHeight / 64);

	mBloom64x64VBlurTexture = new Texture();
	mBloom64x64VBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"Bloom64x64VBlurTexture", mWidth / 64, mHeight / 64);

	mSSAOHBlurTexture = new Texture();
	mSSAOHBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R16_FLOAT, L"SSAOHBlurTexture", mWidth, mHeight);

	mSSAOVBlurTexture = new Texture();
	mSSAOVBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R16_FLOAT, L"SSAOVBlurTexture", mWidth, mHeight);
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

GameObject* D3D12App::LoadGameObjectData(std::ifstream& loader, GameObject* parent)
{
	char nameLength = 0;
	char name[64] = {};

	loader.read(reinterpret_cast<char*>(&nameLength), sizeof(char));
	loader.read(reinterpret_cast<char*>(name), nameLength);

	GameObject* gameObject = new GameObject(name);

	gameObject->AddComponent<Transform>();
    Transform& transform = gameObject->GetComponent<Transform>();

	mGameObjects.push_back(gameObject);

	XMFLOAT3 position;
	loader.read(reinterpret_cast<char*>(&position), sizeof(XMFLOAT3));

	XMFLOAT4 rotation;
	loader.read(reinterpret_cast<char*>(&rotation), sizeof(XMFLOAT4));

	XMFLOAT3 scale;
	loader.read(reinterpret_cast<char*>(&scale), sizeof(XMFLOAT3));

	transform.SetParent(parent);
	transform.SetTransformData(position, rotation, scale); 

	//TODO: 익스포터에서 RigidBody & BoxCollider 사용유무 체크해야함. 현재는 그냥 다 넣는 중
	{
		gameObject->AddComponent<RigidBody>();
		RigidBody& rigidBody = gameObject->GetComponent<RigidBody>();
		rigidBody.SetTransform(&transform);

		gameObject->AddComponent<BoxCollider>();
		BoxCollider& boxCollider = gameObject->GetComponent<BoxCollider>();
		boxCollider.Initialize(&transform, { 1.0f, 1.0f, 1.0f }, &rigidBody);
	}

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

		Mesh* mesh = nullptr;
		if (Mesh::MeshList.find(meshPath) != Mesh::MeshList.end())
		{
			mesh = Mesh::MeshList[meshPath];
		}
		else
		{
			mesh = new Mesh;
			mesh->LoadMeshData(md3dDevice, md3dCommandList, meshPath);

			//HACK: TerrainMeshCollider Test.
			{
				std::string test = "Environment_1";
				if (name == test)
				{
					gameObject->AddComponent<TerrainMeshCollider>();
					TerrainMeshCollider& terrainMeshCollider = gameObject->GetComponent<TerrainMeshCollider>();

					terrainMeshCollider.SetTriangles(mesh->GetTriangles(), transform.GetWorldTransform());
				}
			}

		}

		gameObject->AddComponent<MeshRenderer>();
		MeshRenderer& meshRenderer = gameObject->GetComponent<MeshRenderer>();
		meshRenderer.SetMesh(mesh);
		meshRenderer.SetTransform(&transform);
	}

	bool bHasMaterial;
	loader.read(reinterpret_cast<char*>(&bHasMaterial), sizeof(bool));

	if (bHasMaterial)
	{
		UINT materialCount;
		loader.read(reinterpret_cast<char*>(&materialCount), sizeof(UINT));

		for (int i = 0; i < materialCount; ++i)
		{
			int materialLength = 0;
			char materialName[64] = {};

			loader.read(reinterpret_cast<char*>(&materialLength), sizeof(int));
			loader.read(reinterpret_cast<char*>(materialName), materialLength);
			materialName[materialLength] = '\0';
			std::string materialPath = "Assets/Materials/";
			materialPath += materialName;
			materialPath += ".bin";

			Material* material = nullptr;
			if (Material::MaterialList.find(materialPath) != Material::MaterialList.end())
			{
				material = Material::MaterialList[materialPath];
			}
			else
			{
				material = new Material;
				material->LoadMaterialData(md3dDevice, md3dCommandList, mRootSignature, mSrvHeap, materialPath);
			}

			MeshRenderer& meshRenderer = gameObject->GetComponent<MeshRenderer>();
			meshRenderer.AddMaterial(material);
		}

	} 

	int childCount;
	loader.read(reinterpret_cast<char*>(&childCount), sizeof(int));


	for (int i = 0; i < childCount; ++i)
	{
		transform.AddChild(LoadGameObjectData(loader, gameObject));
	}
}

void D3D12App::BuildObjects()
{
	LoadHierarchyData("Assets/Hierarchies/0511Test.bin");

	{
		Shader::Command command = Shader::DefaultCommand();
		command.SampleCount = 1;
		command.DepthEnable = FALSE;
		command.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		mScreenShader = new Shader();
		mScreenShader->Initialize(md3dDevice, mRootSignature, "Screen", command);
	}

	{
		Shader::Command command = Shader::DefaultCommand();
		command.SampleCount = 1;
		command.DepthEnable = FALSE;
		command.Format = DXGI_FORMAT_R16_FLOAT;
		mSSAO = new Shader();
		mSSAO->Initialize(md3dDevice, mRootSignature, "SSAO", command);
	}

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

	RELEASE_COM(mShadowResource);
	RELEASE_COM(mResolveCameraDepth);
	RELEASE_COM(mResolveCameraNormal);
	RELEASE_COM(mResolvePosition);

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
			.Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
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
			mMSAATexture->ChangeResource(md3dDevice, mSrvHeap, L"MSAAMap", mRenderTargets[(int)eRenderTargetType::MSAA], DXGI_FORMAT_R16G16B16A16_FLOAT);
	}

	//Create Position Map
	{
		constexpr SIZE_T POSITION = SIZE_T(eRenderTargetType::POSITION);

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
			.Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
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
			, IID_PPV_ARGS(&mRenderTargets[POSITION])));

		D3D12_RENDER_TARGET_VIEW_DESC RTV_DESC =
		{
			.Format = RT_DESC.Format,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS
		};

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += POSITION * mRtvDescriptorSize;
		md3dDevice->CreateRenderTargetView(mRenderTargets[POSITION], &RTV_DESC, rtvHandle);
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
			.Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
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
			, IID_PPV_ARGS(&mRenderTargets[CAMERA_NORMALS])));

		D3D12_RENDER_TARGET_VIEW_DESC RTV_DESC =
		{
			.Format = RT_DESC.Format,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS
		};

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += CAMERA_NORMALS * mRtvDescriptorSize;
		md3dDevice->CreateRenderTargetView(mRenderTargets[CAMERA_NORMALS], &RTV_DESC, rtvHandle);
	}

	//Create SSAO Map
	{
		constexpr SIZE_T SSAO = SIZE_T(eRenderTargetType::SSAO);

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
			.Format = DXGI_FORMAT_R16_FLOAT,
			.SampleDesc = {.Count = 1, .Quality = 0 },
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
			, IID_PPV_ARGS(&mRenderTargets[SSAO])));

		D3D12_RENDER_TARGET_VIEW_DESC RTV_DESC =
		{
			.Format = RT_DESC.Format,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS
		};

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += SSAO * mRtvDescriptorSize;
		md3dDevice->CreateRenderTargetView(mRenderTargets[SSAO], &RTV_DESC, rtvHandle);
		if (mSSAOTexture)
			mSSAOTexture->ChangeResource(md3dDevice, mSrvHeap, L"SSAOTexture", mRenderTargets[(int)eRenderTargetType::SSAO], DXGI_FORMAT_R16_FLOAT, false);
	}

	//Create Screen Map
	{
		constexpr SIZE_T SCREEN = SIZE_T(eRenderTargetType::SCREEN);

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
			.Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
			.SampleDesc = {.Count = 1, .Quality = 0 },
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
			, IID_PPV_ARGS(&mRenderTargets[SCREEN])));

		D3D12_RENDER_TARGET_VIEW_DESC RTV_DESC =
		{
			.Format = RT_DESC.Format,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D
		};

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += SCREEN * mRtvDescriptorSize;
		md3dDevice->CreateRenderTargetView(mRenderTargets[SCREEN], &RTV_DESC, rtvHandle);
		if (mScreenTexture)
			mScreenTexture->ChangeResource(md3dDevice, mSrvHeap, L"ScreenMap", mRenderTargets[(int)eRenderTargetType::SCREEN], DXGI_FORMAT_R16G16B16A16_FLOAT, false);
	}

	//Create Resolve Position
	{
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
			.Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
			.SampleDesc = {.Count = 1, .Quality = 0 },
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
			, IID_PPV_ARGS(&mResolvePosition)));

		if (mPositionTexture)
			mPositionTexture->ChangeResource(md3dDevice, mSrvHeap, L"PositionMap", mResolvePosition, DXGI_FORMAT_R16G16B16A16_FLOAT, false);
	}


	//Create Resolve CameraNormal
	{
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
			.Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
			.SampleDesc = {.Count = 1, .Quality = 0 },
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
			, IID_PPV_ARGS(&mResolveCameraNormal)));

		if(mNormalTexture)
			mNormalTexture->ChangeResource(md3dDevice, mSrvHeap, L"NormalMap", mResolveCameraNormal, DXGI_FORMAT_R16G16B16A16_FLOAT, false);
	}

	//Create ShadowMap
	{
		D3D12_RESOURCE_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Alignment = 0;
		texDesc.Width = SHADOW_MAP_SIZE;
		texDesc.Height = SHADOW_MAP_SIZE;
		texDesc.DepthOrArraySize = 1;
		texDesc.MipLevels = 1;
		texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear;
		optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;

		auto heapType = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		HR(md3dDevice->CreateCommittedResource(&heapType, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &optClear, IID_PPV_ARGS(&mShadowResource)));
		if(mpShadow)
			mpShadow->ChangeResource(md3dDevice, mSrvHeap, mShadowResource);
	}

	//Create Depth/Stencil Buffer & View.
	{
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
		//if (mDepthTexture)
		//	mDepthTexture->ChangeResource(md3dDevice, mSrvHeap, L"DepthMap", mDepthStencilBuffer, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
	}
	
	//Create Resolve CameraDepth
	{
		constexpr D3D12_HEAP_PROPERTIES HEAP_PROPERTIES =
		{
			.Type = D3D12_HEAP_TYPE_DEFAULT,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1
		};

		D3D12_RESOURCE_DESC depthStencilDesc = {};
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = mWidth;
		depthStencilDesc.Height = mHeight;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = mDepthStencilFormat;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear = {};
		optClear.Format = mDepthStencilFormat;
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;

		HR(md3dDevice->CreateCommittedResource(&HEAP_PROPERTIES
			, D3D12_HEAP_FLAG_NONE
			, &depthStencilDesc
			, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			, &optClear
			, IID_PPV_ARGS(&mResolveCameraDepth)));

		if (mDepthTexture)
			mDepthTexture->ChangeResource(md3dDevice, mSrvHeap, L"DepthMap", mResolveCameraDepth, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, false);
	}


	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = mDepthStencilBuffer;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	md3dCommandList->ResourceBarrier(1, &barrier);

	//Shadow Depth
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.Texture2D.MipSlice = 0;

		auto dsvHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
		dsvHandle.ptr += mDsvDescriptorSize;

		md3dDevice->CreateDepthStencilView(mShadowResource, &dsvDesc, dsvHandle);
	}

	OnResizeUAVTexture();

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
	if (mpCamera)
		mpCamera->SetScreenSize(mWidth, mHeight);

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), GetAspectRatio(), 0.3f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void D3D12App::OnResizeUAVTexture()
{
	//PostProcessing Texture
	if (mPostProcessingTexture)
	{
		ID3D12Resource* resource = mPostProcessingTexture->GetResource();
		RELEASE_COM(resource);

		mPostProcessingTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L" ", mWidth, mHeight);
	}
	if (mBloomTexture)
	{
		ID3D12Resource* resource = mBloomTexture->GetResource();
		RELEASE_COM(resource);

		mBloomTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"BloomTexture", mWidth, mHeight);
	}

	if (mDownScaled4x4BloomTexture)
	{
		ID3D12Resource* resource = mDownScaled4x4BloomTexture->GetResource();
		RELEASE_COM(resource);

		mDownScaled4x4BloomTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"DownScaledBloomTexture", mWidth / 4, mHeight / 4);
	}

	if (mDownScaled16x16BloomTexture)
	{
		ID3D12Resource* resource = mDownScaled16x16BloomTexture->GetResource();
		RELEASE_COM(resource);

		mDownScaled16x16BloomTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"DownScaled6x6BloomTexture0", mWidth / 4 / 4, mHeight / 4 / 4);
	}

	if (mDownScaled64x64BloomTexture)
	{
		ID3D12Resource* resource = mDownScaled64x64BloomTexture->GetResource();
		RELEASE_COM(resource);

		mDownScaled64x64BloomTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"DownScaled6x6BloomTexture1", mWidth / 4 / 4 / 4, mHeight / 4 / 4 / 4);
	}

	if (mBloomHBlurTexture)
	{
		ID3D12Resource* resource = mBloomHBlurTexture->GetResource();
		RELEASE_COM(resource);

		mBloomHBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"BloomHBlurTexture", mWidth, mHeight);
	}

	if (mBloomVBlurTexture)
	{
		ID3D12Resource* resource = mBloomVBlurTexture->GetResource();
		RELEASE_COM(resource);

		mBloomVBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"BloomVBlurTexture", mWidth, mHeight);
	}

	if (mBloom4x4HBlurTexture)
	{
		ID3D12Resource* resource = mBloom4x4HBlurTexture->GetResource();
		RELEASE_COM(resource);

		mBloom4x4HBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"Bloom4x4HBlurTexture", mWidth / 4, mHeight / 4);
	}

	if (mBloom4x4VBlurTexture)
	{
		ID3D12Resource* resource = mBloom4x4VBlurTexture->GetResource();
		RELEASE_COM(resource);

		mBloom4x4VBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"Bloom4x4VBlurTexture", mWidth / 4, mHeight / 4);
	}

	if (mBloom16x16HBlurTexture)
	{
		ID3D12Resource* resource = mBloom16x16HBlurTexture->GetResource();
		RELEASE_COM(resource);

		mBloom16x16HBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"Bloom16x16HBlurTexture", mWidth / 16, mHeight / 16);
	}

	if (mBloom16x16VBlurTexture)
	{
		ID3D12Resource* resource = mBloom16x16VBlurTexture->GetResource();
		RELEASE_COM(resource);

		mBloom16x16VBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"Bloom16x16VBlurTexture", mWidth / 16, mHeight / 16);
	}

	if (mBloom64x64HBlurTexture)
	{
		ID3D12Resource* resource = mBloom64x64HBlurTexture->GetResource();
		RELEASE_COM(resource);

		mBloom64x64HBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"Bloom64x64HBlurTexture", mWidth / 64, mHeight / 64);
	}

	if (mBloom64x64VBlurTexture)
	{
		ID3D12Resource* resource = mBloom64x64VBlurTexture->GetResource();
		RELEASE_COM(resource);

		mBloom64x64VBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R8G8B8A8_UNORM, L"Bloom64x64VBlurTexture", mWidth / 64, mHeight / 64);
	}

	if (mSSAOVBlurTexture)
	{
		ID3D12Resource* resource = mSSAOVBlurTexture->GetResource();
		RELEASE_COM(resource);

		mSSAOVBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R16_FLOAT, L"SSAOVBlurTexture", mWidth, mHeight);
	}


	if (mSSAOHBlurTexture)
	{
		ID3D12Resource* resource = mSSAOHBlurTexture->GetResource();
		RELEASE_COM(resource);

		mSSAOHBlurTexture->InitializeUAV(md3dDevice, mSrvHeap, DXGI_FORMAT_R16_FLOAT, L"SSAOHBlurTexture", mWidth, mHeight);
	}
}

void D3D12App::CollisionCheck()
{
	BoxCollider& playerBoxCollider = mCamera->GetComponent<BoxCollider>();

	for (GameObject* gameobject : mGameObjects)
	{
		if (gameobject->HasComponent<BoxCollider>())
		{
			BoxCollider& collider = gameobject->GetComponent<BoxCollider>();

			if (playerBoxCollider.CollisionCheck(collider))
			{

			}
		}
	}

	//for (int i = 0; i < mGameObjects.size() - 1; ++i)
	//{
	//	for (int j = i + 1; j < mGameObjects.size(); ++j)
	//	{
	//		if (mGameObjects[i]->HasComponent<BoxCollider>() && mGameObjects[j]->HasComponent<BoxCollider>())
	//		{
	//			BoxCollider& colliderA = mGameObjects[i]->GetComponent<BoxCollider>();
	//			BoxCollider& colliderB = mGameObjects[j]->GetComponent<BoxCollider>();

	//			if (colliderA.CollisionCheck(colliderB))
	//			{

	//			}
	//		}
	//	}
	//}
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

		std::wstring meshStr = std::to_wstring(mMeshObject);
		std::wstring cullingStr = std::to_wstring(mCullingObject);

		std::wstring windowText = mMainWndCaption +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr +
			L"   Object Count: " + meshStr +
			L"    /   Culling Object Count: " + cullingStr;

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
				CollisionCheck();
				UpdateComponents();
				UpdateTerrainDistance();
				Draw(mTimer);
				Input::Instance().SaveKeyState();
				Input::Instance().UpdateMousePosition();
			}
			else
			{
				Sleep(100);
			}
		}

	}

	return (int)msg.wParam;
}

void D3D12App::UpdateComponents()
{
	ComponentManager::Instance().UpdateComponent(mTimer.DeltaTime());
}

float D3D12App::UpdateTerrainDistance()
{
	for (GameObject* gameObject : mGameObjects)
	{
		if (!gameObject->HasComponent<TerrainMeshCollider>())
			continue;

		Transform& transform = mCamera->GetComponent<Transform>();

		XMFLOAT3 position = transform.GetPosition();
		constexpr float UP = 30.0f;
		position.y += UP;
		XMVECTOR pivotPosition = XMLoadFloat3(&position);
		XMVECTOR UP_VECTOR = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		TerrainMeshCollider& terrainMeshCollider = gameObject->GetComponent<TerrainMeshCollider>();

		auto node = terrainMeshCollider.FindNode(position);
		if (nullptr == node) continue;

		float minDistance = std::numeric_limits<float>::max();
		float distance = 0.0f;

		for (const Triangle& triangle : node->Triangles)
		{
			XMVECTOR v0 = XMLoadFloat3(&triangle.v0);
			XMVECTOR v1 = XMLoadFloat3(&triangle.v1);
			XMVECTOR v2 = XMLoadFloat3(&triangle.v2);

			TriangleTests::Intersects(pivotPosition, -UP_VECTOR, v0, v1, v2, distance);

			if (distance != 0.0f && minDistance >= distance)
			{
				minDistance = distance;
			}
		}

		constexpr float PIVOT = 0.5f;
		if (transform.GetPosition().y < position.y - minDistance + PIVOT)
		{
			PlayerController& playerController = mCamera->GetComponent<PlayerController>();
			playerController.mbJumping = false;
			RigidBody& rigidBody = mCamera->GetComponent<RigidBody>();
			rigidBody.mGravityAcceleration = { 0,0,0 };
			transform.SetPosition({ position.x, position.y - minDistance + PIVOT, position.z });
		}
		return minDistance;
	}
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
		mRadius = MathTool::Clamp(mRadius, 3.0f, 100.0f);
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

	//렌더타겟 상태 변환 (MSAA)
	{
		D3D12_RESOURCE_BARRIER barrier0 = {};
		barrier0.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier0.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier0.Transition.pResource = mRenderTargets[(int)eRenderTargetType::MSAA];
		barrier0.Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
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
		barrier0.Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
		barrier0.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier0.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		md3dCommandList->ResourceBarrier(1, &barrier0);
	}

	//렌더타겟 상태 변환 (Position)
	{
		D3D12_RESOURCE_BARRIER barrier0 = {};
		barrier0.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier0.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier0.Transition.pResource = mRenderTargets[(int)eRenderTargetType::POSITION];
		barrier0.Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
		barrier0.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier0.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		md3dCommandList->ResourceBarrier(1, &barrier0);
	}

	//렌더타겟 상태 변환 (CameraDepth)
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = mDepthStencilBuffer;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		md3dCommandList->ResourceBarrier(1, &barrier);
	}

	auto rtvHandle = CurrentBackBufferView();
	auto dsvHandle = DepthStencilView();

	D3D12_CPU_DESCRIPTOR_HANDLE MSAAHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	MSAAHandle.ptr += mRtvDescriptorSize * (int)eRenderTargetType::MSAA;

	D3D12_CPU_DESCRIPTOR_HANDLE positionHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	positionHandle.ptr += mRtvDescriptorSize * (int)eRenderTargetType::POSITION;

	D3D12_CPU_DESCRIPTOR_HANDLE cameraNormalHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	cameraNormalHandle.ptr += mRtvDescriptorSize * (int)eRenderTargetType::CAMERA_NORMAL;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvhandles[] = { MSAAHandle, positionHandle, cameraNormalHandle };
	FLOAT clearVal[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	//항상 CBV 내용 변경 전 RootSignature Set 필요.
	md3dCommandList->SetGraphicsRootSignature(mRootSignature);
	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap };
	md3dCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mpShadow->SetViewPortAndScissorRect(md3dCommandList);

	RenderObjectForShadow();
	
	md3dCommandList->ClearRenderTargetView(MSAAHandle, clearVal, 0, nullptr);
	md3dCommandList->ClearRenderTargetView(positionHandle, clearVal, 0, nullptr);
	md3dCommandList->ClearRenderTargetView(cameraNormalHandle, clearVal, 0, nullptr);
	md3dCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	md3dCommandList->OMSetRenderTargets(3, rtvhandles, false, &dsvHandle);
	
	md3dCommandList->RSSetViewports(1, &mViewport);
	md3dCommandList->RSSetScissorRects(1, &mScissorRect);

	//Update Constant Camera Buffer, Light Buffer
	mpCamera->Update(md3dCommandList, mTimer.DeltaTime());
	mpLight->Update(md3dCommandList);

	//Draw Object.
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	// Build the view matrix.
	//mpCamera->SetPosition({ x, y, z }); 
	XMMATRIX view = mpCamera->GetViewMatrix();
	XMStoreFloat4x4(&mView, view);

	Transform& skyboxTransform = mSkybox->GetComponent<Transform>();

	skyboxTransform.SetPosition(mpCamera->GetPosition());
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

		MeshRenderer& meshRenderer = mSkybox->GetComponent<MeshRenderer>();

		Material* skyboxMat = meshRenderer.GetMaterial();
		Mesh* skyboxMesh = meshRenderer.GetMesh();

		skyboxMat->SetConstantBufferView(md3dCommandList, mSrvHeap);
		skyboxMat->UpdateTextureOnSrv(md3dCommandList, mSrvHeap, (UINT)eRootParameter::CUBE_MAP, 0);
		skyboxMesh->Render(md3dCommandList);
	}

	RenderObject(gameTimer.DeltaTime());

	{
		D3D12_RESOURCE_BARRIER barrier1 = {};
		barrier1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier1.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier1.Transition.pResource = mRenderTargets[(int)eRenderTargetType::CAMERA_NORMAL];
		barrier1.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier1.Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
		barrier1.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		md3dCommandList->ResourceBarrier(1, &barrier1);
	}

	{
		D3D12_RESOURCE_BARRIER barrier1 = {};
		barrier1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier1.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier1.Transition.pResource = mRenderTargets[(int)eRenderTargetType::POSITION];
		barrier1.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier1.Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
		barrier1.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		md3dCommandList->ResourceBarrier(1, &barrier1);
	}

	{
		D3D12_RESOURCE_BARRIER barrier0 = {};
		barrier0.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier0.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier0.Transition.pResource = mRenderTargets[(int)eRenderTargetType::MSAA];
		barrier0.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier0.Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
		barrier0.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		md3dCommandList->ResourceBarrier(1, &barrier0);
	}

	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = mDepthStencilBuffer;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		md3dCommandList->ResourceBarrier(1, &barrier);
	}

	
	//후처리 전 기존 MSAA렌더타겟을 SCREEN으로 옮깁니다.
	ID3D12Resource* normalResource = mNormalTexture->GetResource();
	ID3D12Resource* depthResource = mDepthTexture->GetResource();
	ID3D12Resource* positionResource = mPositionTexture->GetResource();

	md3dCommandList->ResolveSubresource(mRenderTargets[(int)eRenderTargetType::SCREEN], 0, mRenderTargets[(int)eRenderTargetType::MSAA], 0, mRenderTargets[(int)eRenderTargetType::SCREEN]->GetDesc().Format);
	md3dCommandList->ResolveSubresource(normalResource, 0, mRenderTargets[(int)eRenderTargetType::CAMERA_NORMAL], 0, normalResource->GetDesc().Format);
	md3dCommandList->ResolveSubresource(positionResource, 0, mRenderTargets[(int)eRenderTargetType::POSITION], 0, positionResource->GetDesc().Format);
	md3dCommandList->ResolveSubresource(depthResource, 0, mDepthStencilBuffer, 0, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);

	//For Bloom Effect
	{
		Bloom();
	}

	//For SSAO
	{
		SSAO();
	}

	//Render Screen
	{
		PostProcessing();

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
			texHandle.ptr += mCbvSrvUavDescriptorSize * (mPostProcessingTexture->GetID());
			md3dCommandList->SetGraphicsRootDescriptorTable((int)eRootParameter::TEXTURE0, texHandle);
		}

		//{
		//	D3D12_GPU_DESCRIPTOR_HANDLE texHandle = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		//	texHandle.ptr += 32 * (mScreenTexture->GetID());
		//	md3dCommandList->SetGraphicsRootDescriptorTable(5, texHandle);
		//}

		mScreenShader->SetPipelineState(md3dCommandList);
		md3dCommandList->DrawInstanced(6, 1, 0, 0);
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

void D3D12App::UpdateShadowTransform()
{
	if (mpShadow)
		mpShadow->UpdateShadowTransform(md3dCommandList);
}

void D3D12App::RenderObject(const float deltaTime)
{
	mMeshObject = 0;
	mCullingObject = 0;

	//Set ShadowTexture
	mpShadow->SetGraphicsRootDescriptorTable(md3dCommandList, mSrvHeap);

	//Draw Object.
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	XMMATRIX view = mpCamera->GetViewMatrix();
	XMStoreFloat4x4(&mView, view);
	mpCamera->SetMatrix(mView, mProj);

	auto boundingFrustum = mpCamera->GetBoundingFrustum();

	int index = 1;
	for (GameObject* gameObject : mGameObjects)
	{
		Transform& cTransform = gameObject->GetComponent<Transform>();

		auto xmf4x4world = cTransform.GetWorldTransform();

		XMMATRIX world = xmf4x4world;
		XMMATRIX proj = XMLoadFloat4x4(&mProj);
		XMMATRIX worldViewProj = world * view * proj;

		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
		mObjectCB->CopyData(index, objConstants);
		md3dCommandList->SetGraphicsRootConstantBufferView((int)eRootParameter::OBJECT, mObjectCB->Resource()->GetGPUVirtualAddress() + index++ * ((sizeof(ObjectConstants) + 255) & ~255));

		if (gameObject->HasComponent<MeshRenderer>())
		{
			mMeshObject++;
			MeshRenderer& meshRenderer = gameObject->GetComponent<MeshRenderer>();

			if (false == meshRenderer.IsCulled(boundingFrustum))
				meshRenderer.Render(md3dCommandList, mSrvHeap);
			else
				mCullingObject++;
		}
	}
}

void D3D12App::RenderObjectForShadow()
{
	auto shadowResource = mpShadow->GetResource();

	auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(shadowResource, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	md3dCommandList->ResourceBarrier(1, &barrier0);

	auto dsvHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	dsvHandle.ptr += mDsvDescriptorSize;

	md3dCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	md3dCommandList->OMSetRenderTargets(0, nullptr, false, &dsvHandle);

	UpdateShadowTransform();
	{
		//Draw Object.
		// Convert Spherical to Cartesian coordinates.
		float x = mRadius * sinf(mPhi) * cosf(mTheta);
		float z = mRadius * sinf(mPhi) * sinf(mTheta);
		float y = mRadius * cosf(mPhi);

		// Build the view matrix.
		//mpCamera->SetPosition({ x, y, z });
		XMMATRIX view = mpCamera->GetViewMatrix();
		XMStoreFloat4x4(&mView, view);

		int index = 1;
		for (GameObject* gameObject : mGameObjects)
		{
			Transform& cTransform = gameObject->GetComponent<Transform>();

			auto xmf4x4world = cTransform.GetWorldTransform();

			XMMATRIX world = xmf4x4world;
			XMMATRIX proj = XMLoadFloat4x4(&mProj);
			XMMATRIX worldViewProj = world * view * proj;

			mpCamera->SetProjMatrix(mProj);
			//mpCamera->SetMatrix(mView, mProj);

			// Update the constant buffer with the latest worldViewProj matrix.
			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
			mObjectCB->CopyData(index, objConstants);
			md3dCommandList->SetGraphicsRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress() + index++ * ((sizeof(ObjectConstants) + 255) & ~255));

			if (gameObject->HasComponent<MeshRenderer>())
			{
				MeshRenderer& meshRenderer = gameObject->GetComponent<MeshRenderer>();

				Material* material = meshRenderer.GetMaterial();
				Mesh* mesh = meshRenderer.GetMesh();
				material->UpdateTextureOnSrv(md3dCommandList, mSrvHeap);
				mesh->Render(md3dCommandList);
			}
		}
	}

	auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(shadowResource, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
	md3dCommandList->ResourceBarrier(1, &barrier1);
}

void D3D12App::PostProcessing()
{
	md3dCommandList->SetComputeRootSignature(mComputeRootSignature);
	mPostProcessingShader->SetPipelineState(md3dCommandList);

	auto handle0 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
	handle0.ptr += mScreenTexture->GetID() * mCbvSrvUavDescriptorSize;

	auto handle1 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
	handle1.ptr += (mPostProcessingTexture->GetID() + 1) * mCbvSrvUavDescriptorSize;

	auto handle2 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
	handle2.ptr += mBloomTexture->GetID() * mCbvSrvUavDescriptorSize;

	auto handle3 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
	handle3.ptr += mSSAOHBlurTexture->GetID() * mCbvSrvUavDescriptorSize;

	md3dCommandList->SetComputeRootDescriptorTable((UINT)eComputeRootParamter::INPUT0, handle0);
	md3dCommandList->SetComputeRootDescriptorTable((UINT)eComputeRootParamter::OUTPUT, handle1);
	md3dCommandList->SetComputeRootDescriptorTable((UINT)eComputeRootParamter::INPUT1, handle2);
	md3dCommandList->SetComputeRootDescriptorTable((UINT)eComputeRootParamter::INPUT2, handle3);

	auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(mPostProcessingTexture->GetResource(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	md3dCommandList->ResourceBarrier(1, &barrier0);

	UINT numGroupsX = (UINT)ceilf(mWidth / 32.0f);
	UINT numGroupsY = (UINT)ceilf(mHeight / 32.0f);
	md3dCommandList->Dispatch(numGroupsX, numGroupsY, 1);

	auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(mPostProcessingTexture->GetResource(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);

	md3dCommandList->ResourceBarrier(1, &barrier1);
}

void D3D12App::Bloom()
{
	//md3dCommandList->ResolveSubresource(mRenderTargets[(int)eRenderTargetType::SCREEN], 0, mRenderTargets[(int)eRenderTargetType::MSAA], 0, mRenderTargets[(int)eRenderTargetType::SCREEN]->GetDesc().Format);
	md3dCommandList->SetComputeRootSignature(mComputeRootSignature);
	mpCamera->UpdateForComputeShader(md3dCommandList);
	//Get Bloom Map
	{
		mBloomShader->SetPipelineState(md3dCommandList);
		{
			auto bloomBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mBloomTexture->GetResource(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			md3dCommandList->ResourceBarrier(1, &bloomBarrier);
		}

		auto handle0 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle0.ptr += mScreenTexture->GetID() * mCbvSrvUavDescriptorSize;

		auto handle1 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle1.ptr += (mBloomTexture->GetID() + 1) * mCbvSrvUavDescriptorSize;

		md3dCommandList->SetComputeRootDescriptorTable(0, handle0);
		md3dCommandList->SetComputeRootDescriptorTable(1, handle1);

		UINT numGroupsX = (UINT)ceilf((float)mWidth / 32.0f);
		UINT numGroupsY = (UINT)ceilf((float)mHeight / 32.0f);
		md3dCommandList->Dispatch(numGroupsX, numGroupsY, 1);

		{
			auto bloomBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mBloomTexture->GetResource(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
			md3dCommandList->ResourceBarrier(1, &bloomBarrier);
		}
	}

	//Down Scaling 4x4
	{
		mDownSampleShader->SetPipelineState(md3dCommandList);

		auto handle0 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle0.ptr += mBloomTexture->GetID() * mCbvSrvUavDescriptorSize;

		auto handle1 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle1.ptr += (mDownScaled4x4BloomTexture->GetID() + 1) * mCbvSrvUavDescriptorSize;

		md3dCommandList->SetComputeRootDescriptorTable(0, handle0);
		md3dCommandList->SetComputeRootDescriptorTable(1, handle1);

		auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(mDownScaled4x4BloomTexture->GetResource(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		md3dCommandList->ResourceBarrier(1, &barrier0);

		UINT numGroupsX = (UINT)ceilf((float)mWidth / 32.0f / 4.0f);
		UINT numGroupsY = (UINT)ceilf((float)mHeight / 32.0f / 4.0f);
		md3dCommandList->Dispatch(numGroupsX, numGroupsY, 1);

		auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(mDownScaled4x4BloomTexture->GetResource(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
		md3dCommandList->ResourceBarrier(1, &barrier1);
	}

	//Down Scaling 4x4
	{
		mDownSampleShader->SetPipelineState(md3dCommandList);

		auto handle0 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle0.ptr += mDownScaled4x4BloomTexture->GetID() * mCbvSrvUavDescriptorSize;

		auto handle1 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle1.ptr += (mDownScaled16x16BloomTexture->GetID() + 1) * mCbvSrvUavDescriptorSize;

		md3dCommandList->SetComputeRootDescriptorTable(0, handle0);
		md3dCommandList->SetComputeRootDescriptorTable(1, handle1);

		auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(mDownScaled16x16BloomTexture->GetResource(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		md3dCommandList->ResourceBarrier(1, &barrier0);

		UINT numGroupsX = (UINT)ceilf((float)mWidth / 32.0f / 4.0f / 4.0f);
		UINT numGroupsY = (UINT)ceilf((float)mHeight / 32.0f / 4.0f / 4.0f);
		md3dCommandList->Dispatch(numGroupsX, numGroupsY, 1);

		auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(mDownScaled16x16BloomTexture->GetResource(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
		md3dCommandList->ResourceBarrier(1, &barrier1);
	}

	//Blurring
	{
		BlurTexture(mDownScaled16x16BloomTexture->GetID(), mBloom16x16VBlurTexture, mBloom16x16HBlurTexture, 16, 2);
	}


	//Up Scaling 4x4
	{
		mUpSampleShader->SetPipelineState(md3dCommandList);

		auto handle0 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle0.ptr += mBloom16x16HBlurTexture->GetID() * mCbvSrvUavDescriptorSize;

		auto handle1 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle1.ptr += (mDownScaled4x4BloomTexture->GetID() + 1) * mCbvSrvUavDescriptorSize;

		md3dCommandList->SetComputeRootDescriptorTable(0, handle0);
		md3dCommandList->SetComputeRootDescriptorTable(1, handle1);

		auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(mDownScaled4x4BloomTexture->GetResource(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		md3dCommandList->ResourceBarrier(1, &barrier0);

		UINT numGroupsX = (UINT)ceilf((float)mWidth / 32.0f / 4.0f);
		UINT numGroupsY = (UINT)ceilf((float)mHeight / 32.0f / 4.0f);
		md3dCommandList->Dispatch(numGroupsX, numGroupsY, 1);

		auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(mDownScaled4x4BloomTexture->GetResource(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
		md3dCommandList->ResourceBarrier(1, &barrier1);
	}

	//Blurring
	{
		BlurTexture(mDownScaled4x4BloomTexture->GetID(), mBloom4x4VBlurTexture, mBloom4x4HBlurTexture, 4, 0);
	}

	//Up Scaling 4x4
	{
		mUpSampleShader->SetPipelineState(md3dCommandList);

		auto handle0 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle0.ptr += mBloom4x4HBlurTexture->GetID() * mCbvSrvUavDescriptorSize;

		auto handle1 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle1.ptr += (mBloomTexture->GetID() + 1) * mCbvSrvUavDescriptorSize;

		md3dCommandList->SetComputeRootDescriptorTable(0, handle0);
		md3dCommandList->SetComputeRootDescriptorTable(1, handle1);

		auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(mBloomTexture->GetResource(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		md3dCommandList->ResourceBarrier(1, &barrier0);

		UINT numGroupsX = (UINT)ceilf((float)mWidth / 32.0f);
		UINT numGroupsY = (UINT)ceilf((float)mHeight / 32.0f);
		md3dCommandList->Dispatch(numGroupsX, numGroupsY, 1);

		auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(mBloomTexture->GetResource(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
		md3dCommandList->ResourceBarrier(1, &barrier1);
	}
}

void D3D12App::BlurTexture(const int originalID, Texture* vBlurTexture, Texture* hBlurTexture, const int sampleSize, const int bluringCount)
{
	//VBlur
	{
		mVBlurShader->SetPipelineState(md3dCommandList);

		auto handle0 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle0.ptr += originalID * mCbvSrvUavDescriptorSize;

		auto handle1 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle1.ptr += (vBlurTexture->GetID() + 1) * mCbvSrvUavDescriptorSize;

		md3dCommandList->SetComputeRootDescriptorTable(0, handle0);
		md3dCommandList->SetComputeRootDescriptorTable(1, handle1);

		//SRV -> UAV
		{
			auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(vBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			md3dCommandList->ResourceBarrier(1, &barrier0);
		}

		UINT numGroupsX = (UINT)ceilf(mWidth / sampleSize);
		UINT numGroupsY = (UINT)ceilf(mHeight / 8.0f / sampleSize);
		md3dCommandList->Dispatch(numGroupsX, numGroupsY, 1);

		//UAV -> SRV
		{
			auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(vBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
			md3dCommandList->ResourceBarrier(1, &barrier0);
		}
	}

	//HBlur
	{
		mHBlurShader->SetPipelineState(md3dCommandList);

		auto handle0 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle0.ptr += vBlurTexture->GetID() * mCbvSrvUavDescriptorSize;

		auto handle1 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle1.ptr += (hBlurTexture->GetID() + 1) * mCbvSrvUavDescriptorSize;

		md3dCommandList->SetComputeRootDescriptorTable(0, handle0);
		md3dCommandList->SetComputeRootDescriptorTable(1, handle1);

		//SRV -> UAV
		{
			auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(hBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			md3dCommandList->ResourceBarrier(1, &barrier0);
		}

		UINT numGroupsX = (UINT)ceilf(mWidth / 8.0f / sampleSize);
		UINT numGroupsY = (UINT)ceilf(mHeight / sampleSize);
		md3dCommandList->Dispatch(numGroupsX, numGroupsY, 1);

		//UAV -> SRV
		{
			auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(hBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
			md3dCommandList->ResourceBarrier(1, &barrier0);
		}
	}


	for (int i = 0; i < bluringCount; ++i)
	{
		//VBlur
		{
			mVBlurShader->SetPipelineState(md3dCommandList);

			auto handle0 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
			handle0.ptr += hBlurTexture->GetID() * mCbvSrvUavDescriptorSize;

			auto handle1 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
			handle1.ptr += (vBlurTexture->GetID() + 1) * mCbvSrvUavDescriptorSize;

			md3dCommandList->SetComputeRootDescriptorTable(0, handle0);
			md3dCommandList->SetComputeRootDescriptorTable(1, handle1);

			auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(vBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			md3dCommandList->ResourceBarrier(1, &barrier0);

			// How many groups do we need to dispatch to cover image, where each
			// group covers 16x16 pixels.
			UINT numGroupsX = (UINT)ceilf(mWidth / sampleSize);
			UINT numGroupsY = (UINT)ceilf(mHeight / 8.0f / sampleSize);
			md3dCommandList->Dispatch(numGroupsX, numGroupsY, 1);

			auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(vBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
			md3dCommandList->ResourceBarrier(1, &barrier1);
		}

		//HBlur
		{
			mHBlurShader->SetPipelineState(md3dCommandList);

			auto handle0 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
			handle0.ptr += vBlurTexture->GetID() * mCbvSrvUavDescriptorSize;

			auto handle1 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
			handle1.ptr += (hBlurTexture->GetID() + 1) * mCbvSrvUavDescriptorSize;

			md3dCommandList->SetComputeRootDescriptorTable(0, handle0);
			md3dCommandList->SetComputeRootDescriptorTable(1, handle1);

			auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(hBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			md3dCommandList->ResourceBarrier(1, &barrier0);

			// How many groups do we need to dispatch to cover image, where each
			// group covers 16x16 pixels.
			UINT numGroupsX = (UINT)ceilf(mWidth / 8.0f / sampleSize);
			UINT numGroupsY = (UINT)ceilf(mHeight / sampleSize);
			md3dCommandList->Dispatch(numGroupsX, numGroupsY, 1);

			auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(hBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
			md3dCommandList->ResourceBarrier(1, &barrier1);
		}
	}
}

void D3D12App::SSAO()
{
	//렌더타겟 상태 변환
	{
		D3D12_RESOURCE_BARRIER barrier0 = {};
		barrier0.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier0.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier0.Transition.pResource = mRenderTargets[(int)eRenderTargetType::SSAO];
		barrier0.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier0.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier0.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		md3dCommandList->ResourceBarrier(1, &barrier0);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE ssaoHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	ssaoHandle.ptr += mRtvDescriptorSize * (int)eRenderTargetType::SSAO;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvhandles[] = { ssaoHandle };
	FLOAT clearVal[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	md3dCommandList->OMSetRenderTargets(1, rtvhandles, false, nullptr);
	md3dCommandList->ClearRenderTargetView(ssaoHandle, clearVal, 0, nullptr);

	mSSAO->SetPipelineState(md3dCommandList);

	// 리소스 바인딩
	auto handle0 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
	handle0.ptr += mDepthTexture->GetID() * mCbvSrvUavDescriptorSize;

	auto handle1 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
	handle1.ptr += mNormalTexture->GetID() * mCbvSrvUavDescriptorSize;

	auto handle2 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
	handle2.ptr += mNoiseTexture->GetID() * mCbvSrvUavDescriptorSize;

	auto handle3 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
	handle3.ptr += mPositionTexture->GetID() * mCbvSrvUavDescriptorSize;

	md3dCommandList->SetGraphicsRootDescriptorTable((UINT)eRootParameter::TEXTURE0, handle0);
	md3dCommandList->SetGraphicsRootDescriptorTable((UINT)eRootParameter::TEXTURE1, handle1);
	md3dCommandList->SetGraphicsRootDescriptorTable((UINT)eRootParameter::TEXTURE2, handle2);
	md3dCommandList->SetGraphicsRootDescriptorTable((UINT)eRootParameter::TEXTURE3, handle3);

	md3dCommandList->DrawInstanced(6, 1, 0, 0);

	//렌더타겟 상태 변환
	{
		D3D12_RESOURCE_BARRIER barrier0 = {};
		barrier0.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier0.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier0.Transition.pResource = mRenderTargets[(int)eRenderTargetType::SSAO];
		barrier0.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier0.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		barrier0.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		md3dCommandList->ResourceBarrier(1, &barrier0);
	}

	//Blur
	{
		md3dCommandList->SetComputeRootSignature(mComputeRootSignature);
		//BlurSSAOTexture(mSSAOTexture->GetID(), mSSAOVBlurTexture, mSSAOHBlurTexture);
		BlurTexture(mSSAOTexture->GetID(), mSSAOVBlurTexture, mSSAOHBlurTexture, 1, 3);
	}

	//렌더타겟 상태 변환
	{
		D3D12_RESOURCE_BARRIER barrier0 = {};
		barrier0.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier0.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier0.Transition.pResource = mRenderTargets[(int)eRenderTargetType::SSAO];
		barrier0.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
		barrier0.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barrier0.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		md3dCommandList->ResourceBarrier(1, &barrier0);
	}
}

void D3D12App::BlurSSAOTexture(const int originalID, Texture* vBlurTexture, Texture* hBlurTexture)
{
	//VBlur
	{
		mSSAOVBlurShader->SetPipelineState(md3dCommandList);

		auto ssaoHandle = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		ssaoHandle.ptr += originalID * mCbvSrvUavDescriptorSize;

		auto outputHandle = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		outputHandle.ptr += (vBlurTexture->GetID() + 1) * mCbvSrvUavDescriptorSize;

		auto normalHandle = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		normalHandle.ptr += mNormalTexture->GetID() * mCbvSrvUavDescriptorSize;

		auto depthHandle = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		depthHandle.ptr += mDepthTexture->GetID() * mCbvSrvUavDescriptorSize;

		md3dCommandList->SetComputeRootDescriptorTable((UINT)eComputeRootParamter::INPUT0, normalHandle);
		md3dCommandList->SetComputeRootDescriptorTable((UINT)eComputeRootParamter::INPUT1, depthHandle);
		md3dCommandList->SetComputeRootDescriptorTable((UINT)eComputeRootParamter::INPUT2, ssaoHandle);
		md3dCommandList->SetComputeRootDescriptorTable((UINT)eComputeRootParamter::OUTPUT, outputHandle);

		//SRV -> UAV
		{
			auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(vBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			md3dCommandList->ResourceBarrier(1, &barrier0);
		}

		UINT numGroupsX = (UINT)ceilf(mWidth);
		UINT numGroupsY = (UINT)ceilf(mHeight / 16.0f);
		md3dCommandList->Dispatch(numGroupsX, numGroupsY, 1);

		//UAV -> SRV
		{
			auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(vBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
			md3dCommandList->ResourceBarrier(1, &barrier0);
		}
	}

	//HBlur
	{
		mSSAOHBlurShader->SetPipelineState(md3dCommandList);

		auto handle0 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle0.ptr += vBlurTexture->GetID() * mCbvSrvUavDescriptorSize;

		auto handle1 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
		handle1.ptr += (hBlurTexture->GetID() + 1) * mCbvSrvUavDescriptorSize;

		md3dCommandList->SetComputeRootDescriptorTable((UINT)eComputeRootParamter::INPUT2, handle0);
		md3dCommandList->SetComputeRootDescriptorTable((UINT)eComputeRootParamter::OUTPUT, handle1);

		//SRV -> UAV
		{
			auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(hBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			md3dCommandList->ResourceBarrier(1, &barrier0);
		}

		UINT numGroupsX = (UINT)ceilf(mWidth / 16.0f);
		UINT numGroupsY = (UINT)ceilf(mHeight);
		md3dCommandList->Dispatch(numGroupsX, numGroupsY, 1);

		//UAV -> SRV
		{
			auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(hBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
			md3dCommandList->ResourceBarrier(1, &barrier0);
		}
	}

	constexpr int BLUR_COUNT = 0;
	for (int i = 0; i < BLUR_COUNT; ++i)
	{
		//VBlur
		{
			mSSAOVBlurShader->SetPipelineState(md3dCommandList);

			auto handle0 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
			handle0.ptr += hBlurTexture->GetID() * mCbvSrvUavDescriptorSize;

			auto handle1 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
			handle1.ptr += (vBlurTexture->GetID() + 1) * mCbvSrvUavDescriptorSize;

			md3dCommandList->SetComputeRootDescriptorTable((UINT)eComputeRootParamter::INPUT2, handle0);
			md3dCommandList->SetComputeRootDescriptorTable((UINT)eComputeRootParamter::OUTPUT, handle1);

			auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(vBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			md3dCommandList->ResourceBarrier(1, &barrier0);

			UINT numGroupsX = (UINT)ceilf(mWidth / 4.0f);
			UINT numGroupsY = (UINT)ceilf(mHeight / 4.0f / 16.0f);
			md3dCommandList->Dispatch(numGroupsX, numGroupsY, 1);

			auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(vBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
			md3dCommandList->ResourceBarrier(1, &barrier1);
		}

		//HBlur
		{
			mSSAOHBlurShader->SetPipelineState(md3dCommandList);

			auto handle0 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
			handle0.ptr += vBlurTexture->GetID() * mCbvSrvUavDescriptorSize;

			auto handle1 = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
			handle1.ptr += (hBlurTexture->GetID() + 1) * mCbvSrvUavDescriptorSize;

			md3dCommandList->SetComputeRootDescriptorTable((UINT)eComputeRootParamter::INPUT2, handle0);
			md3dCommandList->SetComputeRootDescriptorTable((UINT)eComputeRootParamter::OUTPUT, handle1);

			auto barrier0 = CD3DX12_RESOURCE_BARRIER::Transition(hBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			md3dCommandList->ResourceBarrier(1, &barrier0);

			UINT numGroupsX = (UINT)ceilf(mWidth / 4.0f / 16.0f);
			UINT numGroupsY = (UINT)ceilf(mHeight / 4.0f);
			md3dCommandList->Dispatch(numGroupsX, numGroupsY, 1);

			auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(hBlurTexture->GetResource(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
			md3dCommandList->ResourceBarrier(1, &barrier1);
		}
	}
}

void D3D12App::Finalize()
{
	for (auto gameObject : mGameObjects)
	{
		delete gameObject;
	}

	delete mpCamera;
	delete mpLight;
	delete mSkybox;
	delete mSkyboxMat;
	delete mpShadow;

	for (auto shaderPointer : Shader::ShaderList)
	{
		delete shaderPointer.second;
	}
	Shader::ShaderList.clear();

	for (auto materialPointer : Material::MaterialList)
	{
		delete materialPointer.second;
	}
	Material::MaterialList.clear();

	for (auto meshPointer : Mesh::MeshList)
	{
		delete meshPointer.second;
	}
	Mesh::MeshList.clear();

	//Delete Shaders
	{
		delete mScreenShader;
		delete mSSAO;
		delete mPostProcessingShader;
		delete mBloomShader;
		delete mDownSampleShader;
		delete mUpSampleShader;
		delete mVBlurShader;
		delete mHBlurShader;
		delete mSSAOHBlurShader;
		delete mSSAOVBlurShader;
	}

	for (auto& texturePointer : Texture::TextureList)
	{
		if (texturePointer.second)
		{
			delete texturePointer.second;
			texturePointer.second = nullptr;
		}
	}
	Texture::TextureList.clear();

	for (int i = 0; i < SwapChainBufferCount; ++i)
	{
		RELEASE_COM(mRenderTargets[i]);
	}

	RELEASE_COM(mRenderTargets[(UINT)eRenderTargetType::CAMERA_NORMAL]);
	RELEASE_COM(mRenderTargets[(UINT)eRenderTargetType::POSITION]);
	RELEASE_COM(mDepthStencilBuffer);


	RELEASE_COM(mFence);
	RELEASE_COM(md3dCommandQueue);
	RELEASE_COM(md3dCommandAllocator);
	RELEASE_COM(md3dCommandList);
	RELEASE_COM(mRtvHeap); 
	RELEASE_COM(mDsvHeap);
	RELEASE_COM(mCbvHeap);
	RELEASE_COM(mSrvHeap);
	RELEASE_COM(mRootSignature);
	RELEASE_COM(mComputeRootSignature);
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
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			PostMessage(hwnd, WM_DESTROY, 0, 0);
		}
		Input::Instance().KeyDown((int)wParam);
		break;
	case WM_KEYUP:
		Input::Instance().KeyUp((int)wParam);
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


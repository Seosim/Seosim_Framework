#pragma once
#include "GameTimer.h";
#include "UploadBuffer.h"
#include "d3dUtil.h"
#include "GameObject.h"
#include "Camera.h"
#include "Light.h"
#include "Texture.h"

class D3D12App final
{
public:
	struct ObjectConstants
	{
		XMFLOAT4X4 World = {};
		XMFLOAT4X4 WorldViewProj = {};
	};

	static D3D12App& Instance();
	LRESULT MessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool InitWindow();
	bool InitDirect3D();

	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateRtrAndDsvDescriptorHeap();
	void CreateCbvSrvUavDescriptorHeap();
	
	void OnResize();

	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShaderAndInputLayout();
	void BuildBox();
	void BuildLight();
	void BuildCamera();
	void BuildTexture();
	void BuildPipelineStateObject();

	void LoadHierarchyData(const std::string& filePath);
	void LoadGameObjectData(std::ifstream& loader, GameObject* parent = nullptr);
	void BuildObjects();


	void FlushCommandQueue();
	
	void CalculateFrameStats();

	ID3D12Resource* CurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

	int Update();
	void UpdateBox();

	//input Func
	void OnMouseMove(WPARAM btnState, int x, int y);
	float GetAspectRatio() const;
	void Draw(const GameTimer& gameTimer);

	void RenderObject();

	void Finalize();
private:
	HINSTANCE mhAppInst = nullptr;
	HWND      mhMainWnd = nullptr;
	std::wstring mMainWndCaption = L"d3d App";
	bool      mAppPaused = false;  // is the application paused?
	bool      mMinimized = false;  // is the application minimized?
	bool      mMaximized = false;  // is the application maximized?
	bool      mResizing = false;   // are the resize bars being dragged?
	bool      mFullscreenState = false;// fullscreen enabled

	//DXGI
	IDXGIFactory4* mdxgiFactory = nullptr;
	IDXGISwapChain* mSwapChain = nullptr;

	//D3D12
	ID3D12Device* md3dDevice = nullptr;
	ID3D12Fence* mFence = nullptr;
	UINT64 mFenceValue = 0;
	HANDLE mFenceEvent = nullptr;
	ID3D12CommandQueue* md3dCommandQueue = nullptr;
	ID3D12CommandAllocator* md3dCommandAllocator = nullptr;
	ID3D12GraphicsCommandList* md3dCommandList = nullptr;

	ID3D12DescriptorHeap* mRtvHeap = nullptr;
	ID3D12DescriptorHeap* mDsvHeap = nullptr;
	ID3D12DescriptorHeap* mCbvHeap = nullptr;
	ID3D12DescriptorHeap* mSrvHeap = nullptr;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	bool	  m4xMsaaState = false;
	UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;
	ID3D12Resource* mSwapChainBuffer[SwapChainBufferCount];
	ID3D12Resource* mDepthStencilBuffer;

	static constexpr DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	std::unique_ptr<UploadBuffer> mObjectCB = nullptr;
	ID3D12RootSignature* mRootSignature = nullptr;

	D3D12_VIEWPORT mViewport;
	tagRECT mScissorRect;

	GameTimer mTimer;

	//Shader
	ID3DBlob* mVertexBlob = nullptr;
	ID3DBlob* mPixelBlob = nullptr;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	ID3D12PipelineState* mPSO = nullptr;

	//Box
	//std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;
	//XMFLOAT4X4 mWorld = MATRIX::Identify4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;
	XMFLOAT4X4 mView = MATRIX::Identify4x4();
	XMFLOAT4X4 mProj = MATRIX::Identify4x4();

	int mWidth = 800;
	int mHeight = 600;
	POINT mLastMousePos = {};

	//GameObject
	GameObject mGameObject;
	std::vector<GameObject*> mGameObjects;

	//Camera
	Camera* mpCamera = nullptr;

	//Light
	Light* mpLight = nullptr;

	//Texture
	std::unordered_map<std::string, Texture*> mTextures;
	Texture* mTexture = nullptr;
};
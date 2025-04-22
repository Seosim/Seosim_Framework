#pragma once
#include "Input.h"
#include "GameTimer.h";
#include "UploadBuffer.h"
#include "d3dUtil.h"
#include "GameObject.h"
#include "Camera.h"
#include "Light.h"
#include "Shadow.h"
#include "Texture.h"

class D3D12App final
{
public:
	struct ObjectConstants
	{
		XMFLOAT4X4 World = {};
		XMFLOAT4X4 WorldViewProj = {};
	};

	enum class eRenderTargetType {
		FRAME0,
		FRAME1,
		MSAA,
		CAMERA_NORMAL,
		SSAO,
		SCREEN,
		COUNT
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
	void OnResizeUAVTexture();

	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildComputeRootSignature();
	void BuildLight();
	void BuildShadow();
	void BuildCamera();
	void BuildSkybox();
	void BuildSSAO();
	void BuildResourceTexture();
	void BuildComputeShader();
	void BuildUAVTexture();

	void LoadHierarchyData(const std::string& filePath);
	GameObject* LoadGameObjectData(std::ifstream& loader, GameObject* parent = nullptr);
	void BuildObjects();

	//Collision Check
	void CollisionCheck();

	void FlushCommandQueue();
	
	void CalculateFrameStats();

	ID3D12Resource* CurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

	int Update();
	void UpdatePhysics();

	//input Func
	void OnMouseMove(WPARAM btnState, int x, int y);
	float GetAspectRatio() const;
	void Draw(const GameTimer& gameTimer);

	//Shadow func
	void UpdateShadowTransform();

	void RenderObject(const float deltaTime);
	void RenderObjectForShadow();

	void PostProcessing();

	//Bloom
	void Bloom();
	void BlurTexture(const int originalID, Texture* vBlurTexture, Texture* hBlurTexture, const int sampleSize,  const int bluringCount = 0);

	//SSAO
	void SSAO();
	void BlurSSAOTexture(const int originalID, Texture* vBlurTexture, Texture* hBlurTexture);


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
	UINT      m4xMsaaQuality = 4;      // quality level of 4X MSAA

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;
	ID3D12Resource* mRenderTargets[(int)eRenderTargetType::COUNT];
	ID3D12Resource* mDepthStencilBuffer;

	//TEXTURE2DMS -> TEXTURE2D
	ID3D12Resource* mResolveCameraNormal = nullptr;
	ID3D12Resource* mResolveCameraDepth = nullptr;

	static constexpr DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	std::unique_ptr<UploadBuffer> mObjectCB = nullptr;
	ID3D12RootSignature* mRootSignature = nullptr;
	ID3D12RootSignature* mComputeRootSignature = nullptr;

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
	std::vector<GameObject*> mGameObjects;
	std::vector<GameObject*> mOpaqueGameObjects;
	std::vector<GameObject*> mTransparentGameObjects;

	//Camera
	Camera* mpCamera = nullptr;
	GameObject* mCamera = nullptr;

	//Light
	Light* mpLight = nullptr;

	//Skybox
	GameObject* mSkybox = nullptr;
	Material* mSkyboxMat = nullptr;

	Texture* mDepthTexture = nullptr;
	Texture* mNormalTexture = nullptr;
	Texture* mMSAATexture = nullptr;
	Texture* mScreenTexture = nullptr;

	//Shader
	Shader* mScreenShader = nullptr;
	ComputeShader* mComputeShader = nullptr;
	ComputeShader* mBloomShader = nullptr;
	ComputeShader* mDownSampleShader = nullptr;
	ComputeShader* mUpSampleShader = nullptr;
	ComputeShader* mVBlurShader = nullptr;
	ComputeShader* mHBlurShader = nullptr;

	//Shadow
	Shadow* mpShadow = nullptr;
	ID3D12Resource* mShadowResource = nullptr;	//Shader 클래스에 리소스를 연결해주는 용도로 사용중

	//UAV Texture
	Texture* mPostProcessingTexture = nullptr;

	//Bloom
	Texture* mBloomTexture = nullptr;
	Texture* mDownScaled4x4BloomTexture = nullptr;
	Texture* mDownScaled16x16BloomTexture = nullptr;
	Texture* mDownScaled64x64BloomTexture = nullptr;
	Texture* mBloomVBlurTexture = nullptr;
	Texture* mBloomHBlurTexture = nullptr;
	Texture* mBloom4x4VBlurTexture = nullptr;
	Texture* mBloom4x4HBlurTexture = nullptr;
	Texture* mBloom16x16VBlurTexture = nullptr;
	Texture* mBloom16x16HBlurTexture = nullptr;
	Texture* mBloom64x64VBlurTexture = nullptr;
	Texture* mBloom64x64HBlurTexture = nullptr;


	//SSAO non ComputeShader
	Shader* mSSAO = nullptr;
	ComputeShader* mSSAOVBlurShader = nullptr;
	ComputeShader* mSSAOHBlurShader = nullptr;
	Texture* mSSAOTexture = nullptr;
	Texture* mSSAOVBlurTexture = nullptr;
	Texture* mSSAOHBlurTexture = nullptr;
	Texture* mNoiseTexture = nullptr;

};
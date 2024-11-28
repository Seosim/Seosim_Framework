#pragma once
#include "pch.h"

class Shader {
public:
	Shader() = default;
	~Shader();

	//Shader(const Shader& rhs) = delete;

	void Initialize(ID3D12Device* pDevice, ID3D12RootSignature* pRootSignature);
	void SetPipelineState(ID3D12GraphicsCommandList* pCommandList);
private:
	ID3DBlob* mVertexBlob = nullptr;
	ID3DBlob* mPixelBlob = nullptr;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	ID3D12PipelineState* mPSO = nullptr;

	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	bool	  m4xMsaaState = false;
	UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA
};
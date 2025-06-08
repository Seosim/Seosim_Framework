#pragma once
#include "pch.h"
#include "IShader.h"

class ParticleShader : public IShader {
public:
	ParticleShader() = default;
	~ParticleShader();
	ParticleShader(const ParticleShader&) = delete;

	void Initialize(ID3D12Device* pDevice, ID3D12RootSignature* pRootSignature, const std::string& shaderName) override;

	void SetPipelineState(ID3D12GraphicsCommandList* pCommandList) override;
private:
	ID3DBlob* mVertexBlob = nullptr;
	ID3DBlob* mGeometryBlob = nullptr;
	ID3DBlob* mPixelBlob = nullptr;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	bool	  m4xMsaaState = false;
	UINT      m4xMsaaQuality = 0;

	UINT mTextureCount = 0;
};
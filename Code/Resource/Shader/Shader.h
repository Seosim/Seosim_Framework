#pragma once
#include "pch.h"
#include "IShader.h"


class Shader : public IShader {
public:
	enum class eType {
		Default,
		Skybox,
		Count
	};

	struct Command {
		UINT SampleCount;
		D3D12_CULL_MODE CullingMode;
		BOOL DepthEnable;
		DXGI_FORMAT Format;
	};

	Shader() = default;
	virtual ~Shader();
	Shader(const Shader& rhs) = delete;

	static Command DefaultCommand();

	void Initialize(ID3D12Device* pDevice, ID3D12RootSignature* pRootSignature, const std::string& shaderName) override;
	void Initialize(ID3D12Device* pDevice, ID3D12RootSignature* pRootSignature, const std::string& shaderName, const Command& command);

	void SetPipelineState(ID3D12GraphicsCommandList* pCommandList) override;

	static std::unordered_map<eType, Shader*> ShaderList;
private:
	ID3DBlob* mVertexBlob = nullptr;
	ID3DBlob* mPixelBlob = nullptr;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	bool	  m4xMsaaState = false;
	UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

	UINT mTextureCount = 0;
};
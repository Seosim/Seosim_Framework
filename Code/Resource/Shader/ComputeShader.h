#pragma once
#include "pch.h"
#include "IShader.h"

class ComputeShader : public IShader {
public:
	ComputeShader() = default;
	~ComputeShader();
	ComputeShader(const ComputeShader&) = delete;

	void Initialize(ID3D12Device* pDevice, ID3D12RootSignature* pRootSignature, const std::string& shaderName) override;
	void SetPipelineState(ID3D12GraphicsCommandList* pCommandList) override;
private:
	ID3DBlob* mComputeBlob = nullptr;
};
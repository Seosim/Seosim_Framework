#pragma once
#include "pch.h"

struct Command {
	UINT SampleCount;
	D3D12_CULL_MODE CullingMode;
	BOOL DepthEnable;
	DXGI_FORMAT Format;
};

class IShader {
public:
	IShader() = default;
	virtual ~IShader() = default;
	IShader(const IShader&) = delete;

	virtual void Initialize(ID3D12Device* pDevice, ID3D12RootSignature* pRootSignature, const std::string& shaderName) = 0;
	virtual void SetPipelineState(ID3D12GraphicsCommandList* pCommandList) = 0;
protected:
	ID3D12PipelineState* mPSO = nullptr;
};
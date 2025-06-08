#include "pch.h"
#include "ComputeShader.h"
#include "Core/d3dUtil.h"

ComputeShader::~ComputeShader() { RELEASE_COM(mPSO); }

void ComputeShader::Initialize(ID3D12Device* pDevice, ID3D12RootSignature* pRootSignature, const std::string& shaderName)
{
	std::wstring csPath = L"Shaders\\" + std::wstring(shaderName.begin(), shaderName.end()) + L".hlsl";

	mComputeBlob = d3dUtil::CompileShader(csPath, nullptr, "CS", "cs_5_1");


	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	psoDesc.pRootSignature = pRootSignature;
	psoDesc.CS = {
		reinterpret_cast<BYTE*>(mComputeBlob->GetBufferPointer()),
		mComputeBlob->GetBufferSize()
	};

	HR(pDevice->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void ComputeShader::SetPipelineState(ID3D12GraphicsCommandList* pCommandList)
{
	ASSERT(mPSO);

	pCommandList->SetPipelineState(mPSO);
}

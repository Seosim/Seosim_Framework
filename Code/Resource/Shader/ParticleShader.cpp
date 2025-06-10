#include "pch.h"
#include "Shader.h"
#include "Core/d3dUtil.h"
#include "ParticleShader.h"

ParticleShader::~ParticleShader()
{
	RELEASE_COM(mPSO);
}

void ParticleShader::Initialize(ID3D12Device* pDevice, ID3D12RootSignature* pRootSignature, const std::string& shaderName)
{
	HRESULT hr = S_OK;

	std::wstring vsPath = L"Shaders\\" + std::wstring(shaderName.begin(), shaderName.end()) + L".hlsl";
	std::wstring gsPath = L"Shaders\\" + std::wstring(shaderName.begin(), shaderName.end()) + L".hlsl";
	std::wstring psPath = L"Shaders\\" + std::wstring(shaderName.begin(), shaderName.end()) + L".hlsl";

	mVertexBlob = d3dUtil::CompileShader(vsPath, nullptr, "VS", "vs_5_1");
	mGeometryBlob = d3dUtil::CompileShader(gsPath, nullptr, "GS", "gs_5_1");
	mPixelBlob = d3dUtil::CompileShader(psPath, nullptr, "PS", "ps_5_1");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "FACTOR", 0, DXGI_FORMAT_R32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = pRootSignature;
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mVertexBlob->GetBufferPointer()),
		mVertexBlob->GetBufferSize()
	};
	psoDesc.GS =
	{
		reinterpret_cast<BYTE*>(mGeometryBlob->GetBufferPointer()),
		mGeometryBlob->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mPixelBlob->GetBufferPointer()),
		mPixelBlob->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	psoDesc.NumRenderTargets = 2;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.SampleDesc.Count = 4;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	HR(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void ParticleShader::SetPipelineState(ID3D12GraphicsCommandList* pCommandList)
{
	ASSERT(mPSO);

	if (this != IShader::PrevUsedShader)
	{
		IShader::PrevUsedShader = this;
		pCommandList->SetPipelineState(mPSO);
	}
}
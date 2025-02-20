#include "pch.h"
#include "Shader.h"
#include "d3dUtil.h"

std::unordered_map<Shader::eType, Shader*> Shader::ShaderList{};

Shader::~Shader()
{
	RELEASE_COM(mPSO);
}

void Shader::Initialize(ID3D12Device* pDevice, ID3D12RootSignature* pRootSignature, const std::string& shaderName)
{
	HRESULT hr = S_OK;

	std::wstring vsPath = L"Shaders\\" + std::wstring(shaderName.begin(), shaderName.end()) + L".hlsl";
	std::wstring psPath = L"Shaders\\" + std::wstring(shaderName.begin(), shaderName.end()) + L".hlsl";

	mVertexBlob = d3dUtil::CompileShader(vsPath, nullptr, "VS", "vs_5_1");
	mPixelBlob = d3dUtil::CompileShader(psPath, nullptr, "PS", "ps_5_1");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mPixelBlob->GetBufferPointer()),
		mPixelBlob->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);



	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;

	//TODO: 앞으로 쉐이더 별 설정 필요.
	if (shaderName == "Sky")
	{
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;

		psoDesc.DepthStencilState.DepthEnable = FALSE;
		//psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		//psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
		//psoDesc.DepthStencilState.StencilEnable = FALSE;
		//psoDesc.DepthStencilState.StencilReadMask = 0xff;
		//psoDesc.DepthStencilState.StencilWriteMask = 0xff;
		//psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		//psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR;
		//psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		//psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		//psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		//psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR;
		//psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		//psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	}

	HR(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void Shader::SetPipelineState(ID3D12GraphicsCommandList* pCommandList)
{
	ASSERT(mPSO);

	pCommandList->SetPipelineState(mPSO);
}

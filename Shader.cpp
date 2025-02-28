#include "pch.h"
#include "Shader.h"
#include "d3dUtil.h"

std::unordered_map<Shader::eType, Shader*> Shader::ShaderList{};

Shader::~Shader()
{
	RELEASE_COM(mPSO);
}

void Shader::Initialize(ID3D12Device* pDevice, ID3D12RootSignature* pRootSignature, const std::string& shaderName, const Command& command)
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
	if (shaderName == "color")
	{
		psoDesc.NumRenderTargets = 2;
		psoDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	psoDesc.RTVFormats[0] = command.Format;
	psoDesc.SampleDesc.Count = command.SampleCount;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	psoDesc.RasterizerState.CullMode = command.CullingMode;
	psoDesc.DepthStencilState.DepthEnable = command.DepthEnable;

	HR(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void Shader::SetPipelineState(ID3D12GraphicsCommandList* pCommandList)
{
	ASSERT(mPSO);

	pCommandList->SetPipelineState(mPSO);
}

Shader::Command Shader::DefaultCommand()
{
	Command command = {
		.SampleCount = 4,
		.CullingMode = D3D12_CULL_MODE_BACK,
		.DepthEnable = TRUE,
		.Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
	};

	return command;
}

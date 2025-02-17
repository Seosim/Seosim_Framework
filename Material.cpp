#include "pch.h"
#include "Material.h"

void Material::SetConstantBufferView(ID3D12GraphicsCommandList* pCommandList)
{
	auto cbv = mCbvHeap->GetGPUDescriptorHandleForHeapStart();
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mMaterialCB->Resource()->GetGPUVirtualAddress();

	mpShader->SetPipelineState(pCommandList);
	pCommandList->SetGraphicsRootConstantBufferView(1, cbAddress);
}

void Material::LoadMaterialData(ID3D12Device* pDevice, ID3D12RootSignature* pRootSignature, ID3D12DescriptorHeap* pCbvHeap, const std::string& filePath)
{
	std::ifstream in{ filePath, std::ios::binary };

	//TODO: 쉐이더 이름 별로 받아오는 데이터 다르게 처리
	struct TestCBuffer {
		XMFLOAT4 Color;
	};

	TestCBuffer cBuffer;
	in.read(reinterpret_cast<char*>(&cBuffer), sizeof(TestCBuffer));

	Initialize(pDevice, pRootSignature, pCbvHeap, cBuffer);
}


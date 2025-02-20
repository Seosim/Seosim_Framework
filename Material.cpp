#include "pch.h"
#include "Material.h"

void Material::SetConstantBufferView(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* srvHeap)
{
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mMaterialCB->Resource()->GetGPUVirtualAddress();

	mpShader->SetPipelineState(pCommandList);
	pCommandList->SetGraphicsRootConstantBufferView(1, cbAddress);

	for (int i = 0; i < mTextures.size(); ++i)
	{
		if (mTextures[i] != nullptr)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE texHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
			texHandle.ptr += 32 * (mTextures[i]->GetID());
			pCommandList->SetGraphicsRootDescriptorTable(4 + i, texHandle);
		}

	}
}

void Material::LoadMaterialData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12RootSignature* pRootSignature, ID3D12DescriptorHeap* pSrvHeap, const std::string& filePath)
{
	std::ifstream in{ filePath, std::ios::binary };

	//TODO: 쉐이더 이름 별로 받아오는 데이터 다르게 처리
	struct TestCBuffer {
		XMFLOAT4 Color;
	};

	Shader::eType shaderType = {};
	//in.read(reinterpret_cast<char*>(&shaderType), sizeof(int));

	TestCBuffer cBuffer;
	in.read(reinterpret_cast<char*>(&cBuffer), sizeof(TestCBuffer));

	Initialize(pDevice, pCommandList, pRootSignature, pSrvHeap, cBuffer, shaderType);
}

void Material::SetTexture(Texture* texture, UINT index)
{
	mTextures[index] = texture;
}


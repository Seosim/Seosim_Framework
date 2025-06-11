#include "pch.h"
#include "Material.h"

std::unordered_map<std::string, Material*> Material::MaterialList{};
Material* Material::mPrevUsedMaterial = nullptr;

void Material::SetConstantBufferView(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* srvHeap)
{
	if (mPrevUsedMaterial != this)
	{
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mMaterialCB->Resource()->GetGPUVirtualAddress();

		mpShader->SetPipelineState(pCommandList);
		pCommandList->SetGraphicsRootConstantBufferView(1, cbAddress);

		UpdateTextureOnSrv(pCommandList, srvHeap);
	}
}

void Material::LoadMaterialData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12RootSignature* pRootSignature, ID3D12DescriptorHeap* pSrvHeap, const std::string& filePath)
{
	//먼저 MaterialList에서 해당 Material이 있는지 검사하고 없을 시 이 함수 실행
	ASSERT(MaterialList.find(filePath) == MaterialList.end());

	std::ifstream in{ filePath, std::ios::binary };

	Shader::eType shaderType = {};
	in.read(reinterpret_cast<char*>(&shaderType), sizeof(int));

	//TODO: 쉐이더 이름 별로 받아오는 데이터 다르게 처리
	struct LitCBuffer {
		XMFLOAT4 BaseColor;
		XMFLOAT4 EmissionColor;
		float Metailic;
		float Smoothness;
	};

	LitCBuffer cBuffer;
	in.read(reinterpret_cast<char*>(&cBuffer), sizeof(LitCBuffer));

	mMaterialCB = std::make_unique<UploadBuffer>(pDevice, 1, true, sizeof(LitCBuffer));
	UpdateConstantBuffer(cBuffer);

	//텍스처가 있다면 로드 현재는 머티리얼 별 최대 텍스처 1개라고 가정
	bool bHasBasedMap = false;
	in.read(reinterpret_cast<char*>(&bHasBasedMap), sizeof(bool));

	if (bHasBasedMap)
	{
		std::string texturePath;

		//유니티는 바이너리로 문자열 저장할 때 맨 앞 1바이트에 크기도 자동으로 기록함
		char pathLength;
		in.read(reinterpret_cast<char*>(&pathLength), sizeof(char)); // 문자열 길이 먼저 읽기

		texturePath.resize(pathLength);
		in.read(&texturePath[0], pathLength);

		std::wstring name(texturePath.begin(), texturePath.end());
		std::wstring basePath = L"./Assets/Textures/";
		std::wstring fullPath = basePath + std::wstring(name.begin(), name.end()) + L".dds";
		Texture* texture = nullptr;

		if (Texture::TextureList.find(name) == Texture::TextureList.end())
		{
			texture = new Texture();

			texture->LoadTextureFromDDSFile(pDevice, pCommandList, fullPath.c_str(), RESOURCE_TEXTURE2D, 0);
			texture->CreateSrv(pDevice, pSrvHeap, name);
		}
		else
		{
			texture = Texture::TextureList[name];
		}
		SetTexture(texture, 0);
	}

	//기존에 있는 쉐이더면 따로 생성하지 않고 재사용 합니다.
	if (Shader::ShaderList.find(shaderType) != Shader::ShaderList.end())
	{
		mpShader = Shader::ShaderList[shaderType];
	}
	else
	{
		mpShader = new Shader;

		switch (shaderType)
		{
		case Shader::eType::Default: 
		{
			mpShader->Initialize(pDevice, pRootSignature, "color", Shader::DefaultCommand());
			break;
		}
		case Shader::eType::Skybox:
		{
			std::wstring name{ L"Skybox" };
			std::wstring basePath = L"./Assets/Textures/";
			std::wstring fullPath = basePath + std::wstring(name.begin(), name.end()) + L".dds";
			Texture* texture = nullptr;

			if (Texture::TextureList.find(name) == Texture::TextureList.end())
			{
				texture = new Texture();

				texture->LoadTextureFromDDSFile(pDevice, pCommandList, fullPath.c_str(), RESOURCE_TEXTURE_CUBE, 0);
				texture->CreateSrv(pDevice, pSrvHeap, name, true);
			}
			else
			{
				texture = Texture::TextureList[name];
			}
			SetTexture(texture, 0);

			Shader::Command command = Shader::DefaultCommand();
			command.CullingMode = D3D12_CULL_MODE_FRONT;
			command.DepthEnable = FALSE;

			mpShader->Initialize(pDevice, pRootSignature, "Sky", command);
			break;
		}

		case Shader::eType::Count:
			break;
		default:
			break;
		}

		Shader::ShaderList[shaderType] = mpShader;
	}

	MaterialList[filePath] = this;
}

void Material::SetTexture(Texture* texture, UINT index)
{
	mTextures[index] = texture;
}

void Material::UpdateTextureOnSrv(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* srvHeap)
{
	for (int i = 0; i < mTextures.size(); ++i)
	{
		if (mTextures[i] != nullptr)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE texHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
			texHandle.ptr += 32 * (mTextures[i]->GetID());
			pCommandList->SetGraphicsRootDescriptorTable((int)eRootParameter::TEXTURE0 + i, texHandle);
		}

	}
}

void Material::UpdateTextureOnSrv(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* srvHeap, UINT rootParameterIndex, UINT textureIndex)
{
	if (mTextures[textureIndex] != nullptr)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE texHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
		texHandle.ptr += 32 * (mTextures[textureIndex]->GetID());
		pCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, texHandle);
	}
}


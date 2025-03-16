#pragma once

#include "pch.h"

#include "IComponent.h"
#include "Shader.h"
#include "Texture.h"
#include "UploadBuffer.h"

class Material final : public IComponent {
public:
	~Material() {}

	virtual void Awake() {}

	template<typename T>
	void Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12RootSignature* pRootSignature, ID3D12DescriptorHeap* pSrvHeap, const T& data, Shader::eType shaderType)
	{
		mMaterialCB = std::make_unique<UploadBuffer>(pDevice, 1, true, sizeof(T));
		UpdateConstantBuffer(data);

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
			case Shader::eType::Default: {
				std::wstring name{ L"./Assets/Textures/bricks2.dds" };
				Texture* texture = nullptr;

				if (Texture::TextureList.find(name) == Texture::TextureList.end())
				{
					texture = new Texture();

					texture->LoadTextureFromDDSFile(pDevice, pCommandList, name.c_str(), RESOURCE_TEXTURE2D, 0);
					texture->CreateSrv(pDevice, pSrvHeap, name);
				}
				else
				{
					texture = Texture::TextureList[name];
				}
				SetTexture(texture, 0);

				mpShader->Initialize(pDevice, pRootSignature, "color", Shader::DefaultCommand());
				break;
			}
			case Shader::eType::Skybox:
			{
				std::wstring name(L"sunsetcube1024");
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
	}

	template<typename T>
	void UpdateConstantBuffer(const T& data)
	{
		mMaterialCB->CopyData(0, data);
	}

	void SetConstantBufferView(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* srvHeap);

	void LoadMaterialData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12RootSignature* pRootSignature, ID3D12DescriptorHeap* pSrvHeap, const std::string& filePath);

	void SetTexture(Texture* texture, UINT index);
	
	void UpdateTextureOnSrv(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* srvHeap);

	void UpdateTextureOnSrv(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* srvHeap, UINT rootParameterIndex, UINT textureIndex);
private:
	Shader* mpShader = nullptr;
	std::unique_ptr<UploadBuffer> mMaterialCB = nullptr;
	std::array<Texture*, MAX_TEXTURE_COUNT> mTextures = {};
};
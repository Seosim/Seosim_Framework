#pragma once

#include "pch.h"

#include "IComponent.h"
#include "Shader.h"
#include "UploadBuffer.h"

class Material : public IComponent {
public:
	~Material() { delete mpShader; }

	virtual void Awake() {}

	template<typename T>
	void Initialize(ID3D12Device* pDevice, ID3D12RootSignature* pRootSignature, ID3D12DescriptorHeap* cbvHeap, const T& data)
	{
		mpShader = new Shader;
		mpShader->Initialize(pDevice, pRootSignature, "color");

		mCbvHeap = cbvHeap;

		mMaterialCB = std::make_unique<UploadBuffer>(pDevice, 1, true, sizeof(T));

		UpdateConstantBuffer(data);
	}

	template<typename T>
	void UpdateConstantBuffer(const T& data)
	{
		mMaterialCB->CopyData(0, data);
	}

	void SetConstantBufferView(ID3D12GraphicsCommandList* pCommandList);

	void LoadMaterialData(ID3D12Device* pDevice, ID3D12RootSignature* pRootSignature, ID3D12DescriptorHeap* pCbvHeap, const std::string& filePath);
	
private:
	Shader* mpShader = nullptr;
	std::unique_ptr<UploadBuffer> mMaterialCB = nullptr;
	ID3D12DescriptorHeap* mCbvHeap = nullptr;
};
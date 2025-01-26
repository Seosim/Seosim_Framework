#pragma once

#include "pch.h"
#include "IComponent.h"
#include "Shader.h"
#include "UploadBuffer.h"

template<typename T>
class Material : public IComponent {
public:
	virtual void Awake() {}

	void Initialize(ID3D12Device* pDevice, ID3D12DescriptorHeap* mCbvHeap);
private:
	Shader* mpShader = nullptr;
	std::unique_ptr<UploadBuffer<T>> mMaterialCB = nullptr;
};

template<typename T>
inline void Material<T>::Initialize(ID3D12Device* pDevice, ID3D12DescriptorHeap* mCbvHeap)
{
	constexpr int MAX_OBJECT_COUNT = 100000;
	mMaterialCB = std::make_unique<UploadBuffer<T>>(pDevice, MAX_OBJECT_COUNT, true);

	UINT matCBByteSize = (sizeof(T) + 255) & ~255;

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mMaterialCB->Resource()->GetGPUVirtualAddress();
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * matCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = matCBByteSize;

	auto handle = mCbvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += mCbvSrvUavDescriptorSize * boxCBufIndex;

	pDevice->CreateConstantBufferView(&cbvDesc, handle);
}

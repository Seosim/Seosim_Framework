#include "pch.h"
#include "Light.h"

void Light::Update(ID3D12GraphicsCommandList* pCommandList)
{
	mLightCB->CopyData(0, mBuffer);

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mLightCB->Resource()->GetGPUVirtualAddress();
	pCommandList->SetGraphicsRootConstantBufferView((UINT)eRootParameter::LIGHT, cbAddress);
}

void Light::SetUploadBuffer(UploadBuffer* buffer)
{
	mLightCB = buffer;
}

void Light::SetDirection(const XMFLOAT3 direction)
{
	mBuffer.LightDir = direction;
	mLightCB->CopyData(0, mBuffer);
}

void Light::SetColor(const XMFLOAT3 color)
{
	mBuffer.LightColor = color;
	mLightCB->CopyData(0, mBuffer);
}

#include "pch.h"
#include "Camera.h"

void Camera::Initialize(ID3D12Device* pDevice)
{
	mCameraCB = std::make_unique<UploadBuffer>(pDevice, 1, true, sizeof(CameraBuffer));

	mCameraCB->CopyData(0, mCameraBuffer);
}

void Camera::Update(ID3D12GraphicsCommandList* pCommandList)
{
	XMMATRIX view = XMLoadFloat4x4(&mCameraBuffer.View);
	XMMATRIX proj = XMLoadFloat4x4(&mCameraBuffer.Proj);

	XMStoreFloat4x4(&mCameraBuffer.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mCameraBuffer.Proj, XMMatrixTranspose(proj));

	mCameraCB->CopyData(0, mCameraBuffer);

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mCameraCB->Resource()->GetGPUVirtualAddress();
	pCommandList->SetGraphicsRootConstantBufferView(3, cbAddress);
}

void Camera::SetPosition(const XMFLOAT3& position)
{
	mCameraBuffer.CameraPos = position;
}

void Camera::SetMatrix(const XMFLOAT4X4& view, const XMFLOAT4X4& proj)
{
	mCameraBuffer.View = view;
	mCameraBuffer.Proj = proj;
}

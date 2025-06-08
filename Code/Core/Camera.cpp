#include "pch.h"
#include "Camera.h"

void Camera::Initialize(ID3D12Device* pDevice)
{
	mCameraCB = std::make_unique<UploadBuffer>(pDevice, 1, true, sizeof(CameraBuffer));

	mCameraCB->CopyData(0, mCameraBuffer);
}

void Camera::Update(ID3D12GraphicsCommandList* pCommandList, const float deltaTime)
{
	// Transform NDC space [-1,+1]^2 to texture space [0,1]^
	XMMATRIX T =
	{
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	};

	XMMATRIX view = XMLoadFloat4x4(&mCameraBuffer.View);
	XMMATRIX proj = XMLoadFloat4x4(&mCameraBuffer.Proj);
	XMMATRIX projTex = proj * T;
	XMMATRIX projInv = XMMatrixInverse(nullptr, proj);

	XMStoreFloat4x4(&mCameraBuffer.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mCameraBuffer.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mCameraBuffer.ProjectionTex, XMMatrixTranspose(projTex));
	XMStoreFloat4x4(&mCameraBuffer.ProjectionInv, XMMatrixTranspose(projInv));

	mCameraBuffer.ElapsedTime += deltaTime;

	mCameraCB->CopyData(0, mCameraBuffer);

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mCameraCB->Resource()->GetGPUVirtualAddress();
	pCommandList->SetGraphicsRootConstantBufferView((int)eRootParameter::CAMERA, cbAddress);

	XMStoreFloat4x4(&mCameraBuffer.View, view);
	XMStoreFloat4x4(&mCameraBuffer.Proj, proj);
	XMStoreFloat4x4(&mCameraBuffer.ProjectionTex, projTex);
	XMStoreFloat4x4(&mCameraBuffer.ProjectionInv, projInv);
}

void Camera::UpdateForComputeShader(ID3D12GraphicsCommandList* pCommandList)
{
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mCameraCB->Resource()->GetGPUVirtualAddress();
	pCommandList->SetComputeRootConstantBufferView(4, cbAddress);
}

XMFLOAT3 Camera::GetPosition() const
{
	return mCameraBuffer.CameraPos;
}

void Camera::SetPosition(const XMFLOAT3& position)
{
	mCameraBuffer.CameraPos = position;
}

XMFLOAT3 Camera::GetDirection() const
{
	return XMFLOAT3();
}

XMMATRIX Camera::GetViewMatrix() const
{
	XMMATRIX viewMatrix = XMLoadFloat4x4(&mCameraBuffer.View);

	return viewMatrix;
}

BoundingFrustum Camera::GetBoundingFrustum() const
{
	BoundingFrustum frustum = {};

	XMMATRIX viewMatrix = XMLoadFloat4x4(&mCameraBuffer.View);
	XMMATRIX projectionMatrix = XMLoadFloat4x4(&mCameraBuffer.Proj);

	BoundingFrustum::CreateFromMatrix(frustum, projectionMatrix);
	frustum.Transform(frustum, XMMatrixInverse(nullptr, viewMatrix));

	return frustum;
}

void Camera::SetViewMatrix(const XMFLOAT4X4& view)
{
	mCameraBuffer.View = view;
}

void Camera::SetProjMatrix(const XMFLOAT4X4& proj)
{
	mCameraBuffer.Proj = proj;
}

void Camera::SetMatrix(const XMFLOAT4X4& view, const XMFLOAT4X4& proj)
{
	mCameraBuffer.View = view;
	mCameraBuffer.Proj = proj;
}

void Camera::SetScreenSize(float width, float height)
{
	mCameraBuffer.ScreenWidth = width;
	mCameraBuffer.ScreenHeight = height;
}

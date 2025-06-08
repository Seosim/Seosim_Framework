#include "pch.h"
#include "CameraController.h"

void CameraController::Update(const float deltaTime)
{
	XMFLOAT3 position = mTransform->GetPosition();
	XMVECTOR vPosition = XMLoadFloat3(&position);

	XMVECTOR vForward = mTransform->GetForwardVector();
	XMVECTOR vUp = mTransform->GetUpVector();

	XMMATRIX viewMatrix = XMMatrixLookToLH(vPosition, vForward, vUp);
	XMFLOAT4X4 view;
	XMStoreFloat4x4(&view, viewMatrix);

	mCamera->SetPosition(position);
	mCamera->SetViewMatrix(view);
}

void CameraController::Initialize(Camera* camera, Transform* transform)
{
	mCamera = camera;
	mTransform = transform;
}

#pragma once
#include "pch.h"
#include "FrameResource.h"

class Camera final {
public:

	Camera() = default;
	~Camera() {}
	Camera(const Camera&) = delete;


	void Update(ID3D12GraphicsCommandList* pCommandList, const float deltaTime);
	void UpdateForComputeShader(ID3D12GraphicsCommandList* pCommandList);

	XMFLOAT3 GetPosition() const;
	void SetPosition(const XMFLOAT3& position);

	void SetAxis(const XMFLOAT3 axis);

	XMFLOAT3 GetDirection() const;
	XMMATRIX GetViewMatrix() const;
	
	BoundingFrustum GetBoundingFrustum() const;

	void SetViewMatrix(const XMFLOAT4X4& view);
	void SetProjMatrix(const XMFLOAT4X4& proj);
	void SetMatrix(const XMFLOAT4X4& view, const XMFLOAT4X4& proj);

	void SetScreenSize(float width, float height);
	void SetUploadBuffer(UploadBuffer* buffer);
private:
	UploadBuffer* mCameraCB = nullptr;

	CameraBuffer mCameraBuffer = {};
};
#pragma once
#include "pch.h"
#include "UploadBuffer.h"

class Camera final {
public:
	struct CameraBuffer {
		XMFLOAT4X4 View;
		XMFLOAT4X4 Proj;
		XMFLOAT4X4 ProjectionTex;
		XMFLOAT4X4 ProjectionInv;
		XMFLOAT3 CameraPos;
		float ScreenWidth;
		float ScreenHeight;
		float ElapsedTime;
	};

	Camera() = default;
	~Camera() {}
	Camera(const Camera&) = delete;

	void Initialize(ID3D12Device* pDevice);

	void Update(ID3D12GraphicsCommandList* pCommandList, const float deltaTime);
	void UpdateForComputeShader(ID3D12GraphicsCommandList* pCommandList);

	XMFLOAT3 GetPosition() const;
	void SetPosition(const XMFLOAT3& position);

	XMFLOAT3 GetDirection() const;
	XMMATRIX GetViewMatrix() const;

	void SetViewMatrix(const XMFLOAT4X4& view);
	void SetProjMatrix(const XMFLOAT4X4& proj);
	void SetMatrix(const XMFLOAT4X4& view, const XMFLOAT4X4& proj);

	void SetScreenSize(float width, float height);
private:
	std::unique_ptr<UploadBuffer> mCameraCB = nullptr;

	CameraBuffer mCameraBuffer;
};
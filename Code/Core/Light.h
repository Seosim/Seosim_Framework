#pragma once
#include "pch.h"
#include "FrameResource.h"

class Light final {
public:
	Light() = default;
	~Light() {}
	Light(const Light&) = delete;

	void Update(ID3D12GraphicsCommandList* pCommandList);

	void SetUploadBuffer(UploadBuffer* buffer);
	void SetDirection(const XMFLOAT3 direction);
	void SetColor(const XMFLOAT3 color);

private:
	UploadBuffer* mLightCB = nullptr;
	LightBuffer mBuffer = {};
};
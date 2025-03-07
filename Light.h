#pragma once
#include "pch.h"
#include "UploadBuffer.h"

class Light final {
public:
	struct LightBuffer {
		XMFLOAT3 LightDir = {};
		float padding0;
		XMFLOAT3 LightColor = {};
	};

	Light() = default;
	~Light() {}
	Light(const Light&) = delete;

	void Initialize(ID3D12Device* pDevice);

	void Update(ID3D12GraphicsCommandList* pCommandList);

	void SetDirection(const XMFLOAT3 direction);
	void SetColor(const XMFLOAT3 color);

private:
	std::unique_ptr<UploadBuffer> mLightCB = nullptr;
	LightBuffer mBuffer;
};
#pragma once
#include "pch.h"
#include "Resource/Shader/Shader.h"
#include "Resource/Texture.h"
#include "FrameResource.h"

class Shadow final {
public:
	Shadow() = default;
	~Shadow() { delete mShadowShader; }
	Shadow(const Shadow&) = delete;

	void Release();

	void Initialize(ID3D12Device* pDevice, ID3D12RootSignature* pRootSignature, ID3D12DescriptorHeap* pSrvHeap, ID3D12Resource* pResource);

	void ChangeResource(ID3D12Device* pDevice, ID3D12DescriptorHeap* pSrvHeap, ID3D12Resource* pResource);

	void SetViewPortAndScissorRect(ID3D12GraphicsCommandList* pCommandList);
	void SetGraphicsRootDescriptorTable(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* pSrvHeap);

	void UpdateShadowTransform(ID3D12GraphicsCommandList* pCommandList, XMVECTOR cameraPos, XMVECTOR cameraForward, UploadBuffer* shadowCB);

	void Render();


	ID3D12Resource* GetResource();
	XMFLOAT3 GetShadowDirection() const;

private:
	ID3D12Resource* mShadow = nullptr;
	Texture* mShadowTexture = nullptr;
	std::unique_ptr<UploadBuffer> mShadowCB = nullptr;
	ShadowConstants mShadowBuffer = {};
	Shader* mShadowShader = nullptr;
	D3D12_VIEWPORT mShadowViewport = {};
	tagRECT mShadowScissorRect = {};
};
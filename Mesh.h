#pragma once
#include "pch.h"
#include "IComponent.h"


class Mesh : public IComponent {
public:
	struct SubmeshGeometry
	{
		UINT IndexCount = 0;
		UINT StartIndexLocation = 0;
		INT BaseVertexLocation = 0;

		DirectX::BoundingBox Bounds;
	};

	Mesh() = default;
	~Mesh();

	Mesh(const Mesh& rhs) = delete;
	virtual void Awake() {}

	void LoadMeshData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const std::string& filePath);

	virtual void Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);
	void Render(ID3D12GraphicsCommandList* pCommandList);
protected:
	ID3DBlob* mVertexBufferCPU = nullptr;
	ID3DBlob* mIndexBufferCPU = nullptr;

	ID3D12Resource* mPositionBufferGPU = nullptr;
	ID3D12Resource* mNormalBufferGPU = nullptr;
	ID3D12Resource* mUVBufferGPU = nullptr;
	ID3D12Resource* mIndexBufferGPU = nullptr;

	ID3D12Resource* mPositionBufferUploader = nullptr;
	ID3D12Resource* mNormalBufferUploader = nullptr;
	ID3D12Resource* mUVBufferUploader = nullptr;
	ID3D12Resource* mIndexBufferUploader = nullptr;

	UINT mVertexByteStride = 0;
	UINT mVertexBufferByteSize = 0;
	DXGI_FORMAT mIndexFormat = DXGI_FORMAT_R16_UINT;
	UINT mIndexBufferByteSize = 0;

	std::array<D3D12_VERTEX_BUFFER_VIEW, 3> mVertexBufferViews;
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView;

	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;
private:
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = mPositionBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = mVertexByteStride;
		vbv.SizeInBytes = mVertexBufferByteSize;

		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = mIndexFormat;
		ibv.SizeInBytes = mIndexBufferByteSize;

		return ibv;
	}
};
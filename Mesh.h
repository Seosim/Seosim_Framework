#pragma once
#include "pch.h"

class Mesh {
public:
	Mesh() = default;
	~Mesh();

	Mesh(const Mesh& rhs) = delete;

	void LoadMeshData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const std::string& filePath);

	void Render(ID3D12GraphicsCommandList* pCommandList);
	void SetBuffers(ID3D12GraphicsCommandList* pCommandList);
	void RenderSubMeshes(ID3D12GraphicsCommandList* pCommandList, const int subMeshIndex = 0);

	int GetSubMeshCount() const;

	static std::unordered_map<std::string, Mesh*> MeshList;
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

	std::array<D3D12_VERTEX_BUFFER_VIEW, 3> mVertexBufferViews = {};
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView = {};
	std::vector<UINT> mSubMeshIndex = {};
private:
	static Mesh* mPrevUsedMesh;
};
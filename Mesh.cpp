#include "pch.h"
#include "Mesh.h"
#include "Vertex.h"
#include "d3dUtil.h"

std::unordered_map<std::string, Mesh*> Mesh::MeshList{};

Mesh::~Mesh()
{
	RELEASE_COM(mPositionBufferGPU);
	RELEASE_COM(mNormalBufferGPU);
	RELEASE_COM(mUVBufferGPU);
	RELEASE_COM(mIndexBufferGPU);

	RELEASE_COM(mPositionBufferUploader);
	RELEASE_COM(mNormalBufferUploader);
	RELEASE_COM(mUVBufferUploader);
	RELEASE_COM(mIndexBufferUploader);
}

void Mesh::LoadMeshData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const std::string& filePath)
{
	//먼저 MeshList에서 해당 Mesh가 있는지 검사하고 없을 시 이 함수 실행
	ASSERT(MeshList.find(filePath) == MeshList.end());

	std::ifstream in{ filePath, std::ios::binary };

	MeshList[filePath] = this;

	int vertexCount = 0;
	in.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));

	std::vector<XMFLOAT3> positions(vertexCount);
	std::vector<XMFLOAT3> normals(vertexCount);
	std::vector<XMFLOAT2> uvs(vertexCount);

	in.read(reinterpret_cast<char*>(positions.data()), vertexCount * sizeof(XMFLOAT3));
	in.read(reinterpret_cast<char*>(normals.data()), vertexCount * sizeof(XMFLOAT3));
	in.read(reinterpret_cast<char*>(uvs.data()), vertexCount * sizeof(XMFLOAT2));

	int subMeshCount = 0;
	in.read(reinterpret_cast<char*>(&subMeshCount), sizeof(subMeshCount));

	int totalIndicesCount = 0;
	std::vector<UINT> totalIndices;

	for (int i = 0; i < subMeshCount; ++i)
	{
		int indexCount = 0;
		in.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));
		totalIndicesCount = indexCount;

		std::vector<UINT> indices(indexCount);
		in.read(reinterpret_cast<char*>(indices.data()), indexCount * sizeof(UINT));

		totalIndices.insert(totalIndices.end(), indices.begin(), indices.end());

		mSubMeshIndex.push_back(totalIndicesCount);
	}

	mPositionBufferGPU = d3dUtil::CreateDefaultBuffer(pDevice,
		pCommandList, positions.data(), positions.size() * sizeof(XMFLOAT3), mPositionBufferUploader);

	mNormalBufferGPU = d3dUtil::CreateDefaultBuffer(pDevice,
		pCommandList, normals.data(), normals.size() * sizeof(XMFLOAT3), mNormalBufferUploader);

	mUVBufferGPU = d3dUtil::CreateDefaultBuffer(pDevice,
		pCommandList, uvs.data(), uvs.size() * sizeof(XMFLOAT2), mUVBufferUploader);

	mIndexBufferGPU = d3dUtil::CreateDefaultBuffer(pDevice,
		pCommandList, totalIndices.data(), totalIndices.size() * sizeof(UINT), mIndexBufferUploader);

	mVertexBufferViews[0] =
	{
		.BufferLocation = mPositionBufferGPU->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>(sizeof(XMFLOAT3) * positions.size()),
		.StrideInBytes = sizeof(XMFLOAT3),
	};
	mVertexBufferViews[1] =
	{
		.BufferLocation = mNormalBufferGPU->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>(sizeof(XMFLOAT3) * normals.size()),
		.StrideInBytes = sizeof(XMFLOAT3),
	};

	mVertexBufferViews[2] =
	{
		.BufferLocation = mUVBufferGPU->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>(sizeof(XMFLOAT2) * uvs.size()),
		.StrideInBytes = sizeof(XMFLOAT2),
	};

	mIndexBufferView =
	{
		.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>(sizeof(UINT) * totalIndices.size()),
		.Format = DXGI_FORMAT_R32_UINT,
	};
}

void Mesh::Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
{

	std::ifstream in{ "Capsule.bin", std::ios::binary };
	int vertexCount = 0;
	in.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));

	std::vector<XMFLOAT3> positions(vertexCount);
	std::vector<XMFLOAT3> normals(vertexCount);
	std::vector<XMFLOAT2> uvs(vertexCount);

	in.read(reinterpret_cast<char*>(positions.data()), vertexCount * sizeof(XMFLOAT3));
	in.read(reinterpret_cast<char*>(normals.data()), vertexCount * sizeof(XMFLOAT3));
	in.read(reinterpret_cast<char*>(uvs.data()), vertexCount * sizeof(XMFLOAT2));

	int indexCount = 0;
	in.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));

	std::vector<UINT> indices(indexCount);
	in.read(reinterpret_cast<char*>(indices.data()),indexCount * sizeof(UINT));

	//const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	//const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	//HR(D3DCreateBlob(vbByteSize, &mVertexBufferCPU));
	//CopyMemory(mVertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	//HR(D3DCreateBlob(ibByteSize, &mIndexBufferCPU));
	//CopyMemory(mIndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	mPositionBufferGPU = d3dUtil::CreateDefaultBuffer(pDevice,
		pCommandList, positions.data(), positions.size() * sizeof(XMFLOAT3), mPositionBufferUploader);

	mNormalBufferGPU = d3dUtil::CreateDefaultBuffer(pDevice,
		pCommandList, normals.data(), normals.size() * sizeof(XMFLOAT3), mNormalBufferUploader);

	mUVBufferGPU = d3dUtil::CreateDefaultBuffer(pDevice,
		pCommandList, uvs.data(), uvs.size() * sizeof(XMFLOAT2), mUVBufferUploader);

	mIndexBufferGPU = d3dUtil::CreateDefaultBuffer(pDevice,
		pCommandList, indices.data(), indices.size() * sizeof(int), mIndexBufferUploader);

	mVertexBufferViews[0] =
	{
		.BufferLocation = mPositionBufferGPU->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>(sizeof(XMFLOAT3) * positions.size()),
		.StrideInBytes = sizeof(XMFLOAT3),
	};
	mVertexBufferViews[1] =
	{
		.BufferLocation = mNormalBufferGPU->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>(sizeof(XMFLOAT3) * normals.size()),
		.StrideInBytes = sizeof(XMFLOAT3),
	};

	mVertexBufferViews[2] =
	{
		.BufferLocation = mUVBufferGPU->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>(sizeof(XMFLOAT2) * uvs.size()),
		.StrideInBytes = sizeof(XMFLOAT2),
	};

	mIndexBufferView =
	{
		.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>(sizeof(UINT) * indices.size()),
		.Format = DXGI_FORMAT_R32_UINT,
	};

	//mVertexByteStride = sizeof(Vertex);
	//mVertexBufferByteSize = vbByteSize;
	//mIndexFormat = DXGI_FORMAT_R16_UINT;
	//mIndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;
	DrawArgs["box"] = submesh;
}

void Mesh::Render(ID3D12GraphicsCommandList* pCommandList)
{
	SetBuffers(pCommandList);
	RenderSubMeshes(pCommandList);
}

void Mesh::SetBuffers(ID3D12GraphicsCommandList* pCommandList)
{
	pCommandList->IASetVertexBuffers(0, mVertexBufferViews.size(), mVertexBufferViews.data());
	pCommandList->IASetIndexBuffer(&mIndexBufferView);
	pCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Mesh::RenderSubMeshes(ID3D12GraphicsCommandList* pCommandList, const int subMeshIndex)
{
	int offset = 0;
	for (int i = 0; i < mSubMeshIndex.size(); ++i)
	{
		if (i == subMeshIndex)
		{
			pCommandList->DrawIndexedInstanced(mSubMeshIndex[subMeshIndex], 1, offset, 0, 0);
			return;
		}
		offset += mSubMeshIndex[i];
	}
}

int Mesh::GetSubMeshCount() const
{
	return mSubMeshIndex.size();
}

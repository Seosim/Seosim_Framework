#include "pch.h"
#include "Mesh.h"
#include "Vertex.h"
#include "d3dUtil.h"

std::unordered_map<std::string, Mesh*> Mesh::MeshList{};
Mesh* Mesh::mPrevUsedMesh = nullptr;

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
	//Save Triangle Data
	mTriangles.reserve(totalIndices.size() / 3);
	for (int i = 0; i < totalIndices.size(); i += 3)
	{
		mTriangles.emplace_back( positions[totalIndices[i]], positions[totalIndices[i + 1]], positions[totalIndices[i + 2]] );
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

void Mesh::Render(ID3D12GraphicsCommandList* pCommandList)
{
	SetBuffers(pCommandList);
	RenderSubMeshes(pCommandList);
}

void Mesh::SetBuffers(ID3D12GraphicsCommandList* pCommandList)
{
	if (mPrevUsedMesh != this)
	{
		mPrevUsedMesh = this;
		pCommandList->IASetVertexBuffers(0, mVertexBufferViews.size(), mVertexBufferViews.data());
		pCommandList->IASetIndexBuffer(&mIndexBufferView);
		pCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
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

std::vector<Triangle> Mesh::GetTriangles() const
{
	return mTriangles;
}

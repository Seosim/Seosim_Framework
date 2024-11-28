#include "pch.h"
#include "Mesh.h"
#include "Vertex.h"
#include "d3dUtil.h"

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
	std::ifstream in{ filePath, std::ios::binary };
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
	in.read(reinterpret_cast<char*>(indices.data()), indexCount * sizeof(UINT));

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
	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;
	DrawArgs["box"] = submesh;
}

void Mesh::Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
{
	//std::array<Vertex, 8> vertices =
	//{
	//	Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
	//	Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
	//	Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
	//	Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
	//	Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
	//	Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
	//	Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
	//	Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	//};

	//std::array<std::uint16_t, 36> indices =
	//{
	//	// front face
	//	0, 1, 2,
	//	0, 2, 3,

	//	// back face
	//	4, 6, 5,
	//	4, 7, 6,

	//	// left face
	//	4, 5, 1,
	//	4, 1, 0,

	//	// right face
	//	3, 2, 6,
	//	3, 6, 7,

	//	// top face
	//	1, 5, 6,
	//	1, 6, 2,

	//	// bottom face
	//	4, 0, 3,
	//	4, 3, 7
	//};

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

void Mesh::Render(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* pCbvHeap, UINT size)
{
	//auto vbv = VertexBufferView();
	//auto ibv = IndexBufferView();

	pCommandList->IASetVertexBuffers(0, mVertexBufferViews.size(), mVertexBufferViews.data());
	pCommandList->IASetIndexBuffer(&mIndexBufferView);
	pCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	pCommandList->DrawIndexedInstanced(DrawArgs["box"].IndexCount, 1, 0, 0, 0);
}

#pragma once
#include "pch.h"
#include "IComponent.h"
#include "Mesh.h"
#include "Material.h"

class MeshRenderer : public IComponent
{
public:
	MeshRenderer() = default;
	~MeshRenderer() {}
	MeshRenderer(const MeshRenderer&) = delete;

	virtual void Awake() {}

	virtual void Update(const float deltaTime);

	void SetMesh(Mesh* mesh);
	void AddMaterial(Material* material);

	Material* GetMaterial(int index = 0) const;
	Mesh* GetMesh() const;

	void Render(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* pSrvHeap);
private:
	Mesh* mMesh = nullptr;
	std::vector<Material*> mMaterials = {};
	int mSubMeshCount = 0;
};


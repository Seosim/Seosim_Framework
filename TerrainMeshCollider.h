#pragma once
#include "pch.h"
#include "Mesh.h"
#include "IComponent.h"

struct AABB {
	AABB() = default;

	float minX, maxX, minZ, maxZ;

	void Expand(const Triangle& triangle) {
		minX = min(min(min(minX, triangle.v0.x), triangle.v1.x), triangle.v2.x);
		minZ = min(min(min(minZ, triangle.v0.z), triangle.v1.z), triangle.v2.z);

		maxX = max(max(max(maxX, triangle.v0.x), triangle.v1.x), triangle.v2.x);
		maxZ = max(max(max(maxZ, triangle.v0.z), triangle.v1.z), triangle.v2.z);
	}
};

struct KDNode {
	std::unique_ptr<KDNode> left = nullptr;
	std::unique_ptr<KDNode> right = nullptr;

	int depth = 0;
	AABB bound = {};

	std::vector<Triangle> triangles;
};

class TerrainMeshCollider :public IComponent {
public:
	TerrainMeshCollider() = default;
	~TerrainMeshCollider() {}
	TerrainMeshCollider(const TerrainMeshCollider&) = delete;

	virtual void Awake() {}
	virtual void Update(float deltaTime);

	void SetTriangles(std::vector<Triangle> triangles, XMMATRIX worldMatrix);

	std::unique_ptr<KDNode> BuildKDTree(std::vector<Triangle>& triangles, int depth, int maxDepth = 10, int minTriangles = 30);
	const KDNode* FindNode(const KDNode* node, const XMFLOAT3& position);

	std::unique_ptr<KDNode> mKDTree = {};
private:

};

#pragma once
#include "pch.h"
#include "Mesh.h"
#include "IComponent.h"

struct AABB {
	AABB() = default;

	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest();
	float minZ = std::numeric_limits<float>::max();
	float maxZ = std::numeric_limits<float>::lowest();

	void Expand(const Triangle& triangle) {
		minX = std::min(std::min(std::min(minX, triangle.v0.x), triangle.v1.x), triangle.v2.x);
		minZ = std::min(std::min(std::min(minZ, triangle.v0.z), triangle.v1.z), triangle.v2.z);

		maxX = std::max(std::max(std::max(maxX, triangle.v0.x), triangle.v1.x), triangle.v2.x);
		maxZ = std::max(std::max(std::max(maxZ, triangle.v0.z), triangle.v1.z), triangle.v2.z);
	}

	bool Intersect(const Triangle& triangle)
	{
		float triMinX = std::min({ triangle.v0.x, triangle.v1.x, triangle.v2.x });
		float triMaxX = std::max({ triangle.v0.x, triangle.v1.x, triangle.v2.x });
		float triMinZ = std::min({ triangle.v0.z, triangle.v1.z, triangle.v2.z });
		float triMaxZ = std::max({ triangle.v0.z, triangle.v1.z, triangle.v2.z });

		return !(triMaxX < minX || triMinX > maxX || triMaxZ < minZ || triMinZ > maxZ);

		//return (std::clamp(triangle.v0.x, minX, maxX) == triangle.v0.x && std::clamp(triangle.v0.z, minZ, maxZ) == triangle.v0.z) ||
		//	(std::clamp(triangle.v1.x, minX, maxX) == triangle.v1.x && std::clamp(triangle.v1.z, minZ, maxZ) == triangle.v1.z) ||
		//	(std::clamp(triangle.v2.x, minX, maxX) == triangle.v2.x && std::clamp(triangle.v2.z, minZ, maxZ) == triangle.v2.z);
	}
};

struct KDNode {
	std::unique_ptr<KDNode> left = nullptr;
	std::unique_ptr<KDNode> right = nullptr;

	int depth = 0;
	AABB bound = {};

	bool splitAxis = true; // true: x√‡, false: z√‡
	float splitValue = 0.0f;

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
	std::vector<Triangle> mTriangles = {};
private:
};

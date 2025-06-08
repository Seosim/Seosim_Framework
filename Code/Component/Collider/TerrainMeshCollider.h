#pragma once
#include "pch.h"
#include "Resource/Mesh.h"
#include "../IComponent.h"

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

	bool Intersect(const Triangle& triangle) const
	{
		float triMinX = std::min({ triangle.v0.x, triangle.v1.x, triangle.v2.x });
		float triMaxX = std::max({ triangle.v0.x, triangle.v1.x, triangle.v2.x });
		float triMinZ = std::min({ triangle.v0.z, triangle.v1.z, triangle.v2.z });
		float triMaxZ = std::max({ triangle.v0.z, triangle.v1.z, triangle.v2.z });

		return !(triMaxX < minX || triMinX > maxX || triMaxZ < minZ || triMinZ > maxZ);
	}

	bool Intersect(const XMFLOAT3& position) const
	{
		return !(position.x < minX || position.x > maxX || position.z < minZ || position.z > maxZ);
	}
};

struct TrisNode {
	std::unique_ptr<TrisNode> Left = nullptr;
	std::unique_ptr<TrisNode> Right = nullptr;

	int Depth = 0;
	AABB Bound = {};

	bool SplitAxis = true; // true: x√‡, false: z√‡
	float SplitValue = 0.0f;

	std::vector<Triangle> Triangles = {};
};

class TerrainMeshCollider :public IComponent {
public:
	TerrainMeshCollider() = default;
	~TerrainMeshCollider() {}
	TerrainMeshCollider(const TerrainMeshCollider&) = delete;

	virtual void Awake() {}
	virtual void Update(float deltaTime);

	void SetTriangles(std::vector<Triangle> triangles, XMMATRIX worldMatrix);

	std::unique_ptr<TrisNode> BuildTree(std::vector<Triangle>& triangles, int depth, int maxDepth = 10, int minTriangles = 30);
	const TrisNode* FindNode(const XMFLOAT3& position);
private:
	const TrisNode* findNode(const TrisNode* node, const XMFLOAT3& position);

	std::unique_ptr<TrisNode> mRoot = {};
};

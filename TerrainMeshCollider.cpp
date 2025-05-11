#include "pch.h"
#include "TerrainMeshCollider.h"

void TerrainMeshCollider::Update(float deltaTime)
{
}

void TerrainMeshCollider::SetTriangles(std::vector<Triangle> triangles, XMMATRIX worldMatrix)
{
	for (int i = 0; i < triangles.size(); ++i)
	{
		XMVECTOR v0 = XMVector3TransformCoord(XMLoadFloat3(&triangles[i].v0), worldMatrix);
		XMVECTOR v1 = XMVector3TransformCoord(XMLoadFloat3(&triangles[i].v1), worldMatrix);
		XMVECTOR v2 = XMVector3TransformCoord(XMLoadFloat3(&triangles[i].v2), worldMatrix);

		XMStoreFloat3(&triangles[i].v0, v0);
		XMStoreFloat3(&triangles[i].v1, v1);
		XMStoreFloat3(&triangles[i].v2, v2);
	}

	mKDTree = BuildKDTree(triangles, 0);
}

std::unique_ptr<KDNode> TerrainMeshCollider::BuildKDTree(std::vector<Triangle>& triangles, int depth, int maxDepth, int minTriangles)
{
	std::unique_ptr<KDNode> kdNode = std::make_unique<KDNode>();
	kdNode->depth = depth;

	AABB aabb = {};
	for (int i = 0; i < triangles.size(); ++i)
	{
		aabb.Expand(triangles[i]);
	}
	kdNode->bound = aabb;

	if (depth >= maxDepth || triangles.size() <= minTriangles) {
		kdNode->triangles = std::move(triangles);
		return kdNode;
	}

	bool axis = (depth & 1) == 0;	// true: x축, false: z축
	auto mid = triangles.begin() + triangles.size() / 2;

	std::sort(triangles.begin(), triangles.end(), [&](const Triangle& a, const Triangle& b)
		{
			if (axis)
				return (a.v0.x + a.v1.x + a.v2.x) / 3.0f < (b.v0.x + b.v1.x + b.v2.x) / 3.0f;
			else
				return (a.v0.z + a.v1.z + a.v2.z) / 3.0f < (b.v0.z + b.v1.z + b.v2.z) / 3.0f;
		});



	std::vector<Triangle> leftTris(triangles.begin(), mid);
	std::vector<Triangle> rightTris(mid, triangles.end());

	kdNode->left = BuildKDTree(leftTris, depth + 1, maxDepth, minTriangles);
	kdNode->right = BuildKDTree(rightTris, depth + 1, maxDepth, minTriangles);

	return kdNode;
}

const KDNode* TerrainMeshCollider::FindNode(const KDNode* node, const XMFLOAT3& position)
{
	ASSERT(node);

	const AABB& bound = node->bound;
	if (position.x < bound.minX || position.x > bound.maxX ||
		position.z < bound.minZ || position.z > bound.maxZ)
	{
		return nullptr;
	}

	if (!node->left && !node->right)
		return node;

	bool axis = (node->depth & 1) == 0; // true: x축, false: z축

	float splitValue = 0.0f;
	if (node->left && node->right) {
		if (axis)
			splitValue = (node->left->bound.maxX + node->left->bound.minX) * 0.5f;
		else
			splitValue = (node->left->bound.maxZ + node->left->bound.minZ) * 0.5f;
	}

	if (axis) {
		if (position.x < splitValue && node->left)
			return FindNode(node->left.get(), position);
		else if (node->right)
			return FindNode(node->right.get(), position);
	}
	else {
		if (position.z < splitValue && node->left)
			return FindNode(node->left.get(), position);
		else if (node->right)
			return FindNode(node->right.get(), position);
	}

	return node;
}

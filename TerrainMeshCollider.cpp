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

	mTriangles = triangles;
	mKDTree = BuildKDTree(triangles, 0);
}

std::unique_ptr<KDNode> TerrainMeshCollider::BuildKDTree(std::vector<Triangle>& triangles, int depth, int maxDepth, int minTriangles)
{
	std::unique_ptr<KDNode> kdNode = std::make_unique<KDNode>();
	kdNode->depth = depth;

	// 전체 AABB 계산
	AABB aabb = {};
	for (const auto& tri : triangles)
	{
		aabb.Expand(tri);
	}
	kdNode->bound = aabb;

	// 리프 노드 조건
	if (depth >= maxDepth || triangles.size() <= minTriangles) {
		kdNode->triangles = std::move(triangles);
		return kdNode;
	}

	// 분할 축 선택
	bool axis = (depth & 1) == 0; // true: x축, false: z축
	kdNode->splitAxis = axis;

	float splitValue = 0.0f;
	if (axis)
		splitValue = (aabb.minX + aabb.maxX) * 0.5f;
	else
		splitValue = (aabb.minZ + aabb.maxZ) * 0.5f;

	kdNode->splitValue = splitValue;

	// 좌우 Bound 생성
	AABB leftBound = aabb;
	AABB rightBound = aabb;

	if (axis) {
		leftBound.maxX = splitValue;
		rightBound.minX = splitValue;
	}
	else {
		leftBound.maxZ = splitValue;
		rightBound.minZ = splitValue;
	}

	// 삼각형 분배
	std::vector<Triangle> leftTris;
	std::vector<Triangle> rightTris;
	leftTris.reserve(triangles.size() / 2);
	rightTris.reserve(triangles.size() / 2);

	for (const auto& tri : triangles)
	{
		if (leftBound.Intersect(tri))
			leftTris.emplace_back(tri.v0, tri.v1, tri.v2);

		if (rightBound.Intersect(tri))
			rightTris.emplace_back(tri.v0, tri.v1, tri.v2);
	}

	// 재귀 분할
	kdNode->left = BuildKDTree(leftTris, depth + 1, maxDepth, minTriangles);
	kdNode->right = BuildKDTree(rightTris, depth + 1, maxDepth, minTriangles);

	return kdNode;
}

const KDNode* TerrainMeshCollider::FindNode(const KDNode* node, const XMFLOAT3& position)
{
	ASSERT(node);

	if (!node->left && !node->right)
		return node;

	if (node->splitAxis) { // x-axis
		if (position.x < node->splitValue)
			return FindNode(node->left.get(), position);
		else
			return FindNode(node->right.get(), position);
	}
	else { // z-axis
		if (position.z < node->splitValue)
			return FindNode(node->left.get(), position);
		else
			return FindNode(node->right.get(), position);
	}
}

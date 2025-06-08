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

	mRoot = BuildTree(triangles, 0);
}

std::unique_ptr<TrisNode> TerrainMeshCollider::BuildTree(std::vector<Triangle>& triangles, int depth, int maxDepth, int minTriangles)
{
	std::unique_ptr<TrisNode> node = std::make_unique<TrisNode>();
	node->Depth = depth;

	AABB aabb = {};
	for (const auto& tri : triangles)
	{
		aabb.Expand(tri);
	}
	node->Bound = aabb;

	// 리프 노드 조건
	if (depth >= maxDepth || triangles.size() <= minTriangles) {
		node->Triangles = std::move(triangles);
		return node;
	}

	// 분할 축 선택
	bool axis = (depth & 1) == 0; // true: x축, false: z축
	node->SplitAxis = axis;

	float splitValue = 0.0f;
	if (axis)
		splitValue = (aabb.minX + aabb.maxX) * 0.5f;
	else
		splitValue = (aabb.minZ + aabb.maxZ) * 0.5f;

	node->SplitValue = splitValue;

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

	node->Left = BuildTree(leftTris, depth + 1, maxDepth, minTriangles);
	node->Right = BuildTree(rightTris, depth + 1, maxDepth, minTriangles);

	return node;
}

const TrisNode* TerrainMeshCollider::FindNode(const XMFLOAT3& position)
{
	ASSERT(mRoot.get());

	if (mRoot->SplitAxis) { // x-axis
		if (position.x < mRoot->SplitValue)
			return findNode(mRoot->Left.get(), position);
		else
			return findNode(mRoot->Right.get(), position);
	}
	else { // z-axis
		if (position.z < mRoot->SplitValue)
			return findNode(mRoot->Left.get(), position);
		else
			return findNode(mRoot->Right.get(), position);
	}
}

const TrisNode* TerrainMeshCollider::findNode(const TrisNode* node, const XMFLOAT3& position)
{
	ASSERT(node);

	if (!node->Left && !node->Right)
		return node;

	if (false == node->Bound.Intersect(position))
		return nullptr;

	if (node->SplitAxis) // x-axis
	{ 
		if (position.x < node->SplitValue)
			return findNode(node->Left.get(), position);
		else
			return findNode(node->Right.get(), position);
	}
	else // z-axis
	{ 
		if (position.z < node->SplitValue)
			return findNode(node->Left.get(), position);
		else
			return findNode(node->Right.get(), position);
	}
}

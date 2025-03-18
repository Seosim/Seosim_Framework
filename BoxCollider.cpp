#include "pch.h"
#include "BoxCollider.h"

void BoxCollider::Initialize(CTransform* pTransform, const XMFLOAT3& size)
{
	mTransform = pTransform;

	XMFLOAT3 center = mTransform->GetWorldPosition();
	XMFLOAT4 rotation = mTransform->GetRotationQuat();

	mBox.Center = center;
	mBox.Orientation = rotation;
	mBox.Extents = size;
}

void BoxCollider::UpdateTransform()
{
	XMFLOAT3 center = mTransform->GetWorldPosition();
	XMFLOAT4 rotation = mTransform->GetRotationQuat();

	mBox.Center = center;
	mBox.Orientation = rotation;
}

bool BoxCollider::CollisionCheck(const BoxCollider& other)
{
	return mBox.Intersects(other.mBox);
}

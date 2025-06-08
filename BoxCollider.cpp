#include "pch.h"
#include "BoxCollider.h"

void BoxCollider::Update(const float deltaTime)
{
	UpdateTransform();
}

void BoxCollider::Initialize(Transform* pTransform, const XMFLOAT3& size, RigidBody* pRigidBody)
{
	mTransform = pTransform;
	mRigidBody = pRigidBody;

	UpdateTransform();
	mBox.Extents = size;
}

void BoxCollider::UpdateTransform()
{
	XMFLOAT3 center = mTransform->GetPosition();
	XMFLOAT4 rotation = mTransform->GetRotationQuat();

	mBox.Center = center;
	mBox.Orientation = rotation;
}

bool BoxCollider::CollisionCheck(const BoxCollider& other)
{
	if (true == mBox.Intersects(other.mBox))
	{
		if (nullptr != mRigidBody)
		{
			mRigidBody->AddColliderTransform(other.mTransform);
		}

		return true;
	}

	return false;
}

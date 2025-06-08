#include "pch.h"
#include "RigidBody.h"

void RigidBody::Update(const float deltaTime)
{
	UpdatePhysics(deltaTime);
}

void RigidBody::UpdatePhysics(const float deltaTime)
{
	if (UseGravity)
	{
		XMFLOAT3 gravity = { 0.0f, -9.8f * mGravityScale, 0.0f };
		AddForce(gravity);
	}

	XMVECTOR vVel = XMLoadFloat3(&mVelocity);
	XMVECTOR vAcc = XMLoadFloat3(&mAcceleration);

	vVel += vAcc * deltaTime;

	// 드래그 적용
	vVel *= std::max(1.0f - mDrag * deltaTime, 0.0f);

	// 위치 갱신
	XMFLOAT3 position = mTransform->GetLocalPosition();
	XMVECTOR vPosition = XMLoadFloat3(&position);
	vPosition += vVel * deltaTime;

	XMStoreFloat3(&position, vPosition);
	XMStoreFloat3(&mVelocity, vVel);
	mTransform->SetPosition(position);


	mAcceleration = XMFLOAT3(0, 0, 0);
	mCollisionTransforms.clear();
}

void RigidBody::AddForce(const XMFLOAT3& force)
{
	XMVECTOR vAcc = XMLoadFloat3(&mAcceleration);
	XMVECTOR vForce = XMLoadFloat3(&force);

	vAcc += vForce;

	XMStoreFloat3(&mAcceleration, vAcc);
}

void RigidBody::AddImpulse(const XMFLOAT3 force)
{
	XMVECTOR vVel = XMLoadFloat3(&mVelocity);
	XMVECTOR vImpulse = XMLoadFloat3(&force);
	vVel += vImpulse;

	XMStoreFloat3(&mVelocity, vVel);
}

void RigidBody::SetTransform(Transform* pTransform)
{
	mTransform = pTransform;
}

Transform* RigidBody::GetTransform() const
{
	return mTransform;
}

bool RigidBody::IsColliding() const
{
	return mCollisionTransforms.size();
}

void RigidBody::AddColliderTransform(Transform* pTransform)
{
	mCollisionTransforms.push_back(pTransform);
}
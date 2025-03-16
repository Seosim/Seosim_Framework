#include "pch.h"
#include "RigidBody.h"

void RigidBody::UpdatePhysics(const float deltaTime)
{
	XMFLOAT3 position = mTransform->GetPosition();

	XMVECTOR vVel = XMLoadFloat3(&mVelocity);
	XMVECTOR vAcc = XMLoadFloat3(&mAcceleration);
	XMVECTOR vPosition = XMLoadFloat3(&position);

	// 최대 속도 제한
	float maxSpeed = 10.0f;
	float speed = XMVectorGetX(XMVector3Length(vVel));
	if (speed > maxSpeed)
	{
		vVel = XMVector3Normalize(vVel) * maxSpeed;
	}

	vVel += vAcc * deltaTime;
	vVel *= max(1.0f - (mDrag * deltaTime), 0.0f);
	vPosition += vVel;

	XMStoreFloat3(&mVelocity, vVel);
	XMStoreFloat3(&position, vPosition);

	mTransform->SetPosition(position);

	mAcceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
}

void RigidBody::AddForce(const XMFLOAT3& force)
{
	XMVECTOR vAcc = XMLoadFloat3(&mAcceleration);
	XMVECTOR vForce = XMLoadFloat3(&force);

	vAcc += vForce;

	XMStoreFloat3(&mAcceleration, vAcc);
}

void RigidBody::SetTransform(CTransform* pTransform)
{
	mTransform = pTransform;
}

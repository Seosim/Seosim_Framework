#include "pch.h"
#include "RigidBody.h"

void RigidBody::Update(const float deltaTime)
{
	UpdatePhysics(deltaTime);
}

void RigidBody::UpdatePhysics(const float deltaTime)
{
	XMFLOAT3 position = mTransform->GetLocalPosition();

	XMVECTOR vVel = XMLoadFloat3(&mVelocity);
	XMVECTOR vAcc = XMLoadFloat3(&mAcceleration);
	XMVECTOR vPosition = XMLoadFloat3(&position);

	//// 최대 속도 제한
	//float maxSpeed = 100.0f;
	//float speed = XMVectorGetX(XMVector3Length(vVel));
	//if (speed > maxSpeed)
	//{
	//	vVel = XMVector3Normalize(vVel) * maxSpeed;
	//}

	//// 중력 적용 (y축 방향으로 -9.8)
	//XMVECTOR gravity = XMVectorSet(0.0f, -9.81f * deltaTime, 0.0f, 0.0f);
	//vAcc += gravity;

	vVel += vAcc * deltaTime;
	vVel *= std::max(1.0f - (mDrag * deltaTime), 0.0f);

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

void RigidBody::SetTransform(Transform* pTransform)
{
	mTransform = pTransform;
}

Transform* RigidBody::GetTransform() const
{
	return mTransform;
}

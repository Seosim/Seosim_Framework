#include "pch.h"
#include "RigidBody.h"

void RigidBody::UpdatePhysics(const float deltaTime)
{
	XMFLOAT3 position = mTransform->GetLocalPosition();

	XMVECTOR vVel = XMLoadFloat3(&mVelocity);
	XMVECTOR vAcc = XMLoadFloat3(&mAcceleration);
	XMVECTOR vPosition = XMLoadFloat3(&position);

	// �ִ� �ӵ� ����
	float maxSpeed = 100.0f;
	float speed = XMVectorGetX(XMVector3Length(vVel));
	if (speed > maxSpeed)
	{
		vVel = XMVector3Normalize(vVel) * maxSpeed;
	}

	//// �߷� ���� (y�� �������� -9.81 m/s��)
	//XMVECTOR gravity = XMVectorSet(0.0f, -9.81f * deltaTime, 0.0f, 0.0f);
	//vAcc += gravity;
	mDrag = 5.0f;
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

void RigidBody::SetTransform(Transform* pTransform)
{
	mTransform = pTransform;
}

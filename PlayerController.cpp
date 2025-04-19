#include "pch.h"
#include "Input.h"
#include "PlayerController.h"

void PlayerController::Update(const float deltaTime)
{
	Transform* transform = mRigidBody->GetTransform();

	if (Input::Instance().GetKey('W'))
	{
		XMFLOAT3 forwardVector;
		XMStoreFloat3(&forwardVector, transform->GetForwardVector() * mSpeed * deltaTime);
		mRigidBody->AddForce(forwardVector);
	}
	if (Input::Instance().GetKey('S'))
	{
		XMFLOAT3 backVector;
		XMStoreFloat3(&backVector, transform->GetForwardVector() * -mSpeed * deltaTime);
		mRigidBody->AddForce(backVector);
	}
	if (Input::Instance().GetKey('A'))
	{
		XMFLOAT3 leftVector;
		XMStoreFloat3(&leftVector, transform->GetRightVector() * -mSpeed * deltaTime);
		mRigidBody->AddForce(leftVector);
	}
	if (Input::Instance().GetKey('D'))
	{
		XMFLOAT3 rightVector;
		XMStoreFloat3(&rightVector, transform->GetRightVector() * mSpeed * deltaTime);
		mRigidBody->AddForce(rightVector);
	}
}

void PlayerController::SetRigidBody(RigidBody* rigidBody)
{
	mRigidBody = rigidBody;
}

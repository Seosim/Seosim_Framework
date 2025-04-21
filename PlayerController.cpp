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

	if (Input::Instance().GetKey(VK_LEFT))
	{
		transform->Rotate(0.0f, -1.0f, 0.0f);
	}
	if (Input::Instance().GetKey(VK_RIGHT))
	{
		transform->Rotate(0.0f, 1.0f, 0.0f);
	}
	if (Input::Instance().GetKey(VK_UP))
	{
		transform->Rotate(-1.0f, 0.0f, 0.0f);
	}
	if (Input::Instance().GetKey(VK_DOWN))
	{
		transform->Rotate(1.0f, 0.0f, 0.0f);
	}
}

void PlayerController::SetRigidBody(RigidBody* rigidBody)
{
	mRigidBody = rigidBody;
}

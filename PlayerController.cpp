#include "pch.h"
#include "Input.h"
#include "PlayerController.h"

void PlayerController::Update(const float deltaTime)
{
	Transform* transform = mRigidBody->GetTransform();

	XMFLOAT2 mouseDelta = Input::Instance().GetMouseDelta();

	mYaw += mouseDelta.x * mMouseSensitivity;
	mPitch += -mouseDelta.y * mMouseSensitivity;

	mPitch = std::clamp(mPitch, -89.0f, 89.0f);

	transform->SetRotatiton({ 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });
	transform->RotateByWorldAxis(0.0f, mYaw, 0.0f);
	transform->Rotate(mPitch, 0.0f, 0.0f);

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

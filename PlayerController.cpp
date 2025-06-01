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
		XMVECTOR forward = transform->GetForwardVector();
		forward = XMVectorSetY(forward, 0.0f);
		forward = XMVector3Normalize(forward);

		XMFLOAT3 forwardVector;
		XMStoreFloat3(&forwardVector, forward * mSpeed);
		mRigidBody->AddForce(forwardVector);
	}

	if (Input::Instance().GetKey('S'))
	{
		XMVECTOR back = transform->GetForwardVector();
		back = XMVectorSetY(back, 0.0f);
		back = XMVector3Normalize(back);

		XMFLOAT3 backVector;
		XMStoreFloat3(&backVector, back * -mSpeed);
		mRigidBody->AddForce(backVector);
	}

	if (Input::Instance().GetKey('A'))
	{
		XMVECTOR left = transform->GetRightVector();
		left = XMVectorSetY(left, 0.0f);
		left = XMVector3Normalize(left);

		XMFLOAT3 leftVector;
		XMStoreFloat3(&leftVector, left * -mSpeed);
		mRigidBody->AddForce(leftVector);
	}

	if (Input::Instance().GetKey('D'))
	{
		XMVECTOR right = transform->GetRightVector();
		right = XMVectorSetY(right, 0.0f);
		right = XMVector3Normalize(right);

		XMFLOAT3 rightVector;
		XMStoreFloat3(&rightVector, right * mSpeed);
		mRigidBody->AddForce(rightVector);
	}
	if (!mbJumping && Input::Instance().GetKey(VK_SPACE))
	{
		XMFLOAT3 upVector;
		constexpr float JUMP_POWER = 30.0f;
		XMStoreFloat3(&upVector, XMVectorSet(0, 1, 0, 0) * JUMP_POWER);
		mRigidBody->AddImpulse(upVector);
		mRigidBody->IsGround = false;
		mbJumping = true;
	}
}

void PlayerController::SetRigidBody(RigidBody* rigidBody)
{
	mRigidBody = rigidBody;
}

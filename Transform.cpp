#include "pch.h"
#include "GameObject.h"
#include "Transform.h"

XMMATRIX Transform::GetLocalTransform() const
{
	XMFLOAT4X4 rotationTranslation =
	{
		mRight.x, mRight.y, mRight.z, 0.0f,
		mUp.x, mUp.y, mUp.z, 0.0f,
		mForward.x, mForward.y, mForward.z, 0.0f,
		mPosition.x, mPosition.y, mPosition.z, 1.0f,
	};

	XMMATRIX localTransform = XMMatrixScaling(mScale.x, mScale.y, mScale.z) * XMLoadFloat4x4(&rotationTranslation);
	return localTransform;
}

XMMATRIX Transform::GetWorldTransform() const
{

	return GetLocalTransform() * getLocalToWorldTransform();
}

void Transform::SetPosition(const XMFLOAT3& position, Space space)
{
	if (space == Space::Local)
	{
		mPosition = position;
	}
	else if(space == Space::World)
	{
		XMMATRIX localToGlobal = getLocalToWorldTransform();
		XMMATRIX globalToLocal = XMMatrixInverse(nullptr, localToGlobal);

		XMVECTOR vPosition = XMLoadFloat3(&position);
		XMVECTOR localPosition = XMVector3TransformCoord(vPosition, globalToLocal);

		XMStoreFloat3(&mPosition, localPosition);
	}
}

void Transform::SetScale(const XMFLOAT3& scale)
{
	mScale = scale;
}

void Transform::Rotate(const float angleX, const float angleY, const float angleZ)
{
	XMVECTOR right = XMLoadFloat3(&mRight);
	XMVECTOR up = XMLoadFloat3(&mUp);
	XMVECTOR forward = XMLoadFloat3(&mForward);

	if (angleX != 0.0f)
	{
		XMMATRIX rotation = XMMatrixRotationAxis(right, XMConvertToRadians(angleX));
		up = XMVector3TransformNormal(up, rotation);
		forward = XMVector3TransformNormal(forward, rotation);
	}

	if (angleY != 0.0f)
	{
		XMMATRIX rotation = XMMatrixRotationAxis(up, XMConvertToRadians(angleY));
		right = XMVector3TransformNormal(right, rotation);
		forward = XMVector3TransformNormal(forward, rotation);
	}

	if (angleZ != 0.0f)
	{
		XMMATRIX rotation = XMMatrixRotationAxis(forward, XMConvertToRadians(angleZ));
		right = XMVector3TransformNormal(right, rotation);
		up = XMVector3TransformNormal(up, rotation);
	}

	forward = XMVector3Normalize(forward);
	XMStoreFloat3(&mForward, forward);

	right = XMVector3Normalize(XMVector3Cross(up, forward));
	XMStoreFloat3(&mRight, right);

	up = XMVector3Normalize(XMVector3Cross(forward, right));
	XMStoreFloat3(&mUp, up);
}

void Transform::RotateByWorldAxis(const float angleX, const float angleY, const float angleZ)
{
	XMVECTOR right = XMLoadFloat3(&mRight);
	XMVECTOR up = XMLoadFloat3(&mUp);
	XMVECTOR forward = XMLoadFloat3(&mForward);
	//XMVECTOR position = XMLoadFloat3(&mPosition);

	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(angleX), XMConvertToRadians(angleY), XMConvertToRadians(angleZ));
	right = XMVector3TransformNormal(right, rotation);
	up = XMVector3TransformNormal(up, rotation);
	forward = XMVector3TransformNormal(forward, rotation);
	//position = XMVector3TransformCoord(position, rotation);

	forward = XMVector3Normalize(forward);
	XMStoreFloat3(&mForward, forward);

	right = XMVector3Normalize(XMVector3Cross(up, forward));
	XMStoreFloat3(&mRight, right);

	up = XMVector3Normalize(XMVector3Cross(forward, right));
	XMStoreFloat3(&mUp, up);

	//XMStoreFloat3(&mPosition, position);
}


//void Transform::RotateByWorldAxis(const float angleX, const float angleY, const float angleZ)
//{
//	XMVECTOR right = XMLoadFloat3(&mRight);
//	XMVECTOR up = XMLoadFloat3(&mUp);
//	XMVECTOR forward = XMLoadFloat3(&mForward);
//
//	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(angleX), XMConvertToRadians(angleY), XMConvertToRadians(angleZ));
//	right = XMVector3TransformNormal(right, rotation);
//	up = XMVector3TransformNormal(up, rotation);
//	forward = XMVector3TransformNormal(forward, rotation);
//
//	forward = XMVector3Normalize(forward);
//	XMStoreFloat3(&mForward, forward);
//
//	right = XMVector3Normalize(XMVector3Cross(up, forward));
//	XMStoreFloat3(&mRight, right);
//
//	up = XMVector3Normalize(XMVector3Cross(forward, right));
//	XMStoreFloat3(&mUp, up);
//}

void Transform::SetRotatiton(const XMFLOAT3& right, const XMFLOAT3& up, const XMFLOAT3& forward)
{
	mRight = right;
	mUp = up;
	mForward = forward;
}

void Transform::SetRotationByQuat(const XMVECTOR quat)
{
	XMFLOAT4X4 rotation = {};
	XMStoreFloat4x4(&rotation, XMMatrixRotationQuaternion(quat));

	mRight.x = rotation._11;
	mRight.y = rotation._12;
	mRight.z = rotation._13;

	mUp.x = rotation._21;
	mUp.y = rotation._22;
	mUp.z = rotation._23;

	mForward.x = rotation._31;
	mForward.y = rotation._32;
	mForward.z = rotation._33;
}

GameObject* Transform::GetParent() const
{
	return mParent;
}

void Transform::SetParent(GameObject* parent)
{
	mParent = parent;
}

XMMATRIX Transform::getLocalToWorldTransform() const
{
	XMMATRIX global = XMMatrixIdentity();

	GameObject* parent = mParent;

	while (parent != nullptr)
	{
		global *= parent->GetTransform().GetLocalTransform();
		parent = parent->GetTransform().GetParent();
	}

	return global;
}

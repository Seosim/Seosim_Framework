#include "pch.h"
#include "GameObject.h"
#include "Transform.h"

void Transform::Update(const float deltaTime)
{

}

//Component Transform
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

XMFLOAT3 Transform::GetLocalPosition() const
{
	return mPosition;
}

XMFLOAT3 Transform::GetPosition() const
{
	auto matrix = GetWorldTransform();
	XMFLOAT3 worldPosition = { matrix.r[3].m128_f32[0] , matrix.r[3].m128_f32[1], matrix.r[3].m128_f32[2] };

	return worldPosition;
}

void Transform::SetPosition(const XMFLOAT3& position, Space space)
{
	if (space == Space::Local)
	{
		mPosition = position;
	}
	else if (space == Space::World)
	{
		XMMATRIX localToGlobal = getLocalToWorldTransform();
		XMMATRIX globalToLocal = XMMatrixInverse(nullptr, localToGlobal);

		XMVECTOR vPosition = XMLoadFloat3(&position);
		XMVECTOR localPosition = XMVector3TransformCoord(vPosition, globalToLocal);

		XMStoreFloat3(&mPosition, localPosition);
	}
}

void Transform::Move(const XMFLOAT3& velocity, Space space)
{
	XMVECTOR vPosition = XMLoadFloat3(&mPosition);
	XMVECTOR vVelocity = XMLoadFloat3(&velocity);
	vPosition += vVelocity;

	XMStoreFloat3(&mPosition, vPosition);

	return;
	if (space == Space::Local)
	{
		vPosition += vVelocity;
	}
	else if (space == Space::World)
	{
		XMMATRIX localToGlobal = getLocalToWorldTransform();
		XMMATRIX globalToLocal = XMMatrixInverse(nullptr, localToGlobal);

		XMVECTOR vGlobalPosition = XMVector3TransformCoord(vPosition, localToGlobal);
		XMVECTOR vGlobalVelocity = XMVector3TransformCoord(vVelocity, localToGlobal);

		vGlobalPosition += vGlobalVelocity;

		XMVECTOR localPosition = XMVector3TransformCoord(vGlobalPosition, globalToLocal);

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

XMFLOAT3 Transform::GetRight() const
{
	auto matrix = GetWorldTransform();
	return { matrix.r[0].m128_f32[0], matrix.r[0].m128_f32[1], matrix.r[0].m128_f32[2] };
}

XMFLOAT3 Transform::GetUp() const
{
	auto matrix = GetWorldTransform();
	return { matrix.r[1].m128_f32[0], matrix.r[1].m128_f32[1], matrix.r[1].m128_f32[2] };
}

XMFLOAT3 Transform::GetForward() const
{
	auto matrix = GetWorldTransform();
	XMFLOAT3 worldForward = { matrix.r[2].m128_f32[0], matrix.r[2].m128_f32[1], matrix.r[2].m128_f32[2] };

	return worldForward;
}

XMVECTOR Transform::GetRightVector() const
{
	return GetWorldTransform().r[0];
}

XMVECTOR Transform::GetUpVector() const
{
	return GetWorldTransform().r[1];
}

XMVECTOR Transform::GetForwardVector() const
{
	return GetWorldTransform().r[2];
}

void Transform::SetRotatiton(const XMFLOAT3& right, const XMFLOAT3& up, const XMFLOAT3& forward)
{
	mRight = right;
	mUp = up;
	mForward = forward;
}

XMFLOAT4 Transform::GetRotationQuat() const
{
	auto matrix = GetWorldTransform();

	XMVECTOR quat = XMQuaternionRotationMatrix(matrix);
	XMFLOAT4 result;
	XMStoreFloat4(&result, quat);

	return result;
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

GameObject* Transform::GetChild(int index) const
{
	return mChildren[index];
}

void Transform::AddChild(GameObject* child)
{
	mChildren.push_back(child);
}

XMMATRIX Transform::getLocalToWorldTransform() const
{
	XMMATRIX global = XMMatrixIdentity();

	GameObject* parent = mParent;

	while (parent != nullptr)
	{
		global *= parent->GetComponent<Transform>().GetLocalTransform();
		parent = parent->GetComponent<Transform>().GetParent();
	}

	return global;
}

void Transform::SetTransformData(const XMFLOAT3& position, const XMFLOAT4 rotation, const XMFLOAT3& scale)
{
	SetPosition(position);
	SetRotationByQuat(XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w));
	SetScale(scale);
}

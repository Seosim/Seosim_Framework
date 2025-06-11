#include "pch.h"
#include "Core/GameObject.h"
#include "Transform.h"

void Transform::Update(const float deltaTime)
{

}

//Component Transform
XMMATRIX Transform::GetLocalTransform() const
{
	if (mIsDirty)
	{
		XMFLOAT4X4 rotationTranslation =
		{
			mRight.x, mRight.y, mRight.z, 0.0f,
			mUp.x, mUp.y, mUp.z, 0.0f,
			mForward.x, mForward.y, mForward.z, 0.0f,
			mPosition.x, mPosition.y, mPosition.z, 1.0f,
		};

		mLocalTransform = XMMatrixScaling(mScale.x, mScale.y, mScale.z) * XMLoadFloat4x4(&rotationTranslation);
	}

	return mLocalTransform;
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

	mIsDirty = true;
	for (GameObject* child : mChildren)
		child->GetComponent<Transform>().MarkDirty();
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

	mIsDirty = true;
	for (GameObject* child : mChildren)
		child->GetComponent<Transform>().MarkDirty();
}

void Transform::SetScale(const XMFLOAT3& scale)
{
	mScale = scale;

	mIsDirty = true;
	for (GameObject* child : mChildren)
		child->GetComponent<Transform>().MarkDirty();
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

	mIsDirty = true;
	for (GameObject* child : mChildren)
		child->GetComponent<Transform>().MarkDirty();
}

void Transform::RotateByWorldAxis(const float angleX, const float angleY, const float angleZ)
{
	// 월드 기준 축
	XMVECTOR worldX = XMVectorSet(1, 0, 0, 0);
	XMVECTOR worldY = XMVectorSet(0, 1, 0, 0);
	XMVECTOR worldZ = XMVectorSet(0, 0, 1, 0);

	// 회전 행렬을 월드 기준으로 각각 만듦
	XMMATRIX rotX = XMMatrixRotationAxis(worldX, XMConvertToRadians(angleX));
	XMMATRIX rotY = XMMatrixRotationAxis(worldY, XMConvertToRadians(angleY));
	XMMATRIX rotZ = XMMatrixRotationAxis(worldZ, XMConvertToRadians(angleZ));

	// 현재 방향 벡터 가져옴
	XMVECTOR right = XMLoadFloat3(&mRight);
	XMVECTOR up = XMLoadFloat3(&mUp);
	XMVECTOR forward = XMLoadFloat3(&mForward);

	// 월드 기준 회전을 누적 적용 (Z → Y → X 순서, 필요시 순서 변경 가능)
	right = XMVector3TransformNormal(right, rotZ);
	right = XMVector3TransformNormal(right, rotY);
	right = XMVector3TransformNormal(right, rotX);

	up = XMVector3TransformNormal(up, rotZ);
	up = XMVector3TransformNormal(up, rotY);
	up = XMVector3TransformNormal(up, rotX);

	forward = XMVector3TransformNormal(forward, rotZ);
	forward = XMVector3TransformNormal(forward, rotY);
	forward = XMVector3TransformNormal(forward, rotX);

	// 직교화
	forward = XMVector3Normalize(forward);
	right = XMVector3Normalize(XMVector3Cross(up, forward));
	up = XMVector3Normalize(XMVector3Cross(forward, right));

	// 저장
	XMStoreFloat3(&mRight, right);
	XMStoreFloat3(&mUp, up);
	XMStoreFloat3(&mForward, forward);

	mIsDirty = true;
	for (GameObject* child : mChildren)
		child->GetComponent<Transform>().MarkDirty();
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

XMFLOAT3 Transform::GetScale() const
{
	XMFLOAT3 worldScale = mScale;

	GameObject* parent = mParent;

	while (parent != nullptr)
	{
		XMFLOAT3 parentScale = parent->GetComponent<Transform>().mScale;

		worldScale.x *= parentScale.x;
		worldScale.y *= parentScale.y;
		worldScale.z *= parentScale.z;

		parent = parent->GetComponent<Transform>().GetParent();
	}

	return worldScale;
}

void Transform::SetRotatiton(const XMFLOAT3& right, const XMFLOAT3& up, const XMFLOAT3& forward)
{
	mRight = right;
	mUp = up;
	mForward = forward;

	mIsDirty = true;
	for (GameObject* child : mChildren)
		child->GetComponent<Transform>().MarkDirty();
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

	mIsDirty = true;
	for (GameObject* child : mChildren)
		child->GetComponent<Transform>().MarkDirty();
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
	ASSERT(index < mChildren.size());
	return mChildren[index];
}

void Transform::AddChild(GameObject* child)
{
	mChildren.push_back(child);
}

XMMATRIX Transform::getLocalToWorldTransform() const
{
	if (mIsDirty) {
		updateLocalToWorldTransform();
	}
	return mLocalToWorldTransform;
}

void Transform::updateLocalToWorldTransform() const
{
	mLocalToWorldTransform = XMMatrixIdentity();
	XMMATRIX parentWorld = XMMatrixIdentity();

	if (mParent) {
		parentWorld = mParent->GetComponent<Transform>().GetWorldTransform();
	}

	mLocalToWorldTransform *= parentWorld;
	mIsDirty = false;
}

void Transform::SetTransformData(const XMFLOAT3& position, const XMFLOAT4 rotation, const XMFLOAT3& scale)
{
	SetPosition(position);
	SetRotationByQuat(XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w));
	SetScale(scale);
}

void Transform::MarkDirty() const
{
	mIsDirty = true;
	for (GameObject* child : mChildren)
		child->GetComponent<Transform>().MarkDirty();
}

bool Transform::IsStatic() const
{
	return mIsStatic;
}

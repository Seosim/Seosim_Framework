#pragma once
#include "pch.h"

class GameObject;

class Transform final {
public:
	XMMATRIX GetLocalTransform() const;
	XMMATRIX GetWorldTransform() const;

	void SetPosition(XMFLOAT3 position);

	void Rotate(const float angleX, const float angleY, const float angleZ);
	void RotateByWorldAxis(const float angleX, const float angleY, const float angleZ);

	void SetRotatiton(const XMFLOAT3& right, const XMFLOAT3& up, const XMFLOAT3& forward);
	void SetRotationByQuat(const XMVECTOR quat);

	GameObject* GetParent() const;
	void SetParent(GameObject* parent);
private:
	XMFLOAT4X4 mLocalTransform = MATRIX::Identify4x4();

	XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 mForward = { 0.0f, 0.0f, 1.0f };
	XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };

	GameObject* mParent = nullptr;
};
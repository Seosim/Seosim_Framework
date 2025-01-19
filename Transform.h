#pragma once

#include "pch.h"

#include "IComponent.h"

class GameObject;

class CTransform : public IComponent {
public:
	CTransform() = default;
	~CTransform() {

	}

	virtual void Awake() {}

	enum class Space
	{
		World,
		Local
	};

	XMMATRIX GetLocalTransform() const;
	XMMATRIX GetWorldTransform() const;

	void SetPosition(const XMFLOAT3& position, Space space = Space::Local);
	void SetScale(const XMFLOAT3& scale);

	void Rotate(const float angleX, const float angleY, const float angleZ);
	void RotateByWorldAxis(const float angleX, const float angleY, const float angleZ);

	void SetRotatiton(const XMFLOAT3& right, const XMFLOAT3& up, const XMFLOAT3& forward);
	void SetRotationByQuat(const XMVECTOR quat);

	GameObject* GetParent() const;
	void SetParent(GameObject* parent);
	XMMATRIX getLocalToWorldTransform() const;

	void SetTransformData(const XMFLOAT3& position, const XMFLOAT4 rotation, const XMFLOAT3& scale);
private:
	XMFLOAT4X4 mLocalTransform = MATRIX::Identify4x4();

	XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 mForward = { 0.0f, 0.0f, 1.0f };
	XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 mScale = { 1.0f, 1.0f, 1.0f };

	GameObject* mParent = nullptr;
};


class Transform final {
public:
	enum class Space
	{
		World,
		Local
	};

	XMMATRIX GetLocalTransform() const;
	XMMATRIX GetWorldTransform() const;

	void SetPosition(const XMFLOAT3& position, Space space = Space::Local);
	void SetScale(const XMFLOAT3& scale);

	void Rotate(const float angleX, const float angleY, const float angleZ);
	void RotateByWorldAxis(const float angleX, const float angleY, const float angleZ);

	void SetRotatiton(const XMFLOAT3& right, const XMFLOAT3& up, const XMFLOAT3& forward);
	void SetRotationByQuat(const XMVECTOR quat);

	GameObject* GetParent() const;
	void SetParent(GameObject* parent);
	XMMATRIX getLocalToWorldTransform() const;
private:

	XMFLOAT4X4 mLocalTransform = MATRIX::Identify4x4();

	XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 mForward = { 0.0f, 0.0f, 1.0f };
	XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 mScale = { 1.0f, 1.0f, 1.0f };

	GameObject* mParent = nullptr;
};
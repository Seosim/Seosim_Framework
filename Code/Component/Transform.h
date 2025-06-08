#pragma once

#include "pch.h"

#include "IComponent.h"

class GameObject;

class Transform final : public IComponent {
public:
	Transform() = default;
	~Transform() {

	}

	virtual void Awake() {}

	virtual void Update(const float deltaTime);

	enum class Space
	{
		World,
		Local
	};

	XMMATRIX GetLocalTransform() const;
	XMMATRIX GetWorldTransform() const;

	XMFLOAT3 GetLocalPosition() const;
	XMFLOAT3 GetPosition() const;
	void SetPosition(const XMFLOAT3& position, Space space = Space::Local);
	void Move(const XMFLOAT3& velocity, Space space = Space::World);

	void SetScale(const XMFLOAT3& scale);

	void Rotate(const float angleX, const float angleY, const float angleZ);
	void RotateByWorldAxis(const float angleX, const float angleY, const float angleZ);

	XMFLOAT3 GetRight() const;
	XMFLOAT3 GetUp() const;
	XMFLOAT3 GetForward() const;

	XMVECTOR GetRightVector() const;
	XMVECTOR GetUpVector() const;
	XMVECTOR GetForwardVector() const;

	XMFLOAT3 GetScale() const;

	void SetRotatiton(const XMFLOAT3& right, const XMFLOAT3& up, const XMFLOAT3& forward);
	XMFLOAT4 GetRotationQuat() const;
	void SetRotationByQuat(const XMVECTOR quat);

	GameObject* GetParent() const;
	void SetParent(GameObject* parent);

	GameObject* GetChild(int index = 0) const;
	void AddChild(GameObject* child);

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
	std::vector<GameObject*> mChildren;
};
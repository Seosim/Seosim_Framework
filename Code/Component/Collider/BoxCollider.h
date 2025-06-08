#pragma once
#include "../IComponent.h"
#include "../Transform.h"
#include "../RigidBody.h"
#include "pch.h"

class BoxCollider final : public IComponent {
public:
	BoxCollider() = default;
	~BoxCollider() {}
	BoxCollider(const BoxCollider&) = delete;

	virtual void Awake() {};

	virtual void Update(const float deltaTime);

	void Initialize(Transform* pTransform, const XMFLOAT3& size, RigidBody* pRigidBody = nullptr);

	void UpdateTransform();

	bool CollisionCheck(const BoxCollider& other);
private:
	BoundingOrientedBox mBox = {};
	Transform* mTransform = nullptr;
	RigidBody* mRigidBody = nullptr;
};
#pragma once
#include "IComponent.h"
#include "Transform.h"
#include "pch.h"

class RigidBody final : public IComponent {
public:
	RigidBody() = default;
	~RigidBody() {}
	RigidBody(const RigidBody&) = delete;

	virtual void Awake() {}
	

	void UpdatePhysics(const float deltaTime);

	void AddForce(const XMFLOAT3& force);

	void SetTransform(CTransform* pTransform);
private:
	CTransform* mTransform = nullptr;
	XMFLOAT3 mVelocity = {};
	XMFLOAT3 mAcceleration = {};

	float mDrag = 1.0f;
};
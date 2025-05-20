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
	
	virtual void Update(const float deltaTime);

	void UpdatePhysics(const float deltaTime);

	void AddForce(const XMFLOAT3& force);

	void SetTransform(Transform* pTransform);
	Transform* GetTransform() const;
private:
	Transform* mTransform = nullptr;
	XMFLOAT3 mVelocity = {};
	XMFLOAT3 mAcceleration = {};

	float mDrag = 5.0f;
	float mGravityScale = 0.05f;
public:
	XMFLOAT3 mGravityAcceleration = {};
	bool UseGravity = false;
};
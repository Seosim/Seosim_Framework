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
	void AddImpulse(const XMFLOAT3 force);

	void SetTransform(Transform* pTransform);
	Transform* GetTransform() const;

	bool IsColliding() const;
	void AddColliderTransform(Transform* pTransform);
private:
	XMFLOAT3 computeMTV(const XMFLOAT3& minA, const XMFLOAT3& maxA, const XMFLOAT3& minB, const XMFLOAT3& maxB);
private:
	Transform* mTransform = nullptr;
	XMFLOAT3 mVelocity = {};
	XMFLOAT3 mAcceleration = {};

	float mDrag = 5.0f;
	float mGravityScale = 10.0f;
	
	std::vector<const Transform*> mCollisionTransforms;
public:
	XMFLOAT3 mGravityAcceleration = {};
	bool UseGravity = false;
	bool IsGround = true;
};
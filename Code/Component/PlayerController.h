#pragma once
#include "pch.h"
#include "RigidBody.h"

class PlayerController final : public IComponent {
public:
	PlayerController() = default;
	~PlayerController() {}
	PlayerController(const PlayerController&) = delete;

	virtual void Awake() {}

	virtual void Update(const float deltaTime);

	void SetRigidBody(RigidBody* rigidBody);
private:
	RigidBody* mRigidBody = nullptr;
	float mSpeed = 50.0f;

	//Mouse
	float mYaw = 0.0f;
	float mPitch = 0.0f;
	float mMouseSensitivity = 0.15f;
public:
	//Jump
	bool mbJumping = false;
	//bool mbIsGround = false;
};
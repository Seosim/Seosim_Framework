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
	float mSpeed = 20.0f;
};
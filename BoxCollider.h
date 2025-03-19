#pragma once
#include "IComponent.h"
#include "Transform.h"
#include "pch.h"

class BoxCollider final : public IComponent {
public:
	BoxCollider() = default;
	~BoxCollider() {}
	BoxCollider(const BoxCollider&) = delete;

	virtual void Awake() {};

	void Initialize(Transform* pTransform, const XMFLOAT3& size);

	void UpdateTransform();

	bool CollisionCheck(const BoxCollider& other);
private:
	BoundingOrientedBox mBox = {};
	Transform* mTransform = nullptr;
};
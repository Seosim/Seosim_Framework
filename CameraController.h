#pragma once
#include "pch.h"
#include "Camera.h"
#include "Transform.h"
#include "IComponent.h"

class CameraController final : public IComponent {
public:
	CameraController() = default;
	~CameraController() {}
	CameraController(const CameraController&) = delete;

	virtual void Awake() {}
	virtual void Update(const float deltaTime);

	void Initialize(Camera* camera, Transform* transform);
private:
	Camera* mCamera = nullptr;
	Transform* mTransform = nullptr;
};
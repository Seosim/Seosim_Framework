#pragma once

class IComponent {
public:
	IComponent() = default;
	virtual ~IComponent() {}

	IComponent(const IComponent&) = delete;

	virtual void Awake() = 0;

	virtual void Update(const float deltaTime) = 0;
private:
};
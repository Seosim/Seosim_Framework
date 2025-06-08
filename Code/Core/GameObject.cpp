#include "pch.h"
#include "GameObject.h"

unsigned int GameObject::globalID = 0;

GameObject::GameObject()
{
	ID = globalID++;
}

GameObject::GameObject(const std::string& name) : mName(name)
{
	ID = globalID++;
}

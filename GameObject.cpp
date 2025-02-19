#include "pch.h"
#include "GameObject.h"

unsigned int GameObject::globalID = 0;

GameObject::GameObject()
{
	ID = globalID++;
}
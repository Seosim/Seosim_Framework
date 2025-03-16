#include "pch.h"
#include "Input.h"

Input& Input::Instance()
{
	static Input inputController = {};
	return inputController;
}

void Input::SaveKeyState()
{
	for (int i = 0; i < KEY_SIZE; ++i)
		mPrevKeyState[i] = mKeyState[i];
}

void Input::KeyDown(const int key)
{
	mKeyState[key] = true;
}

void Input::KeyUp(const int key)
{
	mKeyState[key] = false;
}

bool Input::GetKeyDown(const int key)
{
	return mKeyState[key] && !mPrevKeyState[key];
}

bool Input::GetKeyUp(const int key)
{
	return !mKeyState[key] && mPrevKeyState[key];
}

bool Input::GetKey(const int key)
{
	return mKeyState[key];
}

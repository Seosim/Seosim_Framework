#include "pch.h"
#include "Input.h"

Input& Input::Instance()
{
	static Input inputController = {};
	return inputController;
}

void Input::SetHWND(const HWND& hwnd)
{
	mHWND = hwnd;
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

void Input::UpdateMousePosition() 
{
    RECT rect;
    GetClientRect(mHWND, &rect);
    POINT center = { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(mHWND, &pt);

    mMouseDelta.x = (float)(pt.x - center.x);
    mMouseDelta.y = (float)(center.y - pt.y);

    ClientToScreen(mHWND, &center);
    SetCursorPos(center.x, center.y);
}

XMFLOAT2 Input::GetMouseDelta() const 
{
	return mMouseDelta;
}
#pragma once
#include "pch.h"

class Input final {
public:
	enum class eState {
		DOWN,
		UP,
		COUNT
	};

	Input() = default;
	~Input() {}

	Input(const Input&) = delete;

	static Input& Instance();

	void SetHWND(const HWND& hwnd);
	void SaveKeyState();

	void KeyDown(const int key);
	void KeyUp(const int key);

	bool GetKeyDown(const int key);
	bool GetKeyUp(const int key);
	bool GetKey(const int key);

	void UpdateMousePosition();

	XMFLOAT2 GetMouseDelta() const;
private:
	static constexpr int KEY_SIZE = 256;

	bool mKeyState[KEY_SIZE] = {};
	bool mPrevKeyState[KEY_SIZE] = {};

	XMFLOAT2 mCurrentMousePosition = {};
	XMFLOAT2 mPrevMousePosition = {};
	XMFLOAT2 mMouseDelta = {};

	HWND mHWND = {};
};
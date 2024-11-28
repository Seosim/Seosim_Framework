#include "pch.h"
#include "D3D12App.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	D3D12App::Instance().InitWindow();
	D3D12App::Instance().InitDirect3D();

	D3D12App::Instance().Update();

	return 0;
}


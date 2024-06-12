#pragma once

#include <Windows.h>
#include <optional>
#include <string>

#include "System/WindowInterface.hpp"

class D3D12Window : public mythSystem::WindowInterface
{
public:
	D3D12Window();
	~D3D12Window();

	// Start IWindowInterface
	virtual int Run() override;
	// Stop IWindowInterface


	static LRESULT __stdcall WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	static std::optional<int> ProcessMessages();

private:
	HWND hWnd;
	HINSTANCE hInstance;
};

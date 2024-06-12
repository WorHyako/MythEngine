#include <RHI/DX12/D3D12Window.hpp>

D3D12Window::D3D12Window() :
	WindowInterface(),
	hInstance(GetModuleHandle(nullptr))
{
	WNDCLASS WindowClass = {};
	WindowClass.lpfnWndProc = WindowProc;
	WindowClass.hInstance = hInstance;
	WindowClass.lpszClassName = WindowParams.WindowTitle.c_str();

	RegisterClass(&WindowClass);

	RECT WinRect;
	WinRect.left = 100;
	WinRect.right = WindowParams.Width + WinRect.left;
	WinRect.top = 100;
	WinRect.bottom = WindowParams.Height + WinRect.top;
	AdjustWindowRect(&WinRect, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE);

	hWnd = CreateWindow(
		WindowParams.WindowTitle.c_str(),
		WindowParams.WindowTitle.c_str(),
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT,
		WinRect.right - WinRect.left, WinRect.bottom - WinRect.top,
		nullptr,
		nullptr,
		hInstance,
		this,
	);

	ShowWindow(hWnd, SW_SHOWDEFAULT);
}

D3D12Window::~D3D12Window()
{
	UnregisterClass(WindowParams.WindowTitle.c_str(), hInstance);
	DestroyWindow(hWnd);
}

int D3D12Window::Run()
{
	while (true)
	{
		if (const auto MsgCode = D3D12Window::ProcessMessages())
		{
			return *MsgCode;
		}
	}
}

std::optional<int> D3D12Window::ProcessMessages()
{
	MSG Message = {};
	while(PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
	{
		if(Message.message == WM_QUIT)
		{
			return Message.wParam;
		}

		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

	return {};
}

LRESULT __stdcall D3D12Window::WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

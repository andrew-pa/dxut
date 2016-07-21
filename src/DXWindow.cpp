//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "dxut\cmmn.h"
#include "dxut\DXWindow.h"
#include <shellapi.h>

static map<HWND, DXWindow*> windows;

DXWindow::DXWindow(UINT width, UINT height, std::wstring name):
	width(width),
	height(height),
	useWarpDevice(false)
{
	ParseCommandLineArgs();

	title = name + (useWarpDevice ? L" (WARP)" : L"");

	WCHAR nassetsPath[512];
	GetAssetsPath(nassetsPath, _countof(nassetsPath));
	assetsPath = nassetsPath;

	aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

DXWindow::~DXWindow()
{
}

int DXWindow::Run(HINSTANCE hInstance, int nCmdShow)
{
	// Initialize the window class.
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"WindowClass1";
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Create the window and store a handle to it.
	hwnd = CreateWindowEx(NULL,
		L"WindowClass1",
		title.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,		// We have no parent window, NULL.
		NULL,		// We aren't using menus, NULL.
		hInstance,
		NULL);		// We aren't using multiple windows, NULL.
	windows[hwnd] = this;
	ShowWindow(hwnd, nCmdShow);

	// Initialize the sample. OnInit is defined in each child-implementation of DXWindow.
	OnInit();

	// Main sample loop.
	MSG msg = { 0 };
	while (true)
	{
		// Process any messages in the queue.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);

			if (msg.message == WM_WINDOWPOSCHANGED) {
				
				break;
			}

			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				break;

			// Pass events into our sample.
			OnEvent(msg);
		}

		OnUpdate();
		OnRender();
	}

	OnDestroy();

	// Return this part of the WQUIT message to Windows.
	return static_cast<char>(msg.wParam);
}

// Helper function for resolving the full path of assets.
std::wstring DXWindow::GetAssetFullPath(LPCWSTR assetName)
{
	return assetsPath + assetName;
}

// Helper function for setting the window's title text.
void DXWindow::SetCustomWindowText(LPCWSTR text)
{
	std::wstring windowText = title + L": " + text;
	SetWindowText(hwnd, windowText.c_str());
}

// Helper function for parsing any supplied command line args.
void DXWindow::ParseCommandLineArgs()
{
	int argc;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	for (int i = 1; i < argc; ++i)
	{
		if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 || 
			_wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
		{
			useWarpDevice = true;
		}
	}
	LocalFree(argv);
}

// Main message handler for the sample.
LRESULT CALLBACK DXWindow::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// Handle destroy/shutdown messages.
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_SIZE: 
		/*auto t = windows[hWnd];
		RECT rct; GetClientRect(hWnd, &rct);
		auto w = rct.right - rct.left;
		auto h = rct.bottom - rct.top;
		if (w != t->width || h != t->height) {
			t->width = w; t->height = h;
			t->aspectRatio = static_cast<float>(t->width) / static_cast<float>(t->height);
		}
		} */return 0;
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(hWnd, message, wParam, lParam);
}

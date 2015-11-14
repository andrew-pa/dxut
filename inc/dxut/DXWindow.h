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

#pragma once

#include "DXSampleHelper.h"

class DXWindow
{
public:
	DXWindow(UINT width, UINT height, std::wstring name);
	virtual ~DXWindow();

	int Run(HINSTANCE hInstance, int nCmdShow);
	void SetCustomWindowText(LPCWSTR text);

	std::wstring GetAssetFullPath(LPCWSTR assetName);
protected:
	virtual void OnInit() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnRender() = 0;
	virtual void OnDestroy() = 0;
	virtual bool OnEvent(MSG msg) = 0;


	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

public:
	// Viewport dimensions.
	UINT width;
	UINT height;
	float aspectRatio;

	// Window handle.
	HWND hwnd;

	// Adapter info.
	bool useWarpDevice;

private:
	void ParseCommandLineArgs();

	// Root assets path.
	std::wstring assetsPath;

	// Window title.
	std::wstring title;
};

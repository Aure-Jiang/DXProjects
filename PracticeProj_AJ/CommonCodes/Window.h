#pragma once
#include "Includes.h"
#include "DXFrame.h"
class DXFrame;
class Window
{
public:
	static int Run(DXFrame *pDXFrame, HINSTANCE hInstance, int nCmdShow);
	static HWND GetHwnd() { return mHwnd; }
protected:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	static HWND mHwnd;
};


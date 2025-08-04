#include "Window.h"

HWND Window::mHwnd = nullptr;
int Window::Run(DXFrame* pDXFrame, HINSTANCE hInstance, int nCmdShow)
{
	int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);//将宽字符命令行字符串拆分为独立参数数组，通过第二个参数输出参数数量
	pDXFrame->ParseCommandLineArgs(argv, argc);
	LocalFree(argv);

	//初始化窗口类
	WNDCLASSEX windowClass = {0};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Window::WndProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.lpszClassName = L"CUBE";
    RegisterClassEx(&windowClass);

	RECT rect = {0, 0, static_cast<LONG>(pDXFrame->GetWidth()), static_cast<LONG>(pDXFrame->GetHeight()) };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	//创建窗口、储存句柄
    mHwnd = CreateWindow(
		windowClass.lpszClassName,
		pDXFrame->GetWindowName(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		nullptr,
		nullptr,
		hInstance,
		pDXFrame
	);

	//初始化基本框架
    pDXFrame->OnInit();
	ShowWindow(mHwnd,nCmdShow);

	//运行主循环
	MSG msg = {};
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
    pDXFrame->OnDestroy();

	return static_cast<char>(msg.wParam);
}

LRESULT Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DXFrame *pDXFrame = reinterpret_cast<DXFrame*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	switch (message) {
    case WM_CREATE:
    {
        //将lParam强制转换为LPCREATESTRUCT结构体指针，获取窗口创建参数
        //通过SetWindowLongPtr将CREATESTRUCT中的lpCreateParams参数保存到窗口的GWLP_USERDATA位置，
        // 用于后续窗口过程函数中访问自定义数据
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
    }
    return 0;

    case WM_KEYDOWN:
        if (pDXFrame) {
            pDXFrame->OnKeyDown(static_cast<UINT>(wParam));
        }
        return 0;

    case WM_KEYUP:
        if (pDXFrame) {
            pDXFrame->OnKeyUp(static_cast<UINT>(wParam));
        }
        return 0;

    case WM_PAINT:
        if (pDXFrame) {
            pDXFrame->OnUpdate();
            pDXFrame->OnRender();
        }
        return 0;

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        pDXFrame->OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        pDXFrame->OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_MOUSEMOVE:
        pDXFrame->OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_SIZE:
    {
        RECT clientRect = {};
        GetClientRect(hWnd, &clientRect);
        pDXFrame->OnResize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, wParam == SIZE_MINIMIZED);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
	}


	return DefWindowProc(hWnd, message, wParam, lParam);
}

#include "Window.h"

HWND Window::mHwnd = nullptr;
int Window::Run(DXFrame* pDXFrame, HINSTANCE hInstance, int nCmdShow)
{
	int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);//�����ַ��������ַ������Ϊ�����������飬ͨ���ڶ������������������
	pDXFrame->ParseCommandLineArgs(argv, argc);
	LocalFree(argv);

	//��ʼ��������
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

	//�������ڡ�������
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

	//��ʼ���������
    pDXFrame->OnInit();
	ShowWindow(mHwnd,nCmdShow);

	//������ѭ��
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
        //��lParamǿ��ת��ΪLPCREATESTRUCT�ṹ��ָ�룬��ȡ���ڴ�������
        //ͨ��SetWindowLongPtr��CREATESTRUCT�е�lpCreateParams�������浽���ڵ�GWLP_USERDATAλ�ã�
        // ���ں������ڹ��̺����з����Զ�������
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

#include "CubeTexture.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	::CoInitialize(nullptr);  //for WIC & COM
	CubeTexture ct(1280, 720, L"CubeInstance");
	return Window::Run(&ct, hInstance, nShowCmd);
}
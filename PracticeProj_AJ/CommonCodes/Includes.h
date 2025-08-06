#pragma once
#include <SDKDDKVer.h>
#include <windows.h>
#include <tchar.h>
#include <wrl.h>  //���WTL֧�� ����ʹ��COM
#include <strsafe.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
//#include <cmath>
#include <d3d12.h>	//for d3d12
#include "d3dx12.h"
#include <DirectXTex.h>  //������������λ���� d3d12.h ֮��
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <wincodec.h>  //for WIC
#include <algorithm>
#include <windowsx.h>
#include <fstream>

#include <atlbase.h>     //ATL�������COM֧��
#include <atlcoll.h>	 //������ģ�壨����/����
#include <atlchecked.h>	 //���Լ���
#include <atlstr.h>		 //�ַ���������CStringT
#include <atlconv.h>	 //�ַ���ת����
#include <atlexcept.h>	 //�쳣������ ��Ҫ���ڼ�Windowsƽ̨COM����е��ڴ�����ַ��������ʹ�����

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

using namespace DirectX;
using namespace Microsoft;
using namespace Microsoft::WRL;
using Microsoft::WRL::ComPtr;

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")
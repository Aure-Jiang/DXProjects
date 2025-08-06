#pragma once
#include <SDKDDKVer.h>
#include <windows.h>
#include <tchar.h>
#include <wrl.h>  //添加WTL支持 方便使用COM
#include <strsafe.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
//#include <cmath>
#include <d3d12.h>	//for d3d12
#include "d3dx12.h"
#include <DirectXTex.h>  //加载纹理，引用位置在 d3d12.h 之后
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <wincodec.h>  //for WIC
#include <algorithm>
#include <windowsx.h>
#include <fstream>

#include <atlbase.h>     //ATL基础类和COM支持
#include <atlcoll.h>	 //集合类模板（数组/链表）
#include <atlchecked.h>	 //调试检查宏
#include <atlstr.h>		 //字符串处理类CStringT
#include <atlconv.h>	 //字符串转换宏
#include <atlexcept.h>	 //异常处理框架 主要用于简化Windows平台COM编程中的内存管理、字符串操作和错误处理。

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
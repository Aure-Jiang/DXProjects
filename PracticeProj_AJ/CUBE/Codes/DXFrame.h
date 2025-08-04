#pragma once
#include "Includes.h"
#include "Tools.h"
#include "Window.h"
class DXFrame
{
public:
	DXFrame(UINT width, UINT height, std::wstring name);

    //渲染相关
	virtual void OnInit()    = 0;
    virtual void OnUpdate()  = 0;
    virtual void OnRender()  = 0;
    virtual void OnDestroy() = 0;
    virtual void OnResize(UINT width, UINT height, bool minimized)  = 0;

    //键盘鼠标事件
	virtual void OnKeyDown(UINT8 /*key*/) {}
	virtual void OnKeyUp(UINT8 /*key*/) {}

	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

	//加载纹理
	virtual void LoadTexture(LPCWSTR fileName);

	//解析命令行参数，检测是否启用WARP设备模式，
	// WARP（Windows Advanced Rasterization Platform）是微软提供的高性能软件光栅化器，
	// 作为 Direct3D 11/12 的软件实现层。
	void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

	//该函数用于查找支持Direct3D 12的硬件图形适配器，优先选择高性能GPU
	void GetHardwareAdapter(
		_In_ IDXGIFactory1* pFactory,
		_Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
		bool requestHighPerformanceAdapter = false);

	void SetCustomWindowText(LPCWSTR text);

	UINT GetWidth() const { return mWidth; }
    UINT GetHeight() const { return mHeight; }
	const WCHAR* GetWindowName() const { return mWindowName.c_str(); }
	std::wstring GetAssetFullPath(LPCWSTR assetName);

	UINT mWidth;
	UINT mHeight;
	float mAspectRatio;

	bool mUseWarpDevice;
private:

    std::wstring mWindowName;
	std::wstring mAssetsPath;


};


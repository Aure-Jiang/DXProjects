#pragma once
#include "Includes.h"
#include "Tools.h"
#include "Window.h"
class DXFrame
{
public:
	DXFrame(UINT width, UINT height, std::wstring name);

    //��Ⱦ���
	virtual void OnInit()    = 0;
    virtual void OnUpdate()  = 0;
    virtual void OnRender()  = 0;
    virtual void OnDestroy() = 0;
    virtual void OnResize(UINT width, UINT height, bool minimized)  = 0;

    //��������¼�
	virtual void OnKeyDown(UINT8 /*key*/) {}
	virtual void OnKeyUp(UINT8 /*key*/) {}

	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

	//��������
	virtual void LoadTexture(LPCWSTR fileName);

	//���������в���������Ƿ�����WARP�豸ģʽ��
	// WARP��Windows Advanced Rasterization Platform����΢���ṩ�ĸ����������դ������
	// ��Ϊ Direct3D 11/12 �����ʵ�ֲ㡣
	void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

	//�ú������ڲ���֧��Direct3D 12��Ӳ��ͼ��������������ѡ�������GPU
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


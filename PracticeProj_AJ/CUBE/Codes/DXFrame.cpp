#include "DXFrame.h"

DXFrame::DXFrame(UINT width, UINT height, std::wstring name)
	: mWidth(width),
	mHeight(height),
	mWindowName(name),
	mUseWarpDevice(false)
{
	WCHAR assetsPath[512];
	GetAssetsPath(assetsPath, _countof(assetsPath));
	mAssetsPath = assetsPath;

	mAspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

void DXFrame::LoadTexture(LPCWSTR fileName)
{
}

//_wcsnicmp是宽字符版本的不区分大小写字符串比较函数，L表示宽字符串字面量;
// wcslen计算宽字符字符串的长度（不包含终止符 L'\0'）
void DXFrame::ParseCommandLineArgs(WCHAR* argv[], int argc)
{
	for (int i = 1; i < argc; ++i)
	{
		if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
			_wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0) {
			mUseWarpDevice = true;
			mWindowName = mWindowName + L" (WARP)";
		}
	}
}

void DXFrame::GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter)
{
	*ppAdapter = nullptr;

	ComPtr<IDXGIAdapter1> adapter;
	ComPtr<IDXGIFactory6> factory6;
	//尝试通过IDXGIFactory6接口按GPU偏好枚举适配器
	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (UINT adapterIndex = 0;
			SUCCEEDED(factory6->EnumAdapterByGpuPreference(
				adapterIndex,
				requestHighPerformanceAdapter ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				IID_PPV_ARGS(&adapter)));
				++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
				continue;
			}

			if (SUCCEEDED(D3D12CreateDevice(
				adapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				__uuidof(ID3D12Device),
				nullptr)))
			{
				break;
			}
		}
	}

	if (adapter.Get() == nullptr) {
		for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
				continue;
			}

			if (SUCCEEDED(D3D12CreateDevice(
				adapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				__uuidof(ID3D12Device),
				nullptr)))
			{
				break;
			}
		}
	}
}

void DXFrame::SetCustomWindowText(LPCWSTR text)
{
	std::wstring windowText = mWindowName + L": " + text;
	SetWindowText(Window::GetHwnd(), windowText.c_str());
}

std::wstring DXFrame::GetAssetFullPath(LPCWSTR assetName)
{
	return mAssetsPath + assetName;
}

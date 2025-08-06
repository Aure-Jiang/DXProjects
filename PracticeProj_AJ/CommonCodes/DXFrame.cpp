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

//_wcsnicmp�ǿ��ַ��汾�Ĳ����ִ�Сд�ַ����ȽϺ�����L��ʾ���ַ���������;
// wcslen������ַ��ַ����ĳ��ȣ���������ֹ�� L'\0'��
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
	//����ͨ��IDXGIFactory6�ӿڰ�GPUƫ��ö��������
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

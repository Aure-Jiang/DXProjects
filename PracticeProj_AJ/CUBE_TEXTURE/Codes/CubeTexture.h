#pragma once
#include "../../CommonCodes/Includes.h"
#include "../../CommonCodes/Window.h"
#include "../../CommonCodes/DXFrame.h"
#include "../../CommonCodes/Tools.h"

class CubeTexture : public DXFrame
{
public:
    CubeTexture(UINT width, UINT height, std::wstring name);

	virtual void OnInit();
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnDestroy();
	virtual void OnResize(UINT width, UINT height, bool minimized);

	virtual void OnKeyDown(UINT8 key);
	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

private:
	struct Vertex
	{
		XMFLOAT3 Position;
		XMFLOAT2 uv;
		XMFLOAT3 Normal;
	};
	//MVP矩阵结构体
	struct MVP_BUFFER
	{
		XMFLOAT4X4 mMvp_mx4;
	};
	//自动旋转需要的变量
		//帧开始时间和当前时间
	ULONGLONG                           mFrameStart_n64;
	ULONGLONG                           mCurrent_n64;
	double                              mModelRotationYAngle_d = 0.0f;
	double                              mPalstance_d = 10.0f * XM_PI / 180.0f;	//物体旋转的角速度，单位：弧度/秒
	double                              mRotationSpeed_d = 1.0f;                  //速度倍率
	double                              mPausedRotationAngle_d = 0.0;
	bool                                mIsRotate = false;

	UINT                                mCurrentSamplerNO_u = 0; //当前使用的采样器索引
	UINT                                mSampleMaxCnt_u = 5;		//创建五个典型的采样器

	//相机
	XMVECTOR                            mEye_v4 = XMVectorSet(5.0f, 5.0f, -10.0f, 0.0f);  //眼睛位置
	XMVECTOR                            mAt_v4 = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);	   //眼睛所盯的位置
	XMVECTOR                            mUp_v4 = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);	   //头部正上方位置


	const UINT                          mFrameBackBufCount_u = 2u;
	UINT                                mFrameIndex_u = 0;

	UINT                                mRTVDescriptorSize_u = 0U;
	MVP_BUFFER*                         mMVPBuffer_p = nullptr;  //MVP缓冲区
	SIZE_T                              mMVPBuffer_sz = GRS_UPPER_DIV(sizeof(MVP_BUFFER), 256) * 256;  //MVP缓冲区大小

	D3D12_VERTEX_BUFFER_VIEW            stVertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW             stIndexBufferView = {};
	//围栏
	ComPtr<ID3D12Fence>					mFence_cp;
	UINT64                              mFenceValue_u64 = 0ui64;
	HANDLE                              mFenceEvent_h = nullptr;

	//纹理相关
	ScratchImage                        mImage;
	UINT                                mTexRowPitch_u = 0;

	UINT64                              mUploadBufferSize_u64 = 0;
	DXGI_FORMAT                         mTextureFormat_fmt = DXGI_FORMAT_UNKNOWN;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT  stTxtLayouts = {};
	D3D12_RESOURCE_DESC                 stTextureDesc = {};
	D3D12_RESOURCE_DESC                 stDestDesc = {};
	UINT                                mSamplerDescriptorSize_U = 0; //采样器大小

	ComPtr<IDXGIFactory5>				mDXGIFactory5_cp;
	ComPtr<IDXGIAdapter1>				mDXGIAdapter_cp;

	ComPtr<ID3D12Device4>				mDevice_cp;
	ComPtr<ID3D12CommandQueue>			mCommandQueue_cp;
	ComPtr<ID3D12CommandAllocator>		mCommandAllocator_cp;
	ComPtr<ID3D12GraphicsCommandList>	mCommandList_cp;

	ComPtr<IDXGISwapChain3>				mSwapChain3_cp;
	ComPtr<ID3D12Resource>				mRenderTargets_cp[3];
	ComPtr<ID3D12DescriptorHeap>		mRTVHeap_cp;

	ComPtr<ID3D12Heap>					mTextureHeap_cp;
	ComPtr<ID3D12Heap>					mUploadHeap_cp;
	ComPtr<ID3D12Resource>				mTexture_cp;
	ComPtr<ID3D12Resource>				mTextureUpload_cp;
	ComPtr<ID3D12Resource>			    mCBVUpload_cp;
	ComPtr<ID3D12Resource>				mVertexBuffer_cp;
	ComPtr<ID3D12Resource>				mIndexBuffer_cp;

	ComPtr<ID3D12DescriptorHeap>		mSRVHeap_cp;
	ComPtr<ID3D12DescriptorHeap>		mSamplerDescriptorHeap_cp;

	ComPtr<ID3D12RootSignature>			mRootSignature_cp;
	ComPtr<ID3D12PipelineState>			mPipelineState_cp;
	//鼠标控制需要的变量
	POINT                               mLastMousePos;
	float                               mTheta = 0.0f;
	float                               mPhi = 0.0f;
	XMMATRIX                            mRotationMatrix;
	XMMATRIX                            mScaleMatrix;
	float                               mScaleFactor = 1.0f;


	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();
	//加载纹理
	void LoadTexture(LPCWSTR fileName);
};
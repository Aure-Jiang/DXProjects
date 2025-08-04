#pragma once
#include "Includes.h"
#include "Tools.h"
#include "Window.h"
#include "DXFrame.h"
class Cube
	: public DXFrame
{
public:
	Cube(UINT width, UINT height, std::wstring name);

	virtual void OnInit();
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnResize(UINT width, UINT height, bool minimized);
	virtual void OnDestroy();

	virtual void OnKeyDown(UINT8 key);
	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;


private:
	struct CB_MVP//register(b0)
	{
		XMFLOAT4X4 mWorld_mx4;
		XMFLOAT4X4 mView_mx4;
		XMFLOAT4X4 mProj_mx4;
		XMFLOAT4X4 mViewProj_mx4;   //视矩阵*投影矩阵
		XMFLOAT4X4 mMvp_mx4;
	};

	struct CB_CAMERA//register(b1)
	{
		XMFLOAT4 mCameraPos_v4;
	};

#define LIGHT_COUNT 8
	struct CB_LIGHTS//register(b2)
	{
		XMFLOAT4 mLightPos_v4[LIGHT_COUNT];
		XMFLOAT4 mLightClr_v4[LIGHT_COUNT];
	};

	struct CB_PBR//register(b3)
	{
		XMFLOAT4X4  mInstancePos_mx4;  //实例在世界坐标系中的位置
		XMFLOAT3    mAlbedo_v3;        // 反射率
		float       mMetallic_f;	   // 金属度
		float       mRoughness_f;      // 粗糙度
		float       mAo_f;		       // 环境光遮蔽
	};

	struct VERTEX
	{
		XMFLOAT4 mPos_v4;
		XMFLOAT4 mNormal_v4;
		XMFLOAT2 mTex_v2;
	};

	//相机
	XMVECTOR                              mEye_v4 = XMVectorSet(0.0f, 0.0f, -20.0f, 0.0f);
	XMVECTOR                              mAt_v4 = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR                              mUp_v4 = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	//材质参数		                      
	float                                 mMetallic_f = 0.91f;
	float                                 mRoughness_f = 0.11f;
	float                                 mAo_f = 0.03f;

	UINT                                  mCurAlbedo_u = 0;
	XMFLOAT3                              mAlbedo_v3[8];


	const UINT                            mFrameBackBufCount_n = 3u;
	UINT                                  mFrameIndex_n = 0u;
	DXGI_FORMAT                           mRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT                           mDepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
	const float                           mClearColor_f[4] = { 0.0f, 0.2f, 0.4f, 1.0f };

	UINT                                  mRtvDescriptorSize_n = 0u;
	UINT                                  mCbvSrvDescriptorSize_n = 0u;
	UINT                                  mSamplerDescriptorSize_n = 0u;
	UINT                                  mDsvDescriptorSize_n = 0u;

	D3D12_VIEWPORT                        mViewport = { 0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight) };
	D3D12_RECT                            mScissorRect = { 0, 0, static_cast<LONG>(mWidth), static_cast<LONG>(mHeight) };

	//加载管线需要的对象
	ComPtr<IDXGIFactory6>                 mDxgiFactory6_p;
	ComPtr<ID3D12Device>                  mDevice_p;

	ComPtr<ID3D12CommandQueue>            mCommandQueue_p;
	ComPtr<ID3D12CommandAllocator>        mCommandAllocator_p;
	ComPtr<ID3D12GraphicsCommandList>     mCommandList_p;

	ComPtr<IDXGISwapChain3>               mSwapChain3_p;
	//加载资源需要的对象
	ComPtr<ID3D12DescriptorHeap>          mRtvHeap_p;
	ComPtr<ID3D12Resource>                mRenderTargets_p[3]; //mFrameBackBufCount_n
	ComPtr<ID3D12DescriptorHeap>          mDsvHeap_p;
	ComPtr<ID3D12Resource>                mDSBuffer_p;

	ComPtr<ID3D12Fence>                   mFence_p;
	UINT                                  mFenceValue_n;
	HANDLE                                mFenceEvent_h;

	const UINT                            mConstBufferCount_n = 4;

	ComPtr<ID3D12RootSignature>           mRootSignature_p;
	ComPtr<ID3D12PipelineState>           mPipelineState_p;

	ComPtr<ID3D12DescriptorHeap>          mCbvSrvHeap_p;

	ComPtr<ID3D12Resource>                mCBMVP_p;
	CB_MVP* mCBMVP_ptr = nullptr;
	ComPtr<ID3D12Resource>                mCbCamera_p;
	CB_CAMERA* mCbCamera_ptr = nullptr;
	ComPtr<ID3D12Resource>                mCbLights_p;
	CB_LIGHTS* mCbLights_ptr = nullptr;
	ComPtr<ID3D12Resource>                mCbPbr_p;
	CB_PBR* mCbPbr_ptr = nullptr;
	ComPtr<ID3D12Resource>                mInstanceBuffer_p;

	UINT                                  mIndexCount_n;
	ComPtr<ID3D12Resource>                mVertexBuffer_p;
	ComPtr<ID3D12Resource>                mIndexBuffer_p;
	D3D12_VERTEX_BUFFER_VIEW              mVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW               mIndexBufferView;
	D3D12_VERTEX_BUFFER_VIEW              mInstanceBufferView;

	//鼠标控制需要的变量
	POINT                                 mLastMousePos;
	float                                 mTheta = 0.0f;
	float                                 mPhi = 0.0f;
	XMMATRIX                              mRotationMatrix;
    XMMATRIX                              mScaleMatrix;
	float                                 mScaleFactor = 1.0f;
	//自动旋转需要的变量
	//帧开始时间和当前时间
	ULONGLONG                             mFrameStart_n64;
	ULONGLONG                             mCurrent_n64;
	double                                mModelRotationYAngle_d = 0.0f;
	double                                mPalstance_d = 10.0f * XM_PI / 180.0f;	//物体旋转的角速度，单位：弧度/秒
	double                                mRotationSpeed_d = 1.0f;                  //速度倍率
	double                                mPausedRotationAngle_d = 0.0;
	bool                                  mIsRotate = false;
	//灯光旋转需要的变量
	bool                                  mIsLightRotate_b = false;
	XMMATRIX                              mLightRotationMatrix_xm;
	double                                mLightRotationAngle_d = 0.0f;
	XMVECTOR                              mLightPos_v = XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f);
	//多实例绘制需要的变量
    UINT                                  mInstanceCount_u = 2;

	virtual void LoadTextures(LPCWSTR fileName);
	BOOL loadMesh(LPCWSTR fileName, UINT& vertexCount_n, VERTEX*& vertex_pp, UINT*& indics_pp);
	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();
	void ChangeInstanceNum(UINT count);
};


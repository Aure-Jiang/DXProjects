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
	//MVP����ṹ��
	struct MVP_BUFFER
	{
		XMFLOAT4X4 mMvp_mx4;
	};
	//�Զ���ת��Ҫ�ı���
		//֡��ʼʱ��͵�ǰʱ��
	ULONGLONG                           mFrameStart_n64;
	ULONGLONG                           mCurrent_n64;
	double                              mModelRotationYAngle_d = 0.0f;
	double                              mPalstance_d = 10.0f * XM_PI / 180.0f;	//������ת�Ľ��ٶȣ���λ������/��
	double                              mRotationSpeed_d = 1.0f;                  //�ٶȱ���
	double                              mPausedRotationAngle_d = 0.0;
	bool                                mIsRotate = false;

	UINT                                mCurrentSamplerNO_u = 0; //��ǰʹ�õĲ���������
	UINT                                mSampleMaxCnt_u = 5;		//����������͵Ĳ�����

	//���
	XMVECTOR                            mEye_v4 = XMVectorSet(5.0f, 5.0f, -10.0f, 0.0f);  //�۾�λ��
	XMVECTOR                            mAt_v4 = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);	   //�۾�������λ��
	XMVECTOR                            mUp_v4 = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);	   //ͷ�����Ϸ�λ��


	const UINT                          mFrameBackBufCount_u = 2u;
	UINT                                mFrameIndex_u = 0;

	UINT                                mRTVDescriptorSize_u = 0U;
	MVP_BUFFER*                         mMVPBuffer_p = nullptr;  //MVP������
	SIZE_T                              mMVPBuffer_sz = GRS_UPPER_DIV(sizeof(MVP_BUFFER), 256) * 256;  //MVP��������С

	D3D12_VERTEX_BUFFER_VIEW            stVertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW             stIndexBufferView = {};
	//Χ��
	ComPtr<ID3D12Fence>					mFence_cp;
	UINT64                              mFenceValue_u64 = 0ui64;
	HANDLE                              mFenceEvent_h = nullptr;

	//�������
	ScratchImage                        mImage;
	UINT                                mTexRowPitch_u = 0;

	UINT64                              mUploadBufferSize_u64 = 0;
	DXGI_FORMAT                         mTextureFormat_fmt = DXGI_FORMAT_UNKNOWN;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT  stTxtLayouts = {};
	D3D12_RESOURCE_DESC                 stTextureDesc = {};
	D3D12_RESOURCE_DESC                 stDestDesc = {};
	UINT                                mSamplerDescriptorSize_U = 0; //��������С

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
	//��������Ҫ�ı���
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
	//��������
	void LoadTexture(LPCWSTR fileName);
};
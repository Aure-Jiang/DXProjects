#include "Cube.h"

Cube::Cube(UINT width, UINT height, std::wstring name)
	: DXFrame(width, height, name)
{
    //��ʼ��
    mAlbedo_v3[0] = XMFLOAT3(0.97f, 0.96f, 0.91f);
    mAlbedo_v3[1] = XMFLOAT3(0.91f, 0.92f, 0.92f);
    mAlbedo_v3[2] = XMFLOAT3(0.76f, 0.73f, 0.69f);
    mAlbedo_v3[3] = XMFLOAT3(0.77f, 0.78f, 0.78f);
    mAlbedo_v3[4] = XMFLOAT3(0.83f, 0.81f, 0.78f);
    mAlbedo_v3[5] = XMFLOAT3(1.00f, 0.85f, 0.57f);
    mAlbedo_v3[6] = XMFLOAT3(0.98f, 0.90f, 0.59f);
    mAlbedo_v3[7] = XMFLOAT3(0.97f, 0.74f, 0.62f);
    //ȷ����ȷ�Χ��ȷ
    mViewport.MinDepth = 0.0f;
    mViewport.MaxDepth = 1.0f;
}

void Cube::OnInit()
{    
    //��ʼ����Ⱦ��ʼ�¼�
    mFrameStart_n64 = GetTickCount64();
    mCurrent_n64 = mFrameStart_n64;

    LoadPipeline();
    LoadAssets();
}

void Cube::OnUpdate()
{
    XMMATRIX model = XMMatrixIdentity();
    //������ž���
    model = XMMatrixMultiply(model, mScaleMatrix);
    //�����Զ���ת
    if (mIsRotate) {
        mCurrent_n64 = GetTickCount64();
        mModelRotationYAngle_d = ((mCurrent_n64 - mFrameStart_n64) / 1000.0f) * mPalstance_d * mIsRotate * mRotationSpeed_d;
        if (mModelRotationYAngle_d > XM_2PI)
        {
            mModelRotationYAngle_d -= XM_2PI;
        }
    }
    XMMATRIX selfRotationMx = XMMatrixRotationY(static_cast<float>(mModelRotationYAngle_d));
    model = XMMatrixMultiply(model, selfRotationMx);
    //�����ת������ת
    model = XMMatrixMultiply(model, mRotationMatrix);

    XMStoreFloat4x4(&mCBMVP_ptr->mWorld_mx4, model);

    XMMATRIX view = XMMatrixLookAtLH(mEye_v4, mAt_v4, mUp_v4);
    XMStoreFloat4x4(&mCBMVP_ptr->mView_mx4, view);

    XMMATRIX projection = XMMatrixPerspectiveFovLH(
        XM_PIDIV4,
        static_cast<float>(mWidth) / static_cast<float>(mHeight),
        0.1f,
        1000.0f
    );
    XMStoreFloat4x4(&mCBMVP_ptr->mProj_mx4, projection);

    XMMATRIX viewProj = XMMatrixMultiply(view, projection);
    XMStoreFloat4x4(&mCBMVP_ptr->mViewProj_mx4, viewProj);

    // MVP
    XMMATRIX mvp = XMMatrixMultiply(model, viewProj);
    XMStoreFloat4x4(&mCBMVP_ptr->mMvp_mx4, mvp);

    //���
    XMStoreFloat4(&mCbCamera_ptr->mCameraPos_v4, mEye_v4);

    //����
    {
        if (mIsLightRotate_b) {
            mLightRotationAngle_d = 0.01f;
        }
        else{
            mLightRotationAngle_d = 0.0f;
        }
        mLightRotationMatrix_xm = XMMatrixRotationY(static_cast<float>(mLightRotationAngle_d));
        XMVECTOR rotatedPos = XMVector3Transform(mLightPos_v, mLightRotationMatrix_xm);
        mLightPos_v = rotatedPos;
        XMFLOAT4 lightPos_f4;
        XMStoreFloat4(&lightPos_f4, mLightPos_v);
        float fZ = -3.0f;
        // 4��ǰ�õ�
        mCbLights_ptr->mLightPos_v4[0] = lightPos_f4;
        mCbLights_ptr->mLightClr_v4[0] = { 13.47f,11.31f,10.79f, 1.0f };

        mCbLights_ptr->mLightPos_v4[1] = { 0.0f,3.0f,fZ,1.0f };
        mCbLights_ptr->mLightClr_v4[1] = { 53.47f,41.31f,40.79f, 1.0f };

        mCbLights_ptr->mLightPos_v4[2] = { 3.0f,-3.0f,fZ,1.0f };
        mCbLights_ptr->mLightClr_v4[2] = { 23.47f,21.31f,20.79f, 1.0f };

        mCbLights_ptr->mLightPos_v4[3] = { -3.0f,-3.0f,fZ,1.0f };
        mCbLights_ptr->mLightClr_v4[3] = { 23.47f,21.31f,20.79f, 1.0f };

        // ������λ��
        fZ = 0.0f;
        float fY = 5.0f;
        mCbLights_ptr->mLightPos_v4[4] = { 0.0f,0.0f,fZ,1.0f };
        mCbLights_ptr->mLightClr_v4[4] = { 23.47f,21.31f,20.79f, 1.0f };

        mCbLights_ptr->mLightPos_v4[5] = { 0.0f,-fY,fZ,1.0f };
        mCbLights_ptr->mLightClr_v4[5] = { 53.47f,41.31f,40.79f, 1.0f };

        mCbLights_ptr->mLightPos_v4[6] = { fY,fY,fZ,1.0f };
        mCbLights_ptr->mLightClr_v4[6] = { 23.47f,21.31f,20.79f, 1.0f };

        mCbLights_ptr->mLightPos_v4[7] = { -fY,fY,fZ,1.0f };
        mCbLights_ptr->mLightClr_v4[7] = { 23.47f,21.31f,20.79f, 1.0f };
    }

    //Ϊÿ��ʵ�����������ı任����
    for (UINT num = 0; num < mInstanceCount_u; num++) {
        // ʵ���ľֲ��任��ƽ�ƣ�
        XMMATRIX instanceTranslation = XMMatrixTranslation(0.0f, 0.0f, 15.0f * num);

        XMStoreFloat4x4(&mCbPbr_ptr[num].mInstancePos_mx4, instanceTranslation);

        // ����PBR����...
        mCbPbr_ptr[num].mAlbedo_v3 = mAlbedo_v3[(mCurAlbedo_u + num) % 8];
        mCbPbr_ptr[num].mMetallic_f = mMetallic_f;
        mCbPbr_ptr[num].mRoughness_f = mRoughness_f;
        mCbPbr_ptr[num].mAo_f = mAo_f;
    }
    
}

void Cube::OnRender()
{
    //��������б�
    PopulateCommandList();
    //ִ�������б�
    ID3D12CommandList* ppCommandLists[] = { mCommandList_p.Get() };
    mCommandQueue_p->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    //�ύ����
    ThrowIfFailed(mSwapChain3_p->Present(1, 0));
    //ͬ��
    WaitForPreviousFrame();
    //�������������
    ThrowIfFailed(mCommandAllocator_p->Reset());
    //���������б�
    ThrowIfFailed(mCommandList_p->Reset(mCommandAllocator_p.Get(), mPipelineState_p.Get()));
}

void Cube::OnResize(UINT width, UINT height, bool minimized)
{
    if ((width != mWidth || height != mHeight) && !minimized)
    {
        // 1. �ȴ�GPU������й���
        WaitForPreviousFrame();

        // 2. �ͷ���ȾĿ����Դ
        for (UINT i = 0; i < mFrameBackBufCount_n; i++) {
            mRenderTargets_p[i].Reset();
        }

        // 3. ������������������С
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        ThrowIfFailed(mSwapChain3_p->GetDesc(&swapChainDesc));
        ThrowIfFailed(mSwapChain3_p->ResizeBuffers(
            mFrameBackBufCount_n,
            width, height,
            swapChainDesc.BufferDesc.Format,
            swapChainDesc.Flags
        ));

        // 4. ���³ߴ����
        mWidth = width;
        mHeight = height;

        // 5. ���´�����ȾĿ����ͼ
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap_p->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < mFrameBackBufCount_n; i++) {
            ThrowIfFailed(mSwapChain3_p->GetBuffer(i, IID_PPV_ARGS(&mRenderTargets_p[i])));
            mDevice_p->CreateRenderTargetView(mRenderTargets_p[i].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += mRtvDescriptorSize_n;
        }

        // 6. �ؽ����/ģ�建����
        mDSBuffer_p.Reset();

        D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            mDepthStencilFormat,
            mWidth,
            mHeight,
            1, // arraySize
            1  // mipLevels
        );
        depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = mDepthStencilFormat;
        depthOptimizedClearValue.DepthStencil = { 1.0f, 0 };

        ThrowIfFailed(mDevice_p->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(&mDSBuffer_p)
        ));

        // 7. �������ģ����ͼ
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = mDepthStencilFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mDsvHeap_p->GetCPUDescriptorHandleForHeapStart();
        mDevice_p->CreateDepthStencilView(mDSBuffer_p.Get(), &dsvDesc, dsvHandle);

        // 8. �����ӿںͲü�����
        mViewport.Width = static_cast<float>(mWidth);
        mViewport.Height = static_cast<float>(mHeight);

        mScissorRect.right = mWidth;
        mScissorRect.bottom = mHeight;

        // 9. ����֡����
        mFrameIndex_n = mSwapChain3_p->GetCurrentBackBufferIndex();
    }
}

void Cube::OnDestroy()
{
}

void Cube::OnKeyDown(UINT8 key)
{
    //��ת
    {
        //������ת
        if (key == VK_ADD) {
            mRotationSpeed_d += 0.1f;
        }
        //������ת
        if (key == VK_SUBTRACT) {
            mRotationSpeed_d -= 0.1f;
            if (mRotationSpeed_d < 0.0f) {
                mRotationSpeed_d = 0.0f;
            }
        }
        //�ո���ͣ�ָ���ת
        if (key == VK_SPACE) {
            if (mIsRotate) {
                // ��¼��ǰ��ת�Ƕ���Ϊ�´λָ�����ʼ�Ƕ�
                mPausedRotationAngle_d = mModelRotationYAngle_d;
            }
            else {
                // �ָ���תʱ������ʼʱ��
                mFrameStart_n64 = GetTickCount64() - static_cast<DWORD>((mPausedRotationAngle_d / mPalstance_d) * 1000);
            }
            mIsRotate = !mIsRotate;
        }
    }
    //�ƹ�λ��
    {
        //L���л��ƹ�λ��
        if ('L' == key || 'l' == key) {
            mIsLightRotate_b = !mIsLightRotate_b;
        }
    }
    
    //ʵ������
    {
        if (key == VK_UP) {  // ����ʵ��
            ChangeInstanceNum(mInstanceCount_u + 1);
        }
        else if (key == VK_DOWN) {  // ����ʵ��
            if (mInstanceCount_u > 1) {
                ChangeInstanceNum(mInstanceCount_u - 1);
            }
        }
    }
       
    //����
    {
        //tab�л�����
        if (VK_TAB == key) {
            mCurAlbedo_u = (mCurAlbedo_u + 1) % 8; // ȷ�������� 0~7 ��Χ��
        }
        // ������
        if ('q' == key || 'Q' == key)
        {
            mMetallic_f += 0.05f;
            if (mMetallic_f > 1.0f)
            {
                mMetallic_f = 1.0f;
            }
            ATLTRACE(L"Metallic = %11.6f\n", mMetallic_f);
        }

        if ('a' == key || 'A' == key)
        {
            mMetallic_f -= 0.05f;
            if (mMetallic_f < 0.05f)
            {
                mMetallic_f = 0.05f;
            }
            ATLTRACE(L"Metallic = %11.6f\n", mMetallic_f);
        }

        // �ֲڶ�
        if ('w' == key || 'W' == key)
        {
            mRoughness_f += 0.05f;
            if (mRoughness_f > 1.0f)
            {
                mRoughness_f = 1.0f;
            }
            ATLTRACE(L"Roughness = %11.6f\n", mRoughness_f);
        }

        if ('s' == key || 'S' == key)
        {
            mRoughness_f -= 0.05f;
            if (mRoughness_f < 0.0f)
            {
                mRoughness_f = 0.0f;
            }
            ATLTRACE(L"Roughness = %11.6f\n", mRoughness_f);
        }

        // �����ڵ�
        if ('e' == key || 'E' == key)
        {
            mAo_f += 0.1f;
            if (mAo_f > 1.0f)
            {
                mAo_f = 1.0f;
            }
            ATLTRACE(L"Ambient Occlusion = %11.6f\n", mAo_f);
        }

        if ('d' == key || 'D' == key)
        {
            mAo_f -= 0.1f;
            if (mAo_f < 0.0f)
            {
                mAo_f = 0.0f;
            }
            ATLTRACE(L"Ambient Occlusion = %11.6f\n", mAo_f);
        }
    }

}

void Cube::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(Window::GetHwnd());
}

void Cube::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void Cube::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        mTheta -= dx;
        mPhi -= dy;

        // ���� mPhi ��Χ���������������
        //mPhi = std::clamp(mPhi, 0.1f, XM_PI - 0.1f);
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        // �Ҽ������߼�
        // ��������ƶ�����
        float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

        // ������������ - �����ƶ���С�������ƶ��Ŵ�
        mScaleFactor += dy;

        // �������ŷ�Χ (��ѡ)
        mScaleFactor = std::clamp(mScaleFactor, 0.1f, 10.0f);
        //mScaleFactor = XMScalarClamp(mScaleFactor, 0.1f, 10.0f);
    }

    mRotationMatrix = XMMatrixRotationY(mTheta) * XMMatrixRotationX(mPhi);
    mScaleMatrix = XMMatrixScaling(mScaleFactor, mScaleFactor, mScaleFactor);
    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void Cube::LoadTextures(LPCWSTR fileName)
{
}

BOOL Cube::loadMesh(LPCWSTR fileName, UINT& vertexCount_n, VERTEX*& vertex_pp, UINT*& indics_pp)
{
    std::ifstream fin;
    char input;
    BOOL bRet = TRUE;
    try {
        fin.open(fileName);
        if (fin.fail()) {
            AtlThrow(E_FAIL);
        }
        fin.get(input);
        while (input != ':')
        {
            fin.get(input);
        }
        fin >> vertexCount_n;

        fin.get(input);
        while (input != ':')
        {
            fin.get(input);
        }
        fin.get(input);
        fin.get(input);

        vertex_pp = (VERTEX*)GRS_CALLOC(vertexCount_n * sizeof(VERTEX));
        indics_pp = (UINT*)GRS_CALLOC(vertexCount_n * sizeof(UINT));

        for (UINT i = 0; i < vertexCount_n; i++)
        {
            fin >> vertex_pp[i].mPos_v4.x >> vertex_pp[i].mPos_v4.y >> vertex_pp[i].mPos_v4.z;
            vertex_pp[i].mPos_v4.w = 1.0f;      //��ĵ�4ά������Ϊ1
            fin >> vertex_pp[i].mTex_v2.x >> vertex_pp[i].mTex_v2.y;
            fin >> vertex_pp[i].mNormal_v4.x >> vertex_pp[i].mNormal_v4.y >> vertex_pp[i].mNormal_v4.z;
            vertex_pp[i].mNormal_v4.w = 0.0f;        //�������ĵ�4ά������Ϊ0
            indics_pp[i] = i;
        }
    }
    catch (CAtlException& e)
    {
        e;
        bRet = FALSE;
    }
    return bRet;
}

void Cube::LoadPipeline()
{
    //����DXGI��������
    {
        UINT dxgiFactoryFlags_n = 0;
#ifdef _DEBUG
        //����ʾ��ϵͳ�ĵ���ģʽ
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            dxgiFactoryFlags_n |= DXGI_CREATE_FACTORY_DEBUG;
        }
#endif
        ComPtr<IDXGIFactory5> idxgiFactory_p;
        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags_n, IID_PPV_ARGS(&idxgiFactory_p)));
        //��ȡIDXGIFactory6�Ľӿ�
        ThrowIfFailed(idxgiFactory_p.As(&mDxgiFactory6_p));
    }

    //ö�������������豸
    {
        ComPtr<IDXGIAdapter1> dxgiAdapter_p;
        ThrowIfFailed(mDxgiFactory6_p->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&dxgiAdapter_p)));

        //�����豸
        ThrowIfFailed(D3D12CreateDevice(dxgiAdapter_p.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice_p)));

        //�õ�ÿ��������Ԫ�صĴ�С
        mRtvDescriptorSize_n = mDevice_p->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        mSamplerDescriptorSize_n = mDevice_p->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        mCbvSrvDescriptorSize_n = mDevice_p->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        mDsvDescriptorSize_n = mDevice_p->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }

    //����������С�����Allocator�������б�
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        ThrowIfFailed(mDevice_p->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue_p)));
        ThrowIfFailed(mDevice_p->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator_p)));
        ThrowIfFailed(mDevice_p->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator_p.Get(), nullptr, IID_PPV_ARGS(&mCommandList_p)));
    }

    //����������
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = mFrameBackBufCount_n;
        swapChainDesc.Width = 0;
        swapChainDesc.Height = 0;
        swapChainDesc.Format = mRenderTargetFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        swapChainDesc.Scaling = DXGI_SCALING_NONE;

        ComPtr<IDXGISwapChain1> swapChain1_p;
        ThrowIfFailed(mDxgiFactory6_p->CreateSwapChainForHwnd(
            mCommandQueue_p.Get(),
            Window::GetHwnd(),
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1_p
        ));
        ThrowIfFailed(swapChain1_p.As(&mSwapChain3_p));

        //�õ���ǰ�󻺳�������ţ�Ҳ������һ����Ҫ������ʾ�Ļ����������
        mFrameIndex_n = mSwapChain3_p->GetCurrentBackBufferIndex();


    }
}

void Cube::LoadAssets()
{
    USES_CONVERSION;//ת����ʼ���꣬ʹT2A������
    //����RTV�������ѡ�����RTV
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = mFrameBackBufCount_n;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(mDevice_p->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap_p)));

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap_p->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < mFrameBackBufCount_n; i++)
        {
            ThrowIfFailed(mSwapChain3_p->GetBuffer(i, IID_PPV_ARGS(&mRenderTargets_p[i])));
            mDevice_p->CreateRenderTargetView(mRenderTargets_p[i].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += mRtvDescriptorSize_n;//�������������ƶ�ָ�뵽��һ��RTV����ȾĿ����ͼ��λ��
        }
    }

     //������Ȼ�������������Ȼ�����������
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = mDepthStencilFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//�������ģ����ͼ��DSV����ά��Ϊ��ά����
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = mDepthStencilFormat;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        D3D12_RESOURCE_DESC dsTex2DDesc = {};
        dsTex2DDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        dsTex2DDesc.Alignment = 0;
        dsTex2DDesc.Width = mWidth;
        dsTex2DDesc.Height = mHeight;
        dsTex2DDesc.DepthOrArraySize = 1;
        dsTex2DDesc.MipLevels = 1;
        dsTex2DDesc.Format = mDepthStencilFormat;
        dsTex2DDesc.SampleDesc.Count = 1;
        dsTex2DDesc.SampleDesc.Quality = 0;
        dsTex2DDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        dsTex2DDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        //ʹ����ʽĬ�϶Ѵ���һ�����ģ�建����
        D3D12_HEAP_PROPERTIES defaultHeapProp = {};
        defaultHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
        defaultHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        defaultHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        defaultHeapProp.CreationNodeMask = 0;
        defaultHeapProp.VisibleNodeMask = 0;
        ThrowIfFailed(mDevice_p->CreateCommittedResource(
            &defaultHeapProp,
            D3D12_HEAP_FLAG_NONE,
            &dsTex2DDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(&mDSBuffer_p)
        ));
        //�������ģ����ͼ
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(mDevice_p->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap_p)));

        mDevice_p->CreateDepthStencilView(mDSBuffer_p.Get(), &dsvDesc, mDsvHeap_p->GetCPUDescriptorHandleForHeapStart());
    }
    
    //����Χ��
    {
        ThrowIfFailed(mDevice_p->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence_p)));
        mFenceValue_n = 1;
        //����һ��Eventͬ���������ڵȴ�Χ���¼�֪ͨ
        mFenceEvent_h = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (mFenceEvent_h == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    //����shader��������Ⱦ����״̬����
    {
        //������ǩ��
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        //����Ƿ�֧��V1.1�汾�ĸ�ǩ��
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if (FAILED(mDevice_p->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            ThrowIfFailed(E_NOTIMPL);
        }

        D3D12_DESCRIPTOR_RANGE1 descriptorRange1[1] = {};
        descriptorRange1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        descriptorRange1[0].NumDescriptors = mConstBufferCount_n;
        descriptorRange1[0].BaseShaderRegister = 0;
        descriptorRange1[0].RegisterSpace = 0;
        descriptorRange1[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
        descriptorRange1[0].OffsetInDescriptorsFromTableStart = 0;

        D3D12_ROOT_PARAMETER1 rootParameters[1] = {};
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRange1[0];

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        rootSignatureDesc.Desc_1_1.NumParameters = _countof(rootParameters);
        rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
        rootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;
        rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;

        ComPtr<ID3DBlob> signatureBlob_p;
        ComPtr<ID3DBlob> errorBlob_p;
        HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signatureBlob_p, &errorBlob_p);
        if (FAILED(hr)) {
            ATLTRACE("������ǩ��ʧ�ܣ�%s\n", (CHAR*)(errorBlob_p ? errorBlob_p->GetBufferPointer() : "δ֪����"));
            AtlThrow(hr);
        }
        ThrowIfFailed(mDevice_p->CreateRootSignature(
            0,
            signatureBlob_p->GetBufferPointer(),
            signatureBlob_p->GetBufferSize(),
            IID_PPV_ARGS(&mRootSignature_p)));

        //����shader
        UINT compileFlags = 0;
#ifdef _DEBUG
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG
        //����Ϊ�о�����ʽ
        compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

        ComPtr<ID3DBlob> vsMode1_p, psMode1_p;
        //vs
        errorBlob_p.Reset();
        std::wstring vsFilePath_ws = GetAssetFullPath(L"CUBE\\Shaders\\VertexShaderCubePBR.hlsl");
        LPCWSTR vsFilePath = vsFilePath_ws.c_str();
        hr = D3DCompileFromFile(vsFilePath, nullptr, nullptr, "VSMain", "vs_5_1", compileFlags, 0, &vsMode1_p, &errorBlob_p);
        if (FAILED(hr)) {
            ATLTRACE("����vs\"&s\"��������%s\n",
                T2A(vsFilePath),
                errorBlob_p ? errorBlob_p->GetBufferPointer() : "�޷���ȡ�ļ�");
            ThrowIfFailed(hr);
        }
        //ps
        errorBlob_p.Reset();
        std::wstring psFilePath_ws = GetAssetFullPath(L"CUBE\\Shaders\\PixelShaderCubePBR.hlsl");
        LPCWSTR psFilePath = psFilePath_ws.c_str();
        hr = D3DCompileFromFile(psFilePath, nullptr, nullptr, "PSMain", "ps_5_1", compileFlags, 0, &psMode1_p, &errorBlob_p);
        if (FAILED(hr)) {
            ATLTRACE("����ps\"&s\"��������%s\n",
                T2A(psFilePath),
                errorBlob_p ? errorBlob_p->GetBufferPointer() : "�޷���ȡ�ļ�");
            ThrowIfFailed(hr);
        }

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            //���0
            {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0,32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

            // �������ʵ�����ݴӲ��1���룬ǰ�ĸ�������ͬ���һ�����󣬽�ʵ����ģ�;ֲ��ռ�任������ռ�
            { "WORLD",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "WORLD",   1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "WORLD",   2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "WORLD",   3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "COLOR",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "COLOR",   1, DXGI_FORMAT_R32_FLOAT,          1,80, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "COLOR",   2, DXGI_FORMAT_R32_FLOAT,          1,84, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "COLOR",   3, DXGI_FORMAT_R32_FLOAT,          1,88, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        };

        //����PSO
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };

        psoDesc.pRootSignature = mRootSignature_p.Get();
        psoDesc.VS.pShaderBytecode = vsMode1_p->GetBufferPointer();
        psoDesc.VS.BytecodeLength = vsMode1_p->GetBufferSize();
        psoDesc.PS.pShaderBytecode = psMode1_p->GetBufferPointer();
        psoDesc.PS.BytecodeLength = psMode1_p->GetBufferSize();

        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

        psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
        psoDesc.BlendState.IndependentBlendEnable = FALSE;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        psoDesc.DSVFormat = mDepthStencilFormat;
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.DepthStencilState.StencilEnable = FALSE;

        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = mRenderTargetFormat;
        psoDesc.SampleDesc.Count = 1;

        ThrowIfFailed(mDevice_p->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState_p)));
    }

    //����SRV CBV Sample��������
    {
        //��������ͼ��������cbv����������һ��
        D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
        cbvSrvHeapDesc.NumDescriptors = mConstBufferCount_n;
        cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(mDevice_p->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&mCbvSrvHeap_p)));
    }

    //������������Ͷ�Ӧ������
    {
        //�ϴ�������
        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        uploadHeapProps.CreationNodeMask = 0;
        uploadHeapProps.VisibleNodeMask = 0;

        D3D12_RESOURCE_DESC bufferResDesc = {};
        bufferResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        bufferResDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferResDesc.Width = 0;
        bufferResDesc.Height = 1;
        bufferResDesc.DepthOrArraySize = 1;
        bufferResDesc.MipLevels = 1;
        bufferResDesc.SampleDesc.Count = 1;
        bufferResDesc.SampleDesc.Quality = 0;

        //mvp��������
        bufferResDesc.Width = GRS_UPPER(sizeof(CB_MVP), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        ThrowIfFailed(mDevice_p->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferResDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mCBMVP_p)
        ));
        //map��ȥ
        ThrowIfFailed(mCBMVP_p->Map(0, nullptr, reinterpret_cast<void**>(&mCBMVP_ptr)));

        //�������
        bufferResDesc.Width = GRS_UPPER(sizeof(CB_CAMERA), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        ThrowIfFailed(mDevice_p->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferResDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mCbCamera_p)
        ));
        //map��ȥ
        ThrowIfFailed(mCbCamera_p->Map(0, nullptr, reinterpret_cast<void**>(&mCbCamera_ptr)));

        //��������
        bufferResDesc.Width = GRS_UPPER(sizeof(CB_LIGHTS), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        ThrowIfFailed(mDevice_p->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferResDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mCbLights_p)
        ));
        //map��ȥ
        mCbLights_p->Map(0, nullptr, reinterpret_cast<void**>(&mCbLights_ptr));

        //��������
        bufferResDesc.Width = GRS_UPPER(sizeof(CB_PBR) * mInstanceCount_u, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        ThrowIfFailed(mDevice_p->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferResDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mCbPbr_p)
        ));
        //map��ȥ
        mCbPbr_p->Map(0, nullptr, reinterpret_cast<void**>(&mCbPbr_ptr));


        //����������������ͼ���
        D3D12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle = mCbvSrvHeap_p->GetCPUDescriptorHandleForHeapStart();
        //����һ��������������ͼ
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = mCBMVP_p->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(CB_MVP), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        mDevice_p->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);

        cbvSrvHandle.ptr += mCbvSrvDescriptorSize_n;
        cbvDesc.BufferLocation = mCbCamera_p->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(CB_CAMERA), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        mDevice_p->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);

        cbvSrvHandle.ptr += mCbvSrvDescriptorSize_n;
        cbvDesc.BufferLocation = mCbLights_p->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(CB_LIGHTS), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        mDevice_p->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);

        cbvSrvHandle.ptr += mCbvSrvDescriptorSize_n;
        cbvDesc.BufferLocation = mCbPbr_p->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(CB_PBR) * mInstanceCount_u, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        mDevice_p->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);
    }

    //����ģ����Դ
    {
        std::wstring meshFilePath_ws = GetAssetFullPath(L"CUBE\\Assets\\meshResources\\CubeBig.txt");
        LPCWSTR meshFilePath = meshFilePath_ws.c_str();

        VERTEX* vertices = nullptr;
        UINT* indices = nullptr;
        UINT vertexCount = 0;
        loadMesh(meshFilePath, vertexCount, vertices, indices);
        mIndexCount_n = vertexCount;

        //�ϴ�������
        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        uploadHeapProps.CreationNodeMask = 0;
        uploadHeapProps.VisibleNodeMask = 0;

        D3D12_RESOURCE_DESC bufferResDesc = {};
        bufferResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        bufferResDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferResDesc.Width = 0;
        bufferResDesc.Height = 1;
        bufferResDesc.DepthOrArraySize = 1;
        bufferResDesc.MipLevels = 1;
        bufferResDesc.SampleDesc.Count = 1;
        bufferResDesc.SampleDesc.Quality = 0;

        bufferResDesc.Width = vertexCount * sizeof(VERTEX);
        //����vertex buffer��ʹ���ϴ���ʽ��
        ThrowIfFailed(mDevice_p->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferResDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mVertexBuffer_p)
        ));
        //��������
        UINT8* vertexDataBegin_n_ptr = nullptr;
        D3D12_RANGE readRange = { 0,0 };
        ThrowIfFailed(mVertexBuffer_p->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin_n_ptr)));
        memcpy(vertexDataBegin_n_ptr, vertices, vertexCount * sizeof(VERTEX));
        mVertexBuffer_p->Unmap(0, nullptr);

        //����index buffer��ʹ���ϴ���ʽ��
        bufferResDesc.Width = vertexCount * sizeof(UINT);
        ThrowIfFailed(mDevice_p->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferResDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mIndexBuffer_p)
        ));
        //��������
        UINT8* indexDataBegin_n_ptr = nullptr;
        ThrowIfFailed(mIndexBuffer_p->Map(0, &readRange, reinterpret_cast<void**>(&indexDataBegin_n_ptr)));
        memcpy(indexDataBegin_n_ptr, indices, mIndexCount_n * sizeof(UINT));
        mIndexBuffer_p->Unmap(0, nullptr);

        //�������㻺������ͼ
        mVertexBufferView.BufferLocation = mVertexBuffer_p->GetGPUVirtualAddress();
        mVertexBufferView.StrideInBytes = sizeof(VERTEX);
        mVertexBufferView.SizeInBytes = vertexCount * sizeof(VERTEX);
        //����������������ͼ
        mIndexBufferView.BufferLocation = mIndexBuffer_p->GetGPUVirtualAddress();
        mIndexBufferView.SizeInBytes = mIndexCount_n * sizeof(UINT);
        mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
        //�ͷ�ָ��
        if (vertices != nullptr) {
            ::HeapFree(GetProcessHeap(), 0, vertices);
            vertices = nullptr;
        }
        if (indices != nullptr) {
            ::HeapFree(GetProcessHeap(), 0, indices);
            indices = nullptr;
        }
        //���ÿʵ������
        mInstanceBufferView.BufferLocation = mCbPbr_p->GetGPUVirtualAddress();
        mInstanceBufferView.SizeInBytes = sizeof(CB_PBR) * mInstanceCount_u;
        mInstanceBufferView.StrideInBytes = sizeof(CB_PBR);
    }
    //�������������
    ThrowIfFailed(mCommandList_p->Close());
    //���������б�
    ThrowIfFailed(mCommandList_p->Reset(mCommandAllocator_p.Get(), mPipelineState_p.Get()));
}

void Cube::PopulateCommandList()
{
    mFrameIndex_n = mSwapChain3_p->GetCurrentBackBufferIndex();
    //�����ӿںͲü�����
    mCommandList_p->RSSetViewports(1, &mViewport);
    mCommandList_p->RSSetScissorRects(1, &mScissorRect);
    //������Դ����
    D3D12_RESOURCE_BARRIER barrierDesc = {};
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrierDesc.Transition.pResource = mRenderTargets_p[mFrameIndex_n].Get();
    barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    mCommandList_p->ResourceBarrier(1, &barrierDesc);
    //��ʼ��¼����
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap_p->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mDsvHeap_p->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += mFrameIndex_n * mRtvDescriptorSize_n;

    // *** �ؼ���ȷ��DSV�����Ч ***
    if (mDsvHeap_p == nullptr || mDSBuffer_p == nullptr) {
        OutputDebugStringA("ERROR: DSV heap or buffer is null!\n");
        return;
    }

    //������ȾĿ��
    mCommandList_p->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);
    //�����ͼ
    mCommandList_p->ClearRenderTargetView(rtvHandle, mClearColor_f, 0, nullptr);
    mCommandList_p->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    //���ø�ǩ��
    mCommandList_p->SetGraphicsRootSignature(mRootSignature_p.Get());
    //����ͼԪ
    mCommandList_p->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //���ö��㻺����������������
    D3D12_VERTEX_BUFFER_VIEW buffers[] = { mVertexBufferView ,mInstanceBufferView };
    //mCommandList_p->IASetVertexBuffers(0, 2, buffers);
    mCommandList_p->IASetVertexBuffers(0, 1, &mVertexBufferView); // ���㻺�����󶨵���� 0
    mCommandList_p->IASetVertexBuffers(1, 1, &mInstanceBufferView); // ʵ���������󶨵���� 1
    mCommandList_p->IASetIndexBuffer(&mIndexBufferView);
    //������������
    ID3D12DescriptorHeap* ppHeaps[] = { mCbvSrvHeap_p.Get() };
    mCommandList_p->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    //���ø����������ô��뽫ͼ�������б�ĸ���������0�󶨵��������ѵ���ʼGPU���
    mCommandList_p->SetGraphicsRootDescriptorTable(0, mCbvSrvHeap_p->GetGPUDescriptorHandleForHeapStart());
    //draw
    mCommandList_p->DrawIndexedInstanced(mIndexCount_n, mInstanceCount_u, 0, 0, 0);

    //������Դ����
    mCommandList_p->ResourceBarrier(
        1,
        &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets_p[mFrameIndex_n].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT));
    //�ر������б�
    ThrowIfFailed(mCommandList_p->Close());

}

void Cube::WaitForPreviousFrame()
{
    if (!mCommandAllocator_p || !mFence_p) return;

    const UINT64 currentFenceValue = mFenceValue_n;
    ThrowIfFailed(mCommandQueue_p->Signal(mFence_p.Get(), currentFenceValue));
    mFenceValue_n++;
    if (mFence_p->GetCompletedValue() < currentFenceValue)
    {
        ThrowIfFailed(mFence_p->SetEventOnCompletion(currentFenceValue, mFenceEvent_h));
        WaitForSingleObject(mFenceEvent_h, INFINITE);
    }
}

void Cube::ChangeInstanceNum(UINT count)
{
    if (count > 0 && count != mInstanceCount_u)
    {
        // 1. �ȴ�GPU��ɵ�ǰ����
        WaitForPreviousFrame();

        // 2. �������Դӳ��
        if (mCbPbr_ptr) {
            mCbPbr_p->Unmap(0, nullptr);
            mCbPbr_ptr = nullptr;
        }
        mCbPbr_p.Reset();

        // 3. ����ʵ������
        mInstanceCount_u = count;

        // 4. ���´���ʵ������������
        D3D12_HEAP_PROPERTIES uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        D3D12_RESOURCE_DESC bufferResDesc = CD3DX12_RESOURCE_DESC::Buffer(
            GRS_UPPER(sizeof(CB_PBR) * mInstanceCount_u, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)
        );

        ThrowIfFailed(mDevice_p->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferResDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mCbPbr_p)
        ));

        // 5. ����ӳ����Դ
        ThrowIfFailed(mCbPbr_p->Map(0, nullptr, reinterpret_cast<void**>(&mCbPbr_ptr)));

        // 6. ����ʵ����������ͼ
        mInstanceBufferView.BufferLocation = mCbPbr_p->GetGPUVirtualAddress();
        mInstanceBufferView.SizeInBytes = sizeof(CB_PBR) * mInstanceCount_u;
        mInstanceBufferView.StrideInBytes = sizeof(CB_PBR);

        // 7. ���������� (CBV)
        D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = mCbvSrvHeap_p->GetCPUDescriptorHandleForHeapStart();
        cbvHandle.ptr += 3 * mCbvSrvDescriptorSize_n; // �ƶ������ĸ�CBVλ��

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = mCbPbr_p->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = (UINT)GRS_UPPER(sizeof(CB_PBR) * mInstanceCount_u,
            D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        mDevice_p->CreateConstantBufferView(&cbvDesc, cbvHandle);
    }
}

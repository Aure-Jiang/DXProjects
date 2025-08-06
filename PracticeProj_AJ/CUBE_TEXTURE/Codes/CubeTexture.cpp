#include "CubeTexture.h"

void CubeTexture::LoadPipeline()
{
	UINT dxgiFactoryFlags_u = 0U;
	//3、打开显示子系统的调试支持
	{
#if defined(_DEBUG)
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			// 打开附加的调试支持
			dxgiFactoryFlags_u |= DXGI_CREATE_FACTORY_DEBUG;
		}
#endif
	}

	//4、创建DXGI Factory对象
	{
		ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags_u, IID_PPV_ARGS(&mDXGIFactory5_cp)));
		// 关闭ALT+ENTER键切换全屏的功能，因为我们没有实现OnSize处理，所以先关闭
		ThrowIfFailed(mDXGIFactory5_cp->MakeWindowAssociation(Window::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));
	}

	//5、枚举适配器创建设备
	{//选择NUMA架构的独显来创建3D设备对象,暂时先不支持集显了，当然你可以修改这些行为
		DXGI_ADAPTER_DESC1 desc = {};
		D3D12_FEATURE_DATA_ARCHITECTURE stArchitecture = {};
		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != mDXGIFactory5_cp->EnumAdapters1(adapterIndex, &mDXGIAdapter_cp); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc = {};
			mDXGIAdapter_cp->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{//跳过软件虚拟适配器设备
				continue;
			}

			ThrowIfFailed(D3D12CreateDevice(mDXGIAdapter_cp.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&mDevice_cp)));
			ThrowIfFailed(mDevice_cp->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE
				, &stArchitecture, sizeof(D3D12_FEATURE_DATA_ARCHITECTURE)));

			if (!stArchitecture.UMA)
			{
				break;
			}

			mDevice_cp.Reset();
		}

		//---------------------------------------------------------------------------------------------
		if (nullptr == mDevice_cp.Get())
		{// 可怜的机器上居然没有独显 还是先退出了事 
			throw EHandle(E_FAIL);
		}
	}

	//6、创建直接命令队列
	{
		D3D12_COMMAND_QUEUE_DESC stQueueDesc = {};
		stQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ThrowIfFailed(mDevice_cp->CreateCommandQueue(&stQueueDesc, IID_PPV_ARGS(&mCommandQueue_cp)));
	}

	//7、创建直接命令列表
	{
		ThrowIfFailed(mDevice_cp->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT
			, IID_PPV_ARGS(&mCommandAllocator_cp)));
		//创建直接命令列表，在其上可以执行几乎所有的引擎命令（3D图形引擎、计算引擎、复制引擎等）
		//注意初始时并没有使用PSO对象，此时其实这个命令列表依然可以记录命令
		ThrowIfFailed(mDevice_cp->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT
			, mCommandAllocator_cp.Get(), nullptr, IID_PPV_ARGS(&mCommandList_cp)));
	}

	//8、创建交换链
	{
		DXGI_SWAP_CHAIN_DESC1 stSwapChainDesc = {};
		stSwapChainDesc.BufferCount = mFrameBackBufCount_u;
		stSwapChainDesc.Width = mWidth;
		stSwapChainDesc.Height = mHeight;
		stSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		stSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		stSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		stSwapChainDesc.SampleDesc.Count = 1;

		ComPtr<IDXGISwapChain1> swapChain1_cp;
		ThrowIfFailed(mDXGIFactory5_cp->CreateSwapChainForHwnd(
			mCommandQueue_cp.Get(),		// Swap chain needs the queue so that it can force a flush on it.
			Window::GetHwnd(),
			&stSwapChainDesc,
			nullptr,
			nullptr,
			&swapChain1_cp
		));

		//注意此处使用了高版本的SwapChain接口的函数
		ThrowIfFailed(swapChain1_cp.As(&mSwapChain3_cp));
		//获取当前交换链的后缓冲区索引，用于标识当前帧需要渲染到的缓冲区位置（如双缓冲 / 三缓冲中的第N个缓冲区）。
		mFrameIndex_u = mSwapChain3_cp->GetCurrentBackBufferIndex();

		//创建RTV(渲染目标视图)描述符堆(这里堆的含义应当理解为数组或者固定大小元素的固定大小显存池)
		D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
		stRTVHeapDesc.NumDescriptors = mFrameBackBufCount_u;
		stRTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		stRTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		ThrowIfFailed(mDevice_cp->CreateDescriptorHeap(&stRTVHeapDesc, IID_PPV_ARGS(&mRTVHeap_cp)));
		//得到每个描述符元素的大小
		mRTVDescriptorSize_u = mDevice_cp->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		//---------------------------------------------------------------------------------------------
		CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(mRTVHeap_cp->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < mFrameBackBufCount_u; i++)
		{//这个循环暴漏了描述符堆实际上是个数组的本质
			ThrowIfFailed(mSwapChain3_cp->GetBuffer(i, IID_PPV_ARGS(&mRenderTargets_cp[i])));
			mDevice_cp->CreateRenderTargetView(mRenderTargets_cp[i].Get(), nullptr, stRTVHandle);
			stRTVHandle.Offset(1, mRTVDescriptorSize_u);
		}
	}
}

void CubeTexture::LoadAssets()
{
	//9、创建 SRV CBV Sample堆
	{
		//我们将纹理视图描述符和CBV描述符放在一个描述符堆上
		D3D12_DESCRIPTOR_HEAP_DESC stSRVHeapDesc = {};
		stSRVHeapDesc.NumDescriptors = 2; //1 SRV + 1 CBV
		stSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		stSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(mDevice_cp->CreateDescriptorHeap(&stSRVHeapDesc, IID_PPV_ARGS(&mSRVHeap_cp)));

		D3D12_DESCRIPTOR_HEAP_DESC stSamplerHeapDesc = {};
		stSamplerHeapDesc.NumDescriptors = mSampleMaxCnt_u;
		stSamplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		stSamplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(mDevice_cp->CreateDescriptorHeap(&stSamplerHeapDesc, IID_PPV_ARGS(&mSamplerDescriptorHeap_cp)));

		//GetDescriptorHandleIncrementSize 是 Direct3D 12 API 中 ID3D12Device 接口的方法，用于查询指定类型描述符在描述符堆中的内存增量步长（以字节为单位）
		mSamplerDescriptorSize_U = mDevice_cp->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}

	//10、创建根签名
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE stFeatureData = {};
		// 检测是否支持V1.1版本的根签名
		stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(mDevice_cp->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &stFeatureData, sizeof(stFeatureData))))
		{
			stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}
		// 在GPU上执行SetGraphicsRootDescriptorTable后，我们不修改命令列表中的SRV，因此我们可以使用默认Rang行为:
		// D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
		// CD3DX12_DESCRIPTOR_RANGE1是DirectX 12中用于描述根签名参数的结构体
		CD3DX12_DESCRIPTOR_RANGE1 stDSPRanges[3];
		stDSPRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
		stDSPRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		stDSPRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

		CD3DX12_ROOT_PARAMETER1 stRootParameters[3];
		stRootParameters[0].InitAsDescriptorTable(1, &stDSPRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);//SRV仅PS可见
		stRootParameters[1].InitAsDescriptorTable(1, &stDSPRanges[1], D3D12_SHADER_VISIBILITY_ALL); //CBV是所有Shader可见
		stRootParameters[2].InitAsDescriptorTable(1, &stDSPRanges[2], D3D12_SHADER_VISIBILITY_PIXEL);//SAMPLE仅PS可见

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc;

		stRootSignatureDesc.Init_1_1(_countof(stRootParameters), stRootParameters
			, 0, nullptr
			, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> pISignatureBlob;
		ComPtr<ID3DBlob> pIErrorBlob;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&stRootSignatureDesc
			, stFeatureData.HighestVersion
			, &pISignatureBlob
			, &pIErrorBlob));

		ThrowIfFailed(mDevice_cp->CreateRootSignature(0
			, pISignatureBlob->GetBufferPointer()
			, pISignatureBlob->GetBufferSize()
			, IID_PPV_ARGS(&mRootSignature_cp)));
	}

	//11、编译Shader创建渲染管线状态对象
	{

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		//编译为行矩阵形式	   
		compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

		std::wstring shaderFullPath = GetAssetFullPath(L"CUBE_TEXTURE\\Shaders\\Shader.hlsl");
		LPCWSTR pszShaderFileName = shaderFullPath.c_str();
		ComPtr<ID3DBlob> blobVS;
		ComPtr<ID3DBlob> blobPS;
		ThrowIfFailed(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
			, "VSMain", "vs_5_0", compileFlags, 0, &blobVS, nullptr));
		ThrowIfFailed(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
			, "PSMain", "ps_5_0", compileFlags, 0, &blobPS, nullptr));

		// 我们多添加了一个法线的定义，但目前Shader中我们并没有使用
		D3D12_INPUT_ELEMENT_DESC stInputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// 创建 graphics pipeline state object (PSO)对象
		D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
		stPSODesc.InputLayout = { stInputElementDescs, _countof(stInputElementDescs) };
		stPSODesc.pRootSignature = mRootSignature_cp.Get();
		stPSODesc.VS = CD3DX12_SHADER_BYTECODE(blobVS.Get());
		stPSODesc.PS = CD3DX12_SHADER_BYTECODE(blobPS.Get());
		stPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		stPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		stPSODesc.DepthStencilState.DepthEnable = FALSE;
		stPSODesc.DepthStencilState.StencilEnable = FALSE;
		stPSODesc.SampleMask = UINT_MAX;
		stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		stPSODesc.NumRenderTargets = 1;
		stPSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		stPSODesc.SampleDesc.Count = 1;

		ThrowIfFailed(mDevice_cp->CreateGraphicsPipelineState(&stPSODesc
			, IID_PPV_ARGS(&mPipelineState_cp)));
	}

	//12、创建纹理的默认堆
	{
		const DirectX::TexMetadata& metadata = mImage.GetMetadata();

		// 创建纹理资源描述
		D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			metadata.format,
			static_cast<UINT64>(metadata.width),
			static_cast<UINT>(metadata.height),
			static_cast<UINT16>(metadata.arraySize),
			static_cast<UINT16>(metadata.mipLevels)
		);

		// 获取纹理资源分配信息
		D3D12_RESOURCE_ALLOCATION_INFO allocInfo =
			mDevice_cp->GetResourceAllocationInfo(0, 1, &textureDesc);

		D3D12_HEAP_DESC stTextureHeapDesc = {};
		stTextureHeapDesc.SizeInBytes = allocInfo.SizeInBytes;
		stTextureHeapDesc.Alignment = allocInfo.Alignment; // 使用API返回的对齐
		stTextureHeapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
		stTextureHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stTextureHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		stTextureHeapDesc.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS;

		ThrowIfFailed(mDevice_cp->CreateHeap(&stTextureHeapDesc, IID_PPV_ARGS(&mTextureHeap_cp)));
	}

	//13、创建2D纹理
	{
		const DirectX::TexMetadata& metadata = mImage.GetMetadata();
		stTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		stTextureDesc.MipLevels = 1;
		stTextureDesc.Format = mTextureFormat_fmt; //DXGI_FORMAT_R8G8B8A8_UNORM;
		stTextureDesc.Width = metadata.width;
		stTextureDesc.Height = metadata.height;
		stTextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		stTextureDesc.DepthOrArraySize = 1;
		stTextureDesc.SampleDesc.Count = 1;
		stTextureDesc.SampleDesc.Quality = 0;

		//-----------------------------------------------------------------------------------------------------------
		//使用“定位方式”来创建纹理，注意下面这个调用内部实际已经没有存储分配和释放的实际操作了，所以性能很高
		//同时可以在这个堆上反复调用CreatePlacedResource来创建不同的纹理，当然前提是它们不在被使用的时候，才考虑
		//重用堆
		ThrowIfFailed(mDevice_cp->CreatePlacedResource(
			mTextureHeap_cp.Get()
			, 0
			, &stTextureDesc				//可以使用CD3DX12_RESOURCE_DESC::Tex2D来简化结构体的初始化
			, D3D12_RESOURCE_STATE_COPY_DEST
			, nullptr
			, IID_PPV_ARGS(&mTexture_cp)));
		//-----------------------------------------------------------------------------------------------------------

		//获取上传堆资源缓冲的大小，这个尺寸通常大于实际图片的尺寸。用于后续在上传堆上计算偏移
		mUploadBufferSize_u64 = GetRequiredIntermediateSize(mTexture_cp.Get(), 0, 1);
	}

	//14、创建上传堆
	{
		//-----------------------------------------------------------------------------------------------------------
		D3D12_HEAP_DESC stUploadHeapDesc = {  };
		//尺寸依然是实际纹理数据大小的2倍并64K边界对齐大小
		stUploadHeapDesc.SizeInBytes = GRS_UPPER(2 * mUploadBufferSize_u64, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		//注意上传堆肯定是Buffer类型，可以不指定对齐方式，其默认是64k边界对齐
		stUploadHeapDesc.Alignment = 0;
		stUploadHeapDesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;		//上传堆类型
		stUploadHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stUploadHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		//上传堆就是缓冲，可以摆放任意数据
		stUploadHeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

		ThrowIfFailed(mDevice_cp->CreateHeap(&stUploadHeapDesc, IID_PPV_ARGS(&mUploadHeap_cp)));
		//-----------------------------------------------------------------------------------------------------------
	}

	//15、使用“定位方式”创建用于上传纹理数据的缓冲资源
	{
		ThrowIfFailed(mDevice_cp->CreatePlacedResource(mUploadHeap_cp.Get()
			, 0
			, &CD3DX12_RESOURCE_DESC::Buffer(mUploadBufferSize_u64)
			, D3D12_RESOURCE_STATE_GENERIC_READ
			, nullptr
			, IID_PPV_ARGS(&mTextureUpload_cp)));
	}

	//16、加载图片数据至上传堆，即完成第一个Copy动作，从memcpy函数可知这是由CPU完成的
	{
		// 使用DirectXTex提供的图像数据
		const DirectX::Image* pImage = mImage.GetImages();
		mTexRowPitch_u = static_cast<UINT>(pImage->rowPitch); // 设置正确的行距
		//按照资源缓冲大小来分配实际图片数据存储的内存大小
		void* pbPicData = ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, mUploadBufferSize_u64);
		if (nullptr == pbPicData)
		{
			throw EHandle(HRESULT_FROM_WIN32(GetLastError()));
		}
		// 直接从DirectXTex拷贝数据
		memcpy(pbPicData, pImage->pixels, pImage->slicePitch);
		
		//获取向上传堆拷贝纹理数据的一些纹理转换尺寸信息
		//对于复杂的DDS纹理这是非常必要的过程

		UINT   nNumSubresources = 1u;  //我们只有一副图片，即子资源个数为1
		UINT   nTextureRowNum = 0u;
		UINT64 n64TextureRowSizes = 0u;
		UINT64 n64RequiredSize = 0u;

		stDestDesc = mTexture_cp->GetDesc();

		mDevice_cp->GetCopyableFootprints(&stDestDesc
			, 0
			, nNumSubresources
			, 0
			, &stTxtLayouts
			, &nTextureRowNum
			, &n64TextureRowSizes
			, &n64RequiredSize);

		//因为上传堆实际就是CPU传递数据到GPU的中介
		//所以我们可以使用熟悉的Map方法将它先映射到CPU内存地址中
		//然后我们按行将数据复制到上传堆中
		//需要注意的是之所以按行拷贝是因为GPU资源的行大小
		//与实际图片的行大小是有差异的,二者的内存边界对齐要求是不一样的
		BYTE* pData = nullptr;
		ThrowIfFailed(mTextureUpload_cp->Map(0, NULL, reinterpret_cast<void**>(&pData)));

		BYTE* pDestSlice = reinterpret_cast<BYTE*>(pData) + stTxtLayouts.Offset;
		const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(pbPicData);
		for (UINT y = 0; y < nTextureRowNum; ++y)
		{
			memcpy(pDestSlice + static_cast<SIZE_T>(stTxtLayouts.Footprint.RowPitch) * y
				, pSrcSlice + static_cast<SIZE_T>(mTexRowPitch_u) * y
				, mTexRowPitch_u);
		}
		//取消映射 对于易变的数据如每帧的变换矩阵等数据，可以撒懒不用Unmap了，
		//让它常驻内存,以提高整体性能，因为每次Map和Unmap是很耗时的操作
		//因为现在起码都是64位系统和应用了，地址空间是足够的，被长期占用不会影响什么
		mTextureUpload_cp->Unmap(0, NULL);

		//释放图片数据，做一个干净的程序员
		::HeapFree(::GetProcessHeap(), 0, pbPicData);
	}

	//17、向直接命令列表发出从上传堆复制纹理数据到默认堆的命令，执行并同步等待，即完成第二个Copy动作，由GPU上的复制引擎完成
	//注意此时直接命令列表还没有绑定PSO对象，因此它也是不能执行3D图形命令的，但是可以执行复制命令，因为复制引擎不需要什么
	//额外的状态设置之类的参数
	{
		CD3DX12_TEXTURE_COPY_LOCATION Dst(mTexture_cp.Get(), 0);
		CD3DX12_TEXTURE_COPY_LOCATION Src(mTextureUpload_cp.Get(), stTxtLayouts);
		mCommandList_cp->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);

		//设置一个资源屏障，同步并确认复制操作完成
		//直接使用结构体然后调用的形式
		D3D12_RESOURCE_BARRIER stResBar = {};
		stResBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		stResBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		stResBar.Transition.pResource = mTexture_cp.Get();
		stResBar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		stResBar.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		stResBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		mCommandList_cp->ResourceBarrier(1, &stResBar);

		//或者使用D3DX12库中的工具类调用的等价形式，下面的方式更简洁一些
		//mCommandList_cp->ResourceBarrier(1
		//	, &CD3DX12_RESOURCE_BARRIER::Transition(pITexture.Get()
		//	, D3D12_RESOURCE_STATE_COPY_DEST
		//	, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		//);

		//---------------------------------------------------------------------------------------------
		// 执行命令列表并等待纹理资源上传完成，这一步是必须的
		ThrowIfFailed(mCommandList_cp->Close());
		ID3D12CommandList* ppCommandLists[] = { mCommandList_cp.Get() };
		mCommandQueue_cp->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		//---------------------------------------------------------------------------------------------
		// 17、创建一个同步对象——围栏，用于等待渲染完成，因为现在Draw Call是异步的了
		ThrowIfFailed(mDevice_cp->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence_cp)));
		mFenceValue_u64 = 1;

		//---------------------------------------------------------------------------------------------
		// 18、创建一个Event同步对象，用于等待围栏事件通知
		mFenceEvent_h = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (mFenceEvent_h == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		//---------------------------------------------------------------------------------------------
		// 19、等待纹理资源正式复制完成先
		const UINT64 fence = mFenceValue_u64;
		ThrowIfFailed(mCommandQueue_cp->Signal(mFence_cp.Get(), fence));
		mFenceValue_u64++;

		//---------------------------------------------------------------------------------------------
		// 看命令有没有真正执行到围栏标记的这里，没有就利用事件去等待，注意使用的是命令队列对象的指针
		if (mFence_cp->GetCompletedValue() < fence)
		{
			ThrowIfFailed(mFence_cp->SetEventOnCompletion(fence, mFenceEvent_h));
			WaitForSingleObject(mFenceEvent_h, INFINITE);
		}

		//---------------------------------------------------------------------------------------------
		//命令分配器先Reset一下，刚才已经执行过了一个复制纹理的命令
		ThrowIfFailed(mCommandAllocator_cp->Reset());
		//Reset命令列表，并重新指定命令分配器和PSO对象
		ThrowIfFailed(mCommandList_cp->Reset(mCommandAllocator_cp.Get(), mPipelineState_cp.Get()));
		//---------------------------------------------------------------------------------------------

	}

	//18、定义正方形的3D数据结构
	float fBoxSize = 3.0f;
	float fTCMax = 3.0f;
	Vertex stBoxVertices[] = {
		{ {-1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 0.0f * fTCMax}, {0.0f,  0.0f, -1.0f} },
		{ {1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f,  0.0f, -1.0f} },
		{ {-1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax}, {0.0f,  0.0f, -1.0f} },
		{ {-1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax}, {0.0f,  0.0f, -1.0f} },
		{ {1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f, 0.0f, -1.0f} },
		{ {1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 1.0f * fTCMax},  {0.0f,  0.0f, -1.0f} },
		{ {1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 0.0f * fTCMax},  {1.0f,  0.0f,  0.0f} },
		{ {1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {1.0f,  0.0f,  0.0f} },
		{ {1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax},  {1.0f,  0.0f,  0.0f} },
		{ {1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax},  {1.0f,  0.0f,  0.0f} },
		{ {1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {1.0f,  0.0f,  0.0f} },
		{ {1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 1.0f * fTCMax},  {1.0f,  0.0f,  0.0f} },
		{ {1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 0.0f * fTCMax},  {0.0f,  0.0f,  1.0f} },
		{ {-1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f,  0.0f,  1.0f} },
		{ {1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax}, {0.0f,  0.0f,  1.0f} },
		{ {1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax},  {0.0f,  0.0f,  1.0f} },
		{ {-1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f,  0.0f,  1.0f} },
		{ {-1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 1.0f * fTCMax},  {0.0f,  0.0f,  1.0f} },
		{ {-1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 0.0f * fTCMax}, {-1.0f,  0.0f,  0.0f} },
		{ {-1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax}, {-1.0f,  0.0f,  0.0f} },
		{ {-1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax}, {-1.0f,  0.0f,  0.0f} },
		{ {-1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax}, {-1.0f,  0.0f,  0.0f} },
		{ {-1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax}, {-1.0f,  0.0f,  0.0f} },
		{ {-1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 1.0f * fTCMax}, {-1.0f,  0.0f,  0.0f} },
		{ {-1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 0.0f * fTCMax},  {0.0f,  1.0f,  0.0f} },
		{ {1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f,  1.0f,  0.0f} },
		{ {-1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax},  {0.0f,  1.0f,  0.0f} },
		{ {-1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax},  {0.0f,  1.0f,  0.0f} },
		{ {1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f,  1.0f,  0.0f} },
		{ {1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 1.0f * fTCMax},  {0.0f,  1.0f,  0.0f }},
		{ {-1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {0.0f * fTCMax, 0.0f * fTCMax},  {0.0f, -1.0f,  0.0f} },
		{ {1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f, -1.0f,  0.0f} },
		{ {-1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax},  {0.0f, -1.0f,  0.0f} },
		{ {-1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {0.0f * fTCMax, 1.0f * fTCMax},  {0.0f, -1.0f,  0.0f} },
		{ {1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize}, {1.0f * fTCMax, 0.0f * fTCMax},  {0.0f, -1.0f,  0.0f} },
		{ {1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 1.0f * fTCMax},  {0.0f, -1.0f,  0.0f} },
	};

	const UINT nVertexBufferSize = sizeof(stBoxVertices);

	UINT32 pBoxIndices[] //立方体索引数组
		= {
		0,1,2,
		3,4,5,

		6,7,8,
		9,10,11,

		12,13,14,
		15,16,17,

		18,19,20,
		21,22,23,

		24,25,26,
		27,28,29,

		30,31,32,
		33,34,35,
	};

	const UINT nszIndexBuffer = sizeof(pBoxIndices);

	UINT64 n64BufferOffset = GRS_UPPER(mUploadBufferSize_u64, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	//19、使用“定位方式”创建顶点缓冲和索引缓冲，使用与上传纹理数据缓冲相同的一个上传堆
	{
		//---------------------------------------------------------------------------------------------
		//使用定位方式在相同的上传堆上以“定位方式”创建顶点缓冲，注意第二个参数指出了堆中的偏移位置
		//按照堆边界对齐的要求，我们主动将偏移位置对齐到了64k的边界上
		ThrowIfFailed(mDevice_cp->CreatePlacedResource(
			mUploadHeap_cp.Get()
			, n64BufferOffset
			, &CD3DX12_RESOURCE_DESC::Buffer(nVertexBufferSize)
			, D3D12_RESOURCE_STATE_GENERIC_READ
			, nullptr
			, IID_PPV_ARGS(&mVertexBuffer_cp)));

		//使用map-memcpy-unmap大法将数据传至顶点缓冲对象
		//注意顶点缓冲使用是和上传纹理数据缓冲相同的一个堆，这很神奇
		UINT8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE stReadRange(0, 0);		// We do not intend to read from this resource on the CPU.

		ThrowIfFailed(mVertexBuffer_cp->Map(0, &stReadRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, stBoxVertices, sizeof(stBoxVertices));
		mVertexBuffer_cp->Unmap(0, nullptr);

		//创建资源视图，实际可以简单理解为指向顶点缓冲的显存指针
		stVertexBufferView.BufferLocation = mVertexBuffer_cp->GetGPUVirtualAddress();
		stVertexBufferView.StrideInBytes = sizeof(Vertex);
		stVertexBufferView.SizeInBytes = nVertexBufferSize;
		//-----------------------------------------------------------//
		//计算边界对齐的正确的偏移位置
		n64BufferOffset = GRS_UPPER(n64BufferOffset + nVertexBufferSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

		ThrowIfFailed(mDevice_cp->CreatePlacedResource(
			mUploadHeap_cp.Get()
			, n64BufferOffset
			, &CD3DX12_RESOURCE_DESC::Buffer(nszIndexBuffer)
			, D3D12_RESOURCE_STATE_GENERIC_READ
			, nullptr
			, IID_PPV_ARGS(&mIndexBuffer_cp)));

		UINT8* pIndexDataBegin = nullptr;
		ThrowIfFailed(mIndexBuffer_cp->Map(0, &stReadRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, pBoxIndices, nszIndexBuffer);
		mIndexBuffer_cp->Unmap(0, nullptr);

		stIndexBufferView.BufferLocation = mIndexBuffer_cp->GetGPUVirtualAddress();
		stIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
		stIndexBufferView.SizeInBytes = nszIndexBuffer;
	}

	//20、在上传堆上以“定位方式”创建常量缓冲
	{
		n64BufferOffset = GRS_UPPER(n64BufferOffset + nszIndexBuffer, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

		// 创建常量缓冲 注意缓冲尺寸设置为256边界对齐大小
		ThrowIfFailed(mDevice_cp->CreatePlacedResource(
			mUploadHeap_cp.Get()
			, n64BufferOffset
			, &CD3DX12_RESOURCE_DESC::Buffer(mMVPBuffer_sz)
			, D3D12_RESOURCE_STATE_GENERIC_READ
			, nullptr
			, IID_PPV_ARGS(&mCBVUpload_cp)));

		// Map 之后就不再Unmap了 直接复制数据进去 这样每帧都不用map-copy-unmap浪费时间了
		ThrowIfFailed(mCBVUpload_cp->Map(0, nullptr, reinterpret_cast<void**>(&mMVPBuffer_p)));

	}

	//21、创建SRV描述符
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
		stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		stSRVDesc.Format = stTextureDesc.Format;
		stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		stSRVDesc.Texture2D.MipLevels = 1;
		mDevice_cp->CreateShaderResourceView(mTexture_cp.Get(), &stSRVDesc, mSRVHeap_cp->GetCPUDescriptorHandleForHeapStart());
	}

	//22、创建CBV描述符
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = mCBVUpload_cp->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = static_cast<UINT>(mMVPBuffer_sz);

		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(mSRVHeap_cp->GetCPUDescriptorHandleForHeapStart()
			, 1
			, mDevice_cp->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

		mDevice_cp->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);
	}

	//23、创建各种采样器
	{

		CD3DX12_CPU_DESCRIPTOR_HANDLE hSamplerHeap(mSamplerDescriptorHeap_cp->GetCPUDescriptorHandleForHeapStart());

		D3D12_SAMPLER_DESC stSamplerDesc = {};
		stSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

		stSamplerDesc.MinLOD = 0;
		stSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		stSamplerDesc.MipLODBias = 0.0f;
		stSamplerDesc.MaxAnisotropy = 1;
		stSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

		// Sampler 1
		stSamplerDesc.BorderColor[0] = 1.0f;
		stSamplerDesc.BorderColor[1] = 0.0f;
		stSamplerDesc.BorderColor[2] = 1.0f;
		stSamplerDesc.BorderColor[3] = 1.0f;
		stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		mDevice_cp->CreateSampler(&stSamplerDesc, hSamplerHeap);

		hSamplerHeap.Offset(mSamplerDescriptorSize_U);

		// Sampler 2
		stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		mDevice_cp->CreateSampler(&stSamplerDesc, hSamplerHeap);

		hSamplerHeap.Offset(mSamplerDescriptorSize_U);

		// Sampler 3
		stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		mDevice_cp->CreateSampler(&stSamplerDesc, hSamplerHeap);

		hSamplerHeap.Offset(mSamplerDescriptorSize_U);

		// Sampler 4
		stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		mDevice_cp->CreateSampler(&stSamplerDesc, hSamplerHeap);

		hSamplerHeap.Offset(mSamplerDescriptorSize_U);

		// Sampler 5
		stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		mDevice_cp->CreateSampler(&stSamplerDesc, hSamplerHeap);
	}
}

void CubeTexture::PopulateCommandList()
{
	//---------------------------------------------------------------------------------------------
	mCommandList_cp->SetGraphicsRootSignature(mRootSignature_cp.Get());
	ID3D12DescriptorHeap* ppHeaps[] = { mSRVHeap_cp.Get(),mSamplerDescriptorHeap_cp.Get() };
	mCommandList_cp->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	//设置SRV
	mCommandList_cp->SetGraphicsRootDescriptorTable(0, mSRVHeap_cp->GetGPUDescriptorHandleForHeapStart());

	CD3DX12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandle(mSRVHeap_cp->GetGPUDescriptorHandleForHeapStart()
		, 1
		, mDevice_cp->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	//设置CBV
	mCommandList_cp->SetGraphicsRootDescriptorTable(1, stGPUCBVHandle);


	CD3DX12_GPU_DESCRIPTOR_HANDLE hGPUSampler(mSamplerDescriptorHeap_cp->GetGPUDescriptorHandleForHeapStart()
		, mCurrentSamplerNO_u
		, mSamplerDescriptorSize_U);
	//设置Sample
	mCommandList_cp->SetGraphicsRootDescriptorTable(2, hGPUSampler);

	CD3DX12_VIEWPORT stViewPort(0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight));
	CD3DX12_RECT	 stScissorRect(0, 0, static_cast<LONG>(mWidth), static_cast<LONG>(mHeight));
	mCommandList_cp->RSSetViewports(1, &stViewPort);
	mCommandList_cp->RSSetScissorRects(1, &stScissorRect);

	//---------------------------------------------------------------------------------------------
	// 通过资源屏障判定后缓冲已经切换完毕可以开始渲染了
	mCommandList_cp->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets_cp[mFrameIndex_u].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//偏移描述符指针到指定帧缓冲视图位置
	CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(mRTVHeap_cp->GetCPUDescriptorHandleForHeapStart(), mFrameIndex_u, mRTVDescriptorSize_u);
	//设置渲染目标
	mCommandList_cp->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);

	// 继续记录命令，并真正开始新一帧的渲染
	const float clearColor[] = { 0.0f, 0.5f, 0.4f, 1.0f };
	mCommandList_cp->ClearRenderTargetView(stRTVHandle, clearColor, 0, nullptr);

	//注意我们使用的渲染手法是三角形列表，也就是通常的Mesh网格
	mCommandList_cp->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList_cp->IASetVertexBuffers(0, 1, &stVertexBufferView);
	mCommandList_cp->IASetIndexBuffer(&stIndexBufferView);

	//---------------------------------------------------------------------------------------------
	//Draw Call！！！
	mCommandList_cp->DrawIndexedInstanced(36, 1, 0, 0, 0);//这里写死了36，这里要写一个变量，根据模型来

	//---------------------------------------------------------------------------------------------
	//又一个资源屏障，用于确定渲染已经结束可以提交画面去显示了
	mCommandList_cp->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets_cp[mFrameIndex_u].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	//关闭命令列表，可以去执行了
	ThrowIfFailed(mCommandList_cp->Close());
}

void CubeTexture::WaitForPreviousFrame()
{
	const UINT64 fence = mFenceValue_u64;
	ThrowIfFailed(mCommandQueue_cp->Signal(mFence_cp.Get(), fence));
	mFenceValue_u64++;

	// Wait until the previous frame is finished.
	if (mFence_cp->GetCompletedValue() < fence)
	{
		ThrowIfFailed(mFence_cp->SetEventOnCompletion(fence, mFenceEvent_h));
		WaitForSingleObject(mFenceEvent_h, INFINITE);
	}

	mFrameIndex_u = mSwapChain3_cp->GetCurrentBackBufferIndex();
}

void CubeTexture::LoadTexture(LPCWSTR fileName)
{
	//获取纹理路径
	std::wstring fullPath = GetAssetFullPath(fileName);

	HRESULT hr = LoadFromWICFile(
		fullPath.c_str(), // 文件路径
		WIC_FLAGS_NONE, // 加载标志（如忽略sRGB）
		nullptr,       // 可选的WIC元数据
		mImage          // 输出图像数据
	);
	// 确保格式被正确设置
	mTextureFormat_fmt = mImage.GetMetadata().format;
	if (mTextureFormat_fmt == DXGI_FORMAT_UNKNOWN)
	{
		// 如果格式未知，默认使用R8G8B8A8_UNORM
		mTextureFormat_fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	
}

CubeTexture::CubeTexture(UINT width, UINT height, std::wstring name) :
	DXFrame(width, height, name)
{
}

void CubeTexture::OnInit()
{
	//初始化渲染开始事件
	mFrameStart_n64 = GetTickCount64();
	mCurrent_n64 = mFrameStart_n64;
	//初始化纹理
	LoadTexture(L"CUBE_TEXTURE\\Assets\\TextureResources\\container.jpg");
	//加载管道
	LoadPipeline();
	//初始化资源
	LoadAssets();
}

void CubeTexture::OnUpdate()
{
	XMMATRIX model = XMMatrixIdentity();
	//左乘缩放矩阵
	model = XMMatrixMultiply(model, mScaleMatrix);
	//设置自动旋转
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
	//左乘旋转矩阵旋转
	model = XMMatrixMultiply(model, mRotationMatrix);

	XMMATRIX view = XMMatrixLookAtLH(mEye_v4, mAt_v4, mUp_v4);

	XMMATRIX projection = XMMatrixPerspectiveFovLH(
		XM_PIDIV4,
		static_cast<float>(mWidth) / static_cast<float>(mHeight),
		0.1f,
		1000.0f
	);
	XMMATRIX mvp = model * view * projection;

	XMStoreFloat4x4(&mMVPBuffer_p->mMvp_mx4, mvp);
	
}

void CubeTexture::OnRender()
{
	//设置命令
	PopulateCommandList();

	//执行命令列表
	ID3D12CommandList* ppCommandLists[] = { mCommandList_cp.Get() };
	mCommandQueue_cp->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//---------------------------------------------------------------------------------------------
	//提交画面
	ThrowIfFailed(mSwapChain3_cp->Present(1, 0));
	//同步
	WaitForPreviousFrame();

	//---------------------------------------------------------------------------------------------
	//命令分配器先Reset一下
	ThrowIfFailed(mCommandAllocator_cp->Reset());
	//Reset命令列表，并重新指定命令分配器和PSO对象
	ThrowIfFailed(mCommandList_cp->Reset(mCommandAllocator_cp.Get(), mPipelineState_cp.Get()));

}

void CubeTexture::OnDestroy()
{
	WaitForPreviousFrame();

	CloseHandle(mFenceEvent_h);
}

void CubeTexture::OnResize(UINT width, UINT height, bool minimized)
{
}

void CubeTexture::OnKeyDown(UINT8 key)
{
	//旋转
	{
		//加速旋转
		if (key == VK_ADD) {
			mRotationSpeed_d += 0.1f;
		}
		//减速旋转
		if (key == VK_SUBTRACT) {
			mRotationSpeed_d -= 0.1f;
			if (mRotationSpeed_d < 0.0f) {
				mRotationSpeed_d = 0.0f;
			}
		}
		//空格暂停恢复旋转
		if (key == VK_SPACE) {
			if (mIsRotate) {
				// 记录当前旋转角度作为下次恢复的起始角度
				mPausedRotationAngle_d = mModelRotationYAngle_d;
			}
			else {
				// 恢复旋转时更新起始时间
				mFrameStart_n64 = GetTickCount64() - static_cast<DWORD>((mPausedRotationAngle_d / mPalstance_d) * 1000);
			}
			mIsRotate = !mIsRotate;
		}
	}
	//切换采样器
	{
		if (key == VK_TAB) {
			mCurrentSamplerNO_u++;
            mCurrentSamplerNO_u = mCurrentSamplerNO_u % mSampleMaxCnt_u;
		}
	}

}

void CubeTexture::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(Window::GetHwnd());
}

void CubeTexture::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CubeTexture::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta -= dx;
		mPhi -= dy;

		// 限制 mPhi 范围，避免万向节死锁
		//mPhi = std::clamp(mPhi, 0.1f, XM_PI - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// 右键缩放逻辑
		// 计算鼠标移动距离
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

		// 更新缩放因子 - 向上移动缩小，向下移动放大
		mScaleFactor += dy;

		// 限制缩放范围 (可选)
		mScaleFactor = std::clamp(mScaleFactor, 0.1f, 10.0f);
		//mScaleFactor = XMScalarClamp(mScaleFactor, 0.1f, 10.0f);
	}

	mRotationMatrix = XMMatrixRotationY(mTheta) * XMMatrixRotationX(mPhi);
	mScaleMatrix = XMMatrixScaling(mScaleFactor, mScaleFactor, mScaleFactor);
	mLastMousePos.x = x;
	mLastMousePos.y = y;
}
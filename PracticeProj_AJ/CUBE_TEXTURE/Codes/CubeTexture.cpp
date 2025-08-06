#include "CubeTexture.h"

void CubeTexture::LoadPipeline()
{
	UINT dxgiFactoryFlags_u = 0U;
	//3������ʾ��ϵͳ�ĵ���֧��
	{
#if defined(_DEBUG)
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			// �򿪸��ӵĵ���֧��
			dxgiFactoryFlags_u |= DXGI_CREATE_FACTORY_DEBUG;
		}
#endif
	}

	//4������DXGI Factory����
	{
		ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags_u, IID_PPV_ARGS(&mDXGIFactory5_cp)));
		// �ر�ALT+ENTER���л�ȫ���Ĺ��ܣ���Ϊ����û��ʵ��OnSize���������ȹر�
		ThrowIfFailed(mDXGIFactory5_cp->MakeWindowAssociation(Window::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));
	}

	//5��ö�������������豸
	{//ѡ��NUMA�ܹ��Ķ���������3D�豸����,��ʱ�Ȳ�֧�ּ����ˣ���Ȼ������޸���Щ��Ϊ
		DXGI_ADAPTER_DESC1 desc = {};
		D3D12_FEATURE_DATA_ARCHITECTURE stArchitecture = {};
		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != mDXGIFactory5_cp->EnumAdapters1(adapterIndex, &mDXGIAdapter_cp); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc = {};
			mDXGIAdapter_cp->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{//������������������豸
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
		{// �����Ļ����Ͼ�Ȼû�ж��� �������˳����� 
			throw EHandle(E_FAIL);
		}
	}

	//6������ֱ���������
	{
		D3D12_COMMAND_QUEUE_DESC stQueueDesc = {};
		stQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ThrowIfFailed(mDevice_cp->CreateCommandQueue(&stQueueDesc, IID_PPV_ARGS(&mCommandQueue_cp)));
	}

	//7������ֱ�������б�
	{
		ThrowIfFailed(mDevice_cp->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT
			, IID_PPV_ARGS(&mCommandAllocator_cp)));
		//����ֱ�������б������Ͽ���ִ�м������е��������3Dͼ�����桢�������桢��������ȣ�
		//ע���ʼʱ��û��ʹ��PSO���󣬴�ʱ��ʵ��������б���Ȼ���Լ�¼����
		ThrowIfFailed(mDevice_cp->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT
			, mCommandAllocator_cp.Get(), nullptr, IID_PPV_ARGS(&mCommandList_cp)));
	}

	//8������������
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

		//ע��˴�ʹ���˸߰汾��SwapChain�ӿڵĺ���
		ThrowIfFailed(swapChain1_cp.As(&mSwapChain3_cp));
		//��ȡ��ǰ�������ĺ󻺳������������ڱ�ʶ��ǰ֡��Ҫ��Ⱦ���Ļ�����λ�ã���˫���� / �������еĵ�N������������
		mFrameIndex_u = mSwapChain3_cp->GetCurrentBackBufferIndex();

		//����RTV(��ȾĿ����ͼ)��������(����ѵĺ���Ӧ�����Ϊ������߹̶���СԪ�صĹ̶���С�Դ��)
		D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
		stRTVHeapDesc.NumDescriptors = mFrameBackBufCount_u;
		stRTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		stRTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		ThrowIfFailed(mDevice_cp->CreateDescriptorHeap(&stRTVHeapDesc, IID_PPV_ARGS(&mRTVHeap_cp)));
		//�õ�ÿ��������Ԫ�صĴ�С
		mRTVDescriptorSize_u = mDevice_cp->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		//---------------------------------------------------------------------------------------------
		CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(mRTVHeap_cp->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < mFrameBackBufCount_u; i++)
		{//���ѭ����©����������ʵ�����Ǹ�����ı���
			ThrowIfFailed(mSwapChain3_cp->GetBuffer(i, IID_PPV_ARGS(&mRenderTargets_cp[i])));
			mDevice_cp->CreateRenderTargetView(mRenderTargets_cp[i].Get(), nullptr, stRTVHandle);
			stRTVHandle.Offset(1, mRTVDescriptorSize_u);
		}
	}
}

void CubeTexture::LoadAssets()
{
	//9������ SRV CBV Sample��
	{
		//���ǽ�������ͼ��������CBV����������һ������������
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

		//GetDescriptorHandleIncrementSize �� Direct3D 12 API �� ID3D12Device �ӿڵķ��������ڲ�ѯָ�����������������������е��ڴ��������������ֽ�Ϊ��λ��
		mSamplerDescriptorSize_U = mDevice_cp->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}

	//10��������ǩ��
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE stFeatureData = {};
		// ����Ƿ�֧��V1.1�汾�ĸ�ǩ��
		stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(mDevice_cp->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &stFeatureData, sizeof(stFeatureData))))
		{
			stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}
		// ��GPU��ִ��SetGraphicsRootDescriptorTable�����ǲ��޸������б��е�SRV��������ǿ���ʹ��Ĭ��Rang��Ϊ:
		// D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
		// CD3DX12_DESCRIPTOR_RANGE1��DirectX 12������������ǩ�������Ľṹ��
		CD3DX12_DESCRIPTOR_RANGE1 stDSPRanges[3];
		stDSPRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
		stDSPRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		stDSPRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

		CD3DX12_ROOT_PARAMETER1 stRootParameters[3];
		stRootParameters[0].InitAsDescriptorTable(1, &stDSPRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);//SRV��PS�ɼ�
		stRootParameters[1].InitAsDescriptorTable(1, &stDSPRanges[1], D3D12_SHADER_VISIBILITY_ALL); //CBV������Shader�ɼ�
		stRootParameters[2].InitAsDescriptorTable(1, &stDSPRanges[2], D3D12_SHADER_VISIBILITY_PIXEL);//SAMPLE��PS�ɼ�

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

	//11������Shader������Ⱦ����״̬����
	{

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		//����Ϊ�о�����ʽ	   
		compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

		std::wstring shaderFullPath = GetAssetFullPath(L"CUBE_TEXTURE\\Shaders\\Shader.hlsl");
		LPCWSTR pszShaderFileName = shaderFullPath.c_str();
		ComPtr<ID3DBlob> blobVS;
		ComPtr<ID3DBlob> blobPS;
		ThrowIfFailed(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
			, "VSMain", "vs_5_0", compileFlags, 0, &blobVS, nullptr));
		ThrowIfFailed(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
			, "PSMain", "ps_5_0", compileFlags, 0, &blobPS, nullptr));

		// ���Ƕ������һ�����ߵĶ��壬��ĿǰShader�����ǲ�û��ʹ��
		D3D12_INPUT_ELEMENT_DESC stInputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// ���� graphics pipeline state object (PSO)����
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

	//12�����������Ĭ�϶�
	{
		const DirectX::TexMetadata& metadata = mImage.GetMetadata();

		// ����������Դ����
		D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			metadata.format,
			static_cast<UINT64>(metadata.width),
			static_cast<UINT>(metadata.height),
			static_cast<UINT16>(metadata.arraySize),
			static_cast<UINT16>(metadata.mipLevels)
		);

		// ��ȡ������Դ������Ϣ
		D3D12_RESOURCE_ALLOCATION_INFO allocInfo =
			mDevice_cp->GetResourceAllocationInfo(0, 1, &textureDesc);

		D3D12_HEAP_DESC stTextureHeapDesc = {};
		stTextureHeapDesc.SizeInBytes = allocInfo.SizeInBytes;
		stTextureHeapDesc.Alignment = allocInfo.Alignment; // ʹ��API���صĶ���
		stTextureHeapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
		stTextureHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stTextureHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		stTextureHeapDesc.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS;

		ThrowIfFailed(mDevice_cp->CreateHeap(&stTextureHeapDesc, IID_PPV_ARGS(&mTextureHeap_cp)));
	}

	//13������2D����
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
		//ʹ�á���λ��ʽ������������ע��������������ڲ�ʵ���Ѿ�û�д洢������ͷŵ�ʵ�ʲ����ˣ��������ܸܺ�
		//ͬʱ������������Ϸ�������CreatePlacedResource��������ͬ��������Ȼǰ�������ǲ��ڱ�ʹ�õ�ʱ�򣬲ſ���
		//���ö�
		ThrowIfFailed(mDevice_cp->CreatePlacedResource(
			mTextureHeap_cp.Get()
			, 0
			, &stTextureDesc				//����ʹ��CD3DX12_RESOURCE_DESC::Tex2D���򻯽ṹ��ĳ�ʼ��
			, D3D12_RESOURCE_STATE_COPY_DEST
			, nullptr
			, IID_PPV_ARGS(&mTexture_cp)));
		//-----------------------------------------------------------------------------------------------------------

		//��ȡ�ϴ�����Դ����Ĵ�С������ߴ�ͨ������ʵ��ͼƬ�ĳߴ硣���ں������ϴ����ϼ���ƫ��
		mUploadBufferSize_u64 = GetRequiredIntermediateSize(mTexture_cp.Get(), 0, 1);
	}

	//14�������ϴ���
	{
		//-----------------------------------------------------------------------------------------------------------
		D3D12_HEAP_DESC stUploadHeapDesc = {  };
		//�ߴ���Ȼ��ʵ���������ݴ�С��2����64K�߽�����С
		stUploadHeapDesc.SizeInBytes = GRS_UPPER(2 * mUploadBufferSize_u64, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		//ע���ϴ��ѿ϶���Buffer���ͣ����Բ�ָ�����뷽ʽ����Ĭ����64k�߽����
		stUploadHeapDesc.Alignment = 0;
		stUploadHeapDesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;		//�ϴ�������
		stUploadHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stUploadHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		//�ϴ��Ѿ��ǻ��壬���԰ڷ���������
		stUploadHeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

		ThrowIfFailed(mDevice_cp->CreateHeap(&stUploadHeapDesc, IID_PPV_ARGS(&mUploadHeap_cp)));
		//-----------------------------------------------------------------------------------------------------------
	}

	//15��ʹ�á���λ��ʽ�����������ϴ��������ݵĻ�����Դ
	{
		ThrowIfFailed(mDevice_cp->CreatePlacedResource(mUploadHeap_cp.Get()
			, 0
			, &CD3DX12_RESOURCE_DESC::Buffer(mUploadBufferSize_u64)
			, D3D12_RESOURCE_STATE_GENERIC_READ
			, nullptr
			, IID_PPV_ARGS(&mTextureUpload_cp)));
	}

	//16������ͼƬ�������ϴ��ѣ�����ɵ�һ��Copy��������memcpy������֪������CPU��ɵ�
	{
		// ʹ��DirectXTex�ṩ��ͼ������
		const DirectX::Image* pImage = mImage.GetImages();
		mTexRowPitch_u = static_cast<UINT>(pImage->rowPitch); // ������ȷ���о�
		//������Դ�����С������ʵ��ͼƬ���ݴ洢���ڴ��С
		void* pbPicData = ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, mUploadBufferSize_u64);
		if (nullptr == pbPicData)
		{
			throw EHandle(HRESULT_FROM_WIN32(GetLastError()));
		}
		// ֱ�Ӵ�DirectXTex��������
		memcpy(pbPicData, pImage->pixels, pImage->slicePitch);
		
		//��ȡ���ϴ��ѿ����������ݵ�һЩ����ת���ߴ���Ϣ
		//���ڸ��ӵ�DDS�������Ƿǳ���Ҫ�Ĺ���

		UINT   nNumSubresources = 1u;  //����ֻ��һ��ͼƬ��������Դ����Ϊ1
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

		//��Ϊ�ϴ���ʵ�ʾ���CPU�������ݵ�GPU���н�
		//�������ǿ���ʹ����Ϥ��Map����������ӳ�䵽CPU�ڴ��ַ��
		//Ȼ�����ǰ��н����ݸ��Ƶ��ϴ�����
		//��Ҫע�����֮���԰��п�������ΪGPU��Դ���д�С
		//��ʵ��ͼƬ���д�С���в����,���ߵ��ڴ�߽����Ҫ���ǲ�һ����
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
		//ȡ��ӳ�� �����ױ��������ÿ֡�ı任��������ݣ�������������Unmap�ˣ�
		//������פ�ڴ�,������������ܣ���Ϊÿ��Map��Unmap�Ǻܺ�ʱ�Ĳ���
		//��Ϊ�������붼��64λϵͳ��Ӧ���ˣ���ַ�ռ����㹻�ģ�������ռ�ò���Ӱ��ʲô
		mTextureUpload_cp->Unmap(0, NULL);

		//�ͷ�ͼƬ���ݣ���һ���ɾ��ĳ���Ա
		::HeapFree(::GetProcessHeap(), 0, pbPicData);
	}

	//17����ֱ�������б������ϴ��Ѹ����������ݵ�Ĭ�϶ѵ����ִ�в�ͬ���ȴ�������ɵڶ���Copy��������GPU�ϵĸ����������
	//ע���ʱֱ�������б�û�а�PSO���������Ҳ�ǲ���ִ��3Dͼ������ģ����ǿ���ִ�и��������Ϊ�������治��Ҫʲô
	//�����״̬����֮��Ĳ���
	{
		CD3DX12_TEXTURE_COPY_LOCATION Dst(mTexture_cp.Get(), 0);
		CD3DX12_TEXTURE_COPY_LOCATION Src(mTextureUpload_cp.Get(), stTxtLayouts);
		mCommandList_cp->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);

		//����һ����Դ���ϣ�ͬ����ȷ�ϸ��Ʋ������
		//ֱ��ʹ�ýṹ��Ȼ����õ���ʽ
		D3D12_RESOURCE_BARRIER stResBar = {};
		stResBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		stResBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		stResBar.Transition.pResource = mTexture_cp.Get();
		stResBar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		stResBar.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		stResBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		mCommandList_cp->ResourceBarrier(1, &stResBar);

		//����ʹ��D3DX12���еĹ�������õĵȼ���ʽ������ķ�ʽ�����һЩ
		//mCommandList_cp->ResourceBarrier(1
		//	, &CD3DX12_RESOURCE_BARRIER::Transition(pITexture.Get()
		//	, D3D12_RESOURCE_STATE_COPY_DEST
		//	, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		//);

		//---------------------------------------------------------------------------------------------
		// ִ�������б��ȴ�������Դ�ϴ���ɣ���һ���Ǳ����
		ThrowIfFailed(mCommandList_cp->Close());
		ID3D12CommandList* ppCommandLists[] = { mCommandList_cp.Get() };
		mCommandQueue_cp->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		//---------------------------------------------------------------------------------------------
		// 17������һ��ͬ�����󡪡�Χ�������ڵȴ���Ⱦ��ɣ���Ϊ����Draw Call���첽����
		ThrowIfFailed(mDevice_cp->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence_cp)));
		mFenceValue_u64 = 1;

		//---------------------------------------------------------------------------------------------
		// 18������һ��Eventͬ���������ڵȴ�Χ���¼�֪ͨ
		mFenceEvent_h = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (mFenceEvent_h == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		//---------------------------------------------------------------------------------------------
		// 19���ȴ�������Դ��ʽ���������
		const UINT64 fence = mFenceValue_u64;
		ThrowIfFailed(mCommandQueue_cp->Signal(mFence_cp.Get(), fence));
		mFenceValue_u64++;

		//---------------------------------------------------------------------------------------------
		// ��������û������ִ�е�Χ����ǵ����û�о������¼�ȥ�ȴ���ע��ʹ�õ���������ж����ָ��
		if (mFence_cp->GetCompletedValue() < fence)
		{
			ThrowIfFailed(mFence_cp->SetEventOnCompletion(fence, mFenceEvent_h));
			WaitForSingleObject(mFenceEvent_h, INFINITE);
		}

		//---------------------------------------------------------------------------------------------
		//�����������Resetһ�£��ղ��Ѿ�ִ�й���һ���������������
		ThrowIfFailed(mCommandAllocator_cp->Reset());
		//Reset�����б�������ָ�������������PSO����
		ThrowIfFailed(mCommandList_cp->Reset(mCommandAllocator_cp.Get(), mPipelineState_cp.Get()));
		//---------------------------------------------------------------------------------------------

	}

	//18�����������ε�3D���ݽṹ
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

	UINT32 pBoxIndices[] //��������������
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
	//19��ʹ�á���λ��ʽ���������㻺����������壬ʹ�����ϴ��������ݻ�����ͬ��һ���ϴ���
	{
		//---------------------------------------------------------------------------------------------
		//ʹ�ö�λ��ʽ����ͬ���ϴ������ԡ���λ��ʽ���������㻺�壬ע��ڶ�������ָ���˶��е�ƫ��λ��
		//���նѱ߽�����Ҫ������������ƫ��λ�ö��뵽��64k�ı߽���
		ThrowIfFailed(mDevice_cp->CreatePlacedResource(
			mUploadHeap_cp.Get()
			, n64BufferOffset
			, &CD3DX12_RESOURCE_DESC::Buffer(nVertexBufferSize)
			, D3D12_RESOURCE_STATE_GENERIC_READ
			, nullptr
			, IID_PPV_ARGS(&mVertexBuffer_cp)));

		//ʹ��map-memcpy-unmap�󷨽����ݴ������㻺�����
		//ע�ⶥ�㻺��ʹ���Ǻ��ϴ��������ݻ�����ͬ��һ���ѣ��������
		UINT8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE stReadRange(0, 0);		// We do not intend to read from this resource on the CPU.

		ThrowIfFailed(mVertexBuffer_cp->Map(0, &stReadRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, stBoxVertices, sizeof(stBoxVertices));
		mVertexBuffer_cp->Unmap(0, nullptr);

		//������Դ��ͼ��ʵ�ʿ��Լ����Ϊָ�򶥵㻺����Դ�ָ��
		stVertexBufferView.BufferLocation = mVertexBuffer_cp->GetGPUVirtualAddress();
		stVertexBufferView.StrideInBytes = sizeof(Vertex);
		stVertexBufferView.SizeInBytes = nVertexBufferSize;
		//-----------------------------------------------------------//
		//����߽�������ȷ��ƫ��λ��
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

	//20�����ϴ������ԡ���λ��ʽ��������������
	{
		n64BufferOffset = GRS_UPPER(n64BufferOffset + nszIndexBuffer, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

		// ������������ ע�⻺��ߴ�����Ϊ256�߽�����С
		ThrowIfFailed(mDevice_cp->CreatePlacedResource(
			mUploadHeap_cp.Get()
			, n64BufferOffset
			, &CD3DX12_RESOURCE_DESC::Buffer(mMVPBuffer_sz)
			, D3D12_RESOURCE_STATE_GENERIC_READ
			, nullptr
			, IID_PPV_ARGS(&mCBVUpload_cp)));

		// Map ֮��Ͳ���Unmap�� ֱ�Ӹ������ݽ�ȥ ����ÿ֡������map-copy-unmap�˷�ʱ����
		ThrowIfFailed(mCBVUpload_cp->Map(0, nullptr, reinterpret_cast<void**>(&mMVPBuffer_p)));

	}

	//21������SRV������
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
		stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		stSRVDesc.Format = stTextureDesc.Format;
		stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		stSRVDesc.Texture2D.MipLevels = 1;
		mDevice_cp->CreateShaderResourceView(mTexture_cp.Get(), &stSRVDesc, mSRVHeap_cp->GetCPUDescriptorHandleForHeapStart());
	}

	//22������CBV������
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = mCBVUpload_cp->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = static_cast<UINT>(mMVPBuffer_sz);

		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(mSRVHeap_cp->GetCPUDescriptorHandleForHeapStart()
			, 1
			, mDevice_cp->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

		mDevice_cp->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);
	}

	//23���������ֲ�����
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

	//����SRV
	mCommandList_cp->SetGraphicsRootDescriptorTable(0, mSRVHeap_cp->GetGPUDescriptorHandleForHeapStart());

	CD3DX12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandle(mSRVHeap_cp->GetGPUDescriptorHandleForHeapStart()
		, 1
		, mDevice_cp->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	//����CBV
	mCommandList_cp->SetGraphicsRootDescriptorTable(1, stGPUCBVHandle);


	CD3DX12_GPU_DESCRIPTOR_HANDLE hGPUSampler(mSamplerDescriptorHeap_cp->GetGPUDescriptorHandleForHeapStart()
		, mCurrentSamplerNO_u
		, mSamplerDescriptorSize_U);
	//����Sample
	mCommandList_cp->SetGraphicsRootDescriptorTable(2, hGPUSampler);

	CD3DX12_VIEWPORT stViewPort(0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight));
	CD3DX12_RECT	 stScissorRect(0, 0, static_cast<LONG>(mWidth), static_cast<LONG>(mHeight));
	mCommandList_cp->RSSetViewports(1, &stViewPort);
	mCommandList_cp->RSSetScissorRects(1, &stScissorRect);

	//---------------------------------------------------------------------------------------------
	// ͨ����Դ�����ж��󻺳��Ѿ��л���Ͽ��Կ�ʼ��Ⱦ��
	mCommandList_cp->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets_cp[mFrameIndex_u].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//ƫ��������ָ�뵽ָ��֡������ͼλ��
	CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(mRTVHeap_cp->GetCPUDescriptorHandleForHeapStart(), mFrameIndex_u, mRTVDescriptorSize_u);
	//������ȾĿ��
	mCommandList_cp->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);

	// ������¼�����������ʼ��һ֡����Ⱦ
	const float clearColor[] = { 0.0f, 0.5f, 0.4f, 1.0f };
	mCommandList_cp->ClearRenderTargetView(stRTVHandle, clearColor, 0, nullptr);

	//ע������ʹ�õ���Ⱦ�ַ����������б�Ҳ����ͨ����Mesh����
	mCommandList_cp->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList_cp->IASetVertexBuffers(0, 1, &stVertexBufferView);
	mCommandList_cp->IASetIndexBuffer(&stIndexBufferView);

	//---------------------------------------------------------------------------------------------
	//Draw Call������
	mCommandList_cp->DrawIndexedInstanced(36, 1, 0, 0, 0);//����д����36������Ҫдһ������������ģ����

	//---------------------------------------------------------------------------------------------
	//��һ����Դ���ϣ�����ȷ����Ⱦ�Ѿ����������ύ����ȥ��ʾ��
	mCommandList_cp->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets_cp[mFrameIndex_u].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	//�ر������б�����ȥִ����
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
	//��ȡ����·��
	std::wstring fullPath = GetAssetFullPath(fileName);

	HRESULT hr = LoadFromWICFile(
		fullPath.c_str(), // �ļ�·��
		WIC_FLAGS_NONE, // ���ر�־�������sRGB��
		nullptr,       // ��ѡ��WICԪ����
		mImage          // ���ͼ������
	);
	// ȷ����ʽ����ȷ����
	mTextureFormat_fmt = mImage.GetMetadata().format;
	if (mTextureFormat_fmt == DXGI_FORMAT_UNKNOWN)
	{
		// �����ʽδ֪��Ĭ��ʹ��R8G8B8A8_UNORM
		mTextureFormat_fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	
}

CubeTexture::CubeTexture(UINT width, UINT height, std::wstring name) :
	DXFrame(width, height, name)
{
}

void CubeTexture::OnInit()
{
	//��ʼ����Ⱦ��ʼ�¼�
	mFrameStart_n64 = GetTickCount64();
	mCurrent_n64 = mFrameStart_n64;
	//��ʼ������
	LoadTexture(L"CUBE_TEXTURE\\Assets\\TextureResources\\container.jpg");
	//���عܵ�
	LoadPipeline();
	//��ʼ����Դ
	LoadAssets();
}

void CubeTexture::OnUpdate()
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
	//��������
	PopulateCommandList();

	//ִ�������б�
	ID3D12CommandList* ppCommandLists[] = { mCommandList_cp.Get() };
	mCommandQueue_cp->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//---------------------------------------------------------------------------------------------
	//�ύ����
	ThrowIfFailed(mSwapChain3_cp->Present(1, 0));
	//ͬ��
	WaitForPreviousFrame();

	//---------------------------------------------------------------------------------------------
	//�����������Resetһ��
	ThrowIfFailed(mCommandAllocator_cp->Reset());
	//Reset�����б�������ָ�������������PSO����
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
	//�л�������
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
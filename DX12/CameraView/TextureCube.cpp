#include "TextureCube.h"

struct VERTEX
{
	//这次我们额外加入了每个顶点的法线，但Shader中还暂时没有用
	DirectX::XMFLOAT3 m_vPos;		//Position
	DirectX::XMFLOAT2 m_vTex;		//Texcoord
	DirectX::XMFLOAT3 m_vNor;		//Normal
};

TextureCube::TextureCube(HINSTANCE hInstance) : D3DApp(hInstance)
{

}

TextureCube::~TextureCube()
{
}

bool TextureCube::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	ThrowIfFailed(CreateRootSig());
	ThrowIfFailed(BuildShader());
	ThrowIfFailed(CreatePSO());
	ThrowIfFailed(LoadRenderData());
	ThrowIfFailed(CreateOthers());
	mn64tmFrameStart = ::GetTickCount64();
	mn64tmCurrent = mn64tmFrameStart;
	return true;
}

void TextureCube::OnResize()
{
	D3DApp::OnResize();
}

void TextureCube::Update(const GameTimer& gt)
{
}

void TextureCube::Draw(const GameTimer& gt)
{
	mn64tmCurrent = ::GetTickCount();

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));
	//计算旋转的角度：旋转角度(弧度) = 时间(秒) * 角速度(弧度/秒)
	//下面这句代码相当于经典游戏消息循环中的OnUpdate函数中需要做的事情
	mdModelRotationYAngle += ((mn64tmCurrent - mn64tmFrameStart) / 1000.0f) * fPalstance;

	mn64tmFrameStart = mn64tmCurrent;

	//旋转角度是2PI周期的倍数，去掉周期数，只留下相对0弧度开始的小于2PI的弧度即可
	if (mdModelRotationYAngle > DirectX::XM_2PI)
	{
		mdModelRotationYAngle = fmod(mdModelRotationYAngle, DirectX::XM_2PI);
	}

	//模型矩阵 model
	DirectX::XMMATRIX xmRot = DirectX::XMMatrixRotationY(static_cast<float>(mdModelRotationYAngle));

	//计算 模型矩阵 model * 视矩阵 view
	DirectX::XMMATRIX xmMVP = DirectX::XMMatrixMultiply(xmRot, DirectX::XMMatrixLookAtLH(Eye, At, Up));

	//投影矩阵 projection
	xmMVP = DirectX::XMMatrixMultiply(xmMVP, (DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, (FLOAT)mClientWidth / (FLOAT)mClientHeight, 0.1f, 1000.0f)));

	DirectX::XMStoreFloat4x4(&mpMVPBuffer->m_MVP, xmMVP);
	//---------------------------------------------------------------------------------------------
	
	//---------------------------------------------------------------------------------------------
	mCommandList->SetGraphicsRootSignature(mpIRootSignature.Get());
	ID3D12DescriptorHeap* ppHeaps[] = { mSrvHeap.Get(),mpISamplerDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	
	//设置SRV
	mCommandList->SetGraphicsRootDescriptorTable(0, mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	
	CD3DX12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandle(mSrvHeap->GetGPUDescriptorHandleForHeapStart()
		, 1
		, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	//设置CBV
	mCommandList->SetGraphicsRootDescriptorTable(1, stGPUCBVHandle);
	
	
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGPUSampler(mpISamplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
		, mnCurrentSamplerNO
		, mnSamplerDescriptorSize);
	//设置Sample
	mCommandList->SetGraphicsRootDescriptorTable(2, hGPUSampler);
	
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);
	
	//---------------------------------------------------------------------------------------------
	// 通过资源屏障判定后缓冲已经切换完毕可以开始渲染了
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	
	//偏移描述符指针到指定帧缓冲视图位置
	CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRtvDescriptorSize);
	//设置渲染目标
	mCommandList->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);
	
	// 继续记录命令，并真正开始新一帧的渲染
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	mCommandList->ClearRenderTargetView(stRTVHandle, clearColor, 0, nullptr);
	
	//注意我们使用的渲染手法是三角形列表，也就是通常的Mesh网格
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->IASetVertexBuffers(0, 1, &mstVertexBufferView);
	mCommandList->IASetIndexBuffer(&mstIndexBufferView);
	
	//---------------------------------------------------------------------------------------------
	//Draw Call！！！
	mCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
	
	//---------------------------------------------------------------------------------------------
	//又一个资源屏障，用于确定渲染已经结束可以提交画面去显示了
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//提交画面
	ThrowIfFailed(mSwapChain->Present(1, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}

HRESULT TextureCube::CreateRootSig()
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE stFeatureData = {};
	// 检测是否支持V1.1版本的根签名
	stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &stFeatureData, sizeof(stFeatureData))))
	{
		stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}
	// 在GPU上执行SetGraphicsRootDescriptorTable后，我们不修改命令列表中的SRV，因此我们可以使用默认Rang行为:
	// D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
	D3D12_DESCRIPTOR_RANGE1 stDSPRanges[3] = {};

	stDSPRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	stDSPRanges[0].NumDescriptors = 1;
	stDSPRanges[0].BaseShaderRegister = 0;
	stDSPRanges[0].RegisterSpace = 0;
	stDSPRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
	stDSPRanges[0].OffsetInDescriptorsFromTableStart = 0;

	stDSPRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	stDSPRanges[1].NumDescriptors = 1;
	stDSPRanges[1].BaseShaderRegister = 0;
	stDSPRanges[1].RegisterSpace = 0;
	stDSPRanges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	stDSPRanges[1].OffsetInDescriptorsFromTableStart = 0;

	stDSPRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	stDSPRanges[2].NumDescriptors = 1;
	stDSPRanges[2].BaseShaderRegister = 0;
	stDSPRanges[2].RegisterSpace = 0;
	stDSPRanges[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	stDSPRanges[2].OffsetInDescriptorsFromTableStart = 0;

	D3D12_ROOT_PARAMETER1 stRootParameters[3] = {};

	stRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	stRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	//SRV仅PS可见
	stRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	stRootParameters[0].DescriptorTable.pDescriptorRanges = &stDSPRanges[0];

	stRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	stRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;		//CBV是所有Shader可见
	stRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	stRootParameters[1].DescriptorTable.pDescriptorRanges = &stDSPRanges[1];

	stRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	stRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	//SAMPLE仅PS可见
	stRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	stRootParameters[2].DescriptorTable.pDescriptorRanges = &stDSPRanges[2];

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc = {};

	stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	stRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	stRootSignatureDesc.Desc_1_1.NumParameters = _countof(stRootParameters);
	stRootSignatureDesc.Desc_1_1.pParameters = stRootParameters;
	stRootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
	stRootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> pISignatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> pIErrorBlob;
	ThrowIfFailed(D3D12SerializeVersionedRootSignature(&stRootSignatureDesc
		, &pISignatureBlob
		, &pIErrorBlob));

	ThrowIfFailed(md3dDevice->CreateRootSignature(0
		, pISignatureBlob->GetBufferPointer()
		, pISignatureBlob->GetBufferSize()
		, IID_PPV_ARGS(&mpIRootSignature)));

	return S_OK;
}

HRESULT TextureCube::BuildShader()
{
#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

	TCHAR pszShaderFileName[] = L"shaders\\TextureCube.hlsl";
	ThrowIfFailed(D3DCompileFromFile(pszShaderFileName,
		nullptr, nullptr, "VSMain", "vs_5_0",
		compileFlags, 0, &mpVertexShader, nullptr));

	ThrowIfFailed(D3DCompileFromFile(pszShaderFileName,
		nullptr, nullptr, "PSMain", "ps_5_0",
		compileFlags, 0, &mpPixelShader, nullptr));
	return S_OK;
}

HRESULT TextureCube::CreatePSO()
{
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// 创建 graphics pipeline state object (PSO)对象
	D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
	stPSODesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };

	stPSODesc.pRootSignature = mpIRootSignature.Get();
	stPSODesc.VS.BytecodeLength = mpVertexShader->GetBufferSize();
	stPSODesc.VS.pShaderBytecode = mpVertexShader->GetBufferPointer();
	stPSODesc.PS.BytecodeLength = mpPixelShader->GetBufferSize();
	stPSODesc.PS.pShaderBytecode = mpPixelShader->GetBufferPointer();

	stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	stPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

	stPSODesc.BlendState.AlphaToCoverageEnable = FALSE;
	stPSODesc.BlendState.IndependentBlendEnable = FALSE;
	stPSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	stPSODesc.DepthStencilState.DepthEnable = FALSE;
	stPSODesc.DepthStencilState.StencilEnable = FALSE;
	stPSODesc.SampleMask = UINT_MAX;
	stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	stPSODesc.NumRenderTargets = 1;
	stPSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	stPSODesc.SampleDesc.Count = 1;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&stPSODesc, IID_PPV_ARGS(&mPSO)));
	return S_OK;
}

HRESULT TextureCube::CreateOthers()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
	stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	stSRVDesc.Format = mstTextureDesc.Format;
	stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	stSRVDesc.Texture2D.MipLevels = 1;
	md3dDevice->CreateShaderResourceView(mpITexture.Get(), &stSRVDesc, mSrvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_DESCRIPTOR_HEAP_DESC stSamplerHeapDesc = {};
	stSamplerHeapDesc.NumDescriptors = mnSampleMaxCnt;
	stSamplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	stSamplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&stSamplerHeapDesc, IID_PPV_ARGS(&mpISamplerDescriptorHeap)));

	mnSamplerDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	//创建CBV描述符
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = mpICBVUpload->GetGPUVirtualAddress();
	SIZE_T szMVPBuffer = TEX_UPPER_DIV(sizeof(ST_FRAME_MVP_BUFFER), 256) * 256;
	cbvDesc.SizeInBytes = static_cast<UINT>(szMVPBuffer);

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(mSrvHeap->GetCPUDescriptorHandleForHeapStart()
		, 1
		, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	md3dDevice->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);

	//创建各种采样器
	CD3DX12_CPU_DESCRIPTOR_HANDLE hSamplerHeap(mpISamplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

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
	md3dDevice->CreateSampler(&stSamplerDesc, hSamplerHeap);

	hSamplerHeap.Offset(mnSamplerDescriptorSize);

	// Sampler 2
	stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	md3dDevice->CreateSampler(&stSamplerDesc, hSamplerHeap);

	hSamplerHeap.Offset(mnSamplerDescriptorSize);

	// Sampler 3
	stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	md3dDevice->CreateSampler(&stSamplerDesc, hSamplerHeap);

	hSamplerHeap.Offset(mnSamplerDescriptorSize);

	// Sampler 4
	stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	md3dDevice->CreateSampler(&stSamplerDesc, hSamplerHeap);

	hSamplerHeap.Offset(mnSamplerDescriptorSize);

	// Sampler 5
	stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	md3dDevice->CreateSampler(&stSamplerDesc, hSamplerHeap);
	return S_OK;
}

HRESULT TextureCube::LoadRenderData()
{
	float fBoxSize = 3.0f;
	float fTCMax = 3.0f;

// 16、使用WIC创建并加载一个2D纹理
//使用纯COM方式创建WIC类厂对象，也是调用WIC第一步要做的事情
	ThrowIfFailed(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&mpIWICFactory)));

	//使用WIC类厂对象接口加载纹理图片，并得到一个WIC解码器对象接口，图片信息就在这个接口代表的对象中了
	WCHAR* pszTexcuteFileName = (L"textures\\boy.jpg");
	ThrowIfFailed(mpIWICFactory->CreateDecoderFromFilename(
		pszTexcuteFileName,              // 文件名
		NULL,                            // 不指定解码器，使用默认
		GENERIC_READ,                    // 访问权限
		WICDecodeMetadataCacheOnDemand,  // 若需要就缓冲数据 
		&mpIWICDecoder                    // 解码器对象
	));

	// 获取第一帧图片(因为GIF等格式文件可能会有多帧图片，其他的格式一般只有一帧图片)
	// 实际解析出来的往往是位图格式数据
	ThrowIfFailed(mpIWICDecoder->GetFrame(0, &mpIWICFrame));

	WICPixelFormatGUID wpf = {};
	//获取WIC图片格式
	ThrowIfFailed(mpIWICFrame->GetPixelFormat(&wpf));
	GUID tgFormat = {};

	//通过第一道转换之后获取DXGI的等价格式
	if (GetTargetPixelFormat(&wpf, &tgFormat))
	{
		mstTextureFormat = GetDXGIFormatFromPixelFormat(&tgFormat);
	}

	if (DXGI_FORMAT_UNKNOWN == mstTextureFormat)
	{// 不支持的图片格式 目前退出了事 
	 // 一般 在实际的引擎当中都会提供纹理格式转换工具，
	 // 图片都需要提前转换好，所以不会出现不支持的现象
		throw DxException(S_FALSE, L"FORMAT_UNKNOWN", L"TextureRender::LoadRenderData", 330);
	}

	// 定义一个位图格式的图片数据对象接口
	Microsoft::WRL::ComPtr<IWICBitmapSource>pIBMP;

	if (!InlineIsEqualGUID(wpf, tgFormat))
	{// 这个判断很重要，如果原WIC格式不是直接能转换为DXGI格式的图片时
	 // 我们需要做的就是转换图片格式为能够直接对应DXGI格式的形式
		//创建图片格式转换器
		Microsoft::WRL::ComPtr<IWICFormatConverter> pIConverter;
		ThrowIfFailed(mpIWICFactory->CreateFormatConverter(&pIConverter));

		//初始化一个图片转换器，实际也就是将图片数据进行了格式转换
		ThrowIfFailed(pIConverter->Initialize(
			mpIWICFrame.Get(),                // 输入原图片数据
			tgFormat,						 // 指定待转换的目标格式
			WICBitmapDitherTypeNone,         // 指定位图是否有调色板，现代都是真彩位图，不用调色板，所以为None
			NULL,                            // 指定调色板指针
			0.f,                             // 指定Alpha阀值
			WICBitmapPaletteTypeCustom       // 调色板类型，实际没有使用，所以指定为Custom
		));
		// 调用QueryInterface方法获得对象的位图数据源接口
		ThrowIfFailed(pIConverter.As(&pIBMP));
	}
	else
	{
		//图片数据格式不需要转换，直接获取其位图数据源接口
		ThrowIfFailed(mpIWICFrame.As(&pIBMP));
	}
	//获得图片大小（单位：像素）
	ThrowIfFailed(pIBMP->GetSize(&mnTextureW, &mnTextureH));

	//获取图片像素的位大小的BPP（Bits Per Pixel）信息，用以计算图片行数据的真实大小（单位：字节）
	Microsoft::WRL::ComPtr<IWICComponentInfo> pIWICmntinfo;
	ThrowIfFailed(mpIWICFactory->CreateComponentInfo(tgFormat, pIWICmntinfo.GetAddressOf()));

	WICComponentType type;
	ThrowIfFailed(pIWICmntinfo->GetComponentType(&type));

	if (type != WICPixelFormat)
	{
		throw DxException(S_FALSE, L"type != WICPixelFormat", L"TextureRender::LoadRenderData", 372);
	}

	Microsoft::WRL::ComPtr<IWICPixelFormatInfo> pIWICPixelinfo;
	ThrowIfFailed(pIWICmntinfo.As(&pIWICPixelinfo));

	// 到这里终于可以得到BPP了，这也是我看的比较吐血的地方，为了BPP居然饶了这么多环节
	ThrowIfFailed(pIWICPixelinfo->GetBitsPerPixel(&mnBPP));

	// 计算图片实际的行大小（单位：字节），这里使用了一个上取整除法即（A+B-1）/B ，
	// 这曾经被传说是微软的面试题,希望你已经对它了如指掌
	UINT nPicRowPitch = (uint64_t(mnTextureW) * uint64_t(mnBPP) + 7u) / 8u;
	//---------------------------------------------------------------------------------------------

	D3D12_HEAP_DESC stTextureHeapDesc = {};
	//为堆指定纹理图片至少2倍大小的空间，这里没有详细去计算了，只是指定了一个足够大的空间，够放纹理就行
	//实际应用中也是要综合考虑分配堆的大小，以便可以重用堆
	stTextureHeapDesc.SizeInBytes = TEX_UPPER(2 * nPicRowPitch * mnTextureH, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	//指定堆的对齐方式，这里使用了默认的64K边界对齐，因为我们暂时不需要MSAA支持
	stTextureHeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	stTextureHeapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;		//默认堆类型
	stTextureHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	stTextureHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	//拒绝渲染目标纹理、拒绝深度蜡板纹理，实际就只是用来摆放普通纹理
	stTextureHeapDesc.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS;

	ThrowIfFailed(md3dDevice->CreateHeap(&stTextureHeapDesc, IID_PPV_ARGS(&mpITextureHeap)));

	mstTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	mstTextureDesc.MipLevels = 1;
	mstTextureDesc.Format = mstTextureFormat; //DXGI_FORMAT_R8G8B8A8_UNORM;
	mstTextureDesc.Width = mnTextureW;
	mstTextureDesc.Height = mnTextureH;
	mstTextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	mstTextureDesc.DepthOrArraySize = 1;
	mstTextureDesc.SampleDesc.Count = 1;
	mstTextureDesc.SampleDesc.Quality = 0;

	//-----------------------------------------------------------------------------------------------------------
	//使用“定位方式”来创建纹理，注意下面这个调用内部实际已经没有存储分配和释放的实际操作了，所以性能很高
	//同时可以在这个堆上反复调用CreatePlacedResource来创建不同的纹理，当然前提是它们不在被使用的时候，才考虑
	//重用堆
	ThrowIfFailed(md3dDevice->CreatePlacedResource(
		mpITextureHeap.Get()
		, 0
		, &mstTextureDesc				//可以使用CD3DX12_RESOURCE_DESC::Tex2D来简化结构体的初始化
		, D3D12_RESOURCE_STATE_COPY_DEST
		, nullptr
		, IID_PPV_ARGS(&mpITexture)));
	//-----------------------------------------------------------------------------------------------------------
	
	//获取上传堆资源缓冲的大小，这个尺寸通常大于实际图片的尺寸
	mn64UploadBufferSize = GetRequiredIntermediateSize(mpITexture.Get(), 0, 1);

	D3D12_HEAP_DESC stUploadHeapDesc = {  };
	//尺寸依然是实际纹理数据大小的2倍并64K边界对齐大小
	stUploadHeapDesc.SizeInBytes = TEX_UPPER(2 * mn64UploadBufferSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	//注意上传堆肯定是Buffer类型，可以不指定对齐方式，其默认是64k边界对齐
	stUploadHeapDesc.Alignment = 0;
	stUploadHeapDesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;		//上传堆类型
	stUploadHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	stUploadHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	//上传堆就是缓冲，可以摆放任意数据
	stUploadHeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

	ThrowIfFailed(md3dDevice->CreateHeap(&stUploadHeapDesc, IID_PPV_ARGS(&mpIUploadHeap)));
	ThrowIfFailed(md3dDevice->CreatePlacedResource(mpIUploadHeap.Get()
		, 0
		, &CD3DX12_RESOURCE_DESC::Buffer(mn64UploadBufferSize)
		, D3D12_RESOURCE_STATE_GENERIC_READ
		, nullptr
		, IID_PPV_ARGS(&mpITextureUpload)));

	//按照资源缓冲大小来分配实际图片数据存储的内存大小
	void* pbPicData = ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, mn64UploadBufferSize);
	if (nullptr == pbPicData)
	{
		throw DxException(HRESULT_FROM_WIN32(GetLastError()), L"",L"",366);
	}

	//从图片中读取出数据
	ThrowIfFailed(pIBMP->CopyPixels(nullptr
		, nPicRowPitch
		, static_cast<UINT>(nPicRowPitch * mnTextureH)   //注意这里才是图片数据真实的大小，这个值通常小于缓冲的大小
		, reinterpret_cast<BYTE*>(pbPicData)));

	//{//下面这段代码来自DX12的示例，直接通过填充缓冲绘制了一个黑白方格的纹理
	// //还原这段代码，然后注释上面的CopyPixels调用可以看到黑白方格纹理的效果
	//	const UINT rowPitch = nPicRowPitch; //nTextureW * 4; //static_cast<UINT>(n64UploadBufferSize / nTextureH);
	//	const UINT cellPitch = rowPitch >> 3;		// The width of a cell in the checkboard texture.
	//	const UINT cellHeight = nTextureW >> 3;	// The height of a cell in the checkerboard texture.
	//	const UINT textureSize = static_cast<UINT>(n64UploadBufferSize);
	//	UINT nTexturePixelSize = static_cast<UINT>(n64UploadBufferSize / nTextureH / nTextureW);

	//	UINT8* pData = reinterpret_cast<UINT8*>(pbPicData);

	//	for (UINT n = 0; n < textureSize; n += nTexturePixelSize)
	//	{
	//		UINT x = n % rowPitch;
	//		UINT y = n / rowPitch;
	//		UINT i = x / cellPitch;
	//		UINT j = y / cellHeight;

	//		if (i % 2 == j % 2)
	//		{
	//			pData[n] = 0x00;		// R
	//			pData[n + 1] = 0x00;	// G
	//			pData[n + 2] = 0x00;	// B
	//			pData[n + 3] = 0xff;	// A
	//		}
	//		else
	//		{
	//			pData[n] = 0xff;		// R
	//			pData[n + 1] = 0xff;	// G
	//			pData[n + 2] = 0xff;	// B
	//			pData[n + 3] = 0xff;	// A
	//		}
	//	}
	//}

	//获取向上传堆拷贝纹理数据的一些纹理转换尺寸信息
	//对于复杂的DDS纹理这是非常必要的过程

	UINT   nNumSubresources = 1u;  //我们只有一副图片，即子资源个数为1
	UINT   nTextureRowNum = 0u;
	UINT64 n64TextureRowSizes = 0u;
	UINT64 n64RequiredSize = 0u;

	D3D12_RESOURCE_DESC stDestDesc = mpITexture->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT stTxtLayouts = {};

	md3dDevice->GetCopyableFootprints(&stDestDesc
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
	ThrowIfFailed(mpITextureUpload->Map(0, NULL, reinterpret_cast<void**>(&pData)));

	BYTE* pDestSlice = reinterpret_cast<BYTE*>(pData) + stTxtLayouts.Offset;
	const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(pbPicData);
	for (UINT y = 0; y < nTextureRowNum; ++y)
	{
		memcpy(pDestSlice + static_cast<SIZE_T>(stTxtLayouts.Footprint.RowPitch) * y
			, pSrcSlice + static_cast<SIZE_T>(nPicRowPitch) * y
			, nPicRowPitch);
	}
	//取消映射 对于易变的数据如每帧的变换矩阵等数据，可以撒懒不用Unmap了，
	//让它常驻内存,以提高整体性能，因为每次Map和Unmap是很耗时的操作
	//因为现在起码都是64位系统和应用了，地址空间是足够的，被长期占用不会影响什么
	mpITextureUpload->Unmap(0, NULL);

	//释放图片数据，做一个干净的程序员
	::HeapFree(::GetProcessHeap(), 0, pbPicData);

	CD3DX12_TEXTURE_COPY_LOCATION Dst(mpITexture.Get(), 0);
	CD3DX12_TEXTURE_COPY_LOCATION Src(mpITextureUpload.Get(), stTxtLayouts);
	ThrowIfFailed(mDirectCmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));
	mCommandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);

	//设置一个资源屏障，同步并确认复制操作完成
	//直接使用结构体然后调用的形式
	D3D12_RESOURCE_BARRIER stResBar = {};
	stResBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	stResBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	stResBar.Transition.pResource = mpITexture.Get();
	stResBar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	stResBar.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	stResBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	mCommandList->ResourceBarrier(1, &stResBar);

	//或者使用D3DX12库中的工具类调用的等价形式，下面的方式更简洁一些
	//pICommandList->ResourceBarrier(1
	//	, &CD3DX12_RESOURCE_BARRIER::Transition(pITexture.Get()
	//	, D3D12_RESOURCE_STATE_COPY_DEST
	//	, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
	//);

	//---------------------------------------------------------------------------------------------
	// 执行命令列表并等待纹理资源上传完成，这一步是必须的
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	FlushCommandQueue();

	VERTEX triangleVertices[] =
	{
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
			{ {1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize}, {1.0f * fTCMax, 1.0f * fTCMax},  {0.0f, -1.0f,  0.0f} }
	};

	const UINT vertexBufferSize = sizeof(triangleVertices);

	UINT32 indices[] = {
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
		33,34,35
	};
	const UINT indexBufferSize = sizeof(indices);

	UINT64 n64BufferOffset = TEX_UPPER(mn64UploadBufferSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	//使用定位方式在相同的上传堆上以“定位方式”创建顶点缓冲，注意第二个参数指出了堆中的偏移位置
			//按照堆边界对齐的要求，我们主动将偏移位置对齐到了64k的边界上
	ThrowIfFailed(md3dDevice->CreatePlacedResource(
		mpIUploadHeap.Get()
		, n64BufferOffset
		, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize)
		, D3D12_RESOURCE_STATE_GENERIC_READ
		, nullptr
		, IID_PPV_ARGS(&mpIVertexBuffer)));

	//使用map-memcpy-unmap大法将数据传至顶点缓冲对象
	//注意顶点缓冲使用是和上传纹理数据缓冲相同的一个堆，这很神奇
	UINT8* pVertexDataBegin = nullptr;
	CD3DX12_RANGE stReadRange(0, 0);		// We do not intend to read from this resource on the CPU.

	ThrowIfFailed(mpIVertexBuffer->Map(0, &stReadRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
	mpIVertexBuffer->Unmap(0, nullptr);

	//创建资源视图，实际可以简单理解为指向顶点缓冲的显存指针
	mstVertexBufferView.BufferLocation = mpIVertexBuffer->GetGPUVirtualAddress();
	mstVertexBufferView.StrideInBytes = sizeof(VERTEX);
	mstVertexBufferView.SizeInBytes = vertexBufferSize;

	//计算边界对齐的正确的偏移位置
	n64BufferOffset = TEX_UPPER(n64BufferOffset + vertexBufferSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

	ThrowIfFailed(md3dDevice->CreatePlacedResource(
		mpIUploadHeap.Get()
		, n64BufferOffset
		, &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize)
		, D3D12_RESOURCE_STATE_GENERIC_READ
		, nullptr
		, IID_PPV_ARGS(&mpIIndexBuffer)));

	UINT8* pIndexDataBegin = nullptr;
	ThrowIfFailed(mpIIndexBuffer->Map(0, &stReadRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, indices, indexBufferSize);
	mpIIndexBuffer->Unmap(0, nullptr);

	mstIndexBufferView.BufferLocation = mpIIndexBuffer->GetGPUVirtualAddress();
	mstIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	mstIndexBufferView.SizeInBytes = indexBufferSize;

	//、在上传堆上以“定位方式”创建常量缓冲
	n64BufferOffset = TEX_UPPER(n64BufferOffset + indexBufferSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	SIZE_T szMVPBuffer = TEX_UPPER_DIV(sizeof(ST_FRAME_MVP_BUFFER), 256) * 256;
	// 创建常量缓冲 注意缓冲尺寸设置为256边界对齐大小
	ThrowIfFailed(md3dDevice->CreatePlacedResource(
		mpIUploadHeap.Get()
		, n64BufferOffset
		, &CD3DX12_RESOURCE_DESC::Buffer(szMVPBuffer)
		, D3D12_RESOURCE_STATE_GENERIC_READ
		, nullptr
		, IID_PPV_ARGS(&mpICBVUpload)));

	// Map 之后就不再Unmap了 直接复制数据进去 这样每帧都不用map-copy-unmap浪费时间了
	ThrowIfFailed(mpICBVUpload->Map(0, nullptr, reinterpret_cast<void**>(&mpMVPBuffer)));
	return S_OK;
}

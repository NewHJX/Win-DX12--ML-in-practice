#include "DDSTextureSky.h"

struct VERTEX
{
	//这次我们额外加入了每个顶点的法线，但Shader中还暂时没有用
	DirectX::XMFLOAT4 m_vPos;		//Position
	DirectX::XMFLOAT2 m_vTex;		//Texcoord
	DirectX::XMFLOAT3 m_vNor;		//Normal
};

struct ST_SKYBOX_VERTEX
{//天空盒子的顶点结构
public:
	DirectX::XMFLOAT4 m_v4Position;
public:
	ST_SKYBOX_VERTEX() :m_v4Position() {}
	ST_SKYBOX_VERTEX(float x, float y, float z)
		:m_v4Position(x, y, z, 1.0f)
	{}
	ST_SKYBOX_VERTEX& operator = (const ST_SKYBOX_VERTEX& vt)
	{
		m_v4Position = vt.m_v4Position;
	}
};

DDSTextureSky::DDSTextureSky(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

DDSTextureSky::~DDSTextureSky()
{
}

bool DDSTextureSky::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	ThrowIfFailed(CreateRootSig());
	ThrowIfFailed(BuildShader());
	ThrowIfFailed(CreatePSO());
	ThrowIfFailed(LoadRenderData());
	ThrowIfFailed(CreateOthers());

	return true;
}

void DDSTextureSky::OnResize()
{
	D3DApp::OnResize();
}

void DDSTextureSky::Update(const GameTimer& gt)
{
}

void DDSTextureSky::Draw(const GameTimer& gt)
{
}

HRESULT DDSTextureSky::CreateRootSig()
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE stFeatureData = {};
	// 检测是否支持V1.1版本的根签名
	stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &stFeatureData, sizeof(stFeatureData))))
	{// 1.0版 直接丢异常退出了
		ThrowIfFailed(E_NOTIMPL);
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
	stRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	stRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	stRootParameters[0].DescriptorTable.pDescriptorRanges = &stDSPRanges[0];

	stRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	stRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	stRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	stRootParameters[1].DescriptorTable.pDescriptorRanges = &stDSPRanges[1];

	stRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	stRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	stRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	stRootParameters[2].DescriptorTable.pDescriptorRanges = &stDSPRanges[2];


	D3D12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc = {};
	stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	stRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	stRootSignatureDesc.Desc_1_1.NumParameters = _countof(stRootParameters);
	stRootSignatureDesc.Desc_1_1.pParameters = stRootParameters;
	stRootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
	stRootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;


	ComPtr<ID3DBlob> pISignatureBlob;
	ComPtr<ID3DBlob> pIErrorBlob;
	ThrowIfFailed(D3D12SerializeVersionedRootSignature(&stRootSignatureDesc
		, &pISignatureBlob
		, &pIErrorBlob));

	ThrowIfFailed(md3dDevice->CreateRootSignature(0
		, pISignatureBlob->GetBufferPointer()
		, pISignatureBlob->GetBufferSize()
		, IID_PPV_ARGS(&mpIRootSignature)));
	return S_OK;
}

HRESULT DDSTextureSky::BuildShader()
{
#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT nCompileFlags = 0;
#endif

	//编译为行矩阵形式	   
	nCompileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

	ThrowIfFailed(D3DCompileFromFile(L"Shaders\\TextureCube.hlsl", nullptr, nullptr
		, "VSMain", "vs_5_0", nCompileFlags, 0, &mpVertexEarthShader, nullptr));
	ThrowIfFailed(D3DCompileFromFile(L"Shaders\\TextureCube.hlsl", nullptr, nullptr
		, "PSMain", "ps_5_0", nCompileFlags, 0, &mpPixelEarthShader, nullptr));


	//编译为行矩阵形式	   
	nCompileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

	ThrowIfFailed(D3DCompileFromFile(L"Shaders\\SkyBox.hlsl", nullptr, nullptr
		, "SkyboxVS", "vs_5_0", nCompileFlags, 0, &mpVertexShader, nullptr));
	ThrowIfFailed(D3DCompileFromFile(L"Shaders\\SkyBox.hlsl", nullptr, nullptr
		, "SkyboxPS", "ps_5_0", nCompileFlags, 0, &mpPixelShader, nullptr));

	return S_OK;
}

HRESULT DDSTextureSky::CreatePSO()
{
	// 我们多添加了一个法线的定义，但目前Shader中我们并没有使用
	D3D12_INPUT_ELEMENT_DESC stIALayoutEarth[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// 创建 graphics pipeline state object (PSO)对象
	D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
	stPSODesc.InputLayout = { stIALayoutEarth, _countof(stIALayoutEarth) };
	stPSODesc.pRootSignature = mpIRootSignature.Get();
	stPSODesc.VS.BytecodeLength = mpVertexEarthShader->GetBufferSize();
	stPSODesc.VS.pShaderBytecode = mpVertexEarthShader->GetBufferPointer();
	stPSODesc.PS.BytecodeLength = mpPixelEarthShader->GetBufferSize();
	stPSODesc.PS.pShaderBytecode = mpPixelEarthShader->GetBufferPointer();

	stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	stPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

	stPSODesc.BlendState.AlphaToCoverageEnable = FALSE;
	stPSODesc.BlendState.IndependentBlendEnable = FALSE;
	stPSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	stPSODesc.SampleMask = UINT_MAX;
	stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	stPSODesc.NumRenderTargets = 1;
	stPSODesc.RTVFormats[0] = mBackBufferFormat;
	stPSODesc.DSVFormat = mDepthStencilFormat;
	stPSODesc.DepthStencilState.DepthEnable = TRUE;
	stPSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//启用深度缓存写入功能
	stPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;     //深度测试函数（该值为普通的深度测试）
	stPSODesc.DepthStencilState.StencilEnable = FALSE;
	stPSODesc.SampleDesc.Count = 1;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&stPSODesc
		, IID_PPV_ARGS(&mEarthPSO)));

	// 天空盒子只有顶点只有位置参数
	D3D12_INPUT_ELEMENT_DESC stIALayoutSkybox[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// 创建Skybox的(PSO)对象 
	stPSODesc.InputLayout = { stIALayoutSkybox, _countof(stIALayoutSkybox) };
	//stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	stPSODesc.DepthStencilState.DepthEnable = FALSE;
	stPSODesc.DepthStencilState.StencilEnable = FALSE;
	stPSODesc.VS.BytecodeLength = mpVertexShader->GetBufferSize();
	stPSODesc.VS.pShaderBytecode = mpVertexShader->GetBufferPointer();
	stPSODesc.PS.BytecodeLength = mpPixelShader->GetBufferSize();
	stPSODesc.PS.pShaderBytecode = mpPixelShader->GetBufferPointer();

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&stPSODesc
		, IID_PPV_ARGS(&mPSO)));
	return S_OK;
}

HRESULT DDSTextureSky::CreateOthers()
{
	ThrowIfFailed(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE
		, IID_PPV_ARGS(&mpICmdAllocEarth)));
	ThrowIfFailed(md3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE
		, mpICmdAllocEarth.Get(), nullptr, IID_PPV_ARGS(&mpIBundlesEarth)));

	ThrowIfFailed(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE
		, IID_PPV_ARGS(&mpICmdAllocSkybox)));
	ThrowIfFailed(md3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE
		, mpICmdAllocSkybox.Get(), nullptr, IID_PPV_ARGS(&mpIBundlesSkybox)));

	D3D12_DESCRIPTOR_HEAP_DESC stSamplerHeapDesc = {};
	stSamplerHeapDesc.NumDescriptors = mnSampleMaxCnt;
	stSamplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	stSamplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&stSamplerHeapDesc, IID_PPV_ARGS(&mpISamplerDescriptorHeap)));
	return S_OK;
}

HRESULT DDSTextureSky::LoadRenderData()
{
	ComPtr<IWICFormatConverter> pIConverter;
	ComPtr<IWICComponentInfo> pIWICmntinfo;
	WICPixelFormatGUID wpf = {};
	GUID tgFormat = {};
	WICComponentType type;
	ComPtr<IWICPixelFormatInfo> pIWICPixelinfo;

	//---------------------------------------------------------------------------------------------
	//使用纯COM方式创建WIC类厂对象，也是调用WIC第一步要做的事情
	ThrowIfFailed(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&mpIWICFactory)));

	//使用WIC类厂对象接口加载纹理图片，并得到一个WIC解码器对象接口，图片信息就在这个接口代表的对象中了

	ThrowIfFailed(mpIWICFactory->CreateDecoderFromFilename(
		L"textures/金星.jpg",              // 文件名
		nullptr,                            // 不指定解码器，使用默认
		GENERIC_READ,                    // 访问权限
		WICDecodeMetadataCacheOnDemand,  // 若需要就缓冲数据 
		&mpIWICDecoder                    // 解码器对象
	));
	// 获取第一帧图片(因为GIF等格式文件可能会有多帧图片，其他的格式一般只有一帧图片)
	// 实际解析出来的往往是位图格式数据
	ThrowIfFailed(mpIWICDecoder->GetFrame(0, &mpIWICFrame));
	//获取WIC图片格式
	ThrowIfFailed(mpIWICFrame->GetPixelFormat(&wpf));
	//通过第一道转换之后获取DXGI的等价格式
	if (GetTargetPixelFormat(&wpf, &tgFormat))
	{
		mstTxtFmtEarth = GetDXGIFormatFromPixelFormat(&tgFormat);
	}

	if (DXGI_FORMAT_UNKNOWN == mstTxtFmtEarth)
	{// 不支持的图片格式 目前退出了事 
	 // 一般 在实际的引擎当中都会提供纹理格式转换工具，
	 // 图片都需要提前转换好，所以不会出现不支持的现象
		throw DxException(S_FALSE, L"FORMAT_UNKNOWN", L"DDSTextureSky::LoadRenderData", 118);
	}

	if (!InlineIsEqualGUID(wpf, tgFormat))
	{// 这个判断很重要，如果原WIC格式不是直接能转换为DXGI格式的图片时
	 // 我们需要做的就是转换图片格式为能够直接对应DXGI格式的形式
		//创建图片格式转换器
		ThrowIfFailed(mpIWICFactory->CreateFormatConverter(&pIConverter));
		//初始化一个图片转换器，实际也就是将图片数据进行了格式转换
		ThrowIfFailed(pIConverter->Initialize(
			mpIWICFrame.Get(),                // 输入原图片数据
			tgFormat,						 // 指定待转换的目标格式
			WICBitmapDitherTypeNone,         // 指定位图是否有调色板，现代都是真彩位图，不用调色板，所以为None
			nullptr,                            // 指定调色板指针
			0.f,                             // 指定Alpha阀值
			WICBitmapPaletteTypeCustom       // 调色板类型，实际没有使用，所以指定为Custom
		));
		// 调用QueryInterface方法获得对象的位图数据源接口
		ThrowIfFailed(pIConverter.As(&mpIBMPEarth));
	}
	else
	{
		//图片数据格式不需要转换，直接获取其位图数据源接口
		ThrowIfFailed(mpIWICFrame.As(&mpIBMPEarth));
	}
	//获得图片大小（单位：像素）
	ThrowIfFailed(mpIBMPEarth->GetSize(&mnTextureEarthW, &mnTextureEarthH));
	//获取图片像素的位大小的BPP（Bits Per Pixel）信息，用以计算图片行数据的真实大小（单位：字节）
	ThrowIfFailed(mpIWICFactory->CreateComponentInfo(tgFormat, pIWICmntinfo.GetAddressOf()));
	ThrowIfFailed(pIWICmntinfo->GetComponentType(&type));
	if (type != WICPixelFormat)
	{
		throw DxException(S_FALSE, L"FORMAT_UNKNOWN", L"DDSTextureSky::LoadRenderData", 150);
	}
	ThrowIfFailed(pIWICmntinfo.As(&pIWICPixelinfo));
	// 到这里终于可以得到BPP了，这也是我看的比较吐血的地方，为了BPP居然饶了这么多环节
	ThrowIfFailed(pIWICPixelinfo->GetBitsPerPixel(&mnBPPEarth));
	// 计算图片实际的行大小（单位：字节），这里使用了一个上取整除法即（A+B-1）/B ，
	// 这曾经被传说是微软的面试题,希望你已经对它了如指掌
	mnRowPitchEarth = TEX_UPPER_DIV(uint64_t(mnTextureEarthH) * uint64_t(mnBPPEarth), 8);
}

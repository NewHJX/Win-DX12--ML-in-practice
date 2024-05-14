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

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT	stTxtLayoutsEarth = {};
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT	stTxtLayoutsSkybox = {};

	// 创建纹理的默认堆、上传堆并加载纹理
	{
		D3D12_HEAP_DESC stTextureHeapDesc = {};
		//为堆指定纹理图片至少2倍大小的空间，这里没有详细去计算了，只是指定了一个足够大的空间，够放纹理就行
		//实际应用中也是要综合考虑分配堆的大小，以便可以重用堆
		stTextureHeapDesc.SizeInBytes = TEX_UPPER(2 * mnRowPitchEarth * mnTextureEarthH, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		//指定堆的对齐方式，这里使用了默认的64K边界对齐，因为我们暂时不需要MSAA支持
		stTextureHeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		stTextureHeapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;		//默认堆类型
		stTextureHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stTextureHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		//拒绝渲染目标纹理、拒绝深度蜡板纹理，实际就只是用来摆放普通纹理
		stTextureHeapDesc.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS;

		ThrowIfFailed(md3dDevice->CreateHeap(&stTextureHeapDesc, IID_PPV_ARGS(&mpIRESHeapEarth)));

		D3D12_RESOURCE_DESC					stTextureDesc = {};
		// 创建2D纹理		
		stTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		stTextureDesc.MipLevels = 1;
		stTextureDesc.Format = mBackBufferFormat; //DXGI_FORMAT_R8G8B8A8_UNORM;
		stTextureDesc.Width = mnTextureEarthW;
		stTextureDesc.Height = mnTextureEarthH;
		stTextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		stTextureDesc.DepthOrArraySize = 1;
		stTextureDesc.SampleDesc.Count = 1;
		stTextureDesc.SampleDesc.Quality = 0;
		//-----------------------------------------------------------------------------------------------------------
		//使用“定位方式”来创建纹理，注意下面这个调用内部实际已经没有存储分配和释放的实际操作了，所以性能很高
		//同时可以在这个堆上反复调用CreatePlacedResource来创建不同的纹理，当然前提是它们不在被使用的时候，才考虑
		//重用堆
		ThrowIfFailed(md3dDevice->CreatePlacedResource(
			mpIRESHeapEarth.Get()
			, 0
			, &stTextureDesc				//可以使用CD3DX12_RESOURCE_DESC::Tex2D来简化结构体的初始化
			, D3D12_RESOURCE_STATE_COPY_DEST
			, nullptr
			, IID_PPV_ARGS(&mpITextureEarth)));
		//-----------------------------------------------------------------------------------------------------------
		//获取上传堆资源缓冲的大小，这个尺寸通常大于实际图片的尺寸
		D3D12_RESOURCE_DESC stCopyDstDesc = mpITextureEarth->GetDesc();
		md3dDevice->GetCopyableFootprints(&stCopyDstDesc, 0, 1, 0, nullptr, nullptr, nullptr, &mn64UploadBufferSizeEarth);

		//-----------------------------------------------------------------------------------------------------------
		// 创建上传堆
		D3D12_HEAP_DESC stUploadHeapDesc = {  };
		//尺寸依然是实际纹理数据大小的2倍并64K边界对齐大小
		stUploadHeapDesc.SizeInBytes = TEX_UPPER(2 * mn64UploadBufferSizeEarth, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		//注意上传堆肯定是Buffer类型，可以不指定对齐方式，其默认是64k边界对齐
		stUploadHeapDesc.Alignment = 0;
		stUploadHeapDesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;		//上传堆类型
		stUploadHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stUploadHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		//上传堆就是缓冲，可以摆放任意数据
		stUploadHeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

		ThrowIfFailed(md3dDevice->CreateHeap(&stUploadHeapDesc, IID_PPV_ARGS(&mpIUploadHeapEarth)));

		//-----------------------------------------------------------------------------------------------------------
		// 使用“定位方式”创建用于上传纹理数据的缓冲资源
		D3D12_RESOURCE_DESC stUploadBufDesc = {};
		stUploadBufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		stUploadBufDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		stUploadBufDesc.Width = mn64UploadBufferSizeEarth;
		stUploadBufDesc.Height = 1;
		stUploadBufDesc.DepthOrArraySize = 1;
		stUploadBufDesc.MipLevels = 1;
		stUploadBufDesc.Format = DXGI_FORMAT_UNKNOWN;
		stUploadBufDesc.SampleDesc.Count = 1;
		stUploadBufDesc.SampleDesc.Quality = 0;
		stUploadBufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		stUploadBufDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ThrowIfFailed(md3dDevice->CreatePlacedResource(mpIUploadHeapEarth.Get()
			, 0
			, &stUploadBufDesc
			, D3D12_RESOURCE_STATE_GENERIC_READ
			, nullptr
			, IID_PPV_ARGS(&mpITextureUploadEarth)));

		// 加载图片数据至上传堆，即完成第一个Copy动作，从memcpy函数可知这是由CPU完成的

		//按照资源缓冲大小来分配实际图片数据存储的内存大小
		void* pbPicData = TEX_CALLOC(mn64UploadBufferSizeEarth);
		if (nullptr == pbPicData)
		{
			throw DxException(HRESULT_FROM_WIN32(GetLastError()), L"FORMAT_UNKNOWN", L"DDSTextureSky::CreateOthers", 330);
		}

		//从图片中读取出数据
		ThrowIfFailed(mpIBMPEarth->CopyPixels(nullptr
			, mnRowPitchEarth
			, static_cast<UINT>(mnRowPitchEarth * mnTextureEarthH)   //注意这里才是图片数据真实的大小，这个值通常小于缓冲的大小
			, reinterpret_cast<BYTE*>(pbPicData)));

		//{//下面这段代码来自DX12的示例，直接通过填充缓冲绘制了一个黑白方格的纹理
		// //还原这段代码，然后注释上面的CopyPixels调用可以看到黑白方格纹理的效果
		//	const UINT rowPitch = nRowPitchEarth; //nTxtWEarth * 4; //static_cast<UINT>(mn64UploadBufferSizeEarth / nTxtHEarth);
		//	const UINT cellPitch = rowPitch >> 3;		// The width of a cell in the checkboard texture.
		//	const UINT cellHeight = nTxtWEarth >> 3;	// The height of a cell in the checkerboard texture.
		//	const UINT textureSize = static_cast<UINT>(mn64UploadBufferSizeEarth);
		//	UINT nTexturePixelSize = static_cast<UINT>(mn64UploadBufferSizeEarth / nTxtHEarth / nTxtWEarth);

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
		D3D12_RESOURCE_DESC					stDestDesc = {};

		stDestDesc = mpITextureEarth->GetDesc();

		md3dDevice->GetCopyableFootprints(&stDestDesc
			, 0
			, nNumSubresources
			, 0
			, &stTxtLayoutsEarth
			, &nTextureRowNum
			, &n64TextureRowSizes
			, &n64RequiredSize);

		//因为上传堆实际就是CPU传递数据到GPU的中介
		//所以我们可以使用熟悉的Map方法将它先映射到CPU内存地址中
		//然后我们按行将数据复制到上传堆中
		//需要注意的是之所以按行拷贝是因为GPU资源的行大小
		//与实际图片的行大小是有差异的,二者的内存边界对齐要求是不一样的
		BYTE* pData = nullptr;
		ThrowIfFailed(mpITextureUploadEarth->Map(0, nullptr, reinterpret_cast<void**>(&pData)));

		BYTE* pDestSlice = reinterpret_cast<BYTE*>(pData) + stTxtLayoutsEarth.Offset;
		BYTE* pSrcSlice = reinterpret_cast<BYTE*>(pbPicData);
		for (UINT y = 0; y < nTextureRowNum; ++y)
		{
			memcpy(pDestSlice + static_cast<SIZE_T>(stTxtLayoutsEarth.Footprint.RowPitch) * y
				, pSrcSlice + static_cast<SIZE_T>(mnRowPitchEarth) * y
				, mnRowPitchEarth);
		}
		//取消映射 对于易变的数据如每帧的变换矩阵等数据，可以撒懒不用Unmap了，
		//让它常驻内存,以提高整体性能，因为每次Map和Unmap是很耗时的操作
		//因为现在起码都是64位系统和应用了，地址空间是足够的，被长期占用不会影响什么
		mpITextureUploadEarth->Unmap(0, nullptr);

		//释放图片数据，做一个干净的程序员
		SAFE_FREE(pbPicData);
	}

	// 使用DDSLoader辅助函数加载Skybox的纹理
	{
		ID3D12Resource* pIResSkyBox = nullptr;
		ThrowIfFailed(CreateDDSTextureFromFileEx(
			md3dDevice.Get()
			, L"Assets\\Sky_cube_1024.dds"
			, &pIResSkyBox
			, ddsData
			, arSubResources
			, SIZE_MAX
			, &emAlphaMode
			, &bIsCube));

		//上面函数加载的纹理在隐式默认堆上，两个Copy动作需要我们自己完成
		mpITexture.Attach(pIResSkyBox);
		//=====================================================================================================
		//获取Skybox的资源大小，并创建上传堆
		D3D12_RESOURCE_DESC stCopyDstDesc = mpITexture->GetDesc();
		// 这里是第一次调用GetCopyableFootprints
		md3dDevice->GetCopyableFootprints(&stCopyDstDesc
			, 0, static_cast<UINT>(arSubResources.size()), 0, nullptr, nullptr, nullptr, &mn64UploadBufferSize);

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

		ThrowIfFailed(md3dDevice->CreateHeap(&stUploadHeapDesc, IID_PPV_ARGS(&mpIUploadHeapSkybox)));

		D3D12_RESOURCE_DESC stUploadBufDesc = {};
		stUploadBufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		stUploadBufDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		stUploadBufDesc.Width = mn64UploadBufferSize;
		stUploadBufDesc.Height = 1;
		stUploadBufDesc.DepthOrArraySize = 1;
		stUploadBufDesc.MipLevels = 1;
		stUploadBufDesc.Format = DXGI_FORMAT_UNKNOWN;
		stUploadBufDesc.SampleDesc.Count = 1;
		stUploadBufDesc.SampleDesc.Quality = 0;
		stUploadBufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		stUploadBufDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ThrowIfFailed(md3dDevice->CreatePlacedResource(mpIUploadHeapSkybox.Get()
			, 0
			, &stUploadBufDesc
			, D3D12_RESOURCE_STATE_GENERIC_READ
			, nullptr
			, IID_PPV_ARGS(&mpITextureUpload)));

		// 上传Skybox的纹理，注意因为这里的天空盒子资源是DDS格式的，并且有多个Mipmap，所以子资源的数量很多
		UINT nFirstSubresource = 0;
		UINT nNumSubresources = static_cast<UINT>(arSubResources.size());
		D3D12_RESOURCE_DESC stUploadResDesc = mpITextureUpload->GetDesc();
		D3D12_RESOURCE_DESC stDefaultResDesc = mpITexture->GetDesc();

		UINT64 n64RequiredSize = 0;
		SIZE_T szMemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT)
			+ sizeof(UINT)
			+ sizeof(UINT64))
			* nNumSubresources;

		void* pMem = TEX_CALLOC(static_cast<SIZE_T>(szMemToAlloc));

		if (nullptr == pMem)
		{
			throw DxException(HRESULT_FROM_WIN32(GetLastError()), L"MEM is null", L"CreateOthers", 491);
		}

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
		UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + nNumSubresources);
		UINT* pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + nNumSubresources);

		// 这里是第二次调用GetCopyableFootprints，就得到了所有子资源的详细信息
		md3dDevice->GetCopyableFootprints(&stDefaultResDesc, nFirstSubresource, nNumSubresources, 0, pLayouts, pNumRows, pRowSizesInBytes, &n64RequiredSize);

		BYTE* pData = nullptr;
		HRESULT hr = mpITextureUpload->Map(0, nullptr, reinterpret_cast<void**>(&pData));
		if (FAILED(hr))
		{
			return 0;
		}

		// 第一遍Copy！注意3重循环每重的意思
		for (UINT i = 0; i < nNumSubresources; ++i)
		{// SubResources
			if (pRowSizesInBytes[i] > (SIZE_T)-1)
			{
				throw DxException(E_FAIL, L"size out of mem", L"CreateOthers", 513);
			}

			D3D12_MEMCPY_DEST stCopyDestData = { pData + pLayouts[i].Offset
				, pLayouts[i].Footprint.RowPitch
				, pLayouts[i].Footprint.RowPitch * pNumRows[i]
			};

			for (UINT z = 0; z < pLayouts[i].Footprint.Depth; ++z)
			{// Mipmap
				BYTE* pDestSlice = reinterpret_cast<BYTE*>(stCopyDestData.pData) + stCopyDestData.SlicePitch * z;
				const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(arSubResources[i].pData) + arSubResources[i].SlicePitch * z;
				for (UINT y = 0; y < pNumRows[i]; ++y)
				{// Rows
					memcpy(pDestSlice + stCopyDestData.RowPitch * y,
						pSrcSlice + arSubResources[i].RowPitch * y,
						(SIZE_T)pRowSizesInBytes[i]);
				}
			}
		}
		mpITextureUpload->Unmap(0, nullptr);

		// 第二次Copy！
		if (stDefaultResDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{// Buffer 一次性复制就可以了，因为没有行对齐和行大小不一致的问题，Buffer中行数就是1
			mCommandList->CopyBufferRegion(
				mpITexture.Get(), 0, mpITextureUpload.Get(), pLayouts[0].Offset, pLayouts[0].Footprint.Width);
		}
		else
		{
			for (UINT i = 0; i < nNumSubresources; ++i)
			{
				D3D12_TEXTURE_COPY_LOCATION stDstCopyLocation = {};
				stDstCopyLocation.pResource = mpITexture.Get();
				stDstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				stDstCopyLocation.SubresourceIndex = i;

				D3D12_TEXTURE_COPY_LOCATION stSrcCopyLocation = {};
				stSrcCopyLocation.pResource = mpITextureUpload.Get();
				stSrcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				stSrcCopyLocation.PlacedFootprint = pLayouts[i];

				mCommandList->CopyTextureRegion(&stDstCopyLocation, 0, 0, 0, &stSrcCopyLocation, nullptr);
			}
		}

		D3D12_RESOURCE_BARRIER stTransResBarrier = {};

		stTransResBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		stTransResBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		stTransResBarrier.Transition.pResource = mpITexture.Get();
		stTransResBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		stTransResBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		stTransResBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		mCommandList->ResourceBarrier(1, &stTransResBarrier);
	}

		//向直接命令列表发出从上传堆复制纹理数据到默认堆的命令，执行并同步等待，即完成第二个Copy动作，由GPU上的复制引擎完成
		//注意此时直接命令列表还没有绑定PSO对象，因此它也是不能执行3D图形命令的，但是可以执行复制命令，因为复制引擎不需要什么
		//额外的状态设置之类的参数
		{
			D3D12_TEXTURE_COPY_LOCATION stDstCopyLocation = {};
			stDstCopyLocation.pResource = mpITextureEarth.Get();
			stDstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			stDstCopyLocation.SubresourceIndex = 0;

			D3D12_TEXTURE_COPY_LOCATION stSrcCopyLocation = {};
			stSrcCopyLocation.pResource =mpITextureUploadEarth.Get();
			stSrcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			stSrcCopyLocation.PlacedFootprint = stTxtLayoutsEarth;

			mCommandList->CopyTextureRegion(&stDstCopyLocation, 0, 0, 0, &stSrcCopyLocation, nullptr);

			//设置一个资源屏障，同步并确认复制操作完成
			//直接使用结构体然后调用的形式
			D3D12_RESOURCE_BARRIER stResBar = {};
			stResBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			stResBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			stResBar.Transition.pResource = mpITextureEarth.Get();
			stResBar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			stResBar.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			stResBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			mCommandList->ResourceBarrier(1, &stResBar);

		}

		// 执行第二个Copy命令并确定所有的纹理都上传到了默认堆中
		FlushCommandQueue();

		UINT								nSphereVertexCnt = 0;
		UINT								nSphereIndexCnt = 0;
		// 加载球体的网格数据
		{
			std::ifstream fin;
			char input;
			//USES_CONVERSION;

			fin.open("textures\\sphere.txt");

			if (fin.fail())
			{
				throw DxException(E_FAIL, L"open failed", L"CreateOthers", 615);
			}
			fin.get(input);
			while (input != ':')
			{
				fin.get(input);
			}
			fin >> nSphereVertexCnt;
			nSphereIndexCnt = nSphereVertexCnt;
			fin.get(input);
			while (input != ':')
			{
				fin.get(input);
			}
			fin.get(input);
			fin.get(input);

			mpstSphereVertices = (VERTEX*)TEX_CALLOC(nSphereVertexCnt * sizeof(VERTEX));
			mpSphereIndices = (UINT*)TEX_CALLOC(nSphereVertexCnt * sizeof(UINT));

			for (UINT i = 0; i < nSphereVertexCnt; i++)
			{
				fin >> mpstSphereVertices[i].m_vPos.x >> mpstSphereVertices[i].m_vPos.y >> mpstSphereVertices[i].m_vPos.z;
				mpstSphereVertices[i].m_vPos.w = 1.0f;
				fin >> mpstSphereVertices[i].m_vTex.x >> mpstSphereVertices[i].m_vTex.y;
				fin >> mpstSphereVertices[i].m_vNor.x >> mpstSphereVertices[i].m_vNor.y >> mpstSphereVertices[i].m_vNor.z;

				mpSphereIndices[i] = i;
			}
		}

		SIZE_T	szMVPBuf = TEX_UPPER(sizeof(ST_FRAME_MVP_BUFFER), 256);
		// 使用“定位方式”创建顶点缓冲和索引缓冲，使用与上传纹理数据缓冲相同的一个上传堆
		{
			UINT64 n64BufferOffset = TEX_UPPER(mn64UploadBufferSizeEarth, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

			D3D12_RESOURCE_DESC stBufResDesc = {};
			stBufResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			stBufResDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			stBufResDesc.Width = nSphereVertexCnt * sizeof(VERTEX);
			stBufResDesc.Height = 1;
			stBufResDesc.DepthOrArraySize = 1;
			stBufResDesc.MipLevels = 1;
			stBufResDesc.Format = DXGI_FORMAT_UNKNOWN;
			stBufResDesc.SampleDesc.Count = 1;
			stBufResDesc.SampleDesc.Quality = 0;
			stBufResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			stBufResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			//使用定位方式在相同的上传堆上以“定位方式”创建顶点缓冲，注意第二个参数指出了堆中的偏移位置
			//按照堆边界对齐的要求，我们主动将偏移位置对齐到了64k的边界上			
			ThrowIfFailed(md3dDevice->CreatePlacedResource(
				mpIUploadHeapEarth.Get()
				, n64BufferOffset
				, &stBufResDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&mpIVBEarth)));

			//使用map-memcpy-unmap大法将数据传至顶点缓冲对象
			//注意顶点缓冲使用是和上传纹理数据缓冲相同的一个堆，这很神奇
			UINT8* pVertexDataBegin = nullptr;
			D3D12_RANGE stReadRange = { 0, 0 };		// We do not intend to read from this resource on the CPU.

			ThrowIfFailed(mpIVBEarth->Map(0, &stReadRange, reinterpret_cast<void**>(&pVertexDataBegin)));
			memcpy(pVertexDataBegin, mpstSphereVertices, nSphereVertexCnt * sizeof(VERTEX));
			mpIVBEarth->Unmap(0, nullptr);

			SAFE_FREE(mpstSphereVertices);

			//创建资源视图，实际可以简单理解为指向顶点缓冲的显存指针
			mstVBVEarth.BufferLocation = mpIVBEarth->GetGPUVirtualAddress();
			mstVBVEarth.StrideInBytes = sizeof(VERTEX);
			mstVBVEarth.SizeInBytes = nSphereVertexCnt * sizeof(VERTEX);

			//计算边界对齐的正确的偏移位置
			n64BufferOffset = TEX_UPPER(n64BufferOffset + nSphereVertexCnt * sizeof(VERTEX), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

			stBufResDesc.Width = nSphereIndexCnt * sizeof(UINT);

			ThrowIfFailed(md3dDevice->CreatePlacedResource(
				mpIUploadHeapEarth.Get()
				, n64BufferOffset
				, &stBufResDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&mpIIBEarth)));

			UINT8* pIndexDataBegin = nullptr;
			ThrowIfFailed(mpIIBEarth->Map(0, &stReadRange, reinterpret_cast<void**>(&pIndexDataBegin)));
			memcpy(pIndexDataBegin, mpSphereIndices, nSphereIndexCnt * sizeof(UINT));
			mpIIBEarth->Unmap(0, nullptr);

			SAFE_FREE(mpSphereIndices);

			mstIBVEarth.BufferLocation = mpIIBEarth->GetGPUVirtualAddress();
			mstIBVEarth.Format = DXGI_FORMAT_R32_UINT;
			mstIBVEarth.SizeInBytes = nSphereIndexCnt * sizeof(UINT);

			n64BufferOffset = TEX_UPPER(n64BufferOffset + nSphereIndexCnt * sizeof(UINT), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

			stBufResDesc.Width = szMVPBuf;
			// 创建常量缓冲 注意缓冲尺寸设置为256边界对齐大小
			ThrowIfFailed(md3dDevice->CreatePlacedResource(
				mpIUploadHeapEarth.Get()
				, n64BufferOffset
				, &stBufResDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&mpICBVUploadEarth)));

			// Map 之后就不再Unmap了 直接复制数据进去 这样每帧都不用map-copy-unmap浪费时间了
			ThrowIfFailed(mpICBVUploadEarth->Map(0, nullptr, reinterpret_cast<void**>(&mpMVPBufferEarth)));
			//---------------------------------------------------------------------------------------------
		}

		//Sky Box的网格数据
		UINT								nSkyboxIndexCnt = 0;
		// 加载天空盒（远平面型）
		{

			float fHighW = -1.0f - (1.0f / (float)mClientWidth);
			float fHighH = -1.0f - (1.0f / (float)mClientHeight);
			float fLowW = 1.0f + (1.0f / (float)mClientWidth);
			float fLowH = 1.0f + (1.0f / (float)mClientHeight);

			VERTEX stSkyboxVertices[4] = {};

			stSkyboxVertices[0].m_vPos = XMFLOAT4(fLowW, fLowH, 1.0f, 1.0f);
			stSkyboxVertices[1].m_vPos = XMFLOAT4(fLowW, fHighH, 1.0f, 1.0f);
			stSkyboxVertices[2].m_vPos = XMFLOAT4(fHighW, fLowH, 1.0f, 1.0f);
			stSkyboxVertices[3].m_vPos = XMFLOAT4(fHighW, fHighH, 1.0f, 1.0f);

			nSkyboxIndexCnt = 4;
			//---------------------------------------------------------------------------------------------
			//加载天空盒子的数据
			D3D12_RESOURCE_DESC stBufResDesc = {};
			stBufResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			stBufResDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			stBufResDesc.Width = nSkyboxIndexCnt * sizeof(VERTEX);
			stBufResDesc.Height = 1;
			stBufResDesc.DepthOrArraySize = 1;
			stBufResDesc.MipLevels = 1;
			stBufResDesc.Format = DXGI_FORMAT_UNKNOWN;
			stBufResDesc.SampleDesc.Count = 1;
			stBufResDesc.SampleDesc.Quality = 0;
			stBufResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			stBufResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			UINT64 n64BufferOffset = TEX_UPPER(mn64UploadBufferSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

			ThrowIfFailed(md3dDevice->CreatePlacedResource(
				mpIUploadHeapSkybox.Get()
				, n64BufferOffset
				, &stBufResDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&mpIVBSkybox)));

			//使用map-memcpy-unmap大法将数据传至顶点缓冲对象
			//注意顶点缓冲使用是和上传纹理数据缓冲相同的一个堆，这很神奇
			VERTEX* pVertexDataBegin = nullptr;

			ThrowIfFailed(mpIVBSkybox->Map(0, nullptr, reinterpret_cast<void**>(&pVertexDataBegin)));
			memcpy(pVertexDataBegin, stSkyboxVertices, nSkyboxIndexCnt * sizeof(VERTEX));
			mpIVBSkybox->Unmap(0, nullptr);

			//创建资源视图，实际可以简单理解为指向顶点缓冲的显存指针
			mstVertexBufferView.BufferLocation = mpIVBSkybox->GetGPUVirtualAddress();
			mstVertexBufferView.StrideInBytes = sizeof(VERTEX);
			mstVertexBufferView.SizeInBytes = nSkyboxIndexCnt * sizeof(VERTEX);

			//计算边界对齐的正确的偏移位置
			n64BufferOffset = TEX_UPPER(n64BufferOffset + nSkyboxIndexCnt * sizeof(VERTEX), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

			// 创建常量缓冲 注意缓冲尺寸设置为256边界对齐大小
			stBufResDesc.Width = szMVPBuf;
			ThrowIfFailed(md3dDevice->CreatePlacedResource(
				mpIUploadHeapSkybox.Get()
				, n64BufferOffset
				, &stBufResDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&mpICBVUpload)));

			// Map 之后就不再Unmap了 直接复制数据进去 这样每帧都不用map-copy-unmap浪费时间了
			ThrowIfFailed(mpICBVUpload->Map(0, nullptr, reinterpret_cast<void**>(&mpMVPBuffer)));
			//---------------------------------------------------------------------------------------------
		}

		// 创建SRV描述符
		{
			D3D12_DESCRIPTOR_HEAP_DESC stSRVHeapDesc = {};
			stSRVHeapDesc.NumDescriptors = 2; //1 SRV + 1 CBV
			stSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			stSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&stSRVHeapDesc, IID_PPV_ARGS(&mpISRVHpEarth)));

			D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
			stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			stSRVDesc.Format = mstTxtFmtEarth;
			stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			stSRVDesc.Texture2D.MipLevels = 1;
			md3dDevice->CreateShaderResourceView(mpITextureEarth.Get(), &stSRVDesc, mpISRVHpEarth->GetCPUDescriptorHandleForHeapStart());

			D3D12_RESOURCE_DESC stDescSkybox = mpITexture->GetDesc();
			stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			stSRVDesc.Format = stDescSkybox.Format;
			stSRVDesc.TextureCube.MipLevels = stDescSkybox.MipLevels;
			md3dDevice->CreateShaderResourceView(mpITexture.Get(), &stSRVDesc, mSrvHeap->GetCPUDescriptorHandleForHeapStart());
		}

		// 创建CBV描述符
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = mpICBVUploadEarth->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = static_cast<UINT>(szMVPBuf);
			UINT nSRVDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			D3D12_CPU_DESCRIPTOR_HANDLE stSRVCBVHandle = mpISRVHpEarth->GetCPUDescriptorHandleForHeapStart();
			stSRVCBVHandle.ptr += nSRVDescriptorSize;

			md3dDevice->CreateConstantBufferView(&cbvDesc, stSRVCBVHandle);

			//---------------------------------------------------------------------------------------------
			cbvDesc.BufferLocation = mpICBVUpload->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = static_cast<UINT>(szMVPBuf);

			D3D12_CPU_DESCRIPTOR_HANDLE cbvSrvHandleSkybox = { mSrvHeap->GetCPUDescriptorHandleForHeapStart() };
			cbvSrvHandleSkybox.ptr += nSRVDescriptorSize;

			md3dDevice->CreateConstantBufferView(&cbvDesc, cbvSrvHandleSkybox);
			//---------------------------------------------------------------------------------------------

		}

		// 创建各种采样器
		{
			UINT nSamplerDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
			D3D12_DESCRIPTOR_HEAP_DESC stSamplerHeapDesc = {};
			stSamplerHeapDesc.NumDescriptors = mnSampleMaxCnt;
			stSamplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			stSamplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&stSamplerHeapDesc, IID_PPV_ARGS(&mpISampleHpEarth)));
			D3D12_CPU_DESCRIPTOR_HANDLE hSamplerHeap = mpISampleHpEarth->GetCPUDescriptorHandleForHeapStart();

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

			hSamplerHeap.ptr += nSamplerDescriptorSize;

			// Sampler 2
			stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			md3dDevice->CreateSampler(&stSamplerDesc, hSamplerHeap);

			hSamplerHeap.ptr += nSamplerDescriptorSize;

			// Sampler 3
			stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			md3dDevice->CreateSampler(&stSamplerDesc, hSamplerHeap);

			hSamplerHeap.ptr += nSamplerDescriptorSize;

			// Sampler 4
			stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			md3dDevice->CreateSampler(&stSamplerDesc, hSamplerHeap);

			hSamplerHeap.ptr += nSamplerDescriptorSize;

			// Sampler 5
			stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
			stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
			stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
			md3dDevice->CreateSampler(&stSamplerDesc, hSamplerHeap);

			//---------------------------------------------------------------------------------------------
			//创建Skybox的采样器
			stSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

			stSamplerDesc.MinLOD = 0;
			stSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
			stSamplerDesc.MipLODBias = 0.0f;
			stSamplerDesc.MaxAnisotropy = 1;
			stSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
			stSamplerDesc.BorderColor[0] = 0.0f;
			stSamplerDesc.BorderColor[1] = 0.0f;
			stSamplerDesc.BorderColor[2] = 0.0f;
			stSamplerDesc.BorderColor[3] = 0.0f;
			stSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			stSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			stSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

			md3dDevice->CreateSampler(&stSamplerDesc, mpISamplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
			//---------------------------------------------------------------------------------------------

		}

		// 用捆绑包记录固化的命令
		{
			//球体的捆绑包
			UINT nSRVDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			UINT nSamplerDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
			mpIBundlesEarth->SetGraphicsRootSignature(mpIRootSignature.Get());
			mpIBundlesEarth->SetPipelineState(mEarthPSO.Get());

			ID3D12DescriptorHeap* ppHeapsEarth[] = { mpISRVHpEarth.Get(),mpISampleHpEarth.Get() };
			mpIBundlesEarth->SetDescriptorHeaps(_countof(ppHeapsEarth), ppHeapsEarth);
			//设置SRV
			mpIBundlesEarth->SetGraphicsRootDescriptorTable(0, mpISRVHpEarth->GetGPUDescriptorHandleForHeapStart());

			D3D12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandleEarth = mpISRVHpEarth->GetGPUDescriptorHandleForHeapStart();
			stGPUCBVHandleEarth.ptr += nSRVDescriptorSize;

			//设置CBV
			mpIBundlesEarth->SetGraphicsRootDescriptorTable(1, stGPUCBVHandleEarth);

			D3D12_GPU_DESCRIPTOR_HANDLE hGPUSamplerEarth = mpISampleHpEarth->GetGPUDescriptorHandleForHeapStart();
			hGPUSamplerEarth.ptr += (mnCurrentSamplerNO * nSamplerDescriptorSize);

			//设置Sample
			mpIBundlesEarth->SetGraphicsRootDescriptorTable(2, hGPUSamplerEarth);
			//注意我们使用的渲染手法是三角形列表，也就是通常的Mesh网格
			mpIBundlesEarth->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			mpIBundlesEarth->IASetVertexBuffers(0, 1, &mstVBVEarth);
			mpIBundlesEarth->IASetIndexBuffer(&mstIBVEarth);

			//Draw Call！！！
			mpIBundlesEarth->DrawIndexedInstanced(nSphereIndexCnt, 1, 0, 0, 0);
			mpIBundlesEarth->Close();



			//Skybox的捆绑包
			mpIBundlesSkybox->SetPipelineState(mPSO.Get());
			mpIBundlesSkybox->SetGraphicsRootSignature(mpIRootSignature.Get());
			ID3D12DescriptorHeap* ppHeaps[] = { mSrvHeap.Get(),mpISamplerDescriptorHeap.Get() };
			mpIBundlesSkybox->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
			//设置SRV
			mpIBundlesSkybox->SetGraphicsRootDescriptorTable(0, mSrvHeap->GetGPUDescriptorHandleForHeapStart());

			D3D12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandleSkybox = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
			stGPUCBVHandleSkybox.ptr += nSRVDescriptorSize;
			//设置CBV
			mpIBundlesSkybox->SetGraphicsRootDescriptorTable(1, stGPUCBVHandleSkybox);
			mpIBundlesSkybox->SetGraphicsRootDescriptorTable(2, mpISamplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			mpIBundlesSkybox->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			mpIBundlesSkybox->IASetVertexBuffers(0, 1, &mstVertexBufferView);

			//Draw Call！！！
			mpIBundlesSkybox->DrawInstanced(4, 1, 0, 0);
			mpIBundlesSkybox->Close();
		}

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

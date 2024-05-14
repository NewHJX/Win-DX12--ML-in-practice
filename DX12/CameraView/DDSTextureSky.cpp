#include "DDSTextureSky.h"

struct VERTEX
{
	//������Ƕ��������ÿ������ķ��ߣ���Shader�л���ʱû����
	DirectX::XMFLOAT4 m_vPos;		//Position
	DirectX::XMFLOAT2 m_vTex;		//Texcoord
	DirectX::XMFLOAT3 m_vNor;		//Normal
};

struct ST_SKYBOX_VERTEX
{//��պ��ӵĶ���ṹ
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
	// ����Ƿ�֧��V1.1�汾�ĸ�ǩ��
	stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &stFeatureData, sizeof(stFeatureData))))
	{// 1.0�� ֱ�Ӷ��쳣�˳���
		ThrowIfFailed(E_NOTIMPL);
	}
	// ��GPU��ִ��SetGraphicsRootDescriptorTable�����ǲ��޸������б��е�SRV��������ǿ���ʹ��Ĭ��Rang��Ϊ:
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

	//����Ϊ�о�����ʽ	   
	nCompileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

	ThrowIfFailed(D3DCompileFromFile(L"Shaders\\TextureCube.hlsl", nullptr, nullptr
		, "VSMain", "vs_5_0", nCompileFlags, 0, &mpVertexEarthShader, nullptr));
	ThrowIfFailed(D3DCompileFromFile(L"Shaders\\TextureCube.hlsl", nullptr, nullptr
		, "PSMain", "ps_5_0", nCompileFlags, 0, &mpPixelEarthShader, nullptr));


	//����Ϊ�о�����ʽ	   
	nCompileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

	ThrowIfFailed(D3DCompileFromFile(L"Shaders\\SkyBox.hlsl", nullptr, nullptr
		, "SkyboxVS", "vs_5_0", nCompileFlags, 0, &mpVertexShader, nullptr));
	ThrowIfFailed(D3DCompileFromFile(L"Shaders\\SkyBox.hlsl", nullptr, nullptr
		, "SkyboxPS", "ps_5_0", nCompileFlags, 0, &mpPixelShader, nullptr));

	return S_OK;
}

HRESULT DDSTextureSky::CreatePSO()
{
	// ���Ƕ������һ�����ߵĶ��壬��ĿǰShader�����ǲ�û��ʹ��
	D3D12_INPUT_ELEMENT_DESC stIALayoutEarth[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// ���� graphics pipeline state object (PSO)����
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
	stPSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//������Ȼ���д�빦��
	stPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;     //��Ȳ��Ժ�������ֵΪ��ͨ����Ȳ��ԣ�
	stPSODesc.DepthStencilState.StencilEnable = FALSE;
	stPSODesc.SampleDesc.Count = 1;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&stPSODesc
		, IID_PPV_ARGS(&mEarthPSO)));

	// ��պ���ֻ�ж���ֻ��λ�ò���
	D3D12_INPUT_ELEMENT_DESC stIALayoutSkybox[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// ����Skybox��(PSO)���� 
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

	// ���������Ĭ�϶ѡ��ϴ��Ѳ���������
	{
		D3D12_HEAP_DESC stTextureHeapDesc = {};
		//Ϊ��ָ������ͼƬ����2����С�Ŀռ䣬����û����ϸȥ�����ˣ�ֻ��ָ����һ���㹻��Ŀռ䣬�����������
		//ʵ��Ӧ����Ҳ��Ҫ�ۺϿ��Ƿ���ѵĴ�С���Ա�������ö�
		stTextureHeapDesc.SizeInBytes = TEX_UPPER(2 * mnRowPitchEarth * mnTextureEarthH, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		//ָ���ѵĶ��뷽ʽ������ʹ����Ĭ�ϵ�64K�߽���룬��Ϊ������ʱ����ҪMSAA֧��
		stTextureHeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		stTextureHeapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;		//Ĭ�϶�����
		stTextureHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stTextureHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		//�ܾ���ȾĿ�������ܾ������������ʵ�ʾ�ֻ�������ڷ���ͨ����
		stTextureHeapDesc.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS;

		ThrowIfFailed(md3dDevice->CreateHeap(&stTextureHeapDesc, IID_PPV_ARGS(&mpIRESHeapEarth)));

		D3D12_RESOURCE_DESC					stTextureDesc = {};
		// ����2D����		
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
		//ʹ�á���λ��ʽ������������ע��������������ڲ�ʵ���Ѿ�û�д洢������ͷŵ�ʵ�ʲ����ˣ��������ܸܺ�
		//ͬʱ������������Ϸ�������CreatePlacedResource��������ͬ��������Ȼǰ�������ǲ��ڱ�ʹ�õ�ʱ�򣬲ſ���
		//���ö�
		ThrowIfFailed(md3dDevice->CreatePlacedResource(
			mpIRESHeapEarth.Get()
			, 0
			, &stTextureDesc				//����ʹ��CD3DX12_RESOURCE_DESC::Tex2D���򻯽ṹ��ĳ�ʼ��
			, D3D12_RESOURCE_STATE_COPY_DEST
			, nullptr
			, IID_PPV_ARGS(&mpITextureEarth)));
		//-----------------------------------------------------------------------------------------------------------
		//��ȡ�ϴ�����Դ����Ĵ�С������ߴ�ͨ������ʵ��ͼƬ�ĳߴ�
		D3D12_RESOURCE_DESC stCopyDstDesc = mpITextureEarth->GetDesc();
		md3dDevice->GetCopyableFootprints(&stCopyDstDesc, 0, 1, 0, nullptr, nullptr, nullptr, &mn64UploadBufferSizeEarth);

		//-----------------------------------------------------------------------------------------------------------
		// �����ϴ���
		D3D12_HEAP_DESC stUploadHeapDesc = {  };
		//�ߴ���Ȼ��ʵ���������ݴ�С��2����64K�߽�����С
		stUploadHeapDesc.SizeInBytes = TEX_UPPER(2 * mn64UploadBufferSizeEarth, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		//ע���ϴ��ѿ϶���Buffer���ͣ����Բ�ָ�����뷽ʽ����Ĭ����64k�߽����
		stUploadHeapDesc.Alignment = 0;
		stUploadHeapDesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;		//�ϴ�������
		stUploadHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stUploadHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		//�ϴ��Ѿ��ǻ��壬���԰ڷ���������
		stUploadHeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

		ThrowIfFailed(md3dDevice->CreateHeap(&stUploadHeapDesc, IID_PPV_ARGS(&mpIUploadHeapEarth)));

		//-----------------------------------------------------------------------------------------------------------
		// ʹ�á���λ��ʽ�����������ϴ��������ݵĻ�����Դ
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

		// ����ͼƬ�������ϴ��ѣ�����ɵ�һ��Copy��������memcpy������֪������CPU��ɵ�

		//������Դ�����С������ʵ��ͼƬ���ݴ洢���ڴ��С
		void* pbPicData = TEX_CALLOC(mn64UploadBufferSizeEarth);
		if (nullptr == pbPicData)
		{
			throw DxException(HRESULT_FROM_WIN32(GetLastError()), L"FORMAT_UNKNOWN", L"DDSTextureSky::CreateOthers", 330);
		}

		//��ͼƬ�ж�ȡ������
		ThrowIfFailed(mpIBMPEarth->CopyPixels(nullptr
			, mnRowPitchEarth
			, static_cast<UINT>(mnRowPitchEarth * mnTextureEarthH)   //ע���������ͼƬ������ʵ�Ĵ�С�����ֵͨ��С�ڻ���Ĵ�С
			, reinterpret_cast<BYTE*>(pbPicData)));

		//{//������δ�������DX12��ʾ����ֱ��ͨ����仺�������һ���ڰ׷��������
		// //��ԭ��δ��룬Ȼ��ע�������CopyPixels���ÿ��Կ����ڰ׷��������Ч��
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

		//��ȡ���ϴ��ѿ����������ݵ�һЩ����ת���ߴ���Ϣ
		//���ڸ��ӵ�DDS�������Ƿǳ���Ҫ�Ĺ���

		UINT   nNumSubresources = 1u;  //����ֻ��һ��ͼƬ��������Դ����Ϊ1
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

		//��Ϊ�ϴ���ʵ�ʾ���CPU�������ݵ�GPU���н�
		//�������ǿ���ʹ����Ϥ��Map����������ӳ�䵽CPU�ڴ��ַ��
		//Ȼ�����ǰ��н����ݸ��Ƶ��ϴ�����
		//��Ҫע�����֮���԰��п�������ΪGPU��Դ���д�С
		//��ʵ��ͼƬ���д�С���в����,���ߵ��ڴ�߽����Ҫ���ǲ�һ����
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
		//ȡ��ӳ�� �����ױ��������ÿ֡�ı任��������ݣ�������������Unmap�ˣ�
		//������פ�ڴ�,������������ܣ���Ϊÿ��Map��Unmap�Ǻܺ�ʱ�Ĳ���
		//��Ϊ�������붼��64λϵͳ��Ӧ���ˣ���ַ�ռ����㹻�ģ�������ռ�ò���Ӱ��ʲô
		mpITextureUploadEarth->Unmap(0, nullptr);

		//�ͷ�ͼƬ���ݣ���һ���ɾ��ĳ���Ա
		SAFE_FREE(pbPicData);
	}

	// ʹ��DDSLoader������������Skybox������
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

		//���溯�����ص���������ʽĬ�϶��ϣ�����Copy������Ҫ�����Լ����
		mpITexture.Attach(pIResSkyBox);
		//=====================================================================================================
		//��ȡSkybox����Դ��С���������ϴ���
		D3D12_RESOURCE_DESC stCopyDstDesc = mpITexture->GetDesc();
		// �����ǵ�һ�ε���GetCopyableFootprints
		md3dDevice->GetCopyableFootprints(&stCopyDstDesc
			, 0, static_cast<UINT>(arSubResources.size()), 0, nullptr, nullptr, nullptr, &mn64UploadBufferSize);

		D3D12_HEAP_DESC stUploadHeapDesc = {  };
		//�ߴ���Ȼ��ʵ���������ݴ�С��2����64K�߽�����С
		stUploadHeapDesc.SizeInBytes = TEX_UPPER(2 * mn64UploadBufferSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		//ע���ϴ��ѿ϶���Buffer���ͣ����Բ�ָ�����뷽ʽ����Ĭ����64k�߽����
		stUploadHeapDesc.Alignment = 0;
		stUploadHeapDesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;		//�ϴ�������
		stUploadHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stUploadHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		//�ϴ��Ѿ��ǻ��壬���԰ڷ���������
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

		// �ϴ�Skybox������ע����Ϊ�������պ�����Դ��DDS��ʽ�ģ������ж��Mipmap����������Դ�������ܶ�
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

		// �����ǵڶ��ε���GetCopyableFootprints���͵õ�����������Դ����ϸ��Ϣ
		md3dDevice->GetCopyableFootprints(&stDefaultResDesc, nFirstSubresource, nNumSubresources, 0, pLayouts, pNumRows, pRowSizesInBytes, &n64RequiredSize);

		BYTE* pData = nullptr;
		HRESULT hr = mpITextureUpload->Map(0, nullptr, reinterpret_cast<void**>(&pData));
		if (FAILED(hr))
		{
			return 0;
		}

		// ��һ��Copy��ע��3��ѭ��ÿ�ص���˼
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

		// �ڶ���Copy��
		if (stDefaultResDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{// Buffer һ���Ը��ƾͿ����ˣ���Ϊû���ж�����д�С��һ�µ����⣬Buffer����������1
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

		//��ֱ�������б������ϴ��Ѹ����������ݵ�Ĭ�϶ѵ����ִ�в�ͬ���ȴ�������ɵڶ���Copy��������GPU�ϵĸ����������
		//ע���ʱֱ�������б�û�а�PSO���������Ҳ�ǲ���ִ��3Dͼ������ģ����ǿ���ִ�и��������Ϊ�������治��Ҫʲô
		//�����״̬����֮��Ĳ���
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

			//����һ����Դ���ϣ�ͬ����ȷ�ϸ��Ʋ������
			//ֱ��ʹ�ýṹ��Ȼ����õ���ʽ
			D3D12_RESOURCE_BARRIER stResBar = {};
			stResBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			stResBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			stResBar.Transition.pResource = mpITextureEarth.Get();
			stResBar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			stResBar.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			stResBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			mCommandList->ResourceBarrier(1, &stResBar);

		}

		// ִ�еڶ���Copy���ȷ�����е������ϴ�����Ĭ�϶���
		FlushCommandQueue();

		UINT								nSphereVertexCnt = 0;
		UINT								nSphereIndexCnt = 0;
		// �����������������
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
		// ʹ�á���λ��ʽ���������㻺����������壬ʹ�����ϴ��������ݻ�����ͬ��һ���ϴ���
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

			//ʹ�ö�λ��ʽ����ͬ���ϴ������ԡ���λ��ʽ���������㻺�壬ע��ڶ�������ָ���˶��е�ƫ��λ��
			//���նѱ߽�����Ҫ������������ƫ��λ�ö��뵽��64k�ı߽���			
			ThrowIfFailed(md3dDevice->CreatePlacedResource(
				mpIUploadHeapEarth.Get()
				, n64BufferOffset
				, &stBufResDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&mpIVBEarth)));

			//ʹ��map-memcpy-unmap�󷨽����ݴ������㻺�����
			//ע�ⶥ�㻺��ʹ���Ǻ��ϴ��������ݻ�����ͬ��һ���ѣ��������
			UINT8* pVertexDataBegin = nullptr;
			D3D12_RANGE stReadRange = { 0, 0 };		// We do not intend to read from this resource on the CPU.

			ThrowIfFailed(mpIVBEarth->Map(0, &stReadRange, reinterpret_cast<void**>(&pVertexDataBegin)));
			memcpy(pVertexDataBegin, mpstSphereVertices, nSphereVertexCnt * sizeof(VERTEX));
			mpIVBEarth->Unmap(0, nullptr);

			SAFE_FREE(mpstSphereVertices);

			//������Դ��ͼ��ʵ�ʿ��Լ����Ϊָ�򶥵㻺����Դ�ָ��
			mstVBVEarth.BufferLocation = mpIVBEarth->GetGPUVirtualAddress();
			mstVBVEarth.StrideInBytes = sizeof(VERTEX);
			mstVBVEarth.SizeInBytes = nSphereVertexCnt * sizeof(VERTEX);

			//����߽�������ȷ��ƫ��λ��
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
			// ������������ ע�⻺��ߴ�����Ϊ256�߽�����С
			ThrowIfFailed(md3dDevice->CreatePlacedResource(
				mpIUploadHeapEarth.Get()
				, n64BufferOffset
				, &stBufResDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&mpICBVUploadEarth)));

			// Map ֮��Ͳ���Unmap�� ֱ�Ӹ������ݽ�ȥ ����ÿ֡������map-copy-unmap�˷�ʱ����
			ThrowIfFailed(mpICBVUploadEarth->Map(0, nullptr, reinterpret_cast<void**>(&mpMVPBufferEarth)));
			//---------------------------------------------------------------------------------------------
		}

		//Sky Box����������
		UINT								nSkyboxIndexCnt = 0;
		// ������պУ�Զƽ���ͣ�
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
			//������պ��ӵ�����
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

			//ʹ��map-memcpy-unmap�󷨽����ݴ������㻺�����
			//ע�ⶥ�㻺��ʹ���Ǻ��ϴ��������ݻ�����ͬ��һ���ѣ��������
			VERTEX* pVertexDataBegin = nullptr;

			ThrowIfFailed(mpIVBSkybox->Map(0, nullptr, reinterpret_cast<void**>(&pVertexDataBegin)));
			memcpy(pVertexDataBegin, stSkyboxVertices, nSkyboxIndexCnt * sizeof(VERTEX));
			mpIVBSkybox->Unmap(0, nullptr);

			//������Դ��ͼ��ʵ�ʿ��Լ����Ϊָ�򶥵㻺����Դ�ָ��
			mstVertexBufferView.BufferLocation = mpIVBSkybox->GetGPUVirtualAddress();
			mstVertexBufferView.StrideInBytes = sizeof(VERTEX);
			mstVertexBufferView.SizeInBytes = nSkyboxIndexCnt * sizeof(VERTEX);

			//����߽�������ȷ��ƫ��λ��
			n64BufferOffset = TEX_UPPER(n64BufferOffset + nSkyboxIndexCnt * sizeof(VERTEX), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

			// ������������ ע�⻺��ߴ�����Ϊ256�߽�����С
			stBufResDesc.Width = szMVPBuf;
			ThrowIfFailed(md3dDevice->CreatePlacedResource(
				mpIUploadHeapSkybox.Get()
				, n64BufferOffset
				, &stBufResDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&mpICBVUpload)));

			// Map ֮��Ͳ���Unmap�� ֱ�Ӹ������ݽ�ȥ ����ÿ֡������map-copy-unmap�˷�ʱ����
			ThrowIfFailed(mpICBVUpload->Map(0, nullptr, reinterpret_cast<void**>(&mpMVPBuffer)));
			//---------------------------------------------------------------------------------------------
		}

		// ����SRV������
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

		// ����CBV������
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

		// �������ֲ�����
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
			//����Skybox�Ĳ�����
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

		// ���������¼�̻�������
		{
			//����������
			UINT nSRVDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			UINT nSamplerDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
			mpIBundlesEarth->SetGraphicsRootSignature(mpIRootSignature.Get());
			mpIBundlesEarth->SetPipelineState(mEarthPSO.Get());

			ID3D12DescriptorHeap* ppHeapsEarth[] = { mpISRVHpEarth.Get(),mpISampleHpEarth.Get() };
			mpIBundlesEarth->SetDescriptorHeaps(_countof(ppHeapsEarth), ppHeapsEarth);
			//����SRV
			mpIBundlesEarth->SetGraphicsRootDescriptorTable(0, mpISRVHpEarth->GetGPUDescriptorHandleForHeapStart());

			D3D12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandleEarth = mpISRVHpEarth->GetGPUDescriptorHandleForHeapStart();
			stGPUCBVHandleEarth.ptr += nSRVDescriptorSize;

			//����CBV
			mpIBundlesEarth->SetGraphicsRootDescriptorTable(1, stGPUCBVHandleEarth);

			D3D12_GPU_DESCRIPTOR_HANDLE hGPUSamplerEarth = mpISampleHpEarth->GetGPUDescriptorHandleForHeapStart();
			hGPUSamplerEarth.ptr += (mnCurrentSamplerNO * nSamplerDescriptorSize);

			//����Sample
			mpIBundlesEarth->SetGraphicsRootDescriptorTable(2, hGPUSamplerEarth);
			//ע������ʹ�õ���Ⱦ�ַ����������б�Ҳ����ͨ����Mesh����
			mpIBundlesEarth->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			mpIBundlesEarth->IASetVertexBuffers(0, 1, &mstVBVEarth);
			mpIBundlesEarth->IASetIndexBuffer(&mstIBVEarth);

			//Draw Call������
			mpIBundlesEarth->DrawIndexedInstanced(nSphereIndexCnt, 1, 0, 0, 0);
			mpIBundlesEarth->Close();



			//Skybox�������
			mpIBundlesSkybox->SetPipelineState(mPSO.Get());
			mpIBundlesSkybox->SetGraphicsRootSignature(mpIRootSignature.Get());
			ID3D12DescriptorHeap* ppHeaps[] = { mSrvHeap.Get(),mpISamplerDescriptorHeap.Get() };
			mpIBundlesSkybox->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
			//����SRV
			mpIBundlesSkybox->SetGraphicsRootDescriptorTable(0, mSrvHeap->GetGPUDescriptorHandleForHeapStart());

			D3D12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandleSkybox = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
			stGPUCBVHandleSkybox.ptr += nSRVDescriptorSize;
			//����CBV
			mpIBundlesSkybox->SetGraphicsRootDescriptorTable(1, stGPUCBVHandleSkybox);
			mpIBundlesSkybox->SetGraphicsRootDescriptorTable(2, mpISamplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			mpIBundlesSkybox->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			mpIBundlesSkybox->IASetVertexBuffers(0, 1, &mstVertexBufferView);

			//Draw Call������
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
	//ʹ�ô�COM��ʽ����WIC�೧����Ҳ�ǵ���WIC��һ��Ҫ��������
	ThrowIfFailed(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&mpIWICFactory)));

	//ʹ��WIC�೧����ӿڼ�������ͼƬ�����õ�һ��WIC����������ӿڣ�ͼƬ��Ϣ��������ӿڴ���Ķ�������

	ThrowIfFailed(mpIWICFactory->CreateDecoderFromFilename(
		L"textures/����.jpg",              // �ļ���
		nullptr,                            // ��ָ����������ʹ��Ĭ��
		GENERIC_READ,                    // ����Ȩ��
		WICDecodeMetadataCacheOnDemand,  // ����Ҫ�ͻ������� 
		&mpIWICDecoder                    // ����������
	));
	// ��ȡ��һ֡ͼƬ(��ΪGIF�ȸ�ʽ�ļ����ܻ��ж�֡ͼƬ�������ĸ�ʽһ��ֻ��һ֡ͼƬ)
	// ʵ�ʽ���������������λͼ��ʽ����
	ThrowIfFailed(mpIWICDecoder->GetFrame(0, &mpIWICFrame));
	//��ȡWICͼƬ��ʽ
	ThrowIfFailed(mpIWICFrame->GetPixelFormat(&wpf));
	//ͨ����һ��ת��֮���ȡDXGI�ĵȼ۸�ʽ
	if (GetTargetPixelFormat(&wpf, &tgFormat))
	{
		mstTxtFmtEarth = GetDXGIFormatFromPixelFormat(&tgFormat);
	}

	if (DXGI_FORMAT_UNKNOWN == mstTxtFmtEarth)
	{// ��֧�ֵ�ͼƬ��ʽ Ŀǰ�˳����� 
	 // һ�� ��ʵ�ʵ����浱�ж����ṩ�����ʽת�����ߣ�
	 // ͼƬ����Ҫ��ǰת���ã����Բ�����ֲ�֧�ֵ�����
		throw DxException(S_FALSE, L"FORMAT_UNKNOWN", L"DDSTextureSky::LoadRenderData", 118);
	}

	if (!InlineIsEqualGUID(wpf, tgFormat))
	{// ����жϺ���Ҫ�����ԭWIC��ʽ����ֱ����ת��ΪDXGI��ʽ��ͼƬʱ
	 // ������Ҫ���ľ���ת��ͼƬ��ʽΪ�ܹ�ֱ�Ӷ�ӦDXGI��ʽ����ʽ
		//����ͼƬ��ʽת����
		ThrowIfFailed(mpIWICFactory->CreateFormatConverter(&pIConverter));
		//��ʼ��һ��ͼƬת������ʵ��Ҳ���ǽ�ͼƬ���ݽ����˸�ʽת��
		ThrowIfFailed(pIConverter->Initialize(
			mpIWICFrame.Get(),                // ����ԭͼƬ����
			tgFormat,						 // ָ����ת����Ŀ���ʽ
			WICBitmapDitherTypeNone,         // ָ��λͼ�Ƿ��е�ɫ�壬�ִ��������λͼ�����õ�ɫ�壬����ΪNone
			nullptr,                            // ָ����ɫ��ָ��
			0.f,                             // ָ��Alpha��ֵ
			WICBitmapPaletteTypeCustom       // ��ɫ�����ͣ�ʵ��û��ʹ�ã�����ָ��ΪCustom
		));
		// ����QueryInterface������ö����λͼ����Դ�ӿ�
		ThrowIfFailed(pIConverter.As(&mpIBMPEarth));
	}
	else
	{
		//ͼƬ���ݸ�ʽ����Ҫת����ֱ�ӻ�ȡ��λͼ����Դ�ӿ�
		ThrowIfFailed(mpIWICFrame.As(&mpIBMPEarth));
	}
	//���ͼƬ��С����λ�����أ�
	ThrowIfFailed(mpIBMPEarth->GetSize(&mnTextureEarthW, &mnTextureEarthH));
	//��ȡͼƬ���ص�λ��С��BPP��Bits Per Pixel����Ϣ�����Լ���ͼƬ�����ݵ���ʵ��С����λ���ֽڣ�
	ThrowIfFailed(mpIWICFactory->CreateComponentInfo(tgFormat, pIWICmntinfo.GetAddressOf()));
	ThrowIfFailed(pIWICmntinfo->GetComponentType(&type));
	if (type != WICPixelFormat)
	{
		throw DxException(S_FALSE, L"FORMAT_UNKNOWN", L"DDSTextureSky::LoadRenderData", 150);
	}
	ThrowIfFailed(pIWICmntinfo.As(&pIWICPixelinfo));
	// ���������ڿ��Եõ�BPP�ˣ���Ҳ���ҿ��ıȽ���Ѫ�ĵط���Ϊ��BPP��Ȼ������ô�໷��
	ThrowIfFailed(pIWICPixelinfo->GetBitsPerPixel(&mnBPPEarth));
	// ����ͼƬʵ�ʵ��д�С����λ���ֽڣ�������ʹ����һ����ȡ����������A+B-1��/B ��
	// ����������˵��΢���������,ϣ�����Ѿ���������ָ��
	mnRowPitchEarth = TEX_UPPER_DIV(uint64_t(mnTextureEarthH) * uint64_t(mnBPPEarth), 8);
}

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

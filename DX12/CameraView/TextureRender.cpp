#include "TextureRender.h"
#include "d3d12.h"

#pragma comment(lib, "dxguid.lib")

struct WICTranslate
{
	GUID wic;
	DXGI_FORMAT format;
};

static WICTranslate g_WICFormats[] =
{//WIC��ʽ��DXGI���ظ�ʽ�Ķ�Ӧ���ñ��еĸ�ʽΪ��֧�ֵĸ�ʽ
	{ GUID_WICPixelFormat128bppRGBAFloat,       DXGI_FORMAT_R32G32B32A32_FLOAT },

	{ GUID_WICPixelFormat64bppRGBAHalf,         DXGI_FORMAT_R16G16B16A16_FLOAT },
	{ GUID_WICPixelFormat64bppRGBA,             DXGI_FORMAT_R16G16B16A16_UNORM },

	{ GUID_WICPixelFormat32bppRGBA,             DXGI_FORMAT_R8G8B8A8_UNORM },
	{ GUID_WICPixelFormat32bppBGRA,             DXGI_FORMAT_B8G8R8A8_UNORM }, // DXGI 1.1
	{ GUID_WICPixelFormat32bppBGR,              DXGI_FORMAT_B8G8R8X8_UNORM }, // DXGI 1.1

	{ GUID_WICPixelFormat32bppRGBA1010102XR,    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM }, // DXGI 1.1
	{ GUID_WICPixelFormat32bppRGBA1010102,      DXGI_FORMAT_R10G10B10A2_UNORM },

	{ GUID_WICPixelFormat16bppBGRA5551,         DXGI_FORMAT_B5G5R5A1_UNORM },
	{ GUID_WICPixelFormat16bppBGR565,           DXGI_FORMAT_B5G6R5_UNORM },

	{ GUID_WICPixelFormat32bppGrayFloat,        DXGI_FORMAT_R32_FLOAT },
	{ GUID_WICPixelFormat16bppGrayHalf,         DXGI_FORMAT_R16_FLOAT },
	{ GUID_WICPixelFormat16bppGray,             DXGI_FORMAT_R16_UNORM },
	{ GUID_WICPixelFormat8bppGray,              DXGI_FORMAT_R8_UNORM },

	{ GUID_WICPixelFormat8bppAlpha,             DXGI_FORMAT_A8_UNORM },
};

// WIC ���ظ�ʽת����.
struct WICConvert
{
	GUID source;
	GUID target;
};

static WICConvert g_WICConvert[] =
{
	// Ŀ���ʽһ������ӽ��ı�֧�ֵĸ�ʽ
	{ GUID_WICPixelFormatBlackWhite,            GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM

	{ GUID_WICPixelFormat1bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat2bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat4bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat8bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM

	{ GUID_WICPixelFormat2bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM
	{ GUID_WICPixelFormat4bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM

	{ GUID_WICPixelFormat16bppGrayFixedPoint,   GUID_WICPixelFormat16bppGrayHalf }, // DXGI_FORMAT_R16_FLOAT
	{ GUID_WICPixelFormat32bppGrayFixedPoint,   GUID_WICPixelFormat32bppGrayFloat }, // DXGI_FORMAT_R32_FLOAT

	{ GUID_WICPixelFormat16bppBGR555,           GUID_WICPixelFormat16bppBGRA5551 }, // DXGI_FORMAT_B5G5R5A1_UNORM

	{ GUID_WICPixelFormat32bppBGR101010,        GUID_WICPixelFormat32bppRGBA1010102 }, // DXGI_FORMAT_R10G10B10A2_UNORM

	{ GUID_WICPixelFormat24bppBGR,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat24bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat32bppPBGRA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat32bppPRGBA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM

	{ GUID_WICPixelFormat48bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat48bppBGR,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppBGRA,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppPRGBA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppPBGRA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

	{ GUID_WICPixelFormat48bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat48bppBGRFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat64bppRGBAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat64bppBGRAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat64bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat48bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat64bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT

	{ GUID_WICPixelFormat128bppPRGBAFloat,      GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
	{ GUID_WICPixelFormat128bppRGBFloat,        GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
	{ GUID_WICPixelFormat128bppRGBAFixedPoint,  GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
	{ GUID_WICPixelFormat128bppRGBFixedPoint,   GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
	{ GUID_WICPixelFormat32bppRGBE,             GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT

	{ GUID_WICPixelFormat32bppCMYK,             GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat64bppCMYK,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat40bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat80bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

	{ GUID_WICPixelFormat32bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat64bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppPRGBAHalf,        GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
};

bool GetTargetPixelFormat(const GUID* pSourceFormat, GUID* pTargetFormat)
{//���ȷ�����ݵ���ӽ���ʽ���ĸ�
	*pTargetFormat = *pSourceFormat;
	for (size_t i = 0; i < _countof(g_WICConvert); ++i)
	{
		if (InlineIsEqualGUID(g_WICConvert[i].source, *pSourceFormat))
		{
			*pTargetFormat = g_WICConvert[i].target;
			return true;
		}
	}
	return false;
}

DXGI_FORMAT GetDXGIFormatFromPixelFormat(const GUID* pPixelFormat)
{//���ȷ�����ն�Ӧ��DXGI��ʽ����һ��
	for (size_t i = 0; i < _countof(g_WICFormats); ++i)
	{
		if (InlineIsEqualGUID(g_WICFormats[i].wic, *pPixelFormat))
		{
			return g_WICFormats[i].format;
		}
	}
	return DXGI_FORMAT_UNKNOWN;
}

TextureRender::TextureRender(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

TextureRender::~TextureRender()
{
	
}

bool TextureRender::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	ThrowIfFailed(CreateRootSig());
	ThrowIfFailed(BuildShader());
	ThrowIfFailed(CreatePSO());
	ThrowIfFailed(LoadRenderData());
	return true;
}

void TextureRender::OnResize()
{
	D3DApp::OnResize();
}

void TextureRender::Update(const GameTimer& gt)
{
}

void TextureRender::Draw(const GameTimer& gt)
{
	//ThrowIfFailed(mDirectCmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));
	mCommandList->SetGraphicsRootSignature(mpIRootSignature.Get());
	ID3D12DescriptorHeap* ppHeaps[] = { mSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	mCommandList->SetGraphicsRootDescriptorTable(0, mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);
	
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	stRTVHandle.ptr += mCurrBackBuffer * mRtvDescriptorSize;

	mCommandList->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);
	mCommandList->ClearRenderTargetView(stRTVHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	mCommandList->IASetVertexBuffers(0, 1, &mstVertexBufferView);

	mCommandList->DrawInstanced(_countof(stTriangleVertices), 1, 0, 0);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//�ύ����
	ThrowIfFailed(mSwapChain->Present(1, 0));
	//if (mSwapChain->Present(1, 0) != S_OK) {
	//	HRESULT hr = md3dDevice->GetDeviceRemovedReason();
	//	throw DxException(S_FALSE, L"", L"", 187);
	//}
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}

HRESULT TextureRender::CreateRootSig()
{
	//11��������������
	D3D12_FEATURE_DATA_ROOT_SIGNATURE stFeatureData = {};
	// ����Ƿ�֧��V1.1�汾�ĸ�ǩ��
	stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &stFeatureData, sizeof(stFeatureData))))
	{
		stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}
	// ��GPU��ִ��SetGraphicsRootDescriptorTable�����ǲ��޸������б��е�SRV��������ǿ���ʹ��Ĭ��Rang��Ϊ:
	// D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
	D3D12_DESCRIPTOR_RANGE1 stDSPRanges1[1] = {};
	stDSPRanges1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	stDSPRanges1[0].NumDescriptors = 1;
	stDSPRanges1[0].BaseShaderRegister = 0;
	stDSPRanges1[0].RegisterSpace = 0;
	stDSPRanges1[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
	stDSPRanges1[0].OffsetInDescriptorsFromTableStart = 0;

	D3D12_ROOT_PARAMETER1 stRootParameters1[1] = {};
	stRootParameters1[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	stRootParameters1[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	stRootParameters1[0].DescriptorTable.NumDescriptorRanges = _countof(stDSPRanges1);
	stRootParameters1[0].DescriptorTable.pDescriptorRanges = stDSPRanges1;

	D3D12_STATIC_SAMPLER_DESC stSamplerDesc[1] = {};
	stSamplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	stSamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	stSamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	stSamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	stSamplerDesc[0].MipLODBias = 0;
	stSamplerDesc[0].MaxAnisotropy = 0;
	stSamplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	stSamplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	stSamplerDesc[0].MinLOD = 0.0f;
	stSamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
	stSamplerDesc[0].ShaderRegister = 0;
	stSamplerDesc[0].RegisterSpace = 0;
	stSamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc = {};
	
	if (D3D_ROOT_SIGNATURE_VERSION_1_1 == stFeatureData.HighestVersion)
	{
		stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		stRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		stRootSignatureDesc.Desc_1_1.NumParameters = _countof(stRootParameters1);
		stRootSignatureDesc.Desc_1_1.pParameters = stRootParameters1;
		stRootSignatureDesc.Desc_1_1.NumStaticSamplers = _countof(stSamplerDesc);
		stRootSignatureDesc.Desc_1_1.pStaticSamplers = stSamplerDesc;
	}
	else
	{
		D3D12_DESCRIPTOR_RANGE stDSPRanges[1] = {};
		stDSPRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		stDSPRanges[0].NumDescriptors = 1;
		stDSPRanges[0].BaseShaderRegister = 0;
		stDSPRanges[0].RegisterSpace = 0;
		stDSPRanges[0].OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER stRootParameters[1] = {};
		stRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		stRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		stRootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(stDSPRanges);
		stRootParameters[0].DescriptorTable.pDescriptorRanges = stDSPRanges;

		stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
		stRootSignatureDesc.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		stRootSignatureDesc.Desc_1_0.NumParameters = _countof(stRootParameters);
		stRootSignatureDesc.Desc_1_0.pParameters = stRootParameters;
		stRootSignatureDesc.Desc_1_0.NumStaticSamplers = _countof(stSamplerDesc);
		stRootSignatureDesc.Desc_1_0.pStaticSamplers = stSamplerDesc;
	}

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

HRESULT TextureRender::BuildShader()
{
	// 12������Shader������Ⱦ����״̬����
#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	TCHAR pszShaderFileName[] = (L"shaders\\texturerender.hlsl");
	ThrowIfFailed(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
		, "VSMain", "vs_5_0", compileFlags, 0, &mpVertexShader, nullptr));
	ThrowIfFailed(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
		, "PSMain", "ps_5_0", compileFlags, 0, &mpPixelShader, nullptr));
	return S_OK;
}

HRESULT TextureRender::CreatePSO()
{
	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC stInputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// ���� graphics pipeline state object (PSO)����
	D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
	stPSODesc.InputLayout = { stInputElementDescs, _countof(stInputElementDescs) };
	stPSODesc.pRootSignature = mpIRootSignature.Get();
	stPSODesc.VS = { reinterpret_cast<BYTE*>(mpVertexShader->GetBufferPointer()),
		mpVertexShader->GetBufferSize() };
	stPSODesc.PS = { reinterpret_cast<BYTE*>(mpPixelShader->GetBufferPointer()),
	mpPixelShader->GetBufferSize() };
	stPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	stPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	stPSODesc.DepthStencilState.DepthEnable = FALSE;
	stPSODesc.DepthStencilState.StencilEnable = FALSE;
	stPSODesc.SampleMask = UINT_MAX;
	stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	stPSODesc.NumRenderTargets = 1;
	stPSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	stPSODesc.SampleDesc.Count = 1;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&stPSODesc
		, IID_PPV_ARGS(&mPSO)));
	return S_OK;
}

HRESULT TextureRender::LoadRenderData()
{
	// 15���������㻺��

	const UINT nVertexBufferSize = sizeof(stTriangleVertices);
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(nVertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mpIVertexBuffer)));

	UINT8* pVertexDataBegin = nullptr;
	CD3DX12_RANGE stReadRange(0, 0);		// We do not intend to read from this resource on the CPU.
	ThrowIfFailed(mpIVertexBuffer->Map(0, &stReadRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, stTriangleVertices, sizeof(stTriangleVertices));
	mpIVertexBuffer->Unmap(0, nullptr);

	mstVertexBufferView.BufferLocation = mpIVertexBuffer->GetGPUVirtualAddress();
	mstVertexBufferView.StrideInBytes = sizeof(VERTEX_TEX);
	mstVertexBufferView.SizeInBytes = nVertexBufferSize;

	//---------------------------------------------------------------------------------------------
	// 16��ʹ��WIC����������һ��2D����
	//ʹ�ô�COM��ʽ����WIC�೧����Ҳ�ǵ���WIC��һ��Ҫ��������
	ThrowIfFailed(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&mpIWICFactory)));

	//ʹ��WIC�೧����ӿڼ�������ͼƬ�����õ�һ��WIC����������ӿڣ�ͼƬ��Ϣ��������ӿڴ���Ķ�������
	WCHAR* pszTexcuteFileName = (L"textures\\boy.jpg");
	ThrowIfFailed(mpIWICFactory->CreateDecoderFromFilename(
		pszTexcuteFileName,              // �ļ���
		NULL,                            // ��ָ����������ʹ��Ĭ��
		GENERIC_READ,                    // ����Ȩ��
		WICDecodeMetadataCacheOnDemand,  // ����Ҫ�ͻ������� 
		&mpIWICDecoder                    // ����������
	));

	// ��ȡ��һ֡ͼƬ(��ΪGIF�ȸ�ʽ�ļ����ܻ��ж�֡ͼƬ�������ĸ�ʽһ��ֻ��һ֡ͼƬ)
	// ʵ�ʽ���������������λͼ��ʽ����
	ThrowIfFailed(mpIWICDecoder->GetFrame(0, &mpIWICFrame));

	WICPixelFormatGUID wpf = {};
	//��ȡWICͼƬ��ʽ
	ThrowIfFailed(mpIWICFrame->GetPixelFormat(&wpf));
	GUID tgFormat = {};

	//ͨ����һ��ת��֮���ȡDXGI�ĵȼ۸�ʽ
	if (GetTargetPixelFormat(&wpf, &tgFormat))
	{
		mstTextureFormat = GetDXGIFormatFromPixelFormat(&tgFormat);
	}

	if (DXGI_FORMAT_UNKNOWN == mstTextureFormat)
	{// ��֧�ֵ�ͼƬ��ʽ Ŀǰ�˳����� 
	 // һ�� ��ʵ�ʵ����浱�ж����ṩ�����ʽת�����ߣ�
	 // ͼƬ����Ҫ��ǰת���ã����Բ�����ֲ�֧�ֵ�����
		throw DxException(S_FALSE, L"FORMAT_UNKNOWN", L"TextureRender::LoadRenderData", 330);
	}

	// ����һ��λͼ��ʽ��ͼƬ���ݶ���ӿ�
	Microsoft::WRL::ComPtr<IWICBitmapSource>pIBMP;

	if (!InlineIsEqualGUID(wpf, tgFormat))
	{// ����жϺ���Ҫ�����ԭWIC��ʽ����ֱ����ת��ΪDXGI��ʽ��ͼƬʱ
	 // ������Ҫ���ľ���ת��ͼƬ��ʽΪ�ܹ�ֱ�Ӷ�ӦDXGI��ʽ����ʽ
		//����ͼƬ��ʽת����
		Microsoft::WRL::ComPtr<IWICFormatConverter> pIConverter;
		ThrowIfFailed(mpIWICFactory->CreateFormatConverter(&pIConverter));

		//��ʼ��һ��ͼƬת������ʵ��Ҳ���ǽ�ͼƬ���ݽ����˸�ʽת��
		ThrowIfFailed(pIConverter->Initialize(
			mpIWICFrame.Get(),                // ����ԭͼƬ����
			tgFormat,						 // ָ����ת����Ŀ���ʽ
			WICBitmapDitherTypeNone,         // ָ��λͼ�Ƿ��е�ɫ�壬�ִ��������λͼ�����õ�ɫ�壬����ΪNone
			NULL,                            // ָ����ɫ��ָ��
			0.f,                             // ָ��Alpha��ֵ
			WICBitmapPaletteTypeCustom       // ��ɫ�����ͣ�ʵ��û��ʹ�ã�����ָ��ΪCustom
		));
		// ����QueryInterface������ö����λͼ����Դ�ӿ�
		ThrowIfFailed(pIConverter.As(&pIBMP));
	}
	else
	{
		//ͼƬ���ݸ�ʽ����Ҫת����ֱ�ӻ�ȡ��λͼ����Դ�ӿ�
		ThrowIfFailed(mpIWICFrame.As(&pIBMP));
	}
	//���ͼƬ��С����λ�����أ�
	ThrowIfFailed(pIBMP->GetSize(&mnTextureW, &mnTextureH));

	//��ȡͼƬ���ص�λ��С��BPP��Bits Per Pixel����Ϣ�����Լ���ͼƬ�����ݵ���ʵ��С����λ���ֽڣ�
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

	// ���������ڿ��Եõ�BPP�ˣ���Ҳ���ҿ��ıȽ���Ѫ�ĵط���Ϊ��BPP��Ȼ������ô�໷��
	ThrowIfFailed(pIWICPixelinfo->GetBitsPerPixel(&mnBPP));

	// ����ͼƬʵ�ʵ��д�С����λ���ֽڣ�������ʹ����һ����ȡ����������A+B-1��/B ��
	// ����������˵��΢���������,ϣ�����Ѿ���������ָ��
	UINT nPicRowPitch = (uint64_t(mnTextureW) * uint64_t(mnBPP) + 7u) / 8u;
	//---------------------------------------------------------------------------------------------

	D3D12_RESOURCE_DESC stTextureDesc = {};
	stTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	stTextureDesc.MipLevels = 1;
	stTextureDesc.Format = mstTextureFormat; //DXGI_FORMAT_R8G8B8A8_UNORM;
	stTextureDesc.Width = mnTextureW;
	stTextureDesc.Height = mnTextureH;
	stTextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	stTextureDesc.DepthOrArraySize = 1;
	stTextureDesc.SampleDesc.Count = 1;
	stTextureDesc.SampleDesc.Quality = 0;


	//����Ĭ�϶��ϵ���Դ��������Texture2D��GPU��Ĭ�϶���Դ�ķ����ٶ�������
	//��Ϊ������Դһ���ǲ��ױ����Դ����������ͨ��ʹ���ϴ��Ѹ��Ƶ�Ĭ�϶���
	//�ڴ�ͳ��D3D11����ǰ��D3D�ӿ��У���Щ���̶�����װ�ˣ�����ֻ��ָ������ʱ������ΪĬ�϶� 
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)
		, D3D12_HEAP_FLAG_NONE
		, &stTextureDesc				//����ʹ��CD3DX12_RESOURCE_DESC::Tex2D���򻯽ṹ��ĳ�ʼ��
		, D3D12_RESOURCE_STATE_COPY_DEST
		, nullptr
		, IID_PPV_ARGS(&mpITexcute)));

	//��ȡ�ϴ�����Դ����Ĵ�С������ߴ�ͨ������ʵ��ͼƬ�ĳߴ�
	UINT64 n64UploadBufferSize = 0;
	D3D12_RESOURCE_DESC stDestDesc = mpITexcute->GetDesc();
	md3dDevice->GetCopyableFootprints(&stDestDesc, 0, 1, 0, nullptr, nullptr, nullptr, &n64UploadBufferSize);

	// ���������ϴ��������Դ,ע����������Buffer
	// �ϴ��Ѷ���GPU������˵�����Ǻܲ�ģ�
	// ���Զ��ڼ������������������������
	// ͨ�������ϴ���GPU���ʸ���Ч��Ĭ�϶���
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(n64UploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mpITextureUpload)));

	//������Դ�����С������ʵ��ͼƬ���ݴ洢���ڴ��С
	void* pbPicData = ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, n64UploadBufferSize);
	if (nullptr == pbPicData)
	{
		throw DxException(HRESULT_FROM_WIN32(GetLastError()), L"HeapAlloc", L"TextureRender::LoadRenderData", 430);
	}

	//��ͼƬ�ж�ȡ������
	ThrowIfFailed(pIBMP->CopyPixels(nullptr
		, nPicRowPitch
		, static_cast<UINT>(nPicRowPitch * mnTextureH)   //ע���������ͼƬ������ʵ�Ĵ�С�����ֵͨ��С�ڻ���Ĵ�С
		, reinterpret_cast<BYTE*>(pbPicData)));

	//{//������δ�������DX12��ʾ����ֱ��ͨ����仺�������һ���ڰ׷��������
	// //��ԭ��δ��룬Ȼ��ע�������CopyPixels���ÿ��Կ����ڰ׷��������Ч��
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

	//��ȡ���ϴ��ѿ����������ݵ�һЩ����ת���ߴ���Ϣ
	//���ڸ��ӵ�DDS�������Ƿǳ���Ҫ�Ĺ���
	UINT64 n64RequiredSize = 0u;
	UINT   nNumSubresources = 1u;  //����ֻ��һ��ͼƬ��������Դ����Ϊ1
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT stTxtLayouts = {};
	UINT64 n64TextureRowSizes = 0u;
	UINT   nTextureRowNum = 0u;

	stDestDesc = mpITexcute->GetDesc();

	md3dDevice->GetCopyableFootprints(&stDestDesc
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
	ThrowIfFailed(mpITextureUpload->Map(0, NULL, reinterpret_cast<void**>(&pData)));

	BYTE* pDestSlice = reinterpret_cast<BYTE*>(pData) + stTxtLayouts.Offset;
	const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(pbPicData);
	for (UINT y = 0; y < nTextureRowNum; ++y)
	{
		memcpy(pDestSlice + static_cast<SIZE_T>(stTxtLayouts.Footprint.RowPitch) * y
			, pSrcSlice + static_cast<SIZE_T>(nPicRowPitch) * y
			, nPicRowPitch);
	}
	//ȡ��ӳ�� �����ױ��������ÿ֡�ı任��������ݣ�������������Unmap�ˣ�
	//������פ�ڴ�,������������ܣ���Ϊÿ��Map��Unmap�Ǻܺ�ʱ�Ĳ���
	//��Ϊ�������붼��64λϵͳ��Ӧ���ˣ���ַ�ռ����㹻�ģ�������ռ�ò���Ӱ��ʲô
	mpITextureUpload->Unmap(0, NULL);

	//�ͷ�ͼƬ���ݣ���һ���ɾ��ĳ���Ա
	::HeapFree(::GetProcessHeap(), 0, pbPicData);

	//��������з������ϴ��Ѹ����������ݵ�Ĭ�϶ѵ�����
	CD3DX12_TEXTURE_COPY_LOCATION Dst(mpITexcute.Get(), 0);
	CD3DX12_TEXTURE_COPY_LOCATION Src(mpITextureUpload.Get(), stTxtLayouts);
	ThrowIfFailed(mDirectCmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));
	mCommandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
	//����һ����Դ���ϣ�ͬ����ȷ�ϸ��Ʋ������
		//ֱ��ʹ�ýṹ��Ȼ����õ���ʽ
	D3D12_RESOURCE_BARRIER stResBar = {};
	stResBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; 
	stResBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	stResBar.Transition.pResource = mpITexcute.Get();
	stResBar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	stResBar.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	stResBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	mCommandList->ResourceBarrier(1, &stResBar);

	//����ʹ��D3DX12���еĹ�������õĵȼ���ʽ������ķ�ʽ�����һЩ
	//pICommandList->ResourceBarrier(1
	//	, &CD3DX12_RESOURCE_BARRIER::Transition(pITexcute.Get()
	//	, D3D12_RESOURCE_STATE_COPY_DEST
	//	, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
	//);

	//---------------------------------------------------------------------------------------------
	// ���մ���SRV������
	D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
	stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	stSRVDesc.Format = stTextureDesc.Format;
	stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	stSRVDesc.Texture2D.MipLevels = 1;
	md3dDevice->CreateShaderResourceView(mpITexcute.Get(), &stSRVDesc, mSrvHeap->GetCPUDescriptorHandleForHeapStart());

	//---------------------------------------------------------------------------------------------
	// ִ�������б��ȴ�������Դ�ϴ���ɣ���һ���Ǳ����
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	return S_OK;
}

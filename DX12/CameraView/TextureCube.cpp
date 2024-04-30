#include "TextureCube.h"

struct VERTEX
{
	//������Ƕ��������ÿ������ķ��ߣ���Shader�л���ʱû����
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
	//������ת�ĽǶȣ���ת�Ƕ�(����) = ʱ��(��) * ���ٶ�(����/��)
	//�����������൱�ھ�����Ϸ��Ϣѭ���е�OnUpdate��������Ҫ��������
	mdModelRotationYAngle += ((mn64tmCurrent - mn64tmFrameStart) / 1000.0f) * fPalstance;

	mn64tmFrameStart = mn64tmCurrent;

	//��ת�Ƕ���2PI���ڵı�����ȥ����������ֻ�������0���ȿ�ʼ��С��2PI�Ļ��ȼ���
	if (mdModelRotationYAngle > DirectX::XM_2PI)
	{
		mdModelRotationYAngle = fmod(mdModelRotationYAngle, DirectX::XM_2PI);
	}

	//ģ�;��� model
	DirectX::XMMATRIX xmRot = DirectX::XMMatrixRotationY(static_cast<float>(mdModelRotationYAngle));

	//���� ģ�;��� model * �Ӿ��� view
	DirectX::XMMATRIX xmMVP = DirectX::XMMatrixMultiply(xmRot, DirectX::XMMatrixLookAtLH(Eye, At, Up));

	//ͶӰ���� projection
	xmMVP = DirectX::XMMatrixMultiply(xmMVP, (DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, (FLOAT)mClientWidth / (FLOAT)mClientHeight, 0.1f, 1000.0f)));

	DirectX::XMStoreFloat4x4(&mpMVPBuffer->m_MVP, xmMVP);
	//---------------------------------------------------------------------------------------------
	
	//---------------------------------------------------------------------------------------------
	mCommandList->SetGraphicsRootSignature(mpIRootSignature.Get());
	ID3D12DescriptorHeap* ppHeaps[] = { mSrvHeap.Get(),mpISamplerDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	
	//����SRV
	mCommandList->SetGraphicsRootDescriptorTable(0, mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	
	CD3DX12_GPU_DESCRIPTOR_HANDLE stGPUCBVHandle(mSrvHeap->GetGPUDescriptorHandleForHeapStart()
		, 1
		, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	//����CBV
	mCommandList->SetGraphicsRootDescriptorTable(1, stGPUCBVHandle);
	
	
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGPUSampler(mpISamplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
		, mnCurrentSamplerNO
		, mnSamplerDescriptorSize);
	//����Sample
	mCommandList->SetGraphicsRootDescriptorTable(2, hGPUSampler);
	
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);
	
	//---------------------------------------------------------------------------------------------
	// ͨ����Դ�����ж��󻺳��Ѿ��л���Ͽ��Կ�ʼ��Ⱦ��
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	
	//ƫ��������ָ�뵽ָ��֡������ͼλ��
	CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRtvDescriptorSize);
	//������ȾĿ��
	mCommandList->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);
	
	// ������¼�����������ʼ��һ֡����Ⱦ
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	mCommandList->ClearRenderTargetView(stRTVHandle, clearColor, 0, nullptr);
	
	//ע������ʹ�õ���Ⱦ�ַ����������б�Ҳ����ͨ����Mesh����
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->IASetVertexBuffers(0, 1, &mstVertexBufferView);
	mCommandList->IASetIndexBuffer(&mstIndexBufferView);
	
	//---------------------------------------------------------------------------------------------
	//Draw Call������
	mCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
	
	//---------------------------------------------------------------------------------------------
	//��һ����Դ���ϣ�����ȷ����Ⱦ�Ѿ����������ύ����ȥ��ʾ��
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//�ύ����
	ThrowIfFailed(mSwapChain->Present(1, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}

HRESULT TextureCube::CreateRootSig()
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE stFeatureData = {};
	// ����Ƿ�֧��V1.1�汾�ĸ�ǩ��
	stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &stFeatureData, sizeof(stFeatureData))))
	{
		stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
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
	stRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	//SRV��PS�ɼ�
	stRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	stRootParameters[0].DescriptorTable.pDescriptorRanges = &stDSPRanges[0];

	stRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	stRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;		//CBV������Shader�ɼ�
	stRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	stRootParameters[1].DescriptorTable.pDescriptorRanges = &stDSPRanges[1];

	stRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	stRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	//SAMPLE��PS�ɼ�
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

	// ���� graphics pipeline state object (PSO)����
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

	//����CBV������
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = mpICBVUpload->GetGPUVirtualAddress();
	SIZE_T szMVPBuffer = TEX_UPPER_DIV(sizeof(ST_FRAME_MVP_BUFFER), 256) * 256;
	cbvDesc.SizeInBytes = static_cast<UINT>(szMVPBuffer);

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(mSrvHeap->GetCPUDescriptorHandleForHeapStart()
		, 1
		, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	md3dDevice->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);

	//�������ֲ�����
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

	D3D12_HEAP_DESC stTextureHeapDesc = {};
	//Ϊ��ָ������ͼƬ����2����С�Ŀռ䣬����û����ϸȥ�����ˣ�ֻ��ָ����һ���㹻��Ŀռ䣬�����������
	//ʵ��Ӧ����Ҳ��Ҫ�ۺϿ��Ƿ���ѵĴ�С���Ա�������ö�
	stTextureHeapDesc.SizeInBytes = TEX_UPPER(2 * nPicRowPitch * mnTextureH, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	//ָ���ѵĶ��뷽ʽ������ʹ����Ĭ�ϵ�64K�߽���룬��Ϊ������ʱ����ҪMSAA֧��
	stTextureHeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	stTextureHeapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;		//Ĭ�϶�����
	stTextureHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	stTextureHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	//�ܾ���ȾĿ�������ܾ������������ʵ�ʾ�ֻ�������ڷ���ͨ����
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
	//ʹ�á���λ��ʽ������������ע��������������ڲ�ʵ���Ѿ�û�д洢������ͷŵ�ʵ�ʲ����ˣ��������ܸܺ�
	//ͬʱ������������Ϸ�������CreatePlacedResource��������ͬ��������Ȼǰ�������ǲ��ڱ�ʹ�õ�ʱ�򣬲ſ���
	//���ö�
	ThrowIfFailed(md3dDevice->CreatePlacedResource(
		mpITextureHeap.Get()
		, 0
		, &mstTextureDesc				//����ʹ��CD3DX12_RESOURCE_DESC::Tex2D���򻯽ṹ��ĳ�ʼ��
		, D3D12_RESOURCE_STATE_COPY_DEST
		, nullptr
		, IID_PPV_ARGS(&mpITexture)));
	//-----------------------------------------------------------------------------------------------------------
	
	//��ȡ�ϴ�����Դ����Ĵ�С������ߴ�ͨ������ʵ��ͼƬ�ĳߴ�
	mn64UploadBufferSize = GetRequiredIntermediateSize(mpITexture.Get(), 0, 1);

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

	ThrowIfFailed(md3dDevice->CreateHeap(&stUploadHeapDesc, IID_PPV_ARGS(&mpIUploadHeap)));
	ThrowIfFailed(md3dDevice->CreatePlacedResource(mpIUploadHeap.Get()
		, 0
		, &CD3DX12_RESOURCE_DESC::Buffer(mn64UploadBufferSize)
		, D3D12_RESOURCE_STATE_GENERIC_READ
		, nullptr
		, IID_PPV_ARGS(&mpITextureUpload)));

	//������Դ�����С������ʵ��ͼƬ���ݴ洢���ڴ��С
	void* pbPicData = ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, mn64UploadBufferSize);
	if (nullptr == pbPicData)
	{
		throw DxException(HRESULT_FROM_WIN32(GetLastError()), L"",L"",366);
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

	UINT   nNumSubresources = 1u;  //����ֻ��һ��ͼƬ��������Դ����Ϊ1
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

	CD3DX12_TEXTURE_COPY_LOCATION Dst(mpITexture.Get(), 0);
	CD3DX12_TEXTURE_COPY_LOCATION Src(mpITextureUpload.Get(), stTxtLayouts);
	ThrowIfFailed(mDirectCmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));
	mCommandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);

	//����һ����Դ���ϣ�ͬ����ȷ�ϸ��Ʋ������
	//ֱ��ʹ�ýṹ��Ȼ����õ���ʽ
	D3D12_RESOURCE_BARRIER stResBar = {};
	stResBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	stResBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	stResBar.Transition.pResource = mpITexture.Get();
	stResBar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	stResBar.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	stResBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	mCommandList->ResourceBarrier(1, &stResBar);

	//����ʹ��D3DX12���еĹ�������õĵȼ���ʽ������ķ�ʽ�����һЩ
	//pICommandList->ResourceBarrier(1
	//	, &CD3DX12_RESOURCE_BARRIER::Transition(pITexture.Get()
	//	, D3D12_RESOURCE_STATE_COPY_DEST
	//	, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
	//);

	//---------------------------------------------------------------------------------------------
	// ִ�������б��ȴ�������Դ�ϴ���ɣ���һ���Ǳ����
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
	//ʹ�ö�λ��ʽ����ͬ���ϴ������ԡ���λ��ʽ���������㻺�壬ע��ڶ�������ָ���˶��е�ƫ��λ��
			//���նѱ߽�����Ҫ������������ƫ��λ�ö��뵽��64k�ı߽���
	ThrowIfFailed(md3dDevice->CreatePlacedResource(
		mpIUploadHeap.Get()
		, n64BufferOffset
		, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize)
		, D3D12_RESOURCE_STATE_GENERIC_READ
		, nullptr
		, IID_PPV_ARGS(&mpIVertexBuffer)));

	//ʹ��map-memcpy-unmap�󷨽����ݴ������㻺�����
	//ע�ⶥ�㻺��ʹ���Ǻ��ϴ��������ݻ�����ͬ��һ���ѣ��������
	UINT8* pVertexDataBegin = nullptr;
	CD3DX12_RANGE stReadRange(0, 0);		// We do not intend to read from this resource on the CPU.

	ThrowIfFailed(mpIVertexBuffer->Map(0, &stReadRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
	mpIVertexBuffer->Unmap(0, nullptr);

	//������Դ��ͼ��ʵ�ʿ��Լ����Ϊָ�򶥵㻺����Դ�ָ��
	mstVertexBufferView.BufferLocation = mpIVertexBuffer->GetGPUVirtualAddress();
	mstVertexBufferView.StrideInBytes = sizeof(VERTEX);
	mstVertexBufferView.SizeInBytes = vertexBufferSize;

	//����߽�������ȷ��ƫ��λ��
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

	//�����ϴ������ԡ���λ��ʽ��������������
	n64BufferOffset = TEX_UPPER(n64BufferOffset + indexBufferSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	SIZE_T szMVPBuffer = TEX_UPPER_DIV(sizeof(ST_FRAME_MVP_BUFFER), 256) * 256;
	// ������������ ע�⻺��ߴ�����Ϊ256�߽�����С
	ThrowIfFailed(md3dDevice->CreatePlacedResource(
		mpIUploadHeap.Get()
		, n64BufferOffset
		, &CD3DX12_RESOURCE_DESC::Buffer(szMVPBuffer)
		, D3D12_RESOURCE_STATE_GENERIC_READ
		, nullptr
		, IID_PPV_ARGS(&mpICBVUpload)));

	// Map ֮��Ͳ���Unmap�� ֱ�Ӹ������ݽ�ȥ ����ÿ֡������map-copy-unmap�˷�ʱ����
	ThrowIfFailed(mpICBVUpload->Map(0, nullptr, reinterpret_cast<void**>(&mpMVPBuffer)));
	return S_OK;
}

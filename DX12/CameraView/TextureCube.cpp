#include "TextureCube.h"

struct VERTEX
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 color;
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
	ThrowIfFailed(mDirectCmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));
	mCommandList->SetGraphicsRootSignature(mpIRootSignature.Get());
	//mCommandList->SetPipelineState(mPSO.Get());
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	stRTVHandle.ptr += mCurrBackBuffer * mRtvDescriptorSize;

	mCommandList->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);
	mCommandList->ClearRenderTargetView(stRTVHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->IASetVertexBuffers(0, 1, &mstVertexBufferView);
	mCommandList->IASetIndexBuffer(&mstIndexBufferView);
	mCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
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
	mRootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	Microsoft::WRL::ComPtr<ID3DBlob> signature;
	Microsoft::WRL::ComPtr<ID3DBlob> error;

	ThrowIfFailed(D3D12SerializeRootSignature(&mRootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature, &error));

	ThrowIfFailed(md3dDevice->CreateRootSignature(0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&mpIRootSignature)));

	return S_OK;
}

HRESULT TextureCube::BuildShader()
{
#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	TCHAR pszShaderFileName[] = L"shaders\\shader1.hlsl";
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
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs,_countof(inputElementDescs) };
	psoDesc.pRootSignature = mpIRootSignature.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(mpVertexShader->GetBufferPointer()),
	mpVertexShader->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(mpPixelShader->GetBufferPointer()),
	mpPixelShader->GetBufferSize() };

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1; // ͬʱ���õ���ȾĿ������(�� RTVFormats ��������ȾĿ���ʽ������)
	psoDesc.RTVFormats[0] = mBackBufferFormat; // ��ȾĿ��ĸ�ʽ
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
	return S_OK;
}

HRESULT TextureCube::LoadRenderData()
{
	VERTEX triangleVertices[] =
	{
		{ { -0.25f * mfAspectRatio, 0.25f * mfAspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { 0.25f * mfAspectRatio, 0.25f * mfAspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.25f * mfAspectRatio, -0.25f * mfAspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.25f * mfAspectRatio, -0.25f * mfAspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
	};

	const UINT vertexBufferSize = sizeof(triangleVertices);

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mpIVertexBuffer)));

	UINT8* pVertexDataBegin = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(mpIVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
	mpIVertexBuffer->Unmap(0, nullptr);

	mstVertexBufferView.BufferLocation = mpIVertexBuffer->GetGPUVirtualAddress();
	mstVertexBufferView.StrideInBytes = sizeof(VERTEX);
	mstVertexBufferView.SizeInBytes = vertexBufferSize;

	std::uint16_t indices[] = {
		0,1,2,
		0,2,3
	};
	const UINT indexBufferSize = sizeof(indices);

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mpIIndexBuffer)));

	UINT8* pIndexDataBegin = nullptr;
	CD3DX12_RANGE readIndexRange(0, 0);
	ThrowIfFailed(mpIIndexBuffer->Map(0, &readIndexRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, indices, sizeof(indices));
	mpIIndexBuffer->Unmap(0, nullptr);

	mstIndexBufferView.BufferLocation = mpIIndexBuffer->GetGPUVirtualAddress();
	mstIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	mstIndexBufferView.SizeInBytes = indexBufferSize;
	return S_OK;
}

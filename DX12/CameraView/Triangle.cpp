#include "Triangle.h"

struct VERTEX
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 color;
};

Triangle::Triangle(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

Triangle::~Triangle()
{
}

bool Triangle::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	ThrowIfFailed(CreateRootSig());
	ThrowIfFailed(BuildShader());
	ThrowIfFailed(CreatePSO());
	ThrowIfFailed(LoadRenderData());
	return true;
}

void Triangle::OnResize()
{
	D3DApp::OnResize();
}

void Triangle::Update(const GameTimer& gt)
{
}

void Triangle::Draw(const GameTimer& gt)
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
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	mCommandList->IASetVertexBuffers(0, 1, &mstVertexBufferView);
	mCommandList->DrawInstanced(3, 1, 0, 0);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//提交画面
	ThrowIfFailed(mSwapChain->Present(1, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}

HRESULT Triangle::CreateRootSig()
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

HRESULT Triangle::BuildShader()
{

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	TCHAR pszShaderFileName[] = L"D:\\TestProjects\\Win-DX12-ML-in-practice\\DX12\\CameraView\\shaders\\shader1.hlsl";
	ThrowIfFailed(D3DCompileFromFile(pszShaderFileName,
		nullptr, nullptr, "VSMain", "vs_5_0",
		compileFlags, 0, &mpVertexShader, nullptr));

	ThrowIfFailed(D3DCompileFromFile(pszShaderFileName,
		nullptr, nullptr, "PSMain", "ps_5_0",
		compileFlags, 0, &mpPixelShader, nullptr));
	return S_OK;
}

HRESULT Triangle::CreatePSO()
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
	psoDesc.NumRenderTargets = 1; // 同时所用的渲染目标数量(即 RTVFormats 数组中渲染目标格式的数量)
	psoDesc.RTVFormats[0] = mBackBufferFormat; // 渲染目标的格式
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
	return S_OK;
}

HRESULT Triangle::LoadRenderData()
{
	VERTEX triangleVertices[] =
	{
		{ { 0.0f, 0.25f * mfAspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
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
	return S_OK;
}

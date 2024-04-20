#pragma once
#include "../Common/d3dApp.h"

class Triangle :
    public D3DApp
{
public:
    Triangle(HINSTANCE hInstance);
    ~Triangle();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	HRESULT CreateRootSig();
	HRESULT BuildShader();
	HRESULT CreatePSO();
	HRESULT LoadRenderData();
private:
	CD3DX12_ROOT_SIGNATURE_DESC							mRootSignatureDesc;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>         mpIRootSignature;
	Microsoft::WRL::ComPtr<ID3DBlob>					mpVertexShader;
	Microsoft::WRL::ComPtr<ID3DBlob>					mpPixelShader;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>			mPSO = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource>              mpIVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW							mstVertexBufferView = {};
	float												mfAspectRatio = 3.0f;
};


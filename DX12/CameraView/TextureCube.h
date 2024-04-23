#pragma once
#include "../Common/d3dApp.h"
class TextureCube : public D3DApp {
public:
	TextureCube(HINSTANCE hInstance);
	~TextureCube();

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
	Microsoft::WRL::ComPtr<ID3D12Resource>              mpIIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW								mstIndexBufferView = {};
	float												mfAspectRatio = 3.0f;

};


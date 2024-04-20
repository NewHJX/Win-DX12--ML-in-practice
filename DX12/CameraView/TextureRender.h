#pragma once
#include "../Common/d3dApp.h"
#include <wincodec.h>

struct VERTEX_TEX
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 txc;			//Texcoord
};

class TextureRender :
    public D3DApp
{
public:
	TextureRender(HINSTANCE hInstance);
	~TextureRender();

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
	Microsoft::WRL::ComPtr<IWICImagingFactory>			mpIWICFactory;
	Microsoft::WRL::ComPtr<IWICBitmapDecoder>			mpIWICDecoder;
	Microsoft::WRL::ComPtr<IWICBitmapFrameDecode>		mpIWICFrame;
	UINT												mnTextureW = 0u;
	UINT												mnTextureH = 0u;
	UINT												mnBPP = 0u;
	Microsoft::WRL::ComPtr<ID3D12Resource>				mpITexcute;
	DXGI_FORMAT											mstTextureFormat = DXGI_FORMAT_UNKNOWN;
	float												mfAspectRatio = 3.0f;

	// 定义正方形的3D数据结构
	VERTEX_TEX stTriangleVertices[4] =
	{
		{ { -0.25f * mfAspectRatio, -0.25f * mfAspectRatio, 0.0f}, { 0.0f, 1.0f } },	// Bottom left.
		{ { -0.25f * mfAspectRatio, 0.25f * mfAspectRatio, 0.0f}, { 0.0f, 0.0f } },	// Top left.
		{ { 0.25f * mfAspectRatio, -0.25f * mfAspectRatio, 0.0f }, { 1.0f, 1.0f } },	// Bottom right.
		{ { 0.25f * mfAspectRatio, 0.25f * mfAspectRatio, 0.0f}, { 1.0f, 0.0f } },		// Top right.
	};
};


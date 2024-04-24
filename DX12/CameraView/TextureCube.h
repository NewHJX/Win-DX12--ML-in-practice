#pragma once
#include "../Common/d3dApp.h"
#include <wincodec.h>

struct ST_FRAME_MVP_BUFFER
{
	DirectX::XMFLOAT4X4 m_MVP;			//经典的Model-view-projection(MVP)矩阵.
};

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
	HRESULT CreateOthers();
	HRESULT LoadRenderData();
private:
	Microsoft::WRL::ComPtr<IWICBitmapFrameDecode>		mpIWICFrame;
	Microsoft::WRL::ComPtr<IWICBitmapDecoder>			mpIWICDecoder;
	Microsoft::WRL::ComPtr<IWICImagingFactory>			mpIWICFactory;

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
	double												mdModelRotationYAngle = 0.0f;

	Microsoft::WRL::ComPtr<ID3D12Heap>					mpITextureHeap;
	Microsoft::WRL::ComPtr<ID3D12Heap>					mpIUploadHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource>				mpITexture;
	Microsoft::WRL::ComPtr<ID3D12Resource>				mpITextureUpload;
	Microsoft::WRL::ComPtr<ID3D12Resource>			    mpICBVUpload;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		mpISamplerDescriptorHeap;
	D3D12_RESOURCE_DESC									mstTextureDesc = {};
	UINT												mnSampleMaxCnt = 5;		//创建五个典型的采样器
	UINT												mnSamplerDescriptorSize = 0;
	UINT												mnTextureW = 0u;
	UINT												mnTextureH = 0u;
	UINT												mnBPP = 0u;
	UINT												mnCurrentSamplerNO = 0;
	UINT64												mn64UploadBufferSize = 0;
	DXGI_FORMAT											mstTextureFormat = DXGI_FORMAT_UNKNOWN;

	ST_FRAME_MVP_BUFFER*								mpMVPBuffer = nullptr;

	ULONGLONG											mn64tmFrameStart = 0;
	ULONGLONG											mn64tmCurrent = 0;
	//初始的默认摄像机的位置
	DirectX::XMVECTOR Eye = DirectX::XMVectorSet(0.0f, 0.0f, -10.0f, 0.0f); //眼睛位置
	DirectX::XMVECTOR At = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);    //眼睛所盯的位置
	DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);    //头部正上方位置

	double fPalstance = 10.0f * DirectX::XM_PI / 180.0f;	//物体旋转的角速度，单位：弧度/秒
};
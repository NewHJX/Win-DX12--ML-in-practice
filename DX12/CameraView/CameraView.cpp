//***************************************************************************************
// Init Direct3D.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Demonstrates the sample framework by initializing Direct3D, clearing 
// the screen, and displaying frame stats.
//
//***************************************************************************************

#include "../Common/d3dApp.h"
#include <DirectXColors.h>
#include "CameraView.h"

using namespace DirectX;

class InitDirect3DApp : public D3DApp
{
public:
	InitDirect3DApp(HINSTANCE hInstance);
	~InitDirect3DApp();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		InitDirect3DApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

InitDirect3DApp::~InitDirect3DApp()
{
}

bool InitDirect3DApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	return true;
}

void InitDirect3DApp::OnResize()
{
	D3DApp::OnResize();
}

void InitDirect3DApp::Update(const GameTimer& gt)
{

}

void InitDirect3DApp::Draw(const GameTimer& gt)
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}

struct PropertiesData {
	std::vector<VideoDevice> devices;
	std::vector<AudioDevice> audioDevices;

	bool GetDevice(VideoDevice& device, const char* encoded_id) const
	{
		DeviceId deviceId;
		DecodeDeviceId(deviceId, encoded_id);

		for (const VideoDevice& curDevice : devices) {
			if (deviceId.name == curDevice.name &&
				deviceId.path == curDevice.path) {
				device = curDevice;
				return true;
			}
		}

		return false;
	}
};

bool CCameraDevice::StartCamera()
{
	if (!mDevice.ResetGraph()) {
		OutputDebugString(L"reset graph failed");
		return false;
	}

	SVideoConfig videoConfig;
	//Integrated Camera:\\?\usb#22vid_13d3&pid_5271&mi_00#226&9b6b8ac&0&0000#22{65e8773d-8f56-11d0-a3b9-00a0c9223196}\global
	SAudioConfig audioConfig;
	if (!UpdateVideoConfig(videoConfig)) {
		OutputDebugString(L"update video config failed");
		return false;
	}
	if (!UpdateAudioConfig(audioConfig)) {
		OutputDebugString(L"update audio config failed");
		return false;
	}
	if (!mDevice.ConnectFilters()) {
		OutputDebugString(L"connect filters failed");
		return false;
	}
	if (mDevice.Start() != Result::Success) {
		OutputDebugString(L"start camera failed");
		return false;
	}


}

bool CCameraDevice::UpdateVideoConfig(const SVideoConfig& videoConfig)
{
	DeviceId id;
	if (!DecodeDeviceId(id, videoConfig.strVideoDeviceId.c_str())) {
		OutputDebugString(L"decode device id failed");
		return false;
	}

	PropertiesData data;
	Device::EnumVideoDevices(data.devices);
	VideoDevice dev;
	if (!data.GetDevice(dev, videoConfig.strVideoDeviceId.c_str())) {
		OutputDebugString(L"get device failed");
		return false;
	}
	int cx = 0, cy = 0;
	long long interval = 0;
	VideoFormat format = VideoFormat::Any;

	mVideoConfig.name = id.name;
	mVideoConfig.path = id.path;
	mVideoConfig.useDefaultConfig = videoConfig.nResType == ResType_Preferred;
	mVideoConfig.cx = cx;
	mVideoConfig.cy_abs = abs(cy);
	mVideoConfig.cy_flip = cy < 0;
	mVideoConfig.frameInterval = interval;
	mVideoConfig.internalFormat = format;

	deviceHasAudio = dev.audioAttached;
	deviceHasSeparateAudioFilter = dev.separateAudioFilter;

	mVideoConfig.callback = std::bind(&CCameraDevice::OnVideoData, this,
		std::placeholders::_1, std::placeholders::_2,
		std::placeholders::_3, std::placeholders::_4,
		std::placeholders::_5, std::placeholders::_6);
	mVideoConfig.reactivateCallback =
		std::bind(&CCameraDevice::OnReactivate, this);

	mVideoConfig.format = mVideoConfig.internalFormat;
	if (!mDevice.SetVideoConfig(&mVideoConfig)) {
		OutputDebugString(L"set video config failed");
		return false;
	}
	return true;
}

bool CCameraDevice::UpdateAudioConfig(const SAudioConfig& audioConfig)
{
	return false;
}
void CCameraDevice::OnReactivate()
{

}

void CCameraDevice::OnVideoData(const VideoConfig& config, unsigned char* data,
	size_t size, long long startTime,
	long long endTime, long rotation) 
{

}

void CCameraDevice::OnAudioData(const AudioConfig& config, unsigned char* data,
	size_t size, long long startTime, long long endTime) 
{

}

bool DecodeDeviceId(DeviceId id, const char* cVideoDeviceId)
{
	if (cVideoDeviceId == nullptr) {
		return false;
	}
	

	return true;
}

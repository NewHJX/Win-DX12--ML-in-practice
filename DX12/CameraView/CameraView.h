#pragma once
#include "dshowcapture.hpp"
#include "../Common/d3dApp.h"
using namespace DShow;

enum ResType {
	ResType_Preferred,
	ResType_Custom,
};

struct SVideoConfig {
	std::string strVideoDeviceId;
	bool bFlip;
	bool bAutoRotation;
	bool bHWDecode;
	int nResType;
};

struct SAudioConfig {

};

bool DecodeDeviceId(DeviceId id, const char* cVideoDeviceId);

class InitDirect3DApp : public D3DApp
{
public:
	InitDirect3DApp(HINSTANCE hInstance);
	~InitDirect3DApp();

	virtual bool Initialize()override;
	void DrawCamera(const VideoConfig& config, unsigned char* data,
		size_t size, long long startTime, long long endTime,
		long rotation);

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

};

class CCameraDevice {
public:
	bool StartCamera();
	bool UpdateVideoConfig(const SVideoConfig& videoConfig);
	bool UpdateAudioConfig(const SAudioConfig& audioConfig);

	void OnReactivate();
	void OnVideoData(const VideoConfig& config, unsigned char* data,
		size_t size, long long startTime, long long endTime,
		long rotation);
	void OnAudioData(const AudioConfig& config, unsigned char* data,
		size_t size, long long startTime, long long endTime);
	void setD3DApp(InitDirect3DApp* d3dApp);
private:
	
private:
	InitDirect3DApp	*mD3DApp = nullptr;
	bool deviceHasAudio = false;
	bool deviceHasSeparateAudioFilter = false;
	Device	mDevice;
	VideoConfig mVideoConfig;
	AudioConfig mAudioConfig;
};

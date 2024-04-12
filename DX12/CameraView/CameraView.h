#pragma once
#include "dshowcapture.hpp"
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

private:
	
private:
	bool deviceHasAudio = false;
	bool deviceHasSeparateAudioFilter = false;
	Device	mDevice;
	VideoConfig mVideoConfig;
	AudioConfig mAudioConfig;
};

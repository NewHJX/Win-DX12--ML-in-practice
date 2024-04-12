#pragma once

#define WIN32_MEAN_AND_LEAN
#define __STREAMS__
#include <windows.h>
#include <dshow.h>
#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include <Amaudio.h>
#include <Dvdmedia.h>

#include "ComPtr.hpp"
#include "CoTaskMemPtr.hpp"

#include <string>
using namespace std;

#define DSHOW_UNUSED(param) (void)param;

namespace DShow {

	bool CreateFilterGraph(IGraphBuilder** graph, ICaptureGraphBuilder2** builder,
		IMediaControl** control);

	void LogFilters(IGraphBuilder* graph);

	bool GetDeviceFilter(const IID& type, const wchar_t* name, const wchar_t* path,
		IBaseFilter** filter);

	bool GetFilterPin(IBaseFilter* filter, const GUID& type, const GUID& category,
		PIN_DIRECTION dir, IPin** pin);

	bool GetPinByName(IBaseFilter* filter, PIN_DIRECTION dir, const wchar_t* name,
		IPin** pin);

	bool GetPinByMedium(IBaseFilter* filter, REGPINMEDIUM& medium, IPin** pin);
	bool GetFilterByMedium(const CLSID& id, REGPINMEDIUM& medium,
		IBaseFilter** filter);

	bool GetPinMedium(IPin* pin, REGPINMEDIUM& medium);

	bool DirectConnectFilters(IFilterGraph* graph, IBaseFilter* filterOut,
		IBaseFilter* filterIn);

	/**
	 * This maps a created demuxer pin to a packet ID for the mux stream.  Note
	 * that this needs to be called after the device filters are connected to the
	 * demux filter.
	 */
	HRESULT MapPinToPacketID(IPin* pin, ULONG packetID);

	wstring ConvertHRToEnglish(HRESULT hr);

	/**
	 * Get audio filter for the same device as the given video device path
	 */
	bool GetDeviceAudioFilter(const wchar_t* videoDevicePath,
		IBaseFilter** audioCaptureFilter);

}; /* namespace DShow */
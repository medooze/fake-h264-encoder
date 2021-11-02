%module medooze_fake_h264_encoder
%{
#include <nan.h>
#include "FakeH264VideoEncoderWorker.h"
#include "MediaFrameListenerBridge.h"

using MediaFrameListener =  MediaFrame::Listener;

class MedoozeFakeH264EncoderModule
{
public:
	static void EnableLog(bool flag)
	{
		//Enable log
		Logger::EnableLog(flag);
	}
	
	static void EnableDebug(bool flag)
	{
		//Enable debug
		Logger::EnableDebug(flag);
	}
	
	static void EnableUltraDebug(bool flag)
	{
		//Enable debug
		Logger::EnableUltraDebug(flag);
	}
	
};

class FakeH264VideoEncoderWorkerFacade:
	public FakeH264VideoEncoderWorker,
	public RTPReceiver
{
public:
	virtual int SendPLI(DWORD ssrc)
	{
		FakeH264VideoEncoderWorker::SendFPU();
		return 1;
	}
	virtual int Reset(DWORD ssrc)
	{
		return 1;
	}
};

%}

%nodefaultctor MediaFrameListener;
%nodefaultdtor MediaFrameListener;
struct MediaFrameListener {};

%nodefaultctor RTPIncomingMediaStream;
%nodefaultdtor RTPIncomingMediaStream;
struct RTPIncomingMediaStream {};

struct MediaFrameListenerBridge : 
	public MediaFrameListener,
	public RTPIncomingMediaStream
{
	MediaFrameListenerBridge(int ssrc, bool smooth);
};

%nodefaultctor RTPReceiver;
%nodefaultdtor RTPReceiver;
struct RTPReceiver {};

struct FakeH264VideoEncoderWorkerFacade : public RTPReceiver
{
	FakeH264VideoEncoderWorkerFacade();
	bool AddListener(MediaFrameListener *listener);
	bool RemoveListener(MediaFrameListener *listener);
	int SetBitrate(int fps, int bitrate);
	int Init();
	int Start();
	int Stop();
	int End();
	int IsEncoding();
};

class MedoozeFakeH264EncoderModule
{
public:
	static void EnableLog(bool flag);
	static void EnableDebug(bool flag);
	static void EnableUltraDebug(bool flag);
};
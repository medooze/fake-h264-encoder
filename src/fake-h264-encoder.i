%module medooze_fake_h264_encoder
%{
#include <nan.h>
#include "FakeH264VideoEncoderWorker.h"
#include "MediaFrameListenerBridge.h"

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

%{
using MediaFrameListener = MediaFrame::Listener;
%}
%nodefaultctor MediaFrameListener;
%nodefaultdtor MediaFrameListener;
struct MediaFrameListener
{
};

%{
using RTPIncomingMediaStreamListener = RTPIncomingMediaStream::Listener;
%}
%nodefaultctor RTPIncomingMediaStreamListener;
%nodefaultdtor RTPIncomingMediaStreamListener;
struct RTPIncomingMediaStreamListener
{
};

%nodefaultctor RTPIncomingMediaStream;
%nodefaultdtor RTPIncomingMediaStream;
struct RTPIncomingMediaStream 
{
	DWORD GetMediaSSRC();
	TimeService& GetTimeService();

	void AddListener(RTPIncomingMediaStreamListener* listener);
	void RemoveListener(RTPIncomingMediaStreamListener* listener);
	void Mute(bool muting);
};

%nodefaultctor MediaFrameListenerBridge;
struct MediaFrameListenerBridge : 
	public RTPIncomingMediaStream,
	public MediaFrameListener
{
	MediaFrameListenerBridge(TimeService& timeService, int ssrc);

	DWORD numFrames;
	DWORD numPackets;
	DWORD numFramesDelta;
	DWORD numPacketsDelta;
	DWORD totalBytes;
	DWORD bitrate;
	DWORD minWaitedTime;
	DWORD maxWaitedTime;
	DWORD avgWaitedTime;
	void Update();
	
	void AddMediaListener(MediaFrameListener* listener);
	void RemoveMediaListener(MediaFrameListener* listener);
	void Stop();
};
//SWIG only supports single class inheritance
MediaFrameListener* MediaFrameListenerBridgeToMediaFrameListener(MediaFrameListenerBridge* bridge);
%{
MediaFrameListener* MediaFrameListenerBridgeToMediaFrameListener(MediaFrameListenerBridge* bridge)
{
	return (MediaFrameListener*)bridge;
}
%}

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
	bool SetThreadName(const std::string& name);
	bool SetPriority(int priority);
};

class MedoozeFakeH264EncoderModule
{
public:
	static void EnableLog(bool flag);
	static void EnableDebug(bool flag);
	static void EnableUltraDebug(bool flag);
};
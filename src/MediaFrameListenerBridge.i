%{
#include "MediaFrameListenerBridge.h"
%}

%include "EventLoop.i"
%include "RTPIncomingMediaStream.i"
%include "RTPReceiver.i"
%include "MediaFrame.i"

%nodefaultctor MediaFrameListenerBridge;
struct MediaFrameListenerBridge : 
	public RTPIncomingMediaStream,
	public RTPReceiver,
	public MediaFrameListener
{
	MediaFrameListenerBridge(TimeService& timeService, int ssrc, bool smooth = false);

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
	
	void AddMediaListener(const MediaFrameListenerShared& listener);
	void RemoveMediaListener(const MediaFrameListenerShared& listener);
	void Stop();
};

SHARED_PTR_BEGIN(MediaFrameListenerBridge)
{
	MediaFrameListenerBridgeShared(TimeService& timeService, int ssrc, bool smooth = false)
	{
		return new std::shared_ptr<MediaFrameListenerBridge>(new MediaFrameListenerBridge(timeService, ssrc, smooth));
	}
	SHARED_PTR_TO(RTPIncomingMediaStream)
	SHARED_PTR_TO(RTPReceiver)
	SHARED_PTR_TO(MediaFrameListener)
	
}
SHARED_PTR_END(MediaFrameListenerBridge)
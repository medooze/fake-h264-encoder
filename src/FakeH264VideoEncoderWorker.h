#ifndef FAKEH264VIDEOENCODERWORKER_H
#define	FAKEH264VIDEOENCODERWORKER_H

#include <set>
#include <vector>
#include "config.h"
#include "video.h"
#include "acumulator.h"
#include "EventLoop.h"

class FakeH264VideoEncoderWorker 
{
public:
	FakeH264VideoEncoderWorker();
	virtual ~FakeH264VideoEncoderWorker();

	int Init();
	int SetBitrate(int fps,int bitrate);
	int End();

	bool AddListener(MediaFrame::Listener *listener);
	bool RemoveListener(MediaFrame::Listener *listener);
	void SendFPU();

	bool SetThreadName(const std::string& name);
	bool SetPriority(int priority);

	TimeService& GetTimeService() { return loop;	}
	
	bool IsEncoding() { return encoding;	}
	
	int Start();
	int Stop();
	
protected:
	void Encode(std::chrono::milliseconds now);
private:
	EventLoop loop;
	Timer::shared encodingTimer;
	std::set<MediaFrame::Listener*> listeners;
	std::vector<std::unique_ptr<VideoFrame>> frames;
	Buffer sps;
	Buffer pps;

	uint32_t frameIndex	= 0;
	int fps			= 0;
	DWORD bitrate		= 0;
	DWORD num		= 0;

	Acumulator<uint32_t> bitrateAcu;
	Acumulator<uint32_t> fpsAcu;

	std::chrono::milliseconds first		= 0ms;
	std::chrono::milliseconds lastFPU	= 0ms;
	bool	encoding	 = false;
	bool	sendFPU		 = false;
	
};


#endif	/* FAKEH264VIDEOENCODERWORKER_H */


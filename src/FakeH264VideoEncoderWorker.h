#ifndef FAKEH264VIDEOENCODERWORKER_H
#define	FAKEH264VIDEOENCODERWORKER_H

#include <pthread.h>
#include <set>
#include <vector>
#include "config.h"
#include "video.h"

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
	
	bool IsEncoding() { return encoding;	}
	
	int Start();
	int Stop();
	
protected:
	int Encode();

private:
	static void *startEncoding(void *par);


	
private:
	std::set<MediaFrame::Listener*> listeners;
	std::vector<std::unique_ptr<VideoFrame>> frames;
	Buffer sps;
	Buffer pps;

	uint32_t frameIndex	= 0;
	int fps			= 0;
	DWORD bitrate		= 0;

	pthread_t	thread;
	pthread_mutex_t mutex;
	pthread_cond_t	cond;
	bool	encoding	 = false;
	bool	sendFPU		 = false;
	
};


#endif	/* FAKEH264VIDEOENCODERWORKER_H */


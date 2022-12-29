%include "shared_ptr.i"
%include "MediaFrame.i"
%include "RTPReceiver.i"

%{
#include "rtp.h"
#include "FakeH264VideoEncoderWorker.h"

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

struct FakeH264VideoEncoderWorkerFacade : public RTPReceiver
{
	FakeH264VideoEncoderWorkerFacade();
	bool AddListener(const MediaFrameListenerShared& listener);
	bool RemoveListener(const MediaFrameListenerShared& listener);
	int SetBitrate(int fps, int bitrate);
	int Init();
	int Start();
	int Stop();
	int End();
	int IsEncoding();
	bool SetThreadName(const std::string& name);
	bool SetPriority(int priority);
	TimeService& GetTimeService();
};


SHARED_PTR_BEGIN(FakeH264VideoEncoderWorkerFacade)
{
	FakeH264VideoEncoderWorkerFacadeShared()
	{
		return new std::shared_ptr<FakeH264VideoEncoderWorkerFacade>(new FakeH264VideoEncoderWorkerFacade());
	}
	SHARED_PTR_TO(RTPReceiver)
}
SHARED_PTR_END(FakeH264VideoEncoderWorkerFacade)

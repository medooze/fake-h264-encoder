/* 
 * File:   FakeH264VideoEncoderWorker.cpp
 * Author: Sergio
 * 
 * Created on 12 de agosto de 2014, 10:32
 */

#include "FakeH264VideoEncoderWorker.h"
#include "log.h"
#include "tools.h"
#include "acumulator.h"
#include "VideoCodecFactory.h"

#include "h264/h264.h"
#include "fake264.h"

FakeH264VideoEncoderWorker::FakeH264VideoEncoderWorker() 
{
	//Create objects
	pthread_mutex_init(&mutex,NULL);
	pthread_cond_init(&cond,NULL);
}

FakeH264VideoEncoderWorker::~FakeH264VideoEncoderWorker()
{
	End();
	//Clean object
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}

int FakeH264VideoEncoderWorker::SetBitrate(int fps,int bitrate)
{
	Debug("-FakeH264VideoEncoderWorker::SetBitrate() [fps:%d,bitrate:%d]\n",fps,bitrate);

	//Store parameters
	this->bitrate	  = bitrate;
	this->fps	  = fps;

	//Good
	return 1;
}

int FakeH264VideoEncoderWorker::Start()
{
	Log("-FakeH264VideoEncoderWorker::Start()\n");
	
	
	//Check if need to restart
	if (encoding)
		//Stop first
		Stop();

	//Start decoding
	encoding = 1;

	//launc thread
	createPriorityThread(&thread,startEncoding,this,0);

	return 1;
}

void * FakeH264VideoEncoderWorker::startEncoding(void *par)
{
	//Get worker
	FakeH264VideoEncoderWorker *worker = (FakeH264VideoEncoderWorker *)par;
	//Block all signals
	blocksignals();
	//Run
	worker->Encode();
	//Exit
	return NULL;
}

bool FakeH264VideoEncoderWorker::SetThreadName( const std::string& name)
{
#if defined(__linux__)
	return !pthread_setname_np(thread, name.c_str());
#else
	return false;
#endif
}

bool FakeH264VideoEncoderWorker::SetPriority(int priority)
{
	sched_param param = {
		.sched_priority = priority
	};
	return !pthread_setschedparam(thread, priority ? SCHED_FIFO : SCHED_OTHER, &param);
}


int FakeH264VideoEncoderWorker::Stop()
{
	Log(">FakeH264VideoEncoderWorker::Stop()\n");

	//If we were started
	if (encoding)
	{
		//Stop
		encoding=0;

		//Cancel sending
		pthread_cond_signal(&cond);

		//Esperamos
		pthread_join(thread,NULL);
	}

	Log("<FakeH264VideoEncoderWorker::Stop()\n");

	return 1;
}

int FakeH264VideoEncoderWorker::Init()
{
	Log(">FakeH264VideoEncoderWorker::Init()\n");

	uint32_t ini = 0;
	uint32_t end = 0;

	//Parse h264 stream
	while (end < test_h264_len)
	{
		uint8_t startCodeLength = 0;

		//Check if we have a nal start
		if ((end + 4< test_h264_len) && get4(test_h264, end) == 0x01)
			startCodeLength = 4;
		else if (end + 3 < test_h264_len && get3(test_h264, end) == 0x01)
			startCodeLength = 3;

		//If we found a nal unit
		if (startCodeLength)
		{
			//Got nal start, check if not first one
			if (ini!=end)
			{
				/* +---------------+
				 * |0|1|2|3|4|5|6|7|
				 * +-+-+-+-+-+-+-+-+
				 * |F|NRI|  Type   |
				 * +---------------+
				 */
				BYTE nri = test_h264[ini] & 0b0'111'00000;
				BYTE nalUnitType = test_h264[ini] & 0b0'000'11111;

				UltraDebug("-FakeH264VideoEncoderWorker::Init() | Got nal [type:%d,ini:%d,end:%d,len:%d]\n", nalUnitType, ini, end, (end-ini), test_h264_len);

				
				Buffer buffer(end - ini);

				//Do RBSP
				for (uint32_t i = ini; i < end; ++i)
				{
					//Check emulation code
					if ((i + 3 < end) && get3(test_h264, i) == 0x000003)
					{
						//Append first two bytes
						buffer.AppendData(test_h264 + i, 2);
						//Skip them
						i+=2;
					} else {
						//Append nyte
						buffer.AppendData(test_h264 + i, 1);
					}
				}

				//Check nal type
				switch(nalUnitType)
				{
					case 7:
					{
						H264SeqParameterSet x;
						if (!x.Decode(buffer.GetData(), buffer.GetSize()))
						{
							Warning("-sps error\n");
							break;
						}
						//move
						sps = std::move(buffer);
						break;
					}
					case 8:
					{
						H264PictureParameterSet x;
						if (!x.Decode(buffer.GetData(), buffer.GetSize()))
						{
							Warning("-pps error\n");
							break;
						}
						//move
						pps = std::move(buffer);
						break;
					}
					case 6:
						//Ignore sei
						break;
					case 5:
					{
						//push new frame
						auto& frame = frames.emplace_back(new VideoFrame(VideoCodec::H264, sps.GetSize()+pps.GetSize()+buffer.GetSize()));
						//Add sps and pps
						frame->AddRtpPacket(frame->AppendMedia(sps),sps.GetSize());
						frame->AddRtpPacket(frame->AppendMedia(pps), pps.GetSize());
						//Add frame
						auto pos = frame->AppendMedia(buffer);
						//If it is small
						if (buffer.GetSize() < RTPPAYLOADSIZE)
						{
							//Single packetization
							frame->AddRtpPacket(pos, buffer.GetSize());
						} else {
							//FU-A
							/* 
							 * The FU indicator octet has the following format:
							 *
							 *       +---------------+
							 *       |0|1|2|3|4|5|6|7|
							 *       +-+-+-+-+-+-+-+-+
							 *       |F|NRI|  Type   |
							 *       +---------------+
							 *
   							 * The FU header has the following format:
							 *
							 *      +---------------+
							 *      |0|1|2|3|4|5|6|7|
							 *      +-+-+-+-+-+-+-+-+
							 *      |S|E|R|  Type   |
							 *      +---------------+
							 */
							BYTE nalHeader[2] = { nri | 28, 0b100'00000 | nalUnitType };
							//Get nal reader
							BufferReader reader(buffer);
							//Skip payload nal header
							reader.Skip(1);
							pos += 1;
							//Part it
							while(reader.GetLeft())
							{
								int len = std::min<uint32_t>(RTPPAYLOADSIZE, reader.GetLeft());
								//Read it
								reader.Skip(len);
								//If all added
								if (!reader.GetLeft())
									//Set end mark
									nalHeader[1] |= 0b010'00000;
								//Add packetization
								frame->AddRtpPacket(pos, len, nalHeader, sizeof(nalHeader));
								//Not first
								nalHeader[1] &= 0b011'11111;
								//Move start
								pos += len;
							}
						}

						//Set intra flag
						frame->SetIntra(true);
						break;
					}
					case 1:
					{	
						//push new frame
						auto& frame = frames.emplace_back(new VideoFrame(VideoCodec::H264, std::move(buffer)));
						//If it is small
						if (frame->GetLength()< RTPPAYLOADSIZE)
							//Single packetization
							frame->AddRtpPacket(0, frame->GetLength());
						break;
					}
					default:
						Warning("-FakeH264VideoEncoderWorker::Init() | Unknown nal type [type:%d]\n", nalUnitType);
				} 

				
			}

			//Skip nal start code
			end += startCodeLength;
			//Set ini of next one
			ini = end;
		} else {
			//Next
			end++;
		}
	}

	Log("<FakeH264VideoEncoderWorker::Init()\n");

	return 1;
	
}

int FakeH264VideoEncoderWorker::End()
{
	Log(">FakeH264VideoEncoderWorker::Stop()\n");

	//Check if already decoding
	if (encoding)
		//Stop
		Stop();

	//Done
	return 1;
}

int FakeH264VideoEncoderWorker::Encode()
{
	timeval first;
	timeval prev;
	timeval lastFPU;
	
	DWORD num = 0;
	QWORD overslept = 0;

	Acumulator bitrateAcu(1000);
	Acumulator fpsAcu(1000);

	Log(">FakeH264VideoEncoderWorker::Encode() [bitrate:%d,fps:%d]\n",bitrate,fps);

	//No wait for first
	QWORD frameTime = 0;

	//The time of the first one
	gettimeofday(&first,NULL);

	//The time of the previos one
	gettimeofday(&prev,NULL);

	//Fist FPU
	gettimeofday(&lastFPU,NULL);

	//Mientras tengamos que capturar
	while(encoding)
	{
		//Check if we need to send intra
		if (sendFPU)
		{
			//Do not send anymore
			sendFPU = false;
			//Do not send if just send one (100ms)
			if (getDifTime(&lastFPU)/100>100)
			{
				//Move to first frame
				frameIndex = 0;
				//Update last FPU
				getUpdDifTime(&lastFPU);
			}
		}
		
		//Get next video frame
		std::unique_ptr<VideoFrame> videoFrame((VideoFrame*)frames[frameIndex]->Clone());

		//Move to first frame
		frameIndex++;

		//If we have to start from start
		if (frameIndex==frames.size())
			//Reset
			frameIndex = 0;

		//Calculate bitrate
		int bitratePerFrame = bitrate*125/fps;

		//Calculate padding data
		int padding = bitratePerFrame > videoFrame->GetLength() ? bitratePerFrame - videoFrame->GetLength() : 0;
		
		while (padding)
		{
			//The filler nal
			BYTE filler[RTPPAYLOADSIZE] = {12};
			//Get max len
			int len = std::min<uint32_t>(RTPPAYLOADSIZE, padding);
			//Add packetization
			videoFrame->AddRtpPacket(videoFrame->AppendMedia(filler,len), len);
			//Remove
			padding -= len;
		}

		//If was failed
		if (!videoFrame)
			//Next
			continue;

		//Increase frame counter
		fpsAcu.Update(getTime()/1000,1);

		//Check
		if (frameTime)
		{
			timespec ts;
			//Lock
			pthread_mutex_lock(&mutex);
			//Calculate slept time
			QWORD sleep = frameTime;
			//Remove extra sleep from prev
			if (overslept<sleep)
				//Remove it
				sleep -= overslept;
			else
				//Do not overflow
				sleep = 1;
			//Calculate timeout
			calcAbsTimeoutNS(&ts,&prev,sleep);
			//Wait next or stopped
			int canceled  = !pthread_cond_timedwait(&cond,&mutex,&ts);
			//Unlock
			pthread_mutex_unlock(&mutex);
			//Check if we have been canceled
			if (canceled)
				//Exit
				break;
			//Get differencence
			QWORD diff = getDifTime(&prev);
			//If it is biffer
			if (diff>frameTime)
				//Get what we have slept more
				overslept = diff-frameTime;
			else
				//No oversletp (shoulddn't be possible)
				overslept = 0;
		}

		//Set frame time
		frameTime = 1E6/fps;
		
		//Add frame size in bits to bitrate calculator
	        bitrateAcu.Update(getDifTime(&first)/1000,videoFrame->GetLength()*8);

		//Set clock rate
		videoFrame->SetClockRate(90000);
		//Get now
		auto now = getDifTime(&first)/1000;
		//Set frame timestamp
		videoFrame->SetTimestamp(now*90);
		videoFrame->SetTime(getTime()/1000);
		//Set dudation
		videoFrame->SetDuration(frameTime*90000/1E6);
		
		//Lock
		pthread_mutex_lock(&mutex);

		//For each listener
		for (const auto& listener : listeners)
			//Call listener
			listener->onMediaFrame(*videoFrame);

		//unlock
		pthread_mutex_unlock(&mutex);

		//Set sending time of previous frame
		getUpdDifTime(&prev);
		
		//Dump statistics
		if (num && ((num%fps*10)==0))
		{
			Debug("-Send bitrate bitrate=%d avg=%llf rate=[%llf,%llf] fps=[%llf,%llf]\n",bitrate,bitrateAcu.GetInstantAvg()/1000,bitrateAcu.GetMinAvg()/1000,bitrateAcu.GetMaxAvg()/1000,fpsAcu.GetMinAvg(),fpsAcu.GetMaxAvg());
			bitrateAcu.ResetMinMax();
			fpsAcu.ResetMinMax();
		}
		num++;
	}

	//Salimos
	Log("<FakeH264VideoEncoderWorker::Encode()  [%d]\n",encoding);
	
	//Done
	return 1;
}

bool FakeH264VideoEncoderWorker::AddListener(MediaFrame::Listener *listener)
{
	//Ensure it is not null
	if (!listener)
		return false;

	//Lock
	pthread_mutex_lock(&mutex);

	//Add to set
	listeners.insert(listener);

	//unlock
	pthread_mutex_unlock(&mutex);

	return true;
}

bool FakeH264VideoEncoderWorker::RemoveListener(MediaFrame::Listener *listener)
{
	//Lock
	pthread_mutex_lock(&mutex);

	//Search
	auto it = listeners.find(listener);

	//If found
	if (it!=listeners.end())
		//Erase it
		listeners.erase(it);

	//Unlock
	pthread_mutex_unlock(&mutex);

	return true;
}


void FakeH264VideoEncoderWorker::SendFPU()
{
	sendFPU = true;
}

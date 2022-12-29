/* 
 * File:   FakeH264VideoEncoderWorker.cpp
 * Author: Sergio
 * 
 * Created on 12 de agosto de 2014, 10:32
 */

#include "FakeH264VideoEncoderWorker.h"
#include "log.h"
#include "tools.h"

#include "VideoCodecFactory.h"

#include "h264/h264.h"
#include "fake264.h"

FakeH264VideoEncoderWorker::FakeH264VideoEncoderWorker() :
	bitrateAcu(1000),
	fpsAcu(1000)
{
	//Create unscheduled encoding timer
	encodingTimer = loop.CreateTimer([&](std::chrono::milliseconds now) {
		Encode(now);
	});
	encodingTimer->SetName("FakeH264VideoEncoderWorker::encodingTimer");
}
FakeH264VideoEncoderWorker::~FakeH264VideoEncoderWorker()
{
	End();
}

int FakeH264VideoEncoderWorker::SetBitrate(int fps,int bitrate)
{
	Debug("-FakeH264VideoEncoderWorker::SetBitrate() [fps:%d,bitrate:%d]\n",fps,bitrate);

	//Store parameters
	this->bitrate	  = bitrate;
	this->fps	  = fps;
	//Calculate new interval
	auto repeat = 1000ms/fps;

	//Reschedule encoding timer if already started to encode
	if (encodingTimer->IsScheduled() && encodingTimer->GetRepeat()!=repeat)
		encodingTimer->Repeat(repeat);
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

	//Schedule timer
	//TODO: increase timer precision
	encodingTimer->Repeat(1000ms/fps);

	return 1;
}

bool FakeH264VideoEncoderWorker::SetThreadName( const std::string& name)
{
	return loop.SetThreadName(name);
}

bool FakeH264VideoEncoderWorker::SetPriority(int priority)
{
	return loop.SetPriority(priority);
}


int FakeH264VideoEncoderWorker::Stop()
{
	Log("-FakeH264VideoEncoderWorker::Stop()\n");

	//Stop
	encoding=0;

	//Schedule timer
	encodingTimer->Cancel();

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

	//Start loop
	loop.Start();


	Log("<FakeH264VideoEncoderWorker::Init()\n");

	return 1;
	
}

int FakeH264VideoEncoderWorker::End()
{
	Log(">FakeH264VideoEncoderWorker::Stop()\n");

	//End loop
	loop.Stop();

	//Done
	return 1;
}

void FakeH264VideoEncoderWorker::Encode(std::chrono::milliseconds now)
{
	//Check if it its first
	if (first==0ms)
		//Now
		first = now;

	//Calculate relative time since first frame
	auto ts = now - first;

	//Check if we need to send intra
	if (sendFPU)
	{
		//Do not send anymore
		sendFPU = false;
		//Do not send if just send one (100ms)
		if (lastFPU == 0ms || now-lastFPU>100ms)
		{
			//Move to first frame
			frameIndex = 0;
			//Update last FPU
			lastFPU = now;
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
		return;

	//Increase frame counter
	fpsAcu.Update(now.count(),1);

	//Add frame size in bits to bitrate calculator
        bitrateAcu.Update(now.count(),videoFrame->GetLength()*8);

	//Set clock rate
	videoFrame->SetClockRate(90000);
	
	//Set frame timestamp
	videoFrame->SetTimestamp(ts.count() * 90);
	videoFrame->SetTime(now.count());
	//Set dudation
	videoFrame->SetDuration(90000/fps);
		
	//For each listener
	for (const auto& listener : listeners)
		//Call listener
		listener->onMediaFrame(*videoFrame);

	//Dump statistics
	if (num && ((num%fps*10)==0) && bitrateAcu.IsInMinMaxWindow() && fpsAcu.IsInMinMaxWindow())
	{
		Debug("-Send bitrate bitrate=%d avg=%llf rate=[%llf,%llf] fps=[%llf,%llf]\n",bitrate,bitrateAcu.GetInstantAvg()/1000,bitrateAcu.GetMinAvg()/1000,bitrateAcu.GetMaxAvg()/1000,fpsAcu.GetMinAvg(),fpsAcu.GetMaxAvg());
		bitrateAcu.ResetMinMax();
		fpsAcu.ResetMinMax();
	}
	num++;
}

bool FakeH264VideoEncoderWorker::AddListener(const MediaFrame::Listener::shared& listener)
{
	//Ensure it is not null
	if (!listener)
		return false;

	Debug("-FakeH264VideoEncoderWorker::AddListener() [listener:%p]\n", listener.get());

	//Add sync
	loop.Sync([&](auto now){
		//Add to set
		listeners.insert(listener);
	});

	return true;
}

bool FakeH264VideoEncoderWorker::RemoveListener(const MediaFrame::Listener::shared& listener)
{
	Debug("-FakeH264VideoEncoderWorker::RemoveListener() [listener:%p]\n", listener.get());

	//Remove sync
	loop.Sync([&](auto now) {
		//Erase it
		listeners.erase(listener);
	});

	return true;
}


void FakeH264VideoEncoderWorker::SendFPU()
{
	sendFPU = true;
}

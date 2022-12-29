%module medooze_fake_h264_encoder
%{
#include <nan.h>
%}

%include "shared_ptr.i"
%include "EventLoop.i"
%include "MediaFrame.i"
%include "MediaFrameListenerBridge.i"
%include "RTPIncomingMediaStream.i"
%include "FakeH264VideoEncoderWorkerFacade.i"
%include  "MedoozeFakeH264EncoderModule.i"
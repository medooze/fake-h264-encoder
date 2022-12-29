const Native		= require("./Native");
const Emitter		= require("./Emitter");
const LFSR		= require('lfsr');
const VideoEncoderIncomingStreamTrack = require("./VideoEncoderIncomingStreamTrack");

class FakeH264VideoEncoder extends Emitter
{
	/**
	 * @ignore
	 * @hideconstructor
	 * private constructor
	 */
	constructor(params)
	{
		//Init emitter
		super();
		//Create code
		const encoder = new Native.FakeH264VideoEncoderWorkerFacadeShared();
		//Set bitrate
		if (!encoder.SetBitrate(parseInt(params.fps),parseInt(params.bitrate)))
			//Error
			throw new Error("Wrong params or video codec not supported");
		//Storea codec
		this.encoder = encoder;
		//Set it in encoder
		this.encoder.Init();
		//Start encoding
		this.encoder.Start();
		
		//Create new sequence generator
		this.lfsr = new LFSR();
		
		//Track maps
		this.tracks = new Map();
	}
	
	createIncomingStreamTrack(trackId, nosmooth)
	{
		//Create the source
		const bridge = new Native.MediaFrameListenerBridgeShared(this.encoder.GetTimeService(), this.lfsr.seq(31), !nosmooth);
		//Add track
		const track = new VideoEncoderIncomingStreamTrack(trackId,this.encoder.toRTPReceiver() ,bridge); 
		//Add it to the encoder
		this.encoder.AddListener(bridge.toMediaFrameListener());
		//Add listener
		track.once("stopped",(track)=>{
			//Remove source from listener
			this.encoder.RemoveListener(bridge.toMediaFrameListener());
			//Stop bridge
			bridge.Stop();
			//Remove from map
			this.tracks.delete(track.getId());
		});
		//Add to tracks
		this.tracks.set(track.getId(),track);
		//Done
		return track;
	}

	setBitrate(fps, bitrate)
	{
		//Set bitrate
		if (!this.encoder.SetBitrate(parseInt(fps), parseInt(bitrate)))
			//Error
			throw new Error("Wrong params or video codec not supported");
	}

	setPriority(priority)
	{
		return this.encoder.SetPriority(parseInt(priority));
	}

	setThreadName(name) {
		return this.encoder.SetThreadName(name);
	}
	
	stop()
	{
		//Don't call it twice
		if (!this.encoder) return;
		
		//Stop encoding
		this.encoder.Stop();
		
		//Stop tracks
		for (const [trackId,track] of this.tracks)
			//Stop it
			track.stop();
		
		/**
		* VideoEncoder stopped event
		*
		* @name stopped
		* @memberof VideoEncoder
		* @kind event
		* @argument {VideoEncoder} encoder
		*/
		this.emitter.emit("stopped", this);
		
		//Stop emitter
		super.stop();
		
		//Stop encoder
		this.encoder.Stop();
		this.encoder.End();
			
		//Remove brdige reference, so destructor is called on GC
		this.encoder = null;
	}
};

module.exports = FakeH264VideoEncoder;

const Native			= require("./Native");
const FakeH264VideoEncoder	= require("./FakeH264VideoEncoder.js");

//Sequence for init the other LFSR instances
const LFSR	  = require('lfsr');
const defaultSeed = new LFSR(8, 92914);

//Replace default seeed
LFSR.prototype._defaultSeed = function(n) {
	if (!n) throw new Error('n is required');
	return defaultSeed.seq(n);
};

/** @namespace */
const VideoCodecs = {};


/**
 * Enable or disable log level traces
 * @memberof VideoCodecs
 * @param {Boolean} flag
 */
VideoCodecs.enableLog = function (flag)
{
	//Set flag
	Native.MedoozeFakeH264EncoderModule.EnableLog(flag);
};


/**
 * Enable or disable debug level traces
 * @memberof VideoCodecs
 * @param {Boolean} flag
 */
VideoCodecs.enableDebug = function (flag)
{
	//Set flag
	Native.MedoozeFakeH264EncoderModule.EnableDebug(flag);
};

/**
 * Enable or disable ultra debug level traces
 * @memberof VideoCodecs
 * @param {Boolean} flag
 */
VideoCodecs.enableUltraDebug = function (flag)
{
	//Set flag
	Native.MedoozeFakeH264EncoderModule.EnableUltraDebug(flag);
};

/**
 * Create a new VideoEncoder
 * @memberof VideoCodecs
 * @param {Number} params.fps
 * @param {Number} params.bitrate
 * @returns {FakeH264VideoEncoder} The new created encoder
 */
VideoCodecs.createFakeH264VideoEncoder = function(params)
{
	return new FakeH264VideoEncoder(params);
};


module.exports = VideoCodecs;

{
	'variables':
	{
		'external_libmediaserver%'		: '<!(echo $LIBMEDIASERVER)',
		'external_libmediaserver_include_dirs%'	: '<!(echo $LIBMEDIASERVER_INCLUDE)',
		'medooze_media_server_src' : "<!(node -e \"require('medooze-media-server-src')\")",
	},
	"targets": 
	[
		{
			"target_name": "medooze-fake-h264-encoder",
			"cflags": 
			[
				"-march=native",
				"-fexceptions",
				"-O3",
				"-g",
				"-Wno-unused-function -Wno-comment",
				#"-O0",
				#"-fsanitize=address"
			],
			"cflags_cc": 
			[
				"-fexceptions",
				"-std=c++17",
				"-O3",
				"-g",
				"-Wno-unused-function",
				"-faligned-new",
				"-Wall"
				#"-O0",
				#"-fsanitize=address,leak"
			],
			"include_dirs" : 
			[
				'/usr/include/nodejs/',
				"<!(node -e \"require('nan')\")"
			],
			"ldflags" : [" -lpthread -lresolv"],
			"link_settings": 
			{
        			'libraries': ["-lpthread -lpthread -lresolv"]
      			},
			"sources": 
			[ 
				"src/fake-h264-encoder_wrap.cxx",
				"src/FakeH264VideoEncoderWorker.cpp",
			],
			"conditions":
			[
				[
					"external_libmediaserver == ''", 
					{
						"include_dirs" :
						[
							'<(medooze_media_server_src)/include',
							'<(medooze_media_server_src)/src',
							'<(medooze_media_server_src)/ext/crc32c/include',
							'<(medooze_media_server_src)/ext/libdatachannels/src',
							'<(medooze_media_server_src)/ext/libdatachannels/src/internal',
						],
						"sources": 
						[
							"<(medooze_media_server_src)/src/EventLoop.cpp",
							"<(medooze_media_server_src)/src/MediaFrameListenerBridge.cpp",
							"<(medooze_media_server_src)/src/rtp/DependencyDescriptor.cpp",
							"<(medooze_media_server_src)/src/rtp/RTPPacket.cpp",
							"<(medooze_media_server_src)/src/rtp/RTPPayload.cpp",
							"<(medooze_media_server_src)/src/rtp/RTPHeader.cpp",
							"<(medooze_media_server_src)/src/rtp/RTPHeaderExtension.cpp",
							"<(medooze_media_server_src)/src/rtp/LayerInfo.cpp",
							"<(medooze_media_server_src)/src/VideoLayerSelector.cpp",
							"<(medooze_media_server_src)/src/DependencyDescriptorLayerSelector.cpp",
							"<(medooze_media_server_src)/src/h264/h264depacketizer.cpp",
							"<(medooze_media_server_src)/src/vp8/vp8depacketizer.cpp",
							"<(medooze_media_server_src)/src/h264/H264LayerSelector.cpp",
							"<(medooze_media_server_src)/src/vp8/VP8LayerSelector.cpp",
							"<(medooze_media_server_src)/src/vp9/VP9PayloadDescription.cpp",
							"<(medooze_media_server_src)/src/vp9/VP9LayerSelector.cpp",
							"<(medooze_media_server_src)/src/vp9/VP9Depacketizer.cpp",
							"<(medooze_media_server_src)/src/av1/AV1Depacketizer.cpp",
						],
  					        "conditions" : [
								['OS=="mac"', {
									"xcode_settings": {
										"CLANG_CXX_LIBRARY": "libc++",
										"CLANG_CXX_LANGUAGE_STANDARD": "c++17",
										"OTHER_CFLAGS": [ "-Wno-aligned-allocation-unavailable","-march=native"]
									},
								}]
						]
					},
					{
						"libraries"	: [ "<(external_libmediaserver)" ],
						"include_dirs"	: [ "<@(external_libmediaserver_include_dirs)" ],
						'conditions':
						[
							['OS=="linux"', {
								"ldflags" : [" -Wl,-Bsymbolic "],
							}],
							['OS=="mac"', {
									"xcode_settings": {
										"CLANG_CXX_LIBRARY": "libc++",
										"CLANG_CXX_LANGUAGE_STANDARD": "c++17",
										"OTHER_CFLAGS": [ "-Wno-aligned-allocation-unavailable","-march=native"]
									},
							}],
						]
					}
				]
			]
		}
	]
}


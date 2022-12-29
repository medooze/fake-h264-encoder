%{
#include "log.h"

class MedoozeFakeH264EncoderModule
{
public:
	static void EnableLog(bool flag)
	{
		//Enable log
		Logger::EnableLog(flag);
	}
	
	static void EnableDebug(bool flag)
	{
		//Enable debug
		Logger::EnableDebug(flag);
	}
	
	static void EnableUltraDebug(bool flag)
	{
		//Enable debug
		Logger::EnableUltraDebug(flag);
	}
	
};

%}


class MedoozeFakeH264EncoderModule
{
public:
	static void EnableLog(bool flag);
	static void EnableDebug(bool flag);
	static void EnableUltraDebug(bool flag);
};
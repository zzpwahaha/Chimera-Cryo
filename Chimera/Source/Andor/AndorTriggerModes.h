// created by Mark O. Brown
#pragma once

#include <string>
#include <array>

struct AndorTriggerMode{	
	enum class mode	{
		External,
		Internal,
		Software,
		ExternalStart,
		ExternalExposure
	};
	static const std::array<mode,5> allModes;
	static std::string toStr ( mode m );
	static mode fromStr ( std::string txt );
	//std::string AndorTriggerModeText ( AndorTriggerMode mode );
	//AndorTriggerMode AndorTriggerModeFromText ( std::string txt );
};





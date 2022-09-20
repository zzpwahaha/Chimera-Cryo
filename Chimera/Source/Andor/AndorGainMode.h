#pragma once
#include <string>
#include <array>

struct AndorGainMode
{
	enum class mode
	{
		FastestFrameRate = 1,
		HighDynamicRange=2
	};
	static const std::array<mode, 2> allModes;
	static std::string toStr(mode m);
	static mode fromStr(std::string txt);
};


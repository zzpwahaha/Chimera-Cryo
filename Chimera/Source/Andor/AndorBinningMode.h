#pragma once
#include <string>
#include <array>

struct AndorBinningMode
{
	enum class mode
	{
		oneByOne = 1,
		twoByTwo = 2,
		threeByThree = 3,
		fourByFour= 4,
		eightByEight = 5

	};
	static const std::array<mode, 5> allModes;
	static std::string toStr(mode m);
	static mode fromStr(std::string txt);
	static unsigned int toInt(mode m);
};


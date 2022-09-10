// created by Mark O. Brown
#pragma once

#include <string>
#include <array>

struct AndorRunModes
{
	enum class mode
	{
		Single = 1,
		Kinetic = 2,
		Accumulate = 3
	};
	static const std::array<mode, 3> allModes;
	static std::string toStr ( mode m );
	static mode fromStr ( std::string txt );
};


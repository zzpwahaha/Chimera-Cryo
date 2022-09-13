#include "stdafx.h"
#include "AndorBinningMode.h"

const std::array<AndorBinningMode::mode, 5> AndorBinningMode::allModes = { mode::oneByOne,
	mode::twoByTwo,
	mode::threeByThree,
	mode::fourByFour,
	mode::eightByEight };

std::string AndorBinningMode::toStr(AndorBinningMode::mode mode)
{
	if (mode == AndorBinningMode::mode::oneByOne)
	{
		return "1x1";
	}
	else if (mode == AndorBinningMode::mode::twoByTwo)
	{
		return "2x2";
	}
	else if (mode == AndorBinningMode::mode::threeByThree)
	{
		return "3x3";
	}
	else if (mode == AndorBinningMode::mode::fourByFour)
	{
		return "4x4";
	}
	else if (mode == AndorBinningMode::mode::eightByEight)
	{
		return "8x8";
	}
	else
	{
		throw;
	}
}

AndorBinningMode::mode AndorBinningMode::fromStr(std::string txt)
{
	for (auto m : AndorBinningMode::allModes)
	{
		if (txt == toStr(m))
		{
			return m;
		}
	}
	thrower("Failed to convert to andor binning mode from string!");
	return mode::oneByOne;
}

unsigned int AndorBinningMode::toInt(AndorBinningMode::mode mode)
{
	if (mode == AndorBinningMode::mode::oneByOne)
	{
		return 1;
	}
	else if (mode == AndorBinningMode::mode::twoByTwo)
	{
		return 2;
	}
	else if (mode == AndorBinningMode::mode::threeByThree)
	{
		return 3;
	}
	else if (mode == AndorBinningMode::mode::fourByFour)
	{
		return 4;
	}
	else if (mode == AndorBinningMode::mode::eightByEight)
	{
		return 8;
	}
	else
	{
		throw;
	}
}

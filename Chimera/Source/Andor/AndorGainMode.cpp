#include "stdafx.h"
#include "AndorGainMode.h"

const std::array<AndorGainMode::mode, 2> AndorGainMode::allModes = { mode::FastestFrameRate,
	mode::HighDynamicRange};

std::string AndorGainMode::toStr(AndorGainMode::mode mode)
{
	if (mode == AndorGainMode::mode::FastestFrameRate)
	{
		return "Fastest frame rate (12-bit)";
	}
	else if (mode == AndorGainMode::mode::HighDynamicRange)
	{
		return "High dynamic range (16-bit)";
	}
	else
	{
		throw;
	}
}

AndorGainMode::mode AndorGainMode::fromStr(std::string txt)
{
	for (auto m : AndorGainMode::allModes)
	{
		if (txt == toStr(m))
		{
			return m;
		}
	}
	thrower("Failed to convert to andor gain mode from string!");
	return mode::FastestFrameRate;
}


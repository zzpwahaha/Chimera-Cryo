#pragma once

#include "ParameterSystem/Expression.h"
#include <vector>

enum class microwaveDevice
{
	RohdeSchwarzGenerator,
	WindFreak,
	NONE // I.e. total safemode.
};


struct microwaveListEntry
{
	Expression frequency;
	Expression power;
	// the chosen channel C0 = RFoutA, C1 = RFoutB
	unsigned channel = 0;
};

struct microwaveSettings
{
	std::vector<microwaveListEntry> list;
	bool control = false;
};

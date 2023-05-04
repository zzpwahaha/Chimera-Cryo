// created by Mark O. Brown
#pragma once

#include "ParameterSystem/Expression.h"
#include "GeneralObjects/commonTypes.h"
#include <array>

enum class AOGrid : size_t
{
	numPERunit = 16,
	numOFunit = 2,
	total = numPERunit*numOFunit
};


struct AoInfo
{
	double currVal = 0;
	std::string name = "";
	double minVal = -9.9999;
	double maxVal = 10;
	double defaultVal = 0;
	// notes are purely cosmetic. Can be used e.g. to store calibration curves for VCOs or detailed reminders about 
	// using a given dac.
	std::string note = "";
};


struct AoCommandForm
{
	// can either be "dac", "dacarange", or "daclinspace"
	std::string commandName;

	unsigned short line = 0;
	timeType time;

	Expression initVal;
	Expression finalVal;
	Expression rampTime;
	Expression rampInc;
	Expression numSteps;

	// stores whether this command is subject to repeats and which repeat it correpsonds to in the tree if so
	//repeatInfoId repeatId = { 0, {0,0} };
};


struct AoCommand
{
	unsigned short line;
	double time;
	double value;
	double endValue;
	double rampTime;
};


struct AoSnapshot
{
	double time;
	std::array<double, size_t(AOGrid::total)> dacValues;
	std::array<double, size_t(AOGrid::total)> dacEndValues;
	std::array<double, size_t(AOGrid::total)> dacRampTimes;
};


struct AoChannelSnapshot
{
	unsigned short channel;
	double time;
	double dacValue;
	double dacEndValue;
	double dacRampTime;
};
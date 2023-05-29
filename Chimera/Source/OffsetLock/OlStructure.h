#pragma once

#include <array>
#include <string>
#include "ParameterSystem/Expression.h"
#include "GeneralObjects/CommonTypes.h"
#include <ExperimentThread/repeatManager.h>

enum class OLGrid : size_t
{
	numPERunit = 2,
	numOFunit = 2,
	total = numPERunit * numOFunit
};

struct OlInfo
{
	std::string name = "";

	double currFreq = 0;
	double minFreq = 1000.0; //in MHz
	double maxFreq = 6000.0;
	double defaultFreq = 1500.0;

	std::string note = "None";

};


struct OlCommandForm
{
	std::string commandName;

	unsigned short line = 0;
	timeType time;

	Expression initVal;
	Expression finalVal;
	Expression rampTime;
	Expression rampInc;
	Expression numSteps;

	// stores whether this command is subject to repeats and which repeat it correpsonds to in the tree if so
	repeatInfoId repeatId = { 0, {0,0} };
};


struct OlCommand
{
	unsigned short line;
	double time;
	double value;
	double endValue;
	unsigned numSteps;
	double rampTime;

	// same as OlCommandForm, this will be used for repeat generation, i.e. copy and extend the std::vector<OlCommand>
	repeatInfoId repeatId = { 0, {0,0} };
};


struct OlSnapshot
{
	double time;
	std::array<double, size_t(OLGrid::total)> olValues;
	std::array<double, size_t(OLGrid::total)> olEndValues;
	std::array<unsigned, size_t(OLGrid::total)> olNumSteps;
	std::array<double, size_t(OLGrid::total)> olRampTimes;
};

struct OlChannelSnapshot
{
	unsigned short channel;
	double time;
	double val;
	double endVal;
	unsigned numSteps;
	double rampTime;
};

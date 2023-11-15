// created by Mark O. Brown
#pragma once
#include <array>
#include "GeneralObjects/commonTypes.h"
#include <ExperimentThread/repeatManager.h>
//#include "DigitalOutput/DoRows.h"

enum class DOGrid : size_t
{
	numPERunit = 8,
	numOFunit = 8,
	total = numPERunit * numOFunit
};



// this struct keeps variable names.
struct DoCommandForm
{
	// the hardware location of this line
	std::pair<unsigned short, unsigned short> line;
	// the time to make the change
	timeType time;
	// the evaluated values of the time for each varation.
	std::vector<double> timeVals; /*not used for zynq, then DoCommandForm and DoCommand are the same*/
	// the value to set it to. 
	bool value;
	// stores whether this command is subject to repeats and which repeat it correpsonds to in the tree if so
	repeatInfoId repeatId = { 0, {0,0}, false };
};

// no variables in this version. It's calculated each variation based on corresponding ComandForm structs.
struct DoCommand
{
	// the hardware location of this line
	std::pair<unsigned short, unsigned short> line;
	// the time to make the change
	double time;
	// the value to set it to. 
	bool value;
	// same as DoCommandForm, this will be used for repeat generation, i.e. copy and extend the std::vector<DoCommand>
	repeatInfoId repeatId = { 0, {0,0}, false };
};


typedef std::array<std::array<bool, size_t(DOGrid::numPERunit)>, size_t(DOGrid::numOFunit)> DOStatus;
// an object constructed for having all info the ttls for a single time
struct DoSnapshot
{
	// the time of the snapshot in the unit of ms
	double time;
	// all values at this time.
	DOStatus ttlStatus;
};


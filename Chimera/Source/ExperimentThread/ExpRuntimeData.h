#pragma once
#include <MiscellaneousExperimentOptions/MainOptionsControl.h>
#include <Scripts/ScriptStream.h>
#include <ParameterSystem/ParameterSystemStructures.h>

struct ExpRuntimeData {
	unsigned repetitions = 1;
	mainOptions mainOpts;
	ScriptStream masterScript;
	std::vector<parameterType> expParams;
};

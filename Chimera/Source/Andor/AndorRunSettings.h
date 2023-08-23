// created by Mark O. Brown
#pragma once
#include "GeneralImaging/imageParameters.h"
#include "Andor/AndorRunMode.h"
#include "Andor/AndorTriggerModes.h"
#include "Andor/AndorGainMode.h"
#include "Andor/AndorBinningMode.h"
#include <string>
#include <vector>

// this structure contains all of the main options which are necessary to set when starting a camera acquisition. All
// of these settings should be possibly modified by the user of the UI.
struct AndorRunSettings{
	//unsigned horShiftSpeedSetting = 0;
	//unsigned vertShiftSpeedSetting = 0;

	imageParameters imageSettings;
	bool controlCamera = true;
	//
	//bool emGainModeIsOn = false;
	//int emGainLevel = 0;
	//int readMode = 4;
	AndorRunModes::mode acquisitionMode = AndorRunModes::mode::Single;
	// 1 means frame transfer mode on, 0 means non-frame transfer mode.
	//int frameTransferMode = 0;
	AndorTriggerMode::mode triggerMode = AndorTriggerMode::mode::External;
	AndorGainMode::mode gainMode = AndorGainMode::mode::FastestFrameRate;
	AndorBinningMode::mode binningMode = AndorBinningMode::mode::oneByOne;
	bool showPicsInRealTime = false;
	//
	double exposureTime = 0.001;
	double frameRate = 10.0;
	//float kineticCycleTime = 0.1f;
	//float accumulationTime = 0;
	//int accumulationNumber = 1;
	std::vector<float> exposureTimes = { 0.026f };
	//
	unsigned picsPerRepetition=1;
	unsigned __int64 repetitionsPerVariation=10;
	unsigned __int64 totalVariations = 3;
	unsigned __int64 totalPicsInVariation();
	std::vector<size_t> variationShuffleIndex; // used for randomize variation case, to look up the true variation
	bool repFirst = false;
	// this is an int to reflect that the final number that's programmed to the camera is an int
	int totalPicsInExperiment();
	int temperatureSetting = 0;
};

/*
	- Includes AndorRunSettings, which are the settings that the camera itself cares about.
	- Also some auxiliary settings which are never directly programmed to the camera, but are key for the way the 
	camera is used in the code.
*/
struct AndorCameraSettings {
	AndorRunSettings andor;
	// not directly programmed to camera
	std::array<std::vector<int>, 4> thresholds;
	std::array<int, 4> palleteNumbers = { 0,0,0,0 };
	double mostRecentTemp;
};

Q_DECLARE_METATYPE(AndorRunSettings)
// created by Mark O. Brown
#pragma once

#include "DigitalOutput/DoSystem.h"
#include "AnalogOutput/AoSystem.h"
#include "DirectDigitalSynthesis/DdsSystem.h"
#include "OffsetLock/OlSystem.h"

#include "ParameterSystem/ParameterSystem.h"
#include "MiscellaneousExperimentOptions/DebugOptionsControl.h"
#include "MiscellaneousExperimentOptions/MainOptionsControl.h"
#include "Andor/AndorCameraCore.h"
#include "expSystemRunList.h" 
#include "Andor/AndorRunSettings.h"
#include "RealTimeDataAnalysis/atomGrid.h"
#include "ConfigurationSystems/profileSettings.h"
#include "RealTimeDataAnalysis/atomCruncherInput.h"
#include "RealTimeDataAnalysis/realTimePlotterInput.h"
#include "ExperimentType.h"
#include "DeviceList.h"

#include <AnalogInput/calInfo.h>
#include <CMOSCamera/CMOSSetting.h>
#include "ZynqTcp/ZynqTcp.h"


#include <chrono>
#include <vector>
#include <atomic>
#include <functional>

class MainWindow;
class DataLogger;
class IChimeraQtWindow;
class ExpThreadWorker;

struct ExperimentThreadInput{
	ExperimentThreadInput ( IChimeraQtWindow* win );
	realTimePlotterInput* plotterInput;
	profileSettings profile;

	DoSystem& ttlSys;
	DoCore& ttls;
	AoSystem& aoSys;
	AoCore& ao;
	DdsSystem& ddsSys;
	DdsCore& dds;
	OlSystem& olSys;
	OlCore& ol;

	ZynqTCP zynqExp;

	DeviceList devices;
	std::vector<calResult> calibrations;
	std::vector<parameterType> globalParameters;
	unsigned sleepTime = 0;
	DataLogger& logger;
	unsigned numVariations = 1;
	bool quiet = false;
	unsigned numAiMeasurements=0;
	bool updatePlotterXVals = false;
	std::atomic<bool>* skipNext = nullptr;
	atomGrid analysisGrid;
	ExperimentType expType = ExperimentType::Normal;
	// only for rearrangement.
	std::mutex* rearrangerLock;
	atomQueue* atomQueueForRearrangement;
	chronoTimes* andorsImageTimes;
	chronoTimes* grabTimes;
	std::condition_variable* conditionVariableForRerng;
};



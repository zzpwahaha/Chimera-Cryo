// created by Mark O. Brown
#pragma once
#include "atomGrid.h"
#include "GeneralImaging/imageParameters.h"
#include "Plotting/tinyPlotInfo.h"
#include <GeneralObjects/Queues.h>
#include <Plotting/dataPoint.h>
#include <atomic>
#include <vector>
#include <mutex>
#include <Andor/AndorRunSettings.h>

class IChimeraQtWindow;
class AnalysisThreadWorker;

struct realTimePlotterInput{
	realTimePlotterInput ( ) { }
	//AndorCameraSettings cameraSettings;
	IChimeraQtWindow* plotParentWindow;

	std::vector<tinyPlotInfo> plotInfo;
	std::vector<atomGrid> grids;
	imageParameters imageShape;

	unsigned picsPerVariation;
	unsigned picsPerRep;
	unsigned variations;

	unsigned alertThreshold;
	bool wantAtomAlerts;

	bool needsCounts;
};



#pragma once
#include <atomic>
#include <vector>
#include "GeneralImaging/imageParameters.h"
#include <CMOSCamera/CMOSSetting.h>


class IChimeraQtWindow;


struct MOTThreadInput {
	//realTimePlotterInput(std::atomic<unsigned>& pltTime) : plotTime(pltTime) { }
	//std::atomic<unsigned>& plotTime;
	//AndorCameraSettings cameraSettings;
	//IChimeraQtWindow* plotParentWindow;

	// This gets set to false, e.g. when the experiment ends, but doesn't tell the plotter to immediately stop, the 
	// plotter can finish it's work if it's backed up on images.
	//std::atomic<bool>* active = nullptr;
	// this tells the plotter to immediately stop.
	//std::atomic<bool>* aborting = nullptr;
	MakoSettings camSet;
};
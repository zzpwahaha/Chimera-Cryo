#pragma once
#include <RealTimeDataAnalysis/atomGrid.h>
#include <GeneralImaging/imageParameters.h>
#include <GeneralObjects/commonTypes.h>
#include <GeneralObjects/Queues.h>
#include <GeneralObjects/ThreadsafeQueue.h>
#include <atomic>
#include <vector>
#include <mutex>
#include <array>

class GigaMoogCore;
struct atomCruncherInput
{
	// timing info is stored in these.
	chronoTimesHR* imageGrabTimes;
	chronoTimesHR* catchPicTimes;
	chronoTimesHR* finTimes;
	// instructions.
	std::vector<atomGrid> grids;
	// the thread watches this to know when to quit.
	std::atomic<bool>* cruncherThreadActive;
	ThreadsafeQueue<NormalImage>* imageQueue;
	// options
	bool andorContinuousMode;
	unsigned picsPerRep;
	unsigned atomThresholdForSkip = UINT_MAX;
	// outer vector here is for each location in the first grid.
	std::array<std::vector<int>, 4> thresholds;
	imageParameters imageDims;
	GigaMoogCore* gmoog;
	// what the thread fills.
	std::atomic<bool>* skipNext;
};

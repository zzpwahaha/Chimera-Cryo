#include "stdafx.h"
#include "AnalysisThreadWorker.h"
#include "DataAnalysisControl.h"
#include <vector>

AnalysisThreadWorker::AnalysisThreadWorker (realTimePlotterInput* input_){
	input = input_;
}

AnalysisThreadWorker::~AnalysisThreadWorker () {
}

void AnalysisThreadWorker::handleNewPic (atomQueue atomPics) {
	/// Start loop waiting for plots
	// if no image, continue. 0th element is queue, 2nd element is grid num, always akot least 1 grid.
	/// get all the atom data
	bool thereIsAtLeastOneAtom = false;
	for (auto gridCount : range (input->grids.size ())) {
		unsigned groupNum = input->grids[gridCount].numAtoms();
		for (auto pixelInc : range (groupNum)) {
			// look at the most recent image.
			if (atomPics[gridCount].image[pixelInc]) {
				thereIsAtLeastOneAtom = true;
				atomPresentData[gridCount][pixelInc].push_back (1);
			}
			else {
				atomPresentData[gridCount][pixelInc].push_back (0);
			}
		}
	}
	if (thereIsAtLeastOneAtom) {
		noAtomsCounter = 0;
	}
	else {
		noAtomsCounter++;
	}
	if (noAtomsCounter >= input->alertThreshold && input->wantAtomAlerts) {
		//input->comm->sendNoAtomsAlert ();
	}
	/// check if have enough data to plot
	auto picStat = atomPics[0].picStat;
	auto picNum = picStat.picNum;
	if ((picNum + 1) % allPlots[0].getPicNumber() != 0) {
		// In this case, not enough data to plot a point yet, but I've just analyzed a pic, so remove that pic.
		// currentThreadPictureNumber++;
		// wait for next picture.
		return;
	}
	for (auto plotI : range (allPlots.size ())) {
		/// Check Post-Selection Conditions
		unsigned whichGrid = input->plotInfo[plotI].whichGrid;
		unsigned groupNum = input->grids[whichGrid].numAtoms();
		std::vector<std::vector<bool> > satisfiesPsc (allPlots[plotI].getDataSetNumber (),
			std::vector<bool> (groupNum, true));
		determineWhichPscsSatisfied (allPlots[plotI], groupNum, atomPresentData[whichGrid], satisfiesPsc);
		if (allPlots[plotI].getPlotType () == "Atoms") {
			auto res = handlePlotAtoms (
				allPlots[plotI], picStat, finalAtomData[plotI], dataContainers[plotI],
				satisfiesPsc, atomPresentData[whichGrid], groupNum);
			if (res.size () != 0) {
				emit newPlotData (res, plotI);
			}
		}
	}
	/// clear data
	// all pixels being recorded, not pixels in a data set.
	for (auto gridCount : range (input->grids.size ())) {
		unsigned groupNum = input->grids[gridCount].numAtoms();
		for (auto pixelI : range (groupNum)) {
			atomPresentData[gridCount][pixelI].clear ();
		}
	}
}

void AnalysisThreadWorker::handleNewPix (PixListQueue pixlist) {
	/// Start loop waiting for plots
	// if no image, continue. 0th element is queue, 2nd element is grid num, always akot least 1 grid.
	/// get all the atom data
	if (!input->needsCounts) {
		return;
	}
	PixListQueue tempPixList( input->grids.size( ) );
	tempPixList = pixlist;
	/// for all pixels... gather count information
	for (auto gridCount : range (input->grids.size ())) {
		unsigned groupNum = input->grids[gridCount].numAtoms();
		for (auto locIndex : range(groupNum)) {
			countData[gridCount][locIndex].push_back(tempPixList[gridCount].image[locIndex]);
		}
	}
	int numberOfLossDataPixels = 0;	
	/// figure out which pixels need any data // this block is not doing anything zzp 20230816
	//for (auto plotInc : range(allPlots.size())){
	//	for (auto pixelInc : range(allPlots[plotInc].getPixelNumber())){
	//		unsigned whichGrid = input->plotInfo[plotInc].whichGrid;
	//		unsigned groupNum = input->grids[whichGrid].numAtoms();
	//		for (auto groupInc : range(groupNum)){
	//			bool alreadyExists = false;
	//		}
	//	}
	//}
	/// check if have enough data to plot
	auto picNum = tempPixList[0].picStat.picNum;
	if ((picNum + 1) % allPlots[0].getPicNumber() != 0) {
		// In this case, not enough data to plot a point yet, but I've just analyzed a pic, so remove that pic.
		// wait for next picture.
		return;
	}
	for (auto plotI : range (allPlots.size ())) {
		/// Check Post-Selection Conditions
		unsigned whichGrid = input->plotInfo[plotI].whichGrid;
		unsigned groupNum = input->grids[whichGrid].numAtoms();
		std::vector<std::vector<bool> > satisfiesPsc (allPlots[plotI].getDataSetNumber (), 
			std::vector<bool> (groupNum, true));
		if (allPlots[plotI].getPlotType () == "Pixel_Count_Histograms") {
			// This is done in the different slot. Should review this chunck of code to make this more efficient and 
			// not do anything 
			auto res = handlePlotHist (allPlots[plotI], countData[whichGrid], 
									   satisfiesPsc, histogramData[plotI], dataContainers[plotI], groupNum);
			if (res.size () != 0) {
				emit newPlotData (res, plotI);
			}
		}
	}
	/// clear data: all pixels being recorded, not pixels in a data set.
	for (auto gridCount : range (input->grids.size ())) {
		unsigned groupNum = input->grids[gridCount].numAtoms();
		for (auto pixelI : range (groupNum)) {
			countData[gridCount][pixelI].clear ();
		}
	}
}

void AnalysisThreadWorker::init (){
	// make vector of plot information classes.
	for (auto plotInc : range (input->plotInfo.size ())) {
		std::string tempFileName = PLOT_FILES_SAVE_LOCATION + "\\" + input->plotInfo[plotInc].name + "."
			+ DataAnalysisControl::PLOTTING_EXTENSION;
		allPlots.push_back (PlottingInfo::PlottingInfo (tempFileName));
		allPlots[plotInc].setGroups (std::vector<coordinate>());
	}
	if (allPlots.size () == 0) {
		// no plots to run so just quit.
		return;
	}
	/// check pictures per experiment
	for (auto plotInc : range (allPlots.size ())) {
		if (allPlots[0].getPicNumber () != allPlots[plotInc].getPicNumber ()) {
			// I think I removed this because this subthread can't create gui objects. 
			// errBox ("ERROR: Number of pictures per experiment don't match between plots. The plotting thread will "
			//	"close.");
			return;
		}
	}
	int numberOfLossDataPixels = 0;
	/// figure out which pixels need any data
	for (auto plotInc : range (allPlots.size ())) {
		for (auto pixelInc : range (allPlots[plotInc].getPixelNumber ())) {
			unsigned whichGrid = input->plotInfo[plotInc].whichGrid;
			unsigned groupNum = input->grids[whichGrid].numAtoms();
			for (auto groupInc : range (groupNum)) {
				bool alreadyExists = false;
			}
		}
	}
	/// Initialize Arrays for data. 
	countData.resize(input->grids.size ());
	atomPresentData.resize(input->grids.size ());
	for (auto gridCount : range (input->grids.size ())) {
		unsigned groupNum = input->grids[gridCount].numAtoms();
		countData[gridCount] = std::vector<std::vector<double>> (groupNum);
		atomPresentData[gridCount] = std::vector<std::vector<int>> (groupNum);
	}

	//finalHistData.resize (allPlots.size ());
	histogramData.resize (allPlots.size ());
	//finalCountData.resize(allPlots.size ());
	dataContainers.resize (allPlots.size ());
	finalAtomData.resize(allPlots.size ());
	//finalAvgs.resize (allPlots.size ()); 
	//finalErrorBars.resize (allPlots.size ());
	//finalXVals .resize(allPlots.size ());
	for (auto plotInc : range (allPlots.size ())) {
		unsigned datasetNumber = allPlots[plotInc].getDataSetNumber ();
		histogramData[plotInc].resize (datasetNumber);
		//finalCountData[plotInc].resize (datasetNumber);
		finalAtomData[plotInc].resize (datasetNumber);
		//finalAvgs[plotInc].resize (datasetNumber);
		//finalErrorBars[plotInc].resize (datasetNumber);
		//finalXVals[plotInc].resize (datasetNumber);
		for (auto dataSetInc : range (datasetNumber)) {
			unsigned whichGrid = input->plotInfo[plotInc].whichGrid;
			unsigned groupNum = input->grids[whichGrid].numAtoms();
			histogramData[plotInc][dataSetInc].resize (groupNum);
			//finalCountData[plotInc][dataSetInc].resize (groupNum);
			//finalAvgs[plotInc][dataSetInc].resize (groupNum);
			finalAtomData[plotInc][dataSetInc].resize (groupNum);
			//finalErrorBars[plotInc][dataSetInc].resize (groupNum);
			//finalXVals[plotInc][dataSetInc].resize (groupNum);
			for (auto varInc : range(groupNum)) {
				finalAtomData[plotInc][dataSetInc][varInc].resize(input->variations);
			}
		}
	}
}

void AnalysisThreadWorker::setXpts (std::vector<double> newXpts) {
	for (auto plotInc : range(dataContainers.size())) {
		auto& plotInfo = allPlots[plotInc];
		// +1 for avg
		dataContainers[plotInc].resize (input->grids[0].numAtoms()+1);
		for (auto& dset : dataContainers[plotInc]) {
			dset.resize (newXpts.size ());
			for (auto dpNum : range(dset.size())) {
				dset[dpNum].x = newXpts[dpNum];
			}
		}
	}
	xvals = newXpts;
}


std::vector<std::vector<dataPoint>> AnalysisThreadWorker::handlePlotAtoms (PlottingInfo plotInfo, picureStatus picStat,
	std::vector<std::vector<std::vector<std::pair<double, unsigned long>>>>& finData,
	std::vector<std::vector<dataPoint>>& dataContainers,
	std::vector<std::vector<bool>>& pscSatisfied,
	std::vector<std::vector<int> > atomPresent, unsigned groupNum) {

	//if (picStat.picNum % input->picsPerVariation == plotInfo.getPicNumber ()) {
	//	// first pic of new variation, so need to update x vals.
	//	finData = std::vector<std::vector<std::pair<double, unsigned long>>> (plotInfo.getDataSetNumber (),
	//		std::vector<std::pair<double, unsigned long>> (groupNum, { 0,0 }));
	//}
	auto variationNum = picStat.varNum;
	/// Check Data Conditions
	for (auto dataSetI : range (plotInfo.getDataSetNumber ())) {
		for (auto groupI : range (groupNum)) {
			if (pscSatisfied[dataSetI][groupI] == false) {
				// no new data.
				continue;
			}
			bool dataVal = true;
			for (auto pixelI : range (plotInfo.getPixelNumber ())) {
				for (auto picI : range (plotInfo.getPicNumber ())) {
					// check if there is a condition at all
					int truthCondition = plotInfo.getResultCondition (dataSetI, pixelI, picI);
					if (truthCondition == 0) {
						continue;
					}
					if (truthCondition == 1 && atomPresent[groupI][picI] != 1) {
						dataVal = false;
					}
					else if (truthCondition == -1 && atomPresent[groupI][picI] != 0) {
						dataVal = false;
					}
				}
			}
			finData[dataSetI][groupI][variationNum].first += dataVal;
			finData[dataSetI][groupI][variationNum].second++;
		}
	}
	// Core data structures have been updated. 
	if (dataContainers.size () == 0) {
		return {};
	}
	/// Calculate averages and standard devations for Data sets AND groups...
	for (auto dataSetI : range (plotInfo.getDataSetNumber ())) {
		unsigned avgId = dataContainers.size () - dataSetI - 1;
		for (auto groupI : range (groupNum)) {
			// Will be function fo groupI and dataSetI; TBD			
			unsigned dataId = (dataSetI + 1) * groupI;
			// calculate new data points
			double mean = finData[dataSetI][groupI][variationNum].first / finData[dataSetI][groupI][variationNum].second;
			double error = mean * (1 - mean) / std::sqrt (finData[dataSetI][groupI][variationNum].second);
			dataContainers[dataId][variationNum].y = mean;
			dataContainers[dataId][variationNum].err = error;
		}
		/// calculate averages for all atoms
		std::pair<double, unsigned long> allDataTempNew (0, 0);
		for (auto data : finData[dataSetI]) {
			allDataTempNew.first += data[variationNum].first;
			allDataTempNew.second += data[variationNum].second;
		}
		if (allDataTempNew.second == 0) {
			dataContainers[avgId][variationNum].y = 0;
			dataContainers[avgId][variationNum].err = 0;
		}
		else {
			double mean = allDataTempNew.first / allDataTempNew.second;
			double error = mean * (1 - mean) / std::sqrt (allDataTempNew.second);
			dataContainers[avgId][variationNum].y = mean;
			dataContainers[avgId][variationNum].err = error;
		}
	}
	return dataContainers;
}

std::vector<std::vector<dataPoint>> AnalysisThreadWorker::handlePlotHist (PlottingInfo plotInfo, std::vector<std::vector<double>> countData,
	std::vector<std::vector<bool>> pscSatisfied,
	std::vector<std::vector<std::map<int, std::pair<int, unsigned long>>>>& histData,
	std::vector<std::vector<dataPoint>>& dataContainers, unsigned groupNum) {
	/// options are fundamentally different for histograms.
	// load pixel counts
	for (auto dataSetI : range (plotInfo.getDataSetNumber ())) {
		for (auto groupI : range (groupNum)) {
			if (pscSatisfied[dataSetI][groupI] == false) {
				// no new data.
				continue;
			}
			double binWidth = plotInfo.getDataSetHistBinWidth (dataSetI);
			for (auto pixelI : range (plotInfo.getPixelNumber ())) {
				for (auto picI : range (plotInfo.getPicNumber ())) {
					// check if there is a condition at all
					if (plotInfo.getResultCondition (dataSetI, pixelI, picI)) {
						//int index = -int (plotInfo.getPicNumber ()) + int (picI);
						//if (int (countData[groupI].size ()) + index < 0) {
						//	return {}; // not enough pictures yet
						//}
						//int binNum = std::round (double (countData[groupI][countData[groupI].size () + index]) / binWidth);
						int binNum = std::round (double (countData[groupI][picI]) / binWidth);
						if (histData[dataSetI][groupI].find (binNum) == histData[dataSetI][groupI].end ()) {
							// if bin doesn't exist
							histData[dataSetI][groupI][binNum] = { binNum * binWidth, 1 };
						}
						else {
							histData[dataSetI][groupI][binNum].second++;
						}
					}
				}
			}
			// find the range of bins
			int min_bin = histData[dataSetI][groupI].begin()->first;
			int max_bin = histData[dataSetI][groupI].rbegin()->first;
			/// check for empty data points and fill them with zeros.
			for (auto bin_i : range (max_bin - min_bin)) {
				auto binNum = bin_i + min_bin;
				if (histData[dataSetI][groupI].find (binNum) == histData[dataSetI][groupI].end ()) {
					// if bin doesn't exist
					histData[dataSetI][groupI][binNum] = { binNum * binWidth, 0 };
				}
			}
			// Will be function fo groupI and dataSetI; TBD			
			unsigned dataId = (dataSetI + 1) * groupI;
			// calculate new data points
			unsigned count = 0;
			if (dataContainers.size () <= dataId) {
				dataContainers.resize (dataId + 1);
			}
			double normalization = 0;
			for (auto bin_i : range(max_bin - min_bin)) {
				normalization += histData[dataSetI][groupI][bin_i + min_bin].second * binWidth;
			}
			dataContainers[dataId].resize (histData[dataSetI][groupI].size ());
			for (auto& bin : histData[dataSetI][groupI]) {
				dataContainers[dataId][count].x = bin.second.first;
				dataContainers[dataId][count].y = bin.second.second / normalization;
				dataContainers[dataId][count].err = 0;
				count++;
			}
		}
	}
	// Core data structures have been updated.
	return dataContainers;
}

void AnalysisThreadWorker::determineWhichPscsSatisfied ( PlottingInfo& info, unsigned groupSize, 
	std::vector<std::vector<int>> atomPresentData, 	std::vector<std::vector<bool>>& pscSatisfied) {
	// There's got to be a better way to iterate through these guys...
	for (auto dataSetI : range (info.getDataSetNumber ())) {
		for (auto groupI : range (groupSize)) {
			for (auto conditionI : range (info.getConditionNumber ())) {
				for (auto pixelI : range (info.getPixelNumber ())) {
					for (auto picI : range (info.getPicNumber ())) {
						// test if condition exists
						int condition = info.getPostSelectionCondition (dataSetI, conditionI, pixelI, picI);
						if (condition == 0) {
							continue;
						}
						if (condition == 1 && atomPresentData[groupI][picI] != 1) {
							pscSatisfied[dataSetI][groupI] = false;
						}
						else if (condition == -1 && atomPresentData[groupI][picI] != 0) {
							pscSatisfied[dataSetI][groupI] = false;
						}
					}
				}
			}
		}
	}
}

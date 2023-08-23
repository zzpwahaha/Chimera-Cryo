#pragma once

#include <qobject.h>
#include <string>
#include "realTimePlotterInput.h"
#include <vector>
#include <deque>
#include <Plotting/PlottingInfo.h>

class AnalysisThreadWorker : public QObject {
    Q_OBJECT
		 
    public:
        AnalysisThreadWorker (realTimePlotterInput* input_);
        ~AnalysisThreadWorker ();

    public Q_SLOTS:
        void init ();
        void handleNewPic (atomQueue atomPics); 
        void handleNewPix (PixListQueue pixlist);
        void setXpts (std::vector<double>);
    Q_SIGNALS:
        void newPlotData (std::vector<std::vector<dataPoint>>, int);

    private:
		// subroutine for handling atom & count plots
		std::vector<std::vector<dataPoint>> handlePlotAtoms (
			PlottingInfo plotInfo, picureStatus picStat, std::vector<std::vector<std::vector<std::pair<double, unsigned long>>>>& finData,
			std::vector<std::vector<dataPoint>>& dataContainers,
			std::vector<std::vector<bool>>& pscSatisfied,
			std::vector<std::vector<int> > atomPresent, unsigned groupNum);
		std::vector<std::vector<dataPoint>> handlePlotHist (
			PlottingInfo plotInfo, std::vector<std::vector<double>> countData,
			std::vector<std::vector<bool>>pscSatisfied,
			std::vector<std::vector<std::map<int, std::pair<int, unsigned long>>>>& histData,
			std::vector<std::vector<dataPoint>>& dataContainers, unsigned groupNum);
		void determineWhichPscsSatisfied (
			PlottingInfo& info, unsigned groupSize, std::vector<std::vector<int>> atomPresentData, 
			std::vector<std::vector<bool>>& pscSatisfied);

        std::vector<double> xvals;
        realTimePlotterInput* input;
        std::vector<std::vector<std::vector<dataPoint>>> dataContainers; // [plot][atom+avg][var] -> dataPoint
        std::vector<std::vector<std::vector<double>>> countData; // [grid][atom][pic]
        std::vector<std::vector<std::vector<int>>> atomPresentData; // [grid][atom][pic]
        //std::vector<std::vector<std::vector<std::vector<long> > > > finalCountData;
        std::vector<std::vector<std::vector<std::vector<std::pair<double, unsigned long>>>>> finalAtomData; // [plot][dataset][atom][var] -> <num of positive event, total event after post selection>
        //std::vector<std::vector<std::vector<std::vector<double>>>> finalAvgs;
        // Averaged over all pixels (avgAvg is the average of averages over repetitions)
        //std::vector<std::vector<std::vector<double>>> avgAvg;
        //std::vector<std::vector<std::vector<bool> > > newData;
        //std::vector<std::vector<std::vector<std::deque<double>>>> finalHistData, finalErrorBars, finalXVals;
        std::vector<std::vector<std::vector<std::map<int, std::pair<int, unsigned long>,std::less<int>>>>> histogramData; // [plot][dataset][atom], this is sorted in ascending order, explicit specific std::less even though it is by default
        unsigned noAtomsCounter = 0, atomCounterTotal = 0, picNumberCount = 0;
        std::vector<PlottingInfo> allPlots;
};


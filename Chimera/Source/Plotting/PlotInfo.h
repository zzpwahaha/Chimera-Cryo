#pragma once
#include "Plotting/dataPoint.h"

enum class plotStyle {
	PicturePlot,
	DensityPlot,
	DensityPlotWithHisto,
	// ttl and dac plot use steps.
	TtlPlot,
	DacPlot,
	// uses circs and error bars to represent data and errors
	BinomialDataPlot,
	GeneralErrorPlot,
	CalibrationPlot,
	// uses bars for histograms
	HistPlot,
	VertHist,
	// 
	OscilloscopePlot
};


typedef std::vector<dataPoint> plotDataVec;
Q_DECLARE_METATYPE(std::vector<std::vector<plotDataVec>>)

struct plotMinMax {
	double min_x, min_y, max_x, max_y;
};


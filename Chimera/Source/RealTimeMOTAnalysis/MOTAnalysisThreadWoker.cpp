#include "stdafx.h"
#include "MOTAnalysisThreadWoker.h"
//#include <RealTimeMOTAnalysis/MOTAnalysisControl.h>
#include <CMOSCamera/GaussianFit.h>
#include <qtconcurrentrun.h>

MOTAnalysisThreadWoker::MOTAnalysisThreadWoker(MOTThreadInput input_)
	: input(input_)
{

}

void MOTAnalysisThreadWoker::init()
{
	for (auto type : MOTAnalysisType::allTypes) {
		if (type != MOTAnalysisType::type::density2d) {
			result1d.insert({ type,std::vector<double>() });
			result1d[type].reserve(input.camSet.totalPictures());
		}
	}
	density2d = std::vector<std::vector<double>>(input.camSet.variations, std::vector<double>());
	for (auto vec : density2d) {
		vec.reserve(input.camSet.dims.size());
	}
}

void MOTAnalysisThreadWoker::handleNewImg(QVector<double> img, int width, int height, size_t currentNum)
{	
	resultOrder.insert({ currentNum, result1d[MOTAnalysisType::type::min].size() });
	size_t var = currentNum % input.camSet.variations;
	size_t rep = currentNum / input.camSet.variations;

	//do calculation
	auto it = std::minmax_element(img.begin(), img.end());
	result1d[MOTAnalysisType::type::min].push_back(*(it.first));
	result1d[MOTAnalysisType::type::max].push_back(*(it.second));
	
	std::vector<double> CrxX = std::vector<double>(width, 0.0);
	for (size_t idx = 0; idx < width; idx++) {
		double tmp = 0.0;
		for (size_t j = 0; j < height; j++) {
			tmp += img[idx + j * width]; // sum over Y
		}
		CrxX[idx] = tmp;
	}
	std::vector<double> CrxY = std::vector<double>(height, 0.0);
	for (size_t idx = 0; idx < height; idx++) {
		CrxY[idx] = std::accumulate(img.begin() + idx * width, img.begin() + (idx + 1) * width, 0.0); // sum over X
	}

	QFuture<std::vector<double>> futurex = QtConcurrent::run(this, &MOTAnalysisThreadWoker::fit1dGaussian, CrxX);
	QFuture<std::vector<double>> futurey = QtConcurrent::run(this, &MOTAnalysisThreadWoker::fit1dGaussian, CrxY);
	std::vector<double> fitx = futurex.result();
	std::vector<double> fity = futurex.result();
	result1d[MOTAnalysisType::type::meanx].push_back(fitx[0]);
	result1d[MOTAnalysisType::type::sigmax].push_back(fitx[2]);
	result1d[MOTAnalysisType::type::meany].push_back(fity[0]);
	result1d[MOTAnalysisType::type::sigmay].push_back(fity[2]);

	double sum = std::accumulate(img.begin(), img.end(), 0.0);
	sum -= img.size() * *(it.first);
	result1d[MOTAnalysisType::type::atomNum].push_back(sum);
	// perform average for 2d density
	if (density2d[var].empty()) {
		density2d[var] = img.toStdVector();
	}
	else {
		auto& den = density2d[var];
		std::transform(den.begin(), den.end(), img.begin(), den.begin(), [rep](double old, double n3w) {
			return (old * (rep - 1) + n3w) / (rep * 1.0); });
	}
	emit newPlotData2D(density2d[var], currentNum);

	//check if one variation is finished and is able to do statistics for 1d result
	std::vector<size_t> trueIdx;
	for (size_t idx = 0; idx < currentNum / input.camSet.variations; idx++) {
		trueIdx.push_back(resultOrder[var + input.camSet.variations * idx]);
	}
	std::vector<double> mean;
	for (auto& [key, val] : result1d) {
		double tmp = 0.0;
		for (auto trueid : trueIdx) {
			tmp += val[trueid];
		}
		if (trueIdx.empty()) {
			tmp += val[0];
		}
		mean.push_back(tmp / (trueIdx.size() + 1));
	}
	if (currentNum / input.camSet.variations) { // the very first round is finished, able to do statistics
		std::vector<double> stdev;
		for (size_t idx = 0; idx < result1d.size(); idx++)
		{
			double tmp = 0.0;
			for (auto trueid : trueIdx) {
				tmp += (result1d[MOTAnalysisType::allTypes[idx]][trueid] - mean[idx])
					* (result1d[MOTAnalysisType::allTypes[idx]][trueid] - mean[idx]);
			}
			stdev.push_back(sqrt( tmp / (trueIdx.size()) ));
		}
		emit newPlotData1D(mean, stdev, currentNum);
	}
	else {
		emit newPlotData1D(mean, std::vector<double>(mean.size(), 0.0), currentNum);
	}


	if (currentNum == input.camSet.totalPictures()) {
		emit finished();
	}
}

void MOTAnalysisThreadWoker::aborting()
{
	emit finished();
}

std::vector<double> MOTAnalysisThreadWoker::fit1dGaussian(std::vector<double> Crx)
{
	int width = Crx.size();
	std::vector<double> CrxKey = std::vector<double>(width, 0.0);
	std::generate(CrxKey.begin(), CrxKey.end(), [n = 0.0]() mutable { return n++; }); // 0,1,2,..., height
	auto [xmin_it, xmax_it] = std::minmax_element(Crx.begin(), Crx.end());
	double a0x = *xmax_it - *xmin_it;
	double b0x = CrxKey.at(xmax_it - Crx.begin());/*although this return a const reference, can nontheless force a copy ctor to get a copy of the returned value*/
	double c0x = 0.5 * width;
	double d0x = *xmin_it;
	/* model function: a * exp( -1/2 * [ (t - b) / c ]^2 ) + d */
	Gaussian1DFit fit(width, CrxKey.data(), Crx.data(), a0x, b0x, c0x, d0x);
	fit.solve_system();
	QVector<double> fitParax = fit.fittedPara();
	QVector<double> confi95x = fit.confidence95Interval();
	return std::vector<double>({ fitParax[1],confi95x[1],fitParax[2],confi95x[2] });

}
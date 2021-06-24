#pragma once
#include <qobject.h>
#include <map>
#include <RealTimeMOTAnalysis/MOTThreadInput.h>
#include <RealTimeMOTAnalysis/MOTAnalysisType.h>
#include <Plotting/dataPoint.h>


class MOTAnalysisThreadWoker : public QObject
{
	Q_OBJECT

public:
    MOTAnalysisThreadWoker(MOTThreadInput input_);
    ~MOTAnalysisThreadWoker() {};


public slots:
    void init();
    void handleNewImg(QVector<double> img, int width, int height, size_t currentNum);
    void aborting();
    //void handleNewPic(atomQueue atomPics);
    //void handleNewPix(PixListQueue pixlist);
    //void setXpts(std::vector<double>) {};
signals:
    void newPlotData1D(std::vector<double> val, std::vector<double> stdev, int currentNum);
    void newPlotData2D(std::vector<double> val, int width, int height, int currentNum);
    void finished(); //for ending the thread

private:
    //return mean, confident95 of mean, std, confident95 of std
    std::vector<double> fit1dGaussian(std::vector<double> Crx);

public:

private:
    MOTThreadInput input;
    std::vector<std::vector<double>> currentImg;
    std::vector<double> xpts;

    std::map<size_t, size_t> resultOrder; // first is the result order, second is 0,1,2,...
    std::map<MOTAnalysisType::type, std::vector<double>> result1d;
    std::vector<std::vector<double>> density2d;
    std::vector<double> fitConfid95; // meanx, meany, widthx, widthy


};

Q_DECLARE_METATYPE(std::vector<double>)
#pragma once
#include <qobject.h>
#include <map>
#include <RealTimeMOTAnalysis/MOTThreadInput.h>
#include <RealTimeMOTAnalysis/MOTAnalysisType.h>
#include <Plotting/dataPoint.h>

// can only handle one image, 1D scan for now
class MOTAnalysisThreadWoker : public QObject
{
	Q_OBJECT

public:
    MOTAnalysisThreadWoker(MOTThreadInput input_);
    ~MOTAnalysisThreadWoker() {};


public slots:
    void init();
    void handleNewImg(QVector<double> img, int width, int height, size_t rep, size_t var);
    void aborting();
    //void handleNewPic(atomQueue atomPics);
    //void handleNewPix(PixListQueue pixlist);
    //void setXpts(std::vector<double>) {};
signals:
    void newPlotData1D(std::vector<double> val, std::vector<double> stdev, int var);
    void newPlotData2D(std::vector<double> val, int width, int height, int var);
    void finished(); //for ending the thread
    void error(QString msg, unsigned errorLevel = 0);

private:
    //return mean, confident95 of mean, std, confident95 of std
    std::vector<double> fit1dGaussian(std::vector<double> Crx);

public:

private:
    MOTThreadInput input;
    std::vector<std::vector<double>> currentImg;
    std::vector<double> xpts;

    //std::map<size_t, size_t> resultOrder; // first is the result order, second is 0,1,2,...
    // the vector index is the variation list: var0, var1, var2, ... in the ascending order, not affected by the randomize variation option
    // the vector content is the repetition counter
    //std::vector<size_t> resultCounter; 
    //std::map<MOTAnalysisType::type, std::vector<double>> result1d; // this 1d means 1d scan

    // the first vector index is the variation list: var0, var1, var2, ... in the ascending order, will change the order if randomize variation option is true
    // the second vector content is the result from one particular repetition
    std::map<MOTAnalysisType::type, std::vector<std::vector<double>>> result2d; // this 2d means 2d store stureture for 1d scan result
    std::vector<std::vector<double>> density2d;
    std::vector<double> fitConfid95; // meanx, meany, widthx, widthy


};

Q_DECLARE_METATYPE(std::vector<double>)
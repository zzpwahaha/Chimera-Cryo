#pragma once
#include <GeneralObjects/IChimeraSystem.h>
#include <RealTimeMOTAnalysis/MOTAnalysisThreadWoker.h>
#include <RealTimeMOTAnalysis/MOTAnalysisType.h>
#include <ConfigurationSystems/ConfigStream.h>
#include <Plotting/QCustomPlotCtrl.h>
#include <map>




class MakoCamera;
class CQComboBox;
class CQCheckBox;
class QCustomPlot;

class MOTAnalysisControl : public IChimeraSystem
{
public:
	MOTAnalysisControl(IChimeraQtWindow* parent);
	MOTAnalysisControl(const MOTAnalysisControl&) = delete;
	void initialize(IChimeraQtWindow* parent);
	void handleOpenConfig(ConfigStream& file) {};
	void handleSaveConfig(ConfigStream& file) {};
	void updateXYKeys();

	void prepareMOTAnalysis(MakoCamera*& cam); // for experiment

public slots:
	void handleNewPlotData1D(std::vector<double> val, std::vector<double> stdev, int var); // all in MOTAnalysisType except density2d
	void handleNewPlotData2D(std::vector<double> val, int width, int height, int var); // now only for density2d

private:
	void updatePlotData1D();

public:
	MOTAnalysisThreadWoker* MOTCalc;
	//std::atomic<bool> calcThreadActive = false;
	//std::atomic<bool> calcThreadAborting = false;
	std::map<MOTAnalysisType::type, QCustomPlotCtrl> calcViewer;

private:
	MakoCamera* makoCam = nullptr;
	QCheckBox* calcActive;
	CQComboBox* MOTCalcCombo;
	CQComboBox* makoSrcCombo;
	CQCheckBox* twoDScanActive;
	CQComboBox* xKeyCombo;
	CQComboBox* yKeyCombo;
	CQComboBox* xKeyValCombo; // now only used for density2d
	CQComboBox* yKeyValCombo; // now only used for density2d

	QCustomPlot* visiblePlot;

	std::vector<parameterType> xKeysList;
	std::vector<parameterType> yKeysList;

	std::vector<double> currentXKeys;
	std::vector<double> currentYKeys;
	std::map<MOTAnalysisType::type, std::vector<double>> plotData1D;
	std::map<MOTAnalysisType::type, std::vector<double>> plotStdev1D;
	std::vector<std::vector<double>> plotData2D;
	int width2D = 0;
	int height2D = 0;
	std::vector<size_t> incomeOrder;
};


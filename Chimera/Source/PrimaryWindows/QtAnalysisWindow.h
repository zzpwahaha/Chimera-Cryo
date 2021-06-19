#pragma once
#include "IChimeraQtWindow.h"
#include "ExperimentThread/ExperimentThreadInput.h"
#include <RealTimeMOTAnalysis/MOTAnalysisSystem.h>


namespace Ui {
	class QtAnalysisWindow;
}

class QtAnalysisWindow : public IChimeraQtWindow
{
	Q_OBJECT
public:
	explicit QtAnalysisWindow(QWidget* parent = nullptr);
	~QtAnalysisWindow() {};
	std::string getSystemStatusString();
	void windowOpenConfig(ConfigStream& configFile) override;
	void windowSaveConfig(ConfigStream& configFile) override;
	void fillExpDeviceList(DeviceList& list) override;
	void initializeWidgets() override;
	//MOTAnalysisControl

public slots:
	void prepareCalcForAcq();

private:



public:
	

private:
	Ui::QtAnalysisWindow* ui;
	MOTAnalysisSystem MOTAnalySys;


};

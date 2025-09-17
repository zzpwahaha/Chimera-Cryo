#pragma once
#include "IChimeraQtWindow.h"
#include "ExperimentThread/ExperimentThreadInput.h"
#include <RealTimeMOTAnalysis/MOTAnalysisSystem.h>
#include <ExperimentMonitoringAndStatus/ExperimentSeqPlotter.h>
#include <StaticAnalogOutput/StaticAoSystem.h>
#include <StaticDirectDigitalSynthesis/StaticDdsSystem.h>
#include <StaticDirectAnalogSynthesis/StaticDasSystem.h>
#include <ElliptecRotationStage/ElliptecSystem.h>
#include <Microwave/MicrowaveSystem.h>


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
	void fillMasterThreadInput(ExperimentThreadInput* input) override {};
	//MOTAnalysisControl
	//StaticDdsSystem& getStaticDds() { return staticDds; };
	StaticDasSystem& getStaticDas() { return staticDas; };

public slots:
	void prepareCalcForAcq();

private:



public:
	MOTAnalysisSystem MOTAnalySys;
	ExperimentSeqPlotter SeqPlotter;
	StaticAoSystem staticDac;
	//StaticDdsSystem staticDds;
	StaticDasSystem staticDas;
	ElliptecSystem elliptec;
	MicrowaveSystem mwSys2;

private:
	Ui::QtAnalysisWindow* ui;


};


#pragma once
#include <GeneralObjects/IChimeraSystem.h>
#include <RealTimeMOTAnalysis/MOTAnalysisControl.h>
#include <RealTimeMOTAnalysis/MOTAnalysisThreadWoker.h>

class MakoCamera;

class MOTAnalysisSystem : public IChimeraSystem
{
public:
	MOTAnalysisSystem(IChimeraQtWindow* parent);
	void initialize();
	void handleOpenConfig(ConfigStream& file) {};
	void handleSaveConfig(ConfigStream& file) {};
	void prepareMOTAnalysis();

	static const unsigned MOTCALCTRL_NUM = 3;

private:
	void prepareMOTAnalysis(int idx); // for all MOTAnalysisThreadWoker

public:
	//std::array<MOTAnalysisControl, MOTCALCTRL_NUM> MOTCalcCtrl;
	std::vector<std::reference_wrapper<MOTAnalysisControl>> MOTCalcCtrl;
	MOTAnalysisControl MOTCalcCtrl1;
	MOTAnalysisControl MOTCalcCtrl2;
	MOTAnalysisControl MOTCalcCtrl3;
	std::array<MOTAnalysisThreadWoker*, MAKO_NUMBER> MOTCalcWkr;
	

private:
	std::vector<MakoCamera*> camExp;



};


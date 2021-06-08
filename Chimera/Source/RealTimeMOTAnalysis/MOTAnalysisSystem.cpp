#include "stdafx.h"
#include "MOTAnalysisSystem.h"
#include <CMOSCamera/MakoCamera.h>
#include <PrimaryWindows/QtMainWindow.h>

MOTAnalysisSystem::MOTAnalysisSystem(IChimeraQtWindow* parent)
	: IChimeraSystem(parent)
	, MOTCalcCtrl{ { MOTAnalysisControl(parent),MOTAnalysisControl(parent),MOTAnalysisControl(parent) } }
	//, MOTCalcCtrl1(parent)
	//, MOTCalcCtrl2(parent)
	//, MOTCalcCtrl3(parent)
{
	//MOTCalcCtrl.resize(1);
	//MOTCalcCtrl.push_back(MOTCalcCtrl1);
	//MOTCalcCtrl.push_back(MOTCalcCtrl2);
	//MOTCalcCtrl.push_back(MOTCalcCtrl3);
}

void MOTAnalysisSystem::initialize()
{
	for (auto& calcCtrl : MOTCalcCtrl) {
		calcCtrl.initialize(this->parentWin);
	}
	//MOTCalcCtrl1.initialize(this->parentWin);
	//MOTCalcCtrl2.initialize(this->parentWin);
	//MOTCalcCtrl3.initialize(this->parentWin);
}

void MOTAnalysisSystem::prepareMOTAnalysis()
{
	camExp.clear(); // find the exp active MOT camera that requires analysis
	std::array<MakoCamera*, MOTCALCTRL_NUM> camPts;
	for (unsigned idx = 0; idx < MOTCALCTRL_NUM; idx++) {
		MOTCalcCtrl[idx].prepareMOTAnalysis(camPts[idx]);
	}
	for (auto pt : camPts) {
		if (pt != nullptr && std::find(camExp.begin(), camExp.end(), pt) == camExp.end()) {
			camExp.push_back(pt);
			emit notification("get MAKO as" + qstr(pt));
		}
	}
	if (camExp.size() > MAKO_NUMBER) {
		thrower("MOT analysis Woker size greater than MAKO number, a low level bug");
	}
	for (size_t idx = 0; idx < camExp.size(); idx++) {
		prepareMOTAnalysis(idx);
	}

}

void MOTAnalysisSystem::prepareMOTAnalysis(int idx)
{
	MOTAnalysisThreadWoker* MOTCalc = MOTCalcWkr[idx];
	MakoCamera* makoCam = camExp[idx];
	MOTAnalysisControl& MOTCtrl = MOTCalcCtrl[idx];

	//calcThreadActive = true;
	//calcThreadAborting = false;
	makoCam->setMOTCalcActive(true);
	MakoSettings mkSet = makoCam->getMakoCore().getRunningSettings();
	MOTThreadInput calcInput;
	//calcInput.aborting = &calcThreadAborting;
	//calcInput.active = &calcThreadActive;
	calcInput.camSet = mkSet;

	MOTCalc = new MOTAnalysisThreadWoker(calcInput);
	QThread* thread = new QThread;
	MOTCalc->moveToThread(thread);

	connect(thread, &QThread::started, MOTCalc, &MOTAnalysisThreadWoker::init);
	connect(thread, &QThread::finished, thread, &QThread::deleteLater);
	connect(MOTCalc, &MOTAnalysisThreadWoker::finished, thread, &QThread::quit);
	connect(MOTCalc, &MOTAnalysisThreadWoker::finished, MOTCalc, &MOTAnalysisThreadWoker::deleteLater);
	connect(this->parentWin->mainWin->getExpThread(), &QThread::finished, MOTCalc, &MOTAnalysisThreadWoker::aborting);

	connect(makoCam, &MakoCamera::imgReadyForAnalysis, MOTCalc, &MOTAnalysisThreadWoker::handleNewImg);
	connect(MOTCalc, &MOTAnalysisThreadWoker::newPlotData1D, &MOTCtrl, &MOTAnalysisControl::handleNewPlotData1D);
	connect(MOTCalc, &MOTAnalysisThreadWoker::newPlotData2D, &MOTCtrl, &MOTAnalysisControl::handleNewPlotData2D);

	thread->start();

}

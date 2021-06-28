#include "stdafx.h"
#include "QtAnalysisWindow.h"
#include <PrimaryWindows/QtScriptWindow.h>
#include <PrimaryWindows/QtAndorWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>
#include <PrimaryWindows/QtMainWindow.h>
#include <ExperimentMonitoringAndStatus/colorbox.h>


QtAnalysisWindow::QtAnalysisWindow(QWidget* parent) 
	: IChimeraQtWindow(parent)
	, MOTAnalySys(this)
	, SeqPlotter(this)
{
	setWindowTitle("Analysis Window");
}


std::string QtAnalysisWindow::getSystemStatusString()
{
	return std::string();
}

void QtAnalysisWindow::windowOpenConfig(ConfigStream& configFile)
{
}

void QtAnalysisWindow::windowSaveConfig(ConfigStream& configFile)
{
}

void QtAnalysisWindow::fillExpDeviceList(DeviceList& list)
{
}

void QtAnalysisWindow::initializeWidgets()
{
	statBox = new ColorBox(this, mainWin->getDevices());
	QWidget* centralWidget = new QWidget(this);
	setCentralWidget(centralWidget);

	QHBoxLayout* layout = new QHBoxLayout(centralWidget);
	QVBoxLayout* layoutMOT = new QVBoxLayout(this);
	layoutMOT->setContentsMargins(0, 0, 0, 0);
	MOTAnalySys.initialize();
	for (auto& p : MOTAnalySys.MOTCalcCtrl) {
		layoutMOT->addWidget(&p, 0);
	}
	layoutMOT->addStretch(1);
	layout->addLayout(layoutMOT);

	SeqPlotter.initialize(this);
	QVBoxLayout* layoutSeq = new QVBoxLayout(this);
	layoutSeq->setContentsMargins(0, 0, 0, 0);
	for (auto* p : SeqPlotter.aoPlots) {
		layoutSeq->addWidget(p->plot);
	}
	for (auto* p : SeqPlotter.ttlPlots) {
		layoutSeq->addWidget(p->plot);
	}
	for (auto* p : SeqPlotter.olPlots) {
		layoutSeq->addWidget(p->plot);
	}
	layout->addLayout(layoutSeq);

	layout->addStretch(1);
}

void QtAnalysisWindow::prepareCalcForAcq()
{
	MOTAnalySys.prepareMOTAnalysis();
}
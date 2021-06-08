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
	MOTAnalySys.initialize();
	layout->addWidget(&MOTAnalySys.MOTCalcCtrl[0].get(), 0);
	layout->addWidget(&MOTAnalySys.MOTCalcCtrl[1].get(), 0);
	layout->addWidget(&MOTAnalySys.MOTCalcCtrl[2].get(), 0);
}

void QtAnalysisWindow::prepareCalcForAcq()
{
	MOTAnalySys.prepareMOTAnalysis();
}
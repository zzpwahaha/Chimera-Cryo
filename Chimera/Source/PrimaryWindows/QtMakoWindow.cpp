#include "stdafx.h"
#include "QtMakoWindow.h"
#include <qdesktopwidget.h>
#include <PrimaryWindows/QtScriptWindow.h>
#include <PrimaryWindows/QtAndorWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>
#include <PrimaryWindows/QtMainWindow.h>
#include "ExperimentMonitoringAndStatus/colorbox.h"
#include <ExcessDialogs/saveWithExplorer.h>
#include <ExcessDialogs/openWithExplorer.h>
#include <qlayout.h>

QtMakoWindow::QtMakoWindow(QWidget* parent)
	: IChimeraQtWindow(parent)
	, cam{ MakoCamera(camInfo1, this),
	MakoCamera(camInfo2, this) }
{
	setWindowTitle("Mako Camera Window");
}

QtMakoWindow::~QtMakoWindow()
{
}

void QtMakoWindow::initializeWidgets()
{
	statBox = new ColorBox(this, mainWin->getDevices());
	QWidget* centralWidget = new QWidget(this);
	setCentralWidget(centralWidget);

	QHBoxLayout* layout = new QHBoxLayout(centralWidget);
	for (auto& camera : cam) {
		camera.initialize();
		camera.setParent(this);
	}

	layout->addWidget(&cam[0], 1);
	layout->addWidget(&cam[1], 1);
}

void QtMakoWindow::windowOpenConfig(ConfigStream& configFile)
{
	for (auto& camera : cam) {
		MakoSettings settings;
		ConfigSystem::stdGetFromConfig(configFile, camera.getMakoCore(), settings, Version("1.0"));
		try {
			camera.getMakoCore().getMakoCtrl().setSettings(settings);
			camera.updateStatusBar();
		}
		catch(ChimeraError& e) {
			errBox("Error in open config for MAKO window\n" + str(e.trace()));
		}
	}
}

void QtMakoWindow::windowSaveConfig(ConfigStream& newFile)
{
	for (auto& camera : cam) {
		try {
			camera.getMakoCore().getMakoCtrl().handleSavingConfig(newFile, camera.getMakoCore().getDelim());
		}
		catch (ChimeraError& e) {
			errBox("Error in open config for MAKO window\n" + str(e.trace()));
		}
	}
	
}

void QtMakoWindow::fillExpDeviceList(DeviceList& list)
{
	for (auto& camera : cam) {
		list.list.push_back(camera.getMakoCore());
	}
}

std::string QtMakoWindow::getSystemStatusString()
{
	std::string msg;
	msg += "CMOS System:\n";
	for (unsigned idx = 0; idx < MAKO_NUMBER; idx++)
	{
		if (MAKO_SAFEMODE[idx]) {
			msg += "\tCMOS camera at " + MAKO_IPADDRS[idx] + ": is in safemode. Enable in \"constants.h\" \r\n";
		}
		else {
			msg += "\tCMOS camera at " + MAKO_IPADDRS[idx] + ": " + cam[idx].getMakoCore().CameraName() + " is running \r\n";
			msg += "\tAttached trigger line is \n\t\t";
			{
				msg += "(" + str(MAKO_TRIGGER_LINE[idx].first) + "," + str(MAKO_TRIGGER_LINE[idx].second) + ") ";
			}
			msg += "\n";
		}
	}
	
	return msg;
}

void QtMakoWindow::CMOSChkFinished()
{
	bool allfinished = true;
	for (auto& camera : cam) {
		allfinished = allfinished && (!camera.isExpStillRunning());
	}
	if (!andorWin->cameraIsRunning() && allfinished) {
		// else it will close when the mako camera finishes.
		andorWin->getLogger().normalCloseFile();
	}
}

//Qt::BlockingQueuedConnection, expthread will continue after this call returns
void QtMakoWindow::prepareWinForAcq(MakoSettings* , CameraInfo info) {
	try {
		for (auto& camera : cam) {
			if (info.camName == camera.getCameraInfo().camName) {
				camera.prepareForExperiment();
			}
		}
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}
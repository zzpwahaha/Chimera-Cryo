#include "stdafx.h"
#include "CommandModulator.h"
#include "PrimaryWindows/IChimeraQtWindow.h"
#include "PrimaryWindows/QtMainWindow.h"
#include "PrimaryWindows/QtAndorWindow.h"
#include "PrimaryWindows/QtAuxiliaryWindow.h"
#include "PrimaryWindows/QtMakoWindow.h"
#include "PrimaryWindows/QtScriptWindow.h"
#include <GeneralUtilityFunctions/commonFunctions.h>
#include <algorithm>

CommandModulator::CommandModulator(IChimeraSystem* parentSys):
	QObject(parentSys)
{}

void CommandModulator::initialize(IChimeraQtWindow* win)
{
	mainWin = win->mainWin;
	andorWin = win->andorWin;
	scriptWin = win->scriptWin;
	auxWin = win->auxWin;
	makoWin1 = win->makoWin1;
	makoWin2 = win->makoWin2;
}

void CommandModulator::openConfiguration(QString addressName, ErrorStatus& status)
{
	auto filePath = convertToUnixPath(CONFIGURATION_PATH + str(addressName));
	mainWin->profile.openConfigFromPath(filePath, mainWin);
}

void CommandModulator::openMasterScript(QString addressName, ErrorStatus& status)
{
	try {
		status.error = false;
		auto filePath = convertToUnixPath(CONFIGURATION_PATH + str(addressName));
		scriptWin->openMasterScript(filePath, false);
	}
	catch (ChimeraError& err) {
		mainWin->reportErr(err.qtrace());
		status.error = true;
		status.errorMsg = err.trace();
	}
}

void CommandModulator::saveAll(ErrorStatus& status)
{
	try {
		status.error = false;
		scriptWin->saveAllScript();
		mainWin->profile.saveConfiguration(mainWin, false);
		mainWin->masterConfig.save(mainWin, auxWin, andorWin);
	}
	catch (ChimeraError& err) {
		mainWin->reportErr(err.qtrace());
		status.error = true;
		status.errorMsg = err.trace();
	}
}

void CommandModulator::startExperiment(QString expDataName, ErrorStatus& status)
{
	try {
		status.error = false;
		AllExperimentInput input;
		if (mainWin->masterIsRunning()) {
			thrower("Experiment is still running.");
		}
		andorWin->setTimerText("Starting...");
		saveAll(status);
		commonFunctions::prepareMasterThread(0, mainWin, input, true, true, true, true);
		input.masterInput->expType = ExperimentType::Normal;

		commonFunctions::logStandard(input, andorWin->getLogger(), str(expDataName));
		commonFunctions::startExperimentThread(mainWin, input);
	
	}
	catch (ChimeraError& err) {
		mainWin->reportErr("EXITED WITH ERROR!\n " + err.qtrace());
		mainWin->reportStatus("EXITED WITH ERROR!\r\nInitialized Default Waveform\r\n");
		andorWin->setTimerText("ERROR!");
		andorWin->assertOff();
		status.error = true;
		status.errorMsg = err.trace();
	}
}

void CommandModulator::abortExperiment(bool keepData, QString dataName, ErrorStatus& status)
{
	status.error = false;
	bool andorAborted = false, masterAborted = false;
	andorWin->wakeRearranger();
	try {
		if (mainWin->expIsRunning()) {
			commonFunctions::abortMaster(mainWin);
			masterAborted = true;
		}
		andorWin->assertOff();
		andorWin->assertDataFileClosed();
	}
	catch (ChimeraError& err) {
		mainWin->reportErr("Abort Master thread exited with Error! Error Message: "
			+ err.qtrace());
		mainWin->reportStatus("Abort Master thread exited with Error!\r\n");
		andorWin->setTimerText("ERROR!");
		status.error = true;
		status.errorMsg += "\n" + err.trace();
	}

	try {
		//below is adapted commonFunctions::abortCamera(mainWin);
		if (!andorWin->cameraIsRunning()) {
			mainWin->reportErr("System was not running. Can't Abort.\r\n");
		}
		else {
			// abort acquisition if in progress
			Sleep(200); // leave time for other stuff to save into datafile
			andorWin->abortCameraRun(false);
			mainWin->reportStatus("Aborted Camera Operation.\r\n");
			andorAborted = true;

			if (!keepData) {
				try {
					andorWin->getLogger().deleteFile(str(dataName));
				}
				catch (ChimeraError& err) {
					andorWin->reportErr(qstr(err.trace()));
					status.error = true;
					status.errorMsg += "\n" + err.trace();
				}
			}
		}
	}
	catch (ChimeraError& err) {
		mainWin->reportErr("Andor Camera threw error while aborting! Error: " + err.qtrace());
		mainWin->reportStatus("Abort camera threw error\r\n");
		andorWin->setTimerText("ERROR!");
		status.error = true;
		status.errorMsg += "\n" + err.trace();
	}
	
	if (!andorAborted && !masterAborted) {
		for (auto& dev : mainWin->getDevices().list) {
			mainWin->handleColorboxUpdate("Black", qstr(dev.get().getDelim()));
		}
		mainWin->reportErr("Andor camera, Master, and Basler camera were not running. "
			"Can't Abort.\r\n");
	}
}

void CommandModulator::isExperimentRunning(bool& running, ErrorStatus& status)
{
	status.error = false;
	running = mainWin->masterIsRunning();
}

std::string CommandModulator::convertToUnixPath(std::string mixedPath)
{
	// not that the mixed syntax path wouldn't work. 
	// Just to make Chimera consistent with one type of format so that 
	// the loc of scripts etc can be determined to see if they are at the same loc as config
	
	// Replace all occurrences of '\\' with '/'
	std::replace(mixedPath.begin(), mixedPath.end(), '\\', '/');
	std::string result;
	bool lastWasSlash = false;

	for (char ch : mixedPath) {
		if (ch == '/') {
			if (!lastWasSlash) {
				result += ch;
				lastWasSlash = true;
			}
		}
		else {
			result += ch;
			lastWasSlash = false;
		}
	}
	return result;
}


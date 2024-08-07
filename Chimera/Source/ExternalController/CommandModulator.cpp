#include "stdafx.h"
#include "CommandModulator.h"
#include "PrimaryWindows/IChimeraQtWindow.h"
#include "PrimaryWindows/QtMainWindow.h"
#include "PrimaryWindows/QtAndorWindow.h"
#include "PrimaryWindows/QtAuxiliaryWindow.h"
#include "PrimaryWindows/QtMakoWindow.h"
#include "PrimaryWindows/QtScriptWindow.h"
#include <GeneralUtilityFunctions/commonFunctions.h>

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
	mainWin->profile.openConfigFromPath(CONFIGURATION_PATH + str(addressName), mainWin);
}

void CommandModulator::openMasterScript(QString addressName, ErrorStatus& status)
{
	try {
		status.error = false;
		scriptWin->openMasterScript(CONFIGURATION_PATH + str(addressName), false);
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
		scriptWin->saveMasterScript();
		mainWin->profile.saveConfiguration(mainWin, false);
		mainWin->masterConfig.save(mainWin, auxWin, andorWin);
	}
	catch (ChimeraError& err) {
		mainWin->reportErr(err.qtrace());
		status.error = true;
		status.errorMsg = err.trace();
	}
}

void CommandModulator::startExperiment(ErrorStatus& status)
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

		commonFunctions::logStandard(input, andorWin->getLogger());
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

void CommandModulator::abortExperiment(ErrorStatus& status)
{
	status.error = false;
	commonFunctions::handleCommonMessage(ID_ACCELERATOR_ESC, mainWin);
}

void CommandModulator::isExperimentRunning(bool& running, ErrorStatus& status)
{
	status.error = false;
	running = mainWin->masterIsRunning();
}


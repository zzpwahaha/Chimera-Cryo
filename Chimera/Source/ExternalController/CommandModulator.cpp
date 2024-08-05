#include "stdafx.h"
#include "CommandModulator.h"
#include "PrimaryWindows/IChimeraQtWindow.h"
#include "PrimaryWindows/QtMainWindow.h"
#include "PrimaryWindows/QtAndorWindow.h"
#include "PrimaryWindows/QtAuxiliaryWindow.h"
#include "PrimaryWindows/QtMakoWindow.h"
#include "PrimaryWindows/QtScriptWindow.h"

CommandModulator::CommandModulator()
{

}

void CommandModulator::initialize(IChimeraQtWindow* win)
{
	mainWin = win->mainWin;
	andorWin = win->andorWin;
	scriptWin = win->scriptWin;
	auxWin = win->auxWin;
	makoWin1 = win->makoWin1;
	makoWin2 = win->makoWin2;
}

void CommandModulator::openConfiguration(QString addressName) 
{
	mainWin->profile.openConfigFromPath(CONFIGURATION_PATH + str(addressName), mainWin);
}
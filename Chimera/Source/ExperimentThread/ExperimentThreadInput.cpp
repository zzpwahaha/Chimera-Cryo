#include "stdafx.h"
#include "ExperimentThread/ExperimentThreadInput.h"
#include "PrimaryWindows/QtAuxiliaryWindow.h"
#include "PrimaryWindows/QtMainWindow.h"
#include "PrimaryWindows/QtAndorWindow.h"
#include "PrimaryWindows/QtScriptWindow.h"
#include "PrimaryWindows/IChimeraQtWindow.h"

ExperimentThreadInput::ExperimentThreadInput(IChimeraQtWindow* win) :
	ttlSys(win->auxWin->getTtlSystem()),
	ttls(win->auxWin->getTtlCore()),
	aoSys(win->auxWin->getAoSys()),
	ao(win->auxWin->getAoSys().getCore()),
	ddsSys(win->auxWin->getDdsSys()),
	dds(win->auxWin->getDdsSys().getCore()),
	olSys(win->auxWin->getOlSys()),
	ol(win->auxWin->getOlSys().getCore()),
	logger(win->andorWin->getLogger())
{
	devices = win->mainWin->getDevices ();
};


#pragma once
#include <qobject.h>

class IChimeraQtWindow;
class QtMainWindow;
class QtAndorWindow;
class QtMakoWindow;
class QtAuxiliaryWindow;
class QtScriptWindow;
class QtAnalysisWindow;

// THIS SHOULD LIVE IN MAIN THREAD DUE TO QT GUI OPREATIONS
class CommandModulator : public QObject
{
	Q_OBJECT
public:
	CommandModulator();
	void initialize(IChimeraQtWindow* win);

public slots:
	void openConfiguration(QString addressName);

private:


	QtMainWindow* mainWin = nullptr;
	QtScriptWindow* scriptWin = nullptr;
	QtAndorWindow* andorWin = nullptr;
	QtAuxiliaryWindow* auxWin = nullptr;
	QtMakoWindow* makoWin1 = nullptr;
	QtMakoWindow* makoWin2 = nullptr;
	QtAnalysisWindow* analysisWin = nullptr;

};


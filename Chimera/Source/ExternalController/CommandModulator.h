#pragma once
#include <qobject.h>

class IChimeraSystem;
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
	CommandModulator(IChimeraSystem* parentSys);
	void initialize(IChimeraQtWindow* win);
	struct ErrorStatus {
		bool error = false;
		std::string errorMsg;
	};

public slots:
	void openConfiguration(QString addressName, ErrorStatus& status);
	void openMasterScript(QString addressName, ErrorStatus& status);
	void saveAll(ErrorStatus& status);
	void startExperiment(ErrorStatus& status);
	void abortExperiment(ErrorStatus& status);
	void isExperimentRunning(bool& running, ErrorStatus& status);

private:
	QtMainWindow* mainWin = nullptr;
	QtScriptWindow* scriptWin = nullptr;
	QtAndorWindow* andorWin = nullptr;
	QtAuxiliaryWindow* auxWin = nullptr;
	QtMakoWindow* makoWin1 = nullptr;
	QtMakoWindow* makoWin2 = nullptr;
	QtAnalysisWindow* analysisWin = nullptr;

};


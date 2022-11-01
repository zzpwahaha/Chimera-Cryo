// created by Mark O. Brown
#pragma once
#include "ConfigurationSystems/Version.h"
#include "ConfigurationSystems/ConfigStream.h"
#include "GeneralObjects/commonTypes.h"
#include "debugInfo.h"
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>

class MainWindow;
class IChimeraQtWindow;

class DebugOptionsControl : public QWidget
{
	Q_OBJECT
	public:
		void handleSaveConfig(ConfigStream& saveFile);
		void handleOpenConfig(ConfigStream& openFile );
		void initialize( IChimeraQtWindow* parent );
		void handleEvent(unsigned id, MainWindow* comm);
		debugInfo getOptions();
		void setOptions(debugInfo options);

	private:

		QLabel* header;
		QLabel* pauseText;
		QLineEdit* pauseEdit;

		debugInfo currentOptions;
};
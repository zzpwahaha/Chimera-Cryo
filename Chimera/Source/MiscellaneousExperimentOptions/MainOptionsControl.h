// created by Mark O. Brown
#pragma once

#include "ConfigurationSystems/Version.h"
#include "GeneralObjects/commonTypes.h"
#include "ConfigurationSystems/ConfigStream.h"
#include <MiscellaneousExperimentOptions/InExpCalControl.h>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <qtimer.h>
#include <atomic>

class IChimeraQtWindow;

struct mainOptions{
	bool randomizeVariations=false;
	bool repetitionFirst = false;
	bool inExpCalibration = false;
	double inExpCalInterval = 10.0;
	bool delayAutoCal = false;
	unsigned atomSkipThreshold=UINT_MAX;
};

// this got whittled down recently, but keeping so that I can put more stuff in later.
class MainOptionsControl : public QWidget
{
	Q_OBJECT
	public:
		MainOptionsControl& operator=(const MainOptionsControl&) = delete;
		MainOptionsControl(const MainOptionsControl&) = delete;
		MainOptionsControl() = default;
		void handleSaveConfig(ConfigStream& saveFile);
		static mainOptions getSettingsFromConfig(ConfigStream& openFile );
		void setOptions ( mainOptions opts );
		void initialize( IChimeraQtWindow* parent );
		mainOptions getOptions();
		std::atomic<bool>* interruptPointer();
	public slots:
		void startInExpCalibrationTimer();


	private:
		QLabel* header;
		QCheckBox* randomizeVariationsButton;
		QCheckBox* repetitionFirstButton;
		InExpCalControl* inExpCalControl;
		QCheckBox* delayAutoCal;
		QLabel* atomThresholdForSkipText;
		QLineEdit* atomThresholdForSkipEdit;
		mainOptions currentOptions;
};

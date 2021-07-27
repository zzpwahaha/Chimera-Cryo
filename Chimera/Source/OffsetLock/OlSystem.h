#pragma once
#include "GeneralObjects/IChimeraSystem.h"
#include "OffsetLockOutput.h"
#include "OlStructure.h"
#include "OlCore.h"
#include "GeneralFlumes/QtSerialFlume.h"
#include "DigitalOutput/DoSystem.h"
#include "qlabel.h"
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlineedit.h>

class OlSystem : public IChimeraSystem
{
public:
	// THIS CLASS IS NOT COPYABLE.
	OlSystem& operator=(const OlSystem&) = delete;
	OlSystem(const OlSystem&) = delete;
	OlSystem(IChimeraQtWindow* parent, DoSystem& ttlBoard);

	// standard functions for gui elements
	void initialize(IChimeraQtWindow* master);
	bool eventFilter(QObject* obj, QEvent* event);
	// configs
	void handleSaveConfig(ConfigStream& saveFile);
	void handleOpenConfig(ConfigStream& openFile);
	std::string getDelim();


	void handleRoundToOl();
	void updateEdits();
	void setDefaultValue(unsigned olNum, double val);
	void setName(int olNumber, std::string name);
	void setNote(int olNumber, std::string note);
	void setMinMax(int olNumber, double min, double max);
	void updateCoreNames();

	void handleEditChange(unsigned olNumber);

	// processing to determine how ol's get set
	void handleSetOlsButtonPress(DoCore& doCore, DOStatus doStatus, bool useDefault = false);
	void zeroOffsetLock(DoCore& doCore, DOStatus doStatus);
	void setOLs(DoCore& doCore, DOStatus doStatus);
	void setOlStatusNoForceOut(std::array<double, size_t(OLGrid::total)> status);
	void standardExperimentPrep(unsigned variation, DoCore& doCore, std::string& warning);

	// getters
	double getDefaultValue(unsigned olNum);
	std::string getName(int olNumber);
	std::string getNote(int olNumber);
	double getOlValue(int olNumber);
	std::array<double, size_t(OLGrid::total)> getOlValues();
	unsigned int getNumberOfOls();
	std::pair<double, double> getOlRange(int olNumber);
	std::array<OlInfo, size_t(OLGrid::total)> getOlInfo();


	OlCore& getCore() { return core; }
	
	bool IsquickChange() { return quickChange->isChecked(); }

private:
	QLabel* olTitle;
	CQPushButton* olSetButton;
	CQPushButton* zeroOlsButton;
	CQPushButton* reconnectButton;
	CQCheckBox* quickChange;

	std::array<OffsetLockOutput, size_t(OLGrid::total)> outputs;

	OlCore core;
	DoSystem& ttlBoard;
	bool roundToOlPrecision;


	static constexpr double olFreqResolution = OffsetLockOutput::olFreqResolution; /*12 bit N + 25bit FRAC, 50MHz phase-freq detector frequency*/
	const int numDigits = static_cast<int>(abs(round(log10(olFreqResolution) - 0.49)));
};


#pragma once
#include "GeneralObjects/IDeviceCore.h"
#include "OffsetLockOutput.h"
#include "GeneralFlumes/QtSerialFlume.h"
#include "DigitalOutput/DoCore.h"
#include "Plotting/QCustomPlotCtrl.h"

class OlCore
{
public:
	// THIS CLASS IS NOT COPYABLE.
	OlCore& operator=(const OlCore&) = delete;
	OlCore(const OlCore&) = delete;

	OlCore(bool safemode);
	~OlCore();

	void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker);
	const std::string configDelim = "OFFSETLOCK_SYSTEM";
	std::string getDelim() { return configDelim; }

	bool isValidOLName(std::string name);
	int getOLIdentifier(std::string name);
	std::string getName(int olNumber);
	std::array<std::string, size_t(OLGrid::total)> getName();
	void setNames(std::array<std::string, size_t(OLGrid::total)> namesIn);

	void resetOLEvents();
	void initializeDataObjects(unsigned variationNum);
	void setOLCommandForm(OlCommandForm command);
	void handleOLScriptCommand(OlCommandForm command, std::string name, std::vector<parameterType>& vars);
	void organizeOLCommands(unsigned variation, OlSnapshot initSnap, std::string& warning);
	void makeFinalDataFormat(unsigned variation, DoCore& doCore);
	std::vector<std::vector<plotDataVec>> getPlotData(unsigned var);
	//void standardExperimentPrep(unsigned variation, DoCore& doCore, std::string& warning);
	void writeOLs(unsigned variation);

	void OLForceOutput(std::array<double, size_t(OLGrid::total)> status, DoCore& doCore, DOStatus dostatus);

private:
	int tmp = 0;
	std::array<std::string, size_t(OLGrid::total)> names;

	//std::array <const double, 2> olResolution;
	std::vector<OlCommandForm> olCommandFormList;
	// the first vector is for each variation.
	std::vector<std::vector<OlCommand>> olCommandList;
	std::vector<std::vector<OlSnapshot>> olSnapshots;/*do not need snapshot, can just record the changed channel rather than all of the output*/
	std::vector<std::vector<OlChannelSnapshot>> olChannelSnapshots;
	//std::vector<std::pair<double, std::vector<OlCommand>>> timeOrganizer;
	std::array<std::vector<std::pair<double, OlCommand>>, size_t(OLGrid::total)> timeOrganizer;

	QtSerialFlume qtFlume;

public:
	constexpr static double olFreqResolution = OffsetLockOutput::olFreqResolution; /*12 bit N + 25bit FRAC, 50MHz phase-freq detector frequency*/
	const int numFreqDigits = 4/*static_cast<int>(abs(round(log10(olFreqResolution) - 0.49)))*/;
	const int numTimeDigits= static_cast<int>(abs(round(log10(OL_TIME_RESOLUTION) - 0.49)));

	
};


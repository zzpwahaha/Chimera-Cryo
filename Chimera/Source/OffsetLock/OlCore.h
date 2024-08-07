#pragma once
#include "GeneralObjects/IDeviceCore.h"
#include "OffsetLockOutput.h"
#include "GeneralFlumes/QtSerialFlume.h"
#include <GeneralFlumes/BoostAsyncSerial.h>
#include "DigitalOutput/DoCore.h"
#include "Plotting/QCustomPlotCtrl.h"

class OlCore
{
public:
	// THIS CLASS IS NOT COPYABLE.
	OlCore& operator=(const OlCore&) = delete;
	OlCore(const OlCore&) = delete;

	OlCore(std::vector<bool> safemodes);
	~OlCore();

	void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker);
	void constructRepeats(repeatManager& repeatMgr);
	bool repeatsExistInCommandForm(repeatInfoId repeatId);
	void addPlaceholderRepeatCommand(repeatInfoId repeatId);
	const std::string configDelim = "OFFSETLOCK_SYSTEM";
	std::string getDelim() { return configDelim; }

	bool isValidOLName(std::string name);
	int getOLIdentifier(std::string name);
	std::string getName(int olNumber);
	std::array<std::string, size_t(OLGrid::total)> getName();
	void setNames(std::array<std::string, size_t(OLGrid::total)> namesIn);

	void resetOLEvents();
	void prepareForce();
	void sizeDataStructures(unsigned variations);
	void initializeDataObjects(unsigned variationNum);
	std::vector<OlCommand> getOlCommand(unsigned variation);
	void setOLCommandForm(OlCommandForm command);
	void handleOLScriptCommand(OlCommandForm command, std::string name, std::vector<parameterType>& vars);
	void organizeOLCommands(unsigned variation, OlSnapshot initSnap, std::string& warning);
	void makeFinalDataFormat(unsigned variation, DoCore& doCore);
	std::vector<std::vector<plotDataVec>> getPlotData(unsigned var);
	//void standardExperimentPrep(unsigned variation, DoCore& doCore, std::string& warning);
	void writeOLs(unsigned variation);
	void OLForceOutput(std::array<double, size_t(OLGrid::total)> status, DoCore& doCore, DOStatus dostatus);
	void resetConnection();
	void readCallback(int byte);
	void errorCallback(std::string error);

	unsigned short getNumCmdChannel(unsigned short flumeIdx);
private:
	// return the index for the offsetlock boxes
	unsigned short getFlumeIdx(unsigned short channel);
	//return index for channel number within a particular offsetlock box
	unsigned short getCmdChannelIdx(unsigned short channel);
	double getTimeResolution(unsigned short channel);
	int numTimeDigits(unsigned short channel);

	unsigned long long tmp = 0;
	std::array<std::string, size_t(OLGrid::total)> names;

	//std::array <const double, 2> olResolution;
	std::vector<OlCommandForm> olCommandFormList;
	// the first vector is for each variation.
	std::vector<std::vector<OlCommand>> olCommandList;
	std::vector<std::vector<OlSnapshot>> olSnapshots;/*do not need snapshot, can just record the changed channel rather than all of the output*/
	std::vector<std::vector<OlChannelSnapshot>> olChannelSnapshots;
	//std::vector<std::pair<double, std::vector<OlCommand>>> timeOrganizer;
	std::array<std::vector<std::pair<double, OlCommand>>, size_t(OLGrid::total)> timeOrganizer;

	//std::array<QtSerialFlume, size_t(OLGrid::numOFunit)> qtFlumes;
	std::array<BoostAsyncSerial, size_t(OLGrid::numOFunit)> btFlumes;
	std::atomic<bool> readComplete;	
	std::vector<unsigned char> readRegister;
	std::string errorMsg;

public:
	const unsigned maxCommandNum = 512;
	constexpr static double olFreqResolution = OffsetLockOutput::olFreqResolution; /*12 bit N + 25bit FRAC, 50MHz phase-freq detector frequency*/
	const int numFreqDigits = 4/*static_cast<int>(abs(round(log10(olFreqResolution) - 0.49)))*/;
	//const int numTimeDigits= static_cast<int>(abs(round(log10(OL_TIME_RESOLUTION) - 0.49)));

	
};


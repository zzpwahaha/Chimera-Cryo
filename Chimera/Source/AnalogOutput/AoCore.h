#pragma once

#include "AoStructures.h"
#include "AnalogOutput.h"
#include "Plotting/PlotCtrl.h"
//#include "ParameterSystem/ParameterSystem.h"
//#include "ConfigurationSystems/Version.h"
#include "ZynqTCP/ZynqTCP.h"
//#include <GeneralObjects/IDeviceCore.h>

#include <vector>
#include <array>



class AoCore
{

public:
	AoCore();
	~AoCore() {};

	const std::string configDelim = "DAC_SYSTEM";
	std::string getDelim() { return configDelim; }
	void setNames(std::array<std::string, size_t(AOGrid::total)> namesIn);
	std::array<std::string, size_t(AOGrid::total)> getNames();


	unsigned getNumberSnapshots(unsigned variation);
	std::vector<std::vector<AoSnapshot>> getSnapshots();
	unsigned long getNumberEvents(unsigned variation);

	std::string getDacSequenceMessage(unsigned variation);
	std::vector<std::vector<plotDataVec>> getPlotData(unsigned var);
	std::array<double, size_t(AOGrid::total)> getFinalSnapshot();

	void setDacCommandForm(AoCommandForm command);
	void initializeDataObjects(unsigned cmdNum);
	void resetDacEvents();

	void calculateVariations(std::vector<parameterType>& params, 
		ExpThreadWorker* threadworker, std::vector<calResult> calibrations);
	void organizeDacCommands(unsigned variation, AoSnapshot initSnap);
	void findLoadSkipSnapshots(double time, std::vector<parameterType>& variables, unsigned variation);
	void AoCore::formatDacForFPGA(UINT variation, AoSnapshot initSnap);

	void writeDacs(unsigned variation, bool loadSkip);

	void makeFinalDataFormat(unsigned variation);

	void handleDacScriptCommand(AoCommandForm command, std::string name, std::vector<parameterType>& vars);
	int getDacIdentifier(std::string name);
	static int getBasicDacIdentifier(std::string name);
	bool isValidDACName(std::string name);
	

	void checkTimingsWork(unsigned variation);
	void checkValuesAgainstLimits(unsigned variation, const std::array<AnalogOutput, size_t(AOGrid::total)>& outputs);

	void setGUIDacChange(std::vector<std::vector<AoChannelSnapshot>> channelSnapShot);

private:
	std::array<std::string, size_t(AOGrid::total)> names;

	std::vector<AoCommandForm> dacCommandFormList;
	std::vector<std::vector<AoCommand>> dacCommandList;
	std::vector<std::vector<AoSnapshot>> dacSnapshots, loadSkipDacSnapshots;
	//std::vector<std::array<std::vector<double>, size_t(AOGrid::numOFunit)>> finalFormatDacData, loadSkipDacFinalFormat;

	std::vector<std::vector<AoChannelSnapshot>> finalDacSnapshots;
	//Zynq tcp connection
	ZynqTCP zynq_tcp;
	double dacTriggerTime; // used in checkTimingsWork() to see if time of two points in dac is shorter than 1/ramp freq

	static constexpr double dacResolution = 20.0 / 0xffff; /*16bit dac*/
	const int numDigits = static_cast<int>(abs(round(log10(dacResolution) - 0.49)));


};


#pragma once

#include "ArbGenSettings.h"
#include "ParameterSystem/ParameterSystemStructures.h"
#include "GeneralFlumes/VisaFlume.h"
//#include "DigitalOutput/DoRows.h"
#include "GeneralObjects/IDeviceCore.h"
#include "ConfigurationSystems/ConfigStream.h"
#include <vector>
#include <string>
//#include <AnalogInput/calInfo.h>
#include <AnalogOutput/calInfo.h>

class DoCore;
class ArbGenCore : public IDeviceCore {
public:
	// THIS CLASS IS NOT COPYABLE.
	ArbGenCore& operator=(const ArbGenCore&) = delete;
	ArbGenCore(const ArbGenCore&) = delete;

	ArbGenCore(const arbGenSettings& settings);
	~ArbGenCore();
	void initialize();
	std::string getDelim() { return configDelim; }
	bool connected();
	std::pair<unsigned, unsigned> getTriggerLine();
	std::string getDeviceIdentity();
	void logSettings(DataLogger& log, ExpThreadWorker* threadworker);
	void convertInputToFinalSettings(unsigned chan, deviceOutputInfo& info,
		std::vector<parameterType>& variables = std::vector<parameterType>());
	static double convertPowerToSetPoint(double power, bool conversionOption, calResult calibraiton);
	
	std::vector<std::string> getStartupCommands();
	void programSetupCommands();
	std::string getDeviceInfo();

	void analyzeArbGenScript(scriptedArbInfo& infoObj, std::vector<parameterType>& vars, std::string& warnings);

	deviceOutputInfo getSettingsFromConfig(ConfigStream& file);
	void loadExpSettings(ConfigStream& script);
	void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker);
	void checkTriggers(unsigned variationInc, DoCore& ttls, ExpThreadWorker* threadWorker);
	void normalFinish() {};
	void errorFinish() {};
	void setAgCalibration(calResult newCal, unsigned chan);
	void setRunSettings(deviceOutputInfo newSettings);
	void setArbGen(unsigned variation, std::vector<parameterType>& params, deviceOutputInfo runSettings, ExpThreadWorker* expWorker);
	void programVariation(unsigned variation, std::vector<parameterType>& params, ExpThreadWorker* threadworker);


	virtual void setSync(const deviceOutputInfo& runSettings, ExpThreadWorker* expWorker) = 0;//solely used by setArbGen
	virtual void setDC(int channel, dcInfo info, unsigned variation) = 0;
	virtual void setExistingWaveform(int channel, preloadedArbInfo info) = 0;
	virtual void setSquare(int channel, squareInfo info, unsigned variation) = 0;
	virtual void setSine(int channel, sineInfo info, unsigned variation) = 0;
	virtual void outputOff(int channel) = 0;
	virtual void prepArbGenSettings(unsigned channel) = 0;
	virtual void setScriptOutput(unsigned varNum, scriptedArbInfo scriptInfo, unsigned channel) = 0;
	virtual void programBurstMode(int channel, bool burstOption) = 0;
	virtual void setDefault(int channel) = 0;
	virtual void handleScriptVariation(unsigned variation, scriptedArbInfo& scriptInfo, unsigned channel,
		std::vector<parameterType>& params) = 0;

	/* Taken from scriptedArbGenWaveform since it relates to sending command to arbgen
	 * This function takes the data points (that have already been converted and normalized) and puts them into a string
	 * for the agilent to readbtn. segNum: this is the segment number that this data is for
	 * varNum: This is the variation number for this segment (matters for naming the segments)
	 * totalSegNum: This is the number of segments in the waveform (also matters for naming)
	 */
	virtual std::string compileAndReturnDataSendString(scriptedArbInfo& scriptInfo, int segNum, int varNum, int totalSegNum, unsigned chan) = 0;

	/* Taken from scriptedArbGenWaveform since it relates to sending command to arbgen
	 * This function compiles the sequence string which tells the agilent what waveforms to output when and with what trigger control. The sequence is stored
	 * as a part of the class.
	 */
	virtual void compileSequenceString(scriptedArbInfo& scriptInfo, int totalSegNum, int sequenceNum, unsigned channel, unsigned varNum) = 0;


public:
	const std::string configDelim;
	const std::string arbGenName;

	//const int AGILENT_DEFAULT_POWER = 10;
	const unsigned long sampleRate;
	const std::string memoryLoc;
	const arbGenSettings initSettings;
	const unsigned triggerRow;
	const unsigned triggerNumber;

	// includes burst commands, trigger commands, etc. This is a place for any commands which don't have a 
	// GUI control option. You could also use this to put commands that should be the same for all configurations.
	const std::vector<std::string> setupCommands;

protected:

	deviceOutputInfo expRunSettings;
	VisaFlume visaFlume;
	bool isConnected;
	std::string deviceInfo;
	std::array<calResult, 2> calibrations;



	/* a list of polynomial coefficients for the calibration.
	auto& cc = calibrationCoefficients
	Volt = cc[0] + c[1]*sp + c[2]*sp^2 + ...
	*/
	//const std::vector<double> calibrationCoefficients;

};
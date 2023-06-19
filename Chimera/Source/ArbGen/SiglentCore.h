#pragma once
#include "ArbGenCore.h"
class SiglentCore :  public ArbGenCore
{
public:
	// THIS CLASS IS NOT COPYABLE.
	SiglentCore& operator=(const SiglentCore&) = delete;
	SiglentCore(const SiglentCore&) = delete;

	SiglentCore(const arbGenSettings& settings);
	~SiglentCore();

	//void initialize ();
	//void setAgilent (unsigned variation, std::vector<parameterType>& params, deviceOutputInfo runSettings, ExpThreadWorker* expWorker);
	//
	//std::string getDelim () { return configDelim; }
	//bool connected ();
	//std::pair<unsigned, unsigned> getTriggerLine ();
	//std::string getDeviceIdentity ();
	//void convertInputToFinalSettings (unsigned chan, deviceOutputInfo& info,
	//								  std::vector<parameterType>& variables = std::vector<parameterType> ());
	//static double convertPowerToSetPoint (double power, bool conversionOption, calResult calibraiton);
	//
	//
	//void analyzeAgilentScript (scriptedArbInfo& infoObj, std::vector<parameterType>& vars, std::string& warnings);
	//
	//std::vector<std::string> getStartupCommands ();
	//void programSetupCommands ();
	//std::string getDeviceInfo ();
	//
	//deviceOutputInfo getSettingsFromConfig (ConfigStream& file);
	//void loadExpSettings (ConfigStream& script);
	//void calculateVariations (std::vector<parameterType>& params, ExpThreadWorker* threadworker);
	//void programVariation (unsigned variation, std::vector<parameterType>& params, ExpThreadWorker* threadworker);
	//void checkTriggers (unsigned variationInc, DoCore& ttls, ExpThreadWorker* threadWorker);
	//void normalFinish () {};
	//void errorFinish () {};
	//void setAgCalibration (calResult newCal, unsigned chan);
	//void setRunSettings (deviceOutputInfo newSettings);

	//void logSettings (DataLogger& log, ExpThreadWorker* threadworker);

	void setSync(const deviceOutputInfo& runSettings, ExpThreadWorker* expWorker) override;
	void setDC(int channel, dcInfo info, unsigned variation) override;
	void setExistingWaveform(int channel, preloadedArbInfo info) override;
	void setSquare(int channel, squareInfo info, unsigned variation) override;
	void setSine(int channel, sineInfo info, unsigned variation) override;
	void outputOff(int channel) override;
	void outputOn(int channel) override;
	void prepArbGenSettings(unsigned channel) override;
	void setScriptOutput(unsigned varNum, scriptedArbInfo scriptInfo, unsigned channel) override;
	void programBurstMode(int channel, bool burstOption) override;
	void programNonArbBurstMode(int channel, bool burstOption);
	void setDefault(int channel) override;
	void handleScriptVariation(unsigned variation, scriptedArbInfo& scriptInfo, unsigned channel,
		std::vector<parameterType>& params) override;

	std::string compileAndReturnDataSendString(scriptedArbInfo& scriptInfo, int segNum, int varNum, int totalSegNum, unsigned chan) override;
	void compileSequenceString(scriptedArbInfo& scriptInfo, int totalSegNum, int sequenceNum, unsigned channel, unsigned varNum) override;


private:
	const int SIGLENT_DEFAULT_POWER = 0;
	//const std::string configDelim;
	//deviceOutputInfo expRunSettings;
	//const unsigned long sampleRate;
	//const std::string memoryLoc;
	//const agilentSettings initSettings;
	//// not that important, just used to check that number of triggers in script matches number in agilent.
	//const unsigned triggerRow;
	//const unsigned triggerNumber;
	//VisaFlume visaFlume;
	//bool isConnected;
	//std::string deviceInfo;

	//std::array<calResult, 2> calibrations;

	//// includes burst commands, trigger commands, etc. This is a place for any commands which don't have a 
	//// GUI control option. You could also use this to put commands that should be the same for all configurations.
	//const std::vector<std::string> setupCommands;

	///* a list of polynomial coefficients for the calibration.
	//auto& cc = calibrationCoefficients
	//Volt = cc[0] + c[1]*sp + c[2]*sp^2 + ...
	//*/
	////const std::vector<double> calibrationCoefficients;
	//const std::string agilentName;
};


#pragma once
#include "ConfigurationSystems/Version.h"
#include "Microwave/WindFreakFlume.h"
#include "GeneralObjects/IDeviceCore.h"
#include "Microwave/microwaveSettings.h"
#include "LowLevel/constants.h"

class MicrowaveCore : public IDeviceCore{
	public:
		MicrowaveCore ();
		std::string queryIdentity ();
		void setFmSettings ();
		void setPmSettings ();
		void programVariation (unsigned variationNumber, std::vector<parameterType>& params, ExpThreadWorker* threadworker);
		void calculateVariations (std::vector<parameterType>& params, ExpThreadWorker* threadworker);
		std::pair<unsigned, unsigned> getUWaveTriggerLine ();
		unsigned getNumTriggers (microwaveSettings settings);
		microwaveSettings getSettingsFromConfig (ConfigStream& openFile);
		std::string configDelim = "MICROWAVE_SYSTEM";
		std::string getDelim () { return configDelim; }
		void logSettings (DataLogger& log, ExpThreadWorker* threadworker);
		void loadExpSettings (ConfigStream& stream);
		void normalFinish () {};
		void errorFinish () {};
		std::string getCurrentList ();
		microwaveSettings experimentSettings;
		WindFreakFlume uwFlume;
		void setTrigTime (double time);

		const std::vector<std::string> mwSetupCommands = {
			"C0", /*set control controlling channel to channel0 (RFoutA)*/
			"x0", /*set to external reference*/
			"*010.000", /*set reference frequency to 10MHz*/
			"E1r1", /*Set PLL Power On and Set power amplifier (VGA) Power On*/
			"w0", /*w2 - set trigger connector functions, 2) Trigger single frequency step*/
			"h1", /*set SynthHD output power on*/
		};

	private:
		double triggerTime = 0.5;
		const std::pair<unsigned, unsigned> uwaveTriggerLine = MW_TRIGGER_LINE;
};
#pragma once
#include "GeneralObjects/IDeviceCore.h"
#include "LowLevel/constants.h"
#include "AnalogInput/AiSettings.h"
#include <GeneralFlumes/QtSocketFlume.h>
#include "ConfigurationSystems/ConfigStream.h"
#include <array>


class AiCore :  public IDeviceCore
{
public:
	// THIS CLASS IS NOT COPYABLE.
	AiCore& operator=(const AiCore&) = delete;
	AiCore(const AiCore&) = delete;

	AiCore();

	void getSettingsFromConfig(ConfigStream& file);
	void saveSettingsToConfig(ConfigStream& file);
	void updateChannelRange();
	void getSingleSnap(unsigned n_to_avg);
	std::array<double, size_t(AIGrid::total)> getCurrentValues();


	std::string getDelim() { return configDelim; }
	void programVariation(unsigned variation, std::vector<parameterType>& params, ExpThreadWorker* threadworker) {};
	void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker) {};
	void loadExpSettings(ConfigStream& stream) {};
	void logSettings(DataLogger& log, ExpThreadWorker* threadworker) {};
	void normalFinish() {};
	void errorFinish() {};

	const std::array<AiValue, size_t(AIGrid::total)>& getAiVals() { return aiValues; }
	void setAiRange(unsigned channel, AiChannelRange::which range);

public:
	const std::string configDelim{ "AI-SYSTEM" };
	const QByteArray terminator = QByteArray("#\0", 2);
	const QByteArray placeholder = QByteArray(1, '*');

private:
	std::array<AiValue, size_t(AIGrid::total)> aiValues;

	QtSocketFlume socket;

	static constexpr double adcResolution = 20.0 / 0xffff; /*16bit adc*/
	const int numDigits = static_cast<int>(abs(round(log10(adcResolution) - 0.49)));

};


// created by Mark O. Brown
#pragma once
#include "LowLevel/constants.h"
#include "AnalogInput/AiSettings.h"
#include "GeneralObjects/IChimeraSystem.h"
#include "ConfigurationSystems/Version.h"
#include "Scripts/ScriptStream.h"
#include "AnalogOutput/DaqMxFlume.h"
#include "ConfigurationSystems/ConfigStream.h"
#include "PrimaryWindows/IChimeraQtWindow.h"
#include <QLabel.h>
#include <CustomQtControls/AutoNotifyCtrls.h>

#include <GeneralFlumes/QtSocketFlume.h>
#include "nidaqmx2.h"
#include <array>


/*
 * This is a interface for taking analog input data through an NI card that uses DAQmx. These cards are generally 
 * somewhat flexible, but right now I only use it to readbtn and record voltage values from Analog inputs.
 */
class AiSystem : public IChimeraSystem{
	public:
		static constexpr unsigned AI_NumPerUnit = 8;
		static constexpr unsigned AI_NumOfUnit = 2;
		static constexpr unsigned NUMBER_AI_CHANNELS = AI_NumPerUnit * AI_NumOfUnit;

		// THIS CLASS IS NOT COPYABLE.
		AiSystem& operator=(const AiSystem&) = delete;
		AiSystem (const AiSystem&) = delete;

		AiSystem(IChimeraQtWindow* parent);
		AiSettings getAiSettings ();
		void handleTimer ();
		void initDaqmx( );
		void initialize(IChimeraQtWindow* parent );
		void refreshDisplays( );
		void refreshCurrentValues( );
		std::array<float64, NUMBER_AI_CHANNELS> getSingleSnapArray( unsigned n_to_avg );
		std::vector<float64> getSingleSnap( unsigned n_to_avg );
		double getSingleChannelValue( unsigned chan, unsigned n_to_avg );
		void armAquisition( unsigned numSnapshots );
		void getAquisitionData( );
		std::vector<float64> getCurrentValues( );
		bool wantsQueryBetweenVariations( );
		bool wantsContinuousQuery( );
		std::string getSystemStatus( );
		void setAiSettings (AiSettings settings);
		AiSettings getSettingsFromConfig (ConfigStream& file);
		void handleSaveConfig (ConfigStream& file);
		const std::string configDelim{ "AI-SYSTEM" };
		std::string getDelim () { return configDelim; }
		void programVariation (unsigned variation, std::vector<parameterType>& params, ExpThreadWorker* threadworker) {};
		void calculateVariations (std::vector<parameterType>& params, ExpThreadWorker* threadworker) {};
		void loadExpSettings (ConfigStream& stream) {};

		void logSettings (DataLogger& log, ExpThreadWorker* threadworker);
		void normalFinish () {};
		void errorFinish () {};
	private:
		std::array<QLabel*, NUMBER_AI_CHANNELS> voltDisplays;

		CQPushButton* getValuesButton;
		CQCheckBox* continuousQueryCheck;
		CQCheckBox* queryBetweenVariations;

		CQLineEdit* continuousInterval;
		QLabel* continuousIntervalLabel;

		CQLineEdit* avgNumberEdit;
		QLabel* avgNumberLabel;

		QtSocketFlume socket;
		const QByteArray terminator = QByteArray("#\0", 2);

		// float64 should just be a double type.
		std::array<float64, NUMBER_AI_CHANNELS> currentValues;
		std::vector<float64> aquisitionData;
		TaskHandle analogInTask0 = nullptr;
		DaqMxFlume daqmx;
		const std::string boardName = "dev2";

		static constexpr double adcResolution = 20.0 / 0xffff; /*16bit adc*/
		const int numDigits = static_cast<int>(abs(round(log10(adcResolution) - 0.49)));
};




// created by Mark O. Brown
#pragma once
#include "LowLevel/constants.h"
#include "AiCore.h"
#include "AnalogInput/AiOutput.h"
#include "GeneralObjects/IChimeraSystem.h"
#include "ConfigurationSystems/Version.h"
#include "Scripts/ScriptStream.h"
#include "ConfigurationSystems/ConfigStream.h"
#include <QLabel.h>
#include <CustomQtControls/AutoNotifyCtrls.h>

#include <array>

class IChimeraQtWindow;

/*
 * This is a interface for taking analog input data through an NI card that uses DAQmx. These cards are generally 
 * somewhat flexible, but right now I only use it to readbtn and record voltage values from Analog inputs.
 */
class AiSystem : public IChimeraSystem{
	public:
		// THIS CLASS IS NOT COPYABLE.
		AiSystem& operator=(const AiSystem&) = delete;
		AiSystem (const AiSystem&) = delete;
		AiSystem(IChimeraQtWindow* parent);
		AiSettings getAiSettings ();
		void handleTimer ();
		void initialize(IChimeraQtWindow* parent );
		void refreshDisplays( );

		void refreshCurrentValues( );
		std::array<double, size_t(AIGrid::total)> getSingleSnapArray( unsigned n_to_avg );
		double getSingleChannelValue( unsigned chan, unsigned n_to_avg );

		//void getSingleSnap( unsigned n_to_avg );
		//void updateChannelRange();
		//void armAquisition( unsigned numSnapshots );
		//void getAquisitionData( );
		//std::array<double, NUMBER_AI_CHANNELS> getCurrentValues();

		bool wantsQueryBetweenVariations( );
		bool wantsContinuousQuery( );
		std::string getSystemStatus( );
		void setAiSettings (AiSettings settings);
		AiSettings getSettingsFromConfig (ConfigStream& file);
		void handleSaveConfig(ConfigStream& file);
		void handleOpenConfig(ConfigStream& file);
		std::string getDelim () { return core.configDelim; }

		void setName(int aiNumber, std::string name);
		void setNote(int aiNumber, std::string note);
		std::string getName(int aiNumber);
		std::string getNote(int aiNumber);
		void updateCoreNames();

		AiCore& getCore() { return core; }


	private:
		AiCore core;
		std::array<AiOutput, size_t(AIGrid::total)> outputs; /*A0-A7,B0-B7*/
		//std::array<CQComboBox*, size_t(AIGrid::total)> aiCombox;

		CQPushButton* getValuesButton;
		CQCheckBox* continuousQueryCheck;
		CQCheckBox* queryBetweenVariations;

		CQLineEdit* continuousInterval;
		QLabel* continuousIntervalLabel;

		CQLineEdit* avgNumberEdit;
		QLabel* avgNumberLabel;

		static constexpr double adcResolution = 20.0 / 0xffff; /*16bit adc*/
		const int numDigits = static_cast<int>(abs(round(log10(adcResolution) - 0.49)));
};




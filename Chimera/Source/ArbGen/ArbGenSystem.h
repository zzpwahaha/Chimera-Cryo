// created by Mark O. Brown
#pragma once
#include "ArbGenCore.h"
#include "AgilentCore.h"
#include "SiglentCore.h"
#include "Scripts/ScriptStream.h"
#include "ConfigurationSystems/ConfigStream.h"
#include "GeneralFlumes/VisaFlume.h"
#include "ArbGenStructures.h"
#include "whichAg.h"
#include "Scripts/Script.h"
//#include "DigitalOutput/DoRows.h"
#include <vector>
#include <array>
#include <qlabel.h>
#include <CustomQtControls/AutoNotifyCtrls.h>

class IChimeraQtWindow;

// A class for programming agilent arbitrary waveform generators.
// in essense this includes a wrapper around agilent's implementation of the VISA protocol. 
class ArbGenSystem : public IChimeraSystem 
{
	public:
		// THIS CLASS IS NOT COPYABLE.
		ArbGenSystem& operator=(const ArbGenSystem&) = delete;
		ArbGenSystem(const ArbGenSystem&) = delete;

		ArbGenSystem( const arbGenSettings& settings, ArbGenType type, IChimeraQtWindow* parent );
		~ArbGenSystem();
		void initialize(std::string headerText, IChimeraQtWindow* win);
		void updateButtonDisplay( int chan );
		void checkSave( std::string configPath, RunInfo info );
		void handleChannelPress( int chan, std::string configPath, RunInfo currentRunInfo );
		void handleModeCombo();

		void readGuiSettings();
		void readGuiSettings (int chan);
		bool scriptingModeIsSelected( );
		bool getSavedStatus ();
		void updateSavedStatus (bool isSaved);
		void handleSavingConfig( ConfigStream& saveFile, std::string configPath, RunInfo info );
		std::string getDeviceIdentity();
		void handleOpenConfig(ConfigStream& file);
		void updateSettingsDisplay( int chan, std::string configPath, RunInfo currentRunInfo );
		void updateSettingsDisplay( std::string configPath, RunInfo currentRunInfo );
		deviceOutputInfo getOutputInfo();
/*		void handleScriptVariation( unsigned variation, scriptedArbInfo& scriptInfo, unsigned channel, 
									std::vector<parameterType>& variables );*/
		// making the script public greatly simplifies opening, saving, etc. files from this script.
		Script agilentScript;
		//std::pair<unsigned, unsigned> getTriggerLine( );
		std::string getConfigDelim ();
		void programAgilentNow (std::vector<parameterType> constants);
		
		void setOutputSettings (deviceOutputInfo info);
		void verifyScriptable ( );
		ArbGenCore& getCore ();
		void setDefault (unsigned chan);
	private:
		ArbGenCore* pCore;
		//ArbGenCore& core;
		minMaxDoublet chan2Range;
		const arbGenSettings initSettings;
		// since currently all visaFlume communication is done to communicate with agilent machines, my visaFlume wrappers exist
		// in this class.
		int currentChannel=1;
		std::vector<minMaxDoublet> ranges;
		deviceOutputInfo currentGuiInfo;
		// GUI ELEMENTS
		
		QLabel* header;
		QLabel* deviceInfoDisplay;
		QButtonGroup* channelButtonsGroup;
		CQRadioButton* channel1Button;
		CQRadioButton* channel2Button;
		CQCheckBox* syncedButton;
		CQCheckBox* calibratedButton;
		CQCheckBox* burstButton;
		CQComboBox* settingCombo;
		QLabel* optionsFormat;
		CQPushButton* programNow;
};



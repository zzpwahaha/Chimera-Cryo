// created by Mark O. Brown
#include "stdafx.h"

#include "ArbGenSystem.h"
#include "ParameterSystem/ParameterSystem.h"
#include "ConfigurationSystems/ConfigSystem.h"
#include "boost/cast.hpp"
#include <algorithm>
#include <numeric>
#include <fstream>
#include "GeneralUtilityFunctions/range.h"
#include <PrimaryWindows/QtMainWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>
#include "boost/lexical_cast.hpp"
#include <qbuttongroup.h>
#include <qlayout.h>

ArbGenSystem::ArbGenSystem( const arbGenSettings& settings, ArbGenType type, IChimeraQtWindow* parent )
	: IChimeraSystem(parent)
	, initSettings(settings)
	, arbGenScript(parent)
{
	switch (type) {
	case ArbGenType::Agilent:
		pCore = new AgilentCore(settings);
		break;
	case ArbGenType::Siglent:
		pCore = new SiglentCore(settings);
		break;
	}
}

ArbGenSystem::~ArbGenSystem()
{
	delete pCore;
}

void ArbGenSystem::programArbGenNow(std::vector<parameterType> constants){
	readGuiSettings ();
	std::string warnings_;
	if (currentGuiInfo.channel[0].scriptedArb.fileAddress.expressionStr != ""){
		currentGuiInfo.channel[0].scriptedArb.wave = ScriptedArbGenWaveform();
		pCore->analyzeArbGenScript (currentGuiInfo.channel[0].scriptedArb, constants, warnings_);
	}
	if (currentGuiInfo.channel[1].scriptedArb.fileAddress.expressionStr != ""){
		currentGuiInfo.channel[1].scriptedArb.wave = ScriptedArbGenWaveform();
		pCore->analyzeArbGenScript (currentGuiInfo.channel[1].scriptedArb, constants, warnings_);
	}
	pCore->convertInputToFinalSettings (0, currentGuiInfo, constants);
	pCore->convertInputToFinalSettings (1, currentGuiInfo, constants);
	pCore->setArbGen (0, constants, currentGuiInfo, nullptr);
	if (dynamic_cast<SiglentCore*>(pCore)) {
		burstButton->setChecked(true);
	}
}

std::string ArbGenSystem::getDeviceIdentity (){
	return pCore->getDeviceIdentity ();
}

std::string ArbGenSystem::getConfigDelim (){
	return pCore->configDelim;
}

bool ArbGenSystem::getSavedStatus (){
	return arbGenScript.savedStatus ();
}

void ArbGenSystem::updateSavedStatus (bool isSaved){
	arbGenScript.updateSavedStatus (isSaved);
}

void ArbGenSystem::initialize(std::string headerText, IChimeraQtWindow* win)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	pCore->initialize ();
	header = new QLabel (cstr (headerText), win);
	auto deviceInfo = pCore->getDeviceInfo ();
	if (deviceInfo.size () > 1) {// deal with trailing newline
		deviceInfo.erase (deviceInfo.size ()-1, 1);
	}
	deviceInfoDisplay = new QLabel (qstr (deviceInfo), win);
	deviceInfoDisplay->setStyleSheet ("QLabel { font: 8pt }; ");
	layout->addWidget(header, 0);
	layout->addWidget(deviceInfoDisplay, 0);


	channelButtonsGroup = new QButtonGroup (win);
	QHBoxLayout* layout1 = new QHBoxLayout(this);
	layout1->setContentsMargins(0, 0, 0, 0);
	channel1Button = new CQRadioButton ("Channel 1 - No Control", win);
	channel1Button->setChecked( true );
	win->connect (channel1Button, &QRadioButton::toggled, [win, this]() {
		try {
			auto channel = (channel2Button->isChecked () ? 2 : 1);
			handleChannelPress (channel, win->mainWin->getProfileSettings ().configLocation, win->mainWin->getRunInfo ());
		}
		catch (ChimeraError & err) {
			win->reportErr (err.qtrace ());
		}
		});
	channelButtonsGroup->addButton (channel1Button);

	channel2Button = new CQRadioButton ("Channel 2 - No Control", win);
	channel2Button->setChecked (false);
	win->connect (channel2Button, &QRadioButton::toggled, [win, this]() {
		try {
			auto channel = (channel2Button->isChecked () ? 2 : 1);
			handleChannelPress (channel, win->mainWin->getProfileSettings ().configLocation, win->mainWin->getRunInfo ());
		}
		catch (ChimeraError & err) {
			win->reportErr (err.qtrace ());
		}
	});
	channelButtonsGroup->addButton (channel2Button);
	layout1->addWidget(channel1Button, 1);
	layout1->addWidget(channel2Button, 1);
	layout->addLayout(layout1, 0);


	QHBoxLayout* layout2 = new  QHBoxLayout(this);
	layout2->setContentsMargins(0, 0, 0, 0);
	syncedButton = new CQCheckBox ("Synced?", win);

	calibratedButton = new CQCheckBox ("Use Cal?", win);
	calibratedButton->setChecked( true );

	burstButton = new CQCheckBox ("Burst?", win);
	burstButton->setChecked (false);

	programNow = new CQPushButton ("Program", win);
	win->connect (programNow, &QPushButton::released, [win, this]() {
		try	{ 
			checkSave (win->mainWin->getProfileSettings ().configLocation, win->mainWin->getRunInfo ()); 
			programArbGenNow (win->auxWin->getUsableConstants ()); 
			win->reportStatus (qstr("Programmed Agilent " + getConfigDelim () + ".\r\n")); 
		}
		catch (ChimeraError& err) {
			win->reportErr (qstr("Error while programming agilent " + getConfigDelim () + ": " + err.trace () + "\r\n"));
		}
	});

	layout2->addWidget(syncedButton, 0);
	layout2->addWidget(calibratedButton, 0);
	layout2->addWidget(burstButton, 0);
	layout2->addWidget(programNow, 0);

	layout->addLayout(layout2, 0);

	QHBoxLayout* layout3 = new  QHBoxLayout(this);
	layout3->setContentsMargins(0, 0, 0, 0);
	settingCombo = new CQComboBox (win);
	win->connect ( settingCombo, qOverload<int> (&QComboBox::activated), [win, this](int) {
		try	{
			checkSave (win->mainWin->getProfileSettings ().configLocation, win->mainWin->getRunInfo ());
			readGuiSettings ();
			handleModeCombo ();
			updateSettingsDisplay (win->mainWin->getProfileSettings ().configLocation, win->mainWin->getRunInfo ());
		}
		catch (ChimeraError& err){
			win->reportErr (qstr("Error while handling agilent combo change: " + err.trace ()));
		}
	} );
	settingCombo->addItem ("No Control");
	settingCombo->addItem ("Output Off");
	settingCombo->addItem ("DC");
	settingCombo->addItem ("Sine");
	settingCombo->addItem ("Square");
	settingCombo->addItem ("Preloaded");
	settingCombo->addItem ("Scripted");
	settingCombo->setCurrentIndex (0);

	optionsFormat = new QLabel ("---", win);
	layout3->addWidget(settingCombo, 0);
	layout3->addWidget(optionsFormat, 1);
	layout->addLayout(layout3, 0);

	arbGenScript.initialize(win, "ArbGen", "" );

	currentGuiInfo.channel[0].option = ArbGenChannelMode::which::No_Control;
	currentGuiInfo.channel[1].option = ArbGenChannelMode::which::No_Control;
	arbGenScript.setEnabled ( false, false );
	try {
		pCore->programSetupCommands ();
	}
	catch (ChimeraError & error) {
		errBox ("Failed to program ArbGen " + getConfigDelim () + " initial settings: " + error.trace ());
	}
	layout->addWidget(&arbGenScript, 1);
}


ArbGenCore& ArbGenSystem::getCore (){
	return *pCore;
}


void ArbGenSystem::checkSave( std::string configPath, RunInfo info ){
	if ( currentGuiInfo.channel[currentChannel-1].option == ArbGenChannelMode::which::Script ){
		arbGenScript.checkSave( configPath, info );
	}
}


void ArbGenSystem::verifyScriptable ( ){
	if ( currentGuiInfo.channel[ currentChannel-1 ].option != ArbGenChannelMode::which::Script ){
		thrower ( "Agilent is not in scripting mode!" );
	}
}

void ArbGenSystem::setDefault (unsigned chan){
	pCore->setDefault (chan);
}


void ArbGenSystem::readGuiSettings(int chan ){
	if (chan != 1 && chan != 2){
		thrower ( "Bad argument for agilent channel in ArbGenSystem::handleInput(...)!" );
	}
	// convert to zero-indexed
	auto chani = chan - 1;
	currentGuiInfo.synced = syncedButton->isChecked( );
	std::string textStr(arbGenScript.getScriptText() );
	ConfigStream stream;
	stream << textStr;
	stream.seekg( 0 );
	switch (currentGuiInfo.channel[chani].option){
		case ArbGenChannelMode::which::No_Control:
		case ArbGenChannelMode::which::Output_Off:
			break;
		case ArbGenChannelMode::which::DC:
			stream >> currentGuiInfo.channel[chani].dc.dcLevel;
			currentGuiInfo.channel[chani].dc.useCal = calibratedButton->isChecked ( );
			break;
		case ArbGenChannelMode::which::Sine:
			stream >> currentGuiInfo.channel[chani].sine.frequency;
			stream >> currentGuiInfo.channel[chani].sine.amplitude;
			currentGuiInfo.channel[chani].sine.useCal = calibratedButton->isChecked ( );
			break;
		case ArbGenChannelMode::which::Square:
			stream >> currentGuiInfo.channel[chani].square.frequency;
			stream >> currentGuiInfo.channel[chani].square.amplitude;
			stream >> currentGuiInfo.channel[chani].square.offset;
			currentGuiInfo.channel[chani].square.useCal = calibratedButton->isChecked ( );
			break;
		case ArbGenChannelMode::which::Preloaded:
			stream >> currentGuiInfo.channel[chani].preloadedArb.address;
			currentGuiInfo.channel[chani].preloadedArb.useCal = calibratedButton->isChecked ( );
			currentGuiInfo.channel[chani].preloadedArb.burstMode = burstButton->isChecked ();
			break;
		case ArbGenChannelMode::which::Script:
			currentGuiInfo.channel[chani].scriptedArb.fileAddress = arbGenScript.getScriptPathAndName();
			currentGuiInfo.channel[chani].scriptedArb.useCal = calibratedButton->isChecked ( );
			break;
		default:
			thrower ( "unknown agilent option" );
	}
}


// overload for handling whichever channel is currently selected.
void ArbGenSystem::readGuiSettings(  ){
	// true -> 0 + 1 = 1
	// false -> 1 + 1 = 2
	readGuiSettings( (!channel1Button->isChecked ()) + 1 );
}


void ArbGenSystem::updateSettingsDisplay( std::string configPath, RunInfo currentRunInfo ){
	updateSettingsDisplay( (!channel1Button->isChecked ()) + 1, configPath, currentRunInfo );
}


void ArbGenSystem::updateButtonDisplay( int chan ){
	std::string channelText;
	channelText = chan == 1 ? "Channel 1 - " : "Channel 2 - ";
	channelText += ArbGenChannelMode::toStr ( currentGuiInfo.channel[ chan - 1 ].option );
	if ( chan == 1 ){
		channel1Button->setText ( cstr(channelText) );
	}
	else{
		channel2Button->setText ( cstr( channelText ) );
	}
}


void ArbGenSystem::updateSettingsDisplay(int chan, std::string configPath, RunInfo currentRunInfo){
	updateButtonDisplay( chan ); 
	// convert to zero-indexed.
	chan -= 1;
	switch ( currentGuiInfo.channel[chan].option ){
		case ArbGenChannelMode::which::No_Control:
			arbGenScript.reset ( );
			arbGenScript.setScriptText("");
			arbGenScript.setEnabled ( false, false );
			settingCombo->setCurrentIndex( 0 );
			break;
		case ArbGenChannelMode::which::Output_Off:
			arbGenScript.reset ( );
			arbGenScript.setScriptText("");
			arbGenScript.setEnabled ( false, false );
			settingCombo->setCurrentIndex ( 1 );
			break;
		case ArbGenChannelMode::which::DC:
			arbGenScript.reset ( );
			arbGenScript.setScriptText(currentGuiInfo.channel[chan].dc.dcLevel.expressionStr);
			settingCombo->setCurrentIndex ( 2 );
			calibratedButton->setChecked( currentGuiInfo.channel[chan].dc.useCal );
			arbGenScript.setEnabled ( true, false );
			break;
		case ArbGenChannelMode::which::Sine:
			arbGenScript.reset ( );
			arbGenScript.setScriptText(currentGuiInfo.channel[chan].sine.frequency.expressionStr + " "
										 + currentGuiInfo.channel[chan].sine.amplitude.expressionStr);
			settingCombo->setCurrentIndex ( 3 );
			calibratedButton->setChecked( currentGuiInfo.channel[chan].sine.useCal );
			arbGenScript.setEnabled ( true, false );
			break;
		case ArbGenChannelMode::which::Square:
			arbGenScript.reset ( );
			arbGenScript.setScriptText( currentGuiInfo.channel[chan].square.frequency.expressionStr + " "
										 + currentGuiInfo.channel[chan].square.amplitude.expressionStr + " " 
										 + currentGuiInfo.channel[chan].square.offset.expressionStr );
			calibratedButton->setChecked( currentGuiInfo.channel[chan].square.useCal );
			arbGenScript.setEnabled ( true, false );
			settingCombo->setCurrentIndex (4);
			break;
		case ArbGenChannelMode::which::Preloaded:
			arbGenScript.reset ( );
			arbGenScript.setScriptText(currentGuiInfo.channel[chan].preloadedArb.address.expressionStr);
			calibratedButton->setChecked( currentGuiInfo.channel[chan].preloadedArb.useCal );
			burstButton->setChecked (currentGuiInfo.channel[chan].preloadedArb.burstMode);
			arbGenScript.setEnabled ( true, false );
			settingCombo->setCurrentIndex (5);
			break;
		case ArbGenChannelMode::which::Script:
			// clear it in case the file fails to open.
			arbGenScript.setScriptText( "" );
			arbGenScript.openParentScript( currentGuiInfo.channel[chan].scriptedArb.fileAddress.expressionStr, configPath,
											currentRunInfo );
			calibratedButton->setChecked( currentGuiInfo.channel[chan].scriptedArb.useCal );
			arbGenScript.setEnabled ( true, false );
			settingCombo->setCurrentIndex (6);
			break;
		default:
			thrower ( "unrecognized agilent setting: " + ArbGenChannelMode::toStr(currentGuiInfo.channel[chan].option));
	}
	currentChannel = chan+1;
}


void ArbGenSystem::handleChannelPress( int chan, std::string configPath, RunInfo currentRunInfo ){
	// convert from channel 1/2 to 0/1 to access the right array entr
	readGuiSettings( currentChannel );
	updateSettingsDisplay( chan, configPath, currentRunInfo );
	currentChannel = channel1Button->isChecked( ) ? 1 : 2;
}


void ArbGenSystem::handleModeCombo(){
	if (!optionsFormat) {
		return;
	}
	int selection = settingCombo->currentIndex();
	int selectedChannel = int( !channel1Button->isChecked() );
	switch (selection) {
		case 0:
			optionsFormat->setText( "---" );
			currentGuiInfo.channel[selectedChannel].option = ArbGenChannelMode::which::No_Control;
			arbGenScript.setEnabled ( false, false );
			break;
		case 1:
			optionsFormat->setText ( "---" );
			currentGuiInfo.channel[selectedChannel].option = ArbGenChannelMode::which::Output_Off;
			arbGenScript.setEnabled ( false, false );
			break;
		case 2:
			optionsFormat->setText ( "[DC Level]" );
			currentGuiInfo.channel[selectedChannel].option = ArbGenChannelMode::which::DC;
			arbGenScript.setEnabled ( true, false );
			break;
		case 3:
			optionsFormat->setText ( "[Frequency(kHz)] [Amplitude(Vpp)]" );
			currentGuiInfo.channel[selectedChannel].option = ArbGenChannelMode::which::Sine;
			arbGenScript.setEnabled ( true, false );
			break;
		case 4:
			optionsFormat->setText ( "[Frequency(kHz)] [Amplitude(Vpp)] [Offset(V)]" );
			currentGuiInfo.channel[selectedChannel].option = ArbGenChannelMode::which::Square;
			arbGenScript.setEnabled ( true, false );
			break;
		case 5:
			optionsFormat->setText ( "[File Address]" );
			currentGuiInfo.channel[selectedChannel].option = ArbGenChannelMode::which::Preloaded;
			arbGenScript.setEnabled ( true, false );
			break;
		case 6:
			optionsFormat->setText ( "Hover over \"?\"" );
			currentGuiInfo.channel[selectedChannel].option = ArbGenChannelMode::which::Script;
			arbGenScript.setEnabled ( true, false );
			break;
	}
}


deviceOutputInfo ArbGenSystem::getOutputInfo(){
	return currentGuiInfo;
}

/*
This function outputs a string that contains all of the information that is set by the user for a given configuration. 
*/
void ArbGenSystem::handleSavingConfig(ConfigStream& saveFile, std::string configPath, RunInfo info){	
	// make sure data is up to date.
	readGuiSettings (currentChannel);
	// start outputting.
	saveFile << pCore->configDelim+"\n";
	saveFile << "/*Synced Option:*/ " << str (currentGuiInfo.synced);
	std::vector<std::string> channelStrings = { "\nCHANNEL_1", "\nCHANNEL_2" };
	for (auto chanInc : range (2)){
		auto& channel = currentGuiInfo.channel[chanInc];
		saveFile << channelStrings[chanInc];
		saveFile << "\n/*Channel Mode:*/\t\t\t\t" << ArbGenChannelMode::toStr (channel.option);
		saveFile << "\n/*DC Level:*/\t\t\t\t\t" << channel.dc.dcLevel;
		saveFile << "\n/*DC Calibrated:*/\t\t\t\t" << channel.dc.useCal;
		saveFile << "\n/*Sine Amplitude:*/\t\t\t\t" << channel.sine.amplitude;
		saveFile << "\n/*Sine Freq:*/\t\t\t\t\t" << channel.sine.frequency;
		saveFile << "\n/*Sine Calibrated:*/\t\t\t" << channel.sine.useCal;
		saveFile << "\n/*Square Amplitude:*/\t\t\t" << channel.square.amplitude;
		saveFile << "\n/*Square Freq:*/\t\t\t\t" << channel.square.frequency;
		saveFile << "\n/*Square Offset:*/\t\t\t\t" << channel.square.offset;
		saveFile << "\n/*Square Calibrated:*/\t\t\t" << channel.square.useCal;
		saveFile << "\n/*Preloaded Arb Address:*/\t\t" << channel.preloadedArb.address;
		saveFile << "\n/*Preloaded Arb Calibrated:*/\t" << channel.preloadedArb.useCal;
		saveFile << "\n/*Preloaded Arb Burst:*/\t" << channel.preloadedArb.burstMode;
		saveFile << "\n/*Scripted Arb Address:*/\t\t" << channel.scriptedArb.fileAddress;
		saveFile << "\n/*Scripted Arb Calibrated:*/\t" << channel.scriptedArb.useCal;
	}
	saveFile << "\nEND_" + pCore->configDelim + "\n";
}

void ArbGenSystem::setOutputSettings (deviceOutputInfo info){
	currentGuiInfo = info;
	updateButtonDisplay (1);
	updateButtonDisplay (2);
}


void ArbGenSystem::handleOpenConfig( ConfigStream& file ){
	setOutputSettings (pCore->getSettingsFromConfig (file));
}


bool ArbGenSystem::scriptingModeIsSelected (){
	return currentGuiInfo.channel[currentChannel - 1].option == ArbGenChannelMode::which::Script;
}

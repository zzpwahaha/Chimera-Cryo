#include "stdafx.h"
#include "QtScriptWindow.h"
#include <qdesktopwidget.h>
#include <qlayout.h>
#include <PrimaryWindows/QtScriptWindow.h>
#include <PrimaryWindows/QtAndorWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>
#include <PrimaryWindows/QtMainWindow.h>
#include <ExcessDialogs/saveWithExplorer.h>
#include <ExcessDialogs/openWithExplorer.h>

QtScriptWindow::QtScriptWindow(QWidget* parent) : IChimeraQtWindow(parent)
	, masterScript(this)
	, intensityAgilent(UWAVE_AGILENT_SETTINGS, this)
{
	setWindowTitle ("Script Window");
}

QtScriptWindow::~QtScriptWindow (){
}

void QtScriptWindow::initializeWidgets (){
	statBox = new ColorBox(this, mainWin->getDevices());
	QWidget* centralWidget = new QWidget();
	setCentralWidget(centralWidget);
	QHBoxLayout* layout = new QHBoxLayout(centralWidget);
	//centralWidget->setStyleSheet("border: 2px solid  black; ");
	intensityAgilent.initialize("Tweezer Intensity Agilent", this);
	masterScript.initialize(this, "Master", "Master Script");
	profileDisplay.initialize (this);
	layout->addWidget(&profileDisplay, 1);
	layout->addWidget(&intensityAgilent, 1);
	layout->addWidget(&masterScript, 1);
	
	try {
		// I only do this for the intensity agilent at the moment.
		intensityAgilent.setDefault(1);
	}
	catch (ChimeraError& err) {
		errBox("ERROR: Failed to initialize intensity agilent: " + err.trace());
	}


	updateDoAoDdsNames ();
	updateVarNames ();
}

void QtScriptWindow::updateVarNames() {
	auto params = auxWin->getAllParams ();
	masterScript.highlighter->setOtherParams (params);
	masterScript.highlighter->setLocalParams (masterScript.getLocalParams ());
	masterScript.highlighter->rehighlight();

	intensityAgilent.agilentScript.highlighter->setOtherParams(params);
	intensityAgilent.agilentScript.highlighter->setLocalParams(intensityAgilent.agilentScript.getLocalParams());
	intensityAgilent.agilentScript.highlighter->rehighlight();
}

void QtScriptWindow::updateDoAoDdsNames () {
	auto doNamesArr = auxWin->getTtlNames ();
	auto doNames = std::vector<std::string>(doNamesArr.begin(), doNamesArr.end());
	auto aoNamesArr = auxWin->getDacNames();
	auto aoNames = std::vector<std::string>(aoNamesArr.begin(), aoNamesArr.end());
	auto ddsNamesArr = auxWin->getDdsNames();
	auto ddsNames = std::vector<std::string>(ddsNamesArr.begin(), ddsNamesArr.end());
	auto olNamesArr = auxWin->getOlNames();
	auto olNames = std::vector<std::string>(olNamesArr.begin(), olNamesArr.end());

	
	masterScript.highlighter->setTtlNames(doNames);
	masterScript.highlighter->setDacNames(aoNames);
	masterScript.highlighter->setDdsNames(ddsNames);
	masterScript.highlighter->setOlNames(olNames);
	masterScript.highlighter->rehighlight();

	intensityAgilent.agilentScript.highlighter->setTtlNames(doNames);
	intensityAgilent.agilentScript.highlighter->setDacNames(aoNames);
	intensityAgilent.agilentScript.highlighter->setDdsNames(doNames);
	intensityAgilent.agilentScript.highlighter->setOlNames(aoNames);
	masterScript.highlighter->rehighlight();
}


void QtScriptWindow::handleMasterFunctionChange (){
	try{
		masterScript.functionChangeHandler (mainWin->getProfileSettings ().configLocation);
		masterScript.updateSavedStatus (true);
	}
	catch (ChimeraError& err){
		errBox (err.trace ());
	}
}

void QtScriptWindow::handleIntensityCombo() {
	intensityAgilent.checkSave(mainWin->getProfileSettings().configLocation, mainWin->getRunInfo());
	intensityAgilent.readGuiSettings();
	intensityAgilent.handleModeCombo();
	intensityAgilent.updateSettingsDisplay(mainWin->getProfileSettings().configLocation, mainWin->getRunInfo());
}


void QtScriptWindow::checkScriptSaves (){
	masterScript.checkSave (getProfile ().configLocation, mainWin->getRunInfo());
	intensityAgilent.checkSave(getProfile().configLocation, mainWin->getRunInfo());
}

std::string QtScriptWindow::getSystemStatusString (){
	std::string status = "Intensity Agilent:\n\t" + intensityAgilent.getDeviceIdentity();
	return status;
}

/* 
  This function retuns the names (just the names) of currently active scripts.
*/
scriptInfo<std::string> QtScriptWindow::getScriptNames (){
	scriptInfo<std::string> names;
	names.master = masterScript.getScriptName ();
	names.intensityAgilent = intensityAgilent.agilentScript.getScriptName();
	return names;
}

/*
  This function returns indicators of whether a given script has been saved or not.
*/
scriptInfo<bool> QtScriptWindow::getScriptSavedStatuses (){
	scriptInfo<bool> status;
	status.intensityAgilent = intensityAgilent.agilentScript.savedStatus();
	status.master = masterScript.savedStatus ();
	return status;
}

/*
  This function returns the current addresses of all files in all scripts.
*/
scriptInfo<std::string> QtScriptWindow::getScriptAddresses (){
	scriptInfo<std::string> addresses;
	addresses.intensityAgilent = intensityAgilent.agilentScript.getScriptPathAndName();
	addresses.master = masterScript.getScriptPathAndName ();
	return addresses;
}

void QtScriptWindow::setIntensityDefault() 
{
	try {
		intensityAgilent.setDefault(1);
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}

/// Commonly Called Functions
/*
	The following set of functions, mostly revolving around saving etc. of the script files, are called by all of the
	window objects because they are associated with the menu at the top of each screen
*/

void QtScriptWindow::newIntensityScript() {
	try {
		intensityAgilent.verifyScriptable();
		intensityAgilent.checkSave(getProfile().configLocation, mainWin->getRunInfo());
		intensityAgilent.agilentScript.newScript();
		updateConfigurationSavedStatus(false);
		intensityAgilent.agilentScript.updateScriptNameText(mainWin->getProfileSettings().configLocation);
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}

void QtScriptWindow::openIntensityScript(IChimeraQtWindow* parent) {
	try {
		intensityAgilent.verifyScriptable();
		intensityAgilent.checkSave(getProfile().configLocation, mainWin->getRunInfo());
		std::string intensityOpenName = openWithExplorer(parent, Script::AGILENT_SCRIPT_EXTENSION);
		intensityAgilent.agilentScript.openParentScript(intensityOpenName, getProfile().configLocation,
			mainWin->getRunInfo());
		updateConfigurationSavedStatus(false);
		intensityAgilent.agilentScript.updateScriptNameText(getProfile().configLocation);
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}

void QtScriptWindow::openIntensityScript(std::string name) {
	intensityAgilent.agilentScript.openParentScript(name, getProfile().configLocation, mainWin->getRunInfo());
}

void QtScriptWindow::saveIntensityScript() {
	try {
		// channel 0 is the intensity channel, the 4th option is the scripting option.
		if (intensityAgilent.getOutputInfo().channel[0].option == ArbGenChannelMode::which::Script) {
			intensityAgilent.agilentScript.saveScript(getProfile().configLocation, mainWin->getRunInfo());
			intensityAgilent.agilentScript.updateScriptNameText(getProfile().configLocation);
		}
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}

void QtScriptWindow::saveIntensityScriptAs(IChimeraQtWindow* parent) {
	try {
		intensityAgilent.verifyScriptable();
		std::string extensionNoPeriod = intensityAgilent.agilentScript.getExtension();
		if (extensionNoPeriod.size() == 0) {
			return;
		}
		extensionNoPeriod = extensionNoPeriod.substr(1, extensionNoPeriod.size());
		std::string newScriptAddress = saveWithExplorer(parent, extensionNoPeriod, getProfileSettings());
		intensityAgilent.agilentScript.saveScriptAs(newScriptAddress, mainWin->getRunInfo());
		updateConfigurationSavedStatus(false);
		intensityAgilent.agilentScript.updateScriptNameText(getProfile().configLocation);
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}






// just a quick shortcut.
profileSettings QtScriptWindow::getProfile (){
	return mainWin->getProfileSettings ();
}

void QtScriptWindow::windowOpenConfig (ConfigStream& configFile){
	try{
		ConfigSystem::initializeAtDelim (configFile, "SCRIPTS");
	}
	catch (ChimeraError&){
		reportErr ("Failed to initialize configuration file at scripting window entry point \"SCRIPTS\".");
		return;
	}
	try{
		configFile.get ();
		auto getlineFunc = ConfigSystem::getGetlineFunc (configFile.ver);
		std::string masterName;
		getlineFunc (configFile, masterName);
		ConfigSystem::checkDelimiterLine (configFile, "END_SCRIPTS");

		deviceOutputInfo info;
		ConfigSystem::stdGetFromConfig(configFile, intensityAgilent.getCore(), info, Version("1.0"));
		intensityAgilent.setOutputSettings(info);
		intensityAgilent.updateSettingsDisplay(mainWin->getProfileSettings().configLocation, mainWin->getRunInfo());

		try{
			openMasterScript (masterName);
		}
		catch (ChimeraError& err){
			auto answer = QMessageBox::question (this, "Open Failed", "ERROR: Failed to open master script file: " 
				+ qstr(masterName) + ", with error \r\n" + err.qtrace () + "\r\nAttempt to find file yourself?");
			if (answer == QMessageBox::Yes){
				openMasterScript (openWithExplorer (nullptr, "mScript"));
			}
		}
		considerScriptLocations();
	}
	catch (ChimeraError& err)	{
		reportErr ("Scripting Window failed to read parameters from the configuration file.\n\n" + err.qtrace ());
	}
}

void QtScriptWindow::newMasterScript (){
	try {
		masterScript.checkSave (getProfile ().configLocation, mainWin->getRunInfo());
		masterScript.newScript ();
		updateConfigurationSavedStatus (false);
		masterScript.updateScriptNameText (getProfile ().configLocation);
	}
	catch (ChimeraError & err) {
		reportErr (err.qtrace ());
	}
}

void QtScriptWindow::openMasterScript (IChimeraQtWindow* parent){
	try	{
		masterScript.checkSave (getProfile ().configLocation, mainWin->getRunInfo());
		std::string openName = openWithExplorer (parent, Script::MASTER_SCRIPT_EXTENSION);
		masterScript.openParentScript (openName, getProfile ().configLocation, mainWin->getRunInfo());
		updateConfigurationSavedStatus (false);
		masterScript.updateScriptNameText (getProfile ().configLocation);
	}
	catch (ChimeraError& err){
		reportErr ("Open Master Script Failed: " + err.qtrace () + "\r\n");
	}
}

void QtScriptWindow::saveMasterScript (){
	if (masterScript.isFunction ())	{
		masterScript.saveAsFunction ();
		return;
	}
	masterScript.saveScript (getProfile ().configLocation, mainWin->getRunInfo());
	masterScript.updateScriptNameText (getProfile ().configLocation);
}

void QtScriptWindow::saveMasterScriptAs (IChimeraQtWindow* parent){
	std::string extensionNoPeriod = masterScript.getExtension ();
	if (extensionNoPeriod.size () == 0)	{
		return;
	}
	extensionNoPeriod = extensionNoPeriod.substr (1, extensionNoPeriod.size ());
	std::string newScriptAddress = saveWithExplorer (parent, extensionNoPeriod, getProfileSettings ());
	masterScript.saveScriptAs (newScriptAddress, mainWin->getRunInfo());
	updateConfigurationSavedStatus (false);
	masterScript.updateScriptNameText (getProfile ().configLocation);
}

void QtScriptWindow::newMasterFunction (){
	try{
		masterScript.newFunction ();
	}
	catch (ChimeraError& exception){
		reportErr ("New Master function Failed: " + exception.qtrace () + "\r\n");
	}
}

void QtScriptWindow::saveMasterFunction (){
	try{
		masterScript.saveAsFunction ();
	}
	catch (ChimeraError& exception){
		reportErr ("Save Master Script Function Failed: " + exception.qtrace () + "\r\n");
	}
}

void QtScriptWindow::deleteMasterFunction (){
	// todo. Right now you can just delete the file itself...
}

void QtScriptWindow::windowSaveConfig (ConfigStream& saveFile){
	scriptInfo<std::string> addresses = getScriptAddresses ();
	// order matters!
	saveFile << "SCRIPTS\n";
	saveFile << "/*Master Script Address:*/ " << addresses.master << "\n";
	saveFile << "END_SCRIPTS\n";

	intensityAgilent.handleSavingConfig(saveFile, mainWin->getProfileSettings().configLocation, mainWin->getRunInfo());
}

void QtScriptWindow::checkMasterSave (){
	masterScript.checkSave (getProfile ().configLocation, mainWin->getRunInfo());
}

void QtScriptWindow::openMasterScript (std::string name){
	masterScript.openParentScript (name, getProfile ().configLocation, mainWin->getRunInfo());
}

void QtScriptWindow::considerScriptLocations() {
	intensityAgilent.agilentScript.considerCurrentLocation(getProfile().configLocation, mainWin->getRunInfo());
}

void QtScriptWindow::updateProfile (std::string text){
	profileDisplay.update (text);
}

profileSettings QtScriptWindow::getProfileSettings (){
	return mainWin->getProfileSettings ();
}

void QtScriptWindow::updateConfigurationSavedStatus (bool status){
	mainWin->updateConfigurationSavedStatus (status);
}

void QtScriptWindow::fillExpDeviceList (DeviceList& list) {
	list.list.push_back(intensityAgilent.getCore());
}
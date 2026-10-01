#include "stdafx.h"
#include "QtScriptWindow.h"
#include <qdesktopwidget.h>
#include <qlayout.h>
#include <PrimaryWindows/QtScriptWindow.h>
#include <PrimaryWindows/QtAndorWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>
#include <PrimaryWindows/QtMakoWindow.h>
#include <PrimaryWindows/QtMainWindow.h>
#include <ExcessDialogs/saveWithExplorer.h>
#include <ExcessDialogs/openWithExplorer.h>

QtScriptWindow::QtScriptWindow(QWidget* parent) : IChimeraQtWindow(parent)
	, masterScript(this)
	, arbGens{ {
			ArbGenSystem(UWAVE_SIGLENT_SETTINGS, ArbGenType::Siglent, this),
			ArbGenSystem(UWAVE_SIGLENT2_SETTINGS, ArbGenType::Siglent, this),
			ArbGenSystem(UWAVE_AGILENT_SETTINGS, ArbGenType::Agilent, this),
			ArbGenSystem(UWAVE_SIGLENT3_SETTINGS, ArbGenType::Siglent, this),
			ArbGenSystem(UWAVE_AGILENT2_SETTINGS, ArbGenType::Agilent, this),} }
	, gigaMoog(this)
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
	for (auto name : ArbGenEnum::allAgs) {
		arbGens[(int)name].initialize(arbGens[(int)name].initSettings.deviceName, this);
	}

	

	masterScript.initialize(this, "Master", "Master Script");
	gigaMoog.initialize(this);
	//profileDisplay.initialize (this);
	QVBoxLayout* layout1 = new QVBoxLayout(this);
	layout1->setContentsMargins(0, 0, 0, 0);
	layout1->addWidget(&arbGens[3], 1);
	layout1->addWidget(&arbGens[1], 1);
	layout1->addWidget(&arbGens[0], 1);
	layout1->addWidget(&arbGens[2], 1);
	layout->addLayout(layout1, 1);
	QVBoxLayout* layout2 = new QVBoxLayout(this);
	layout2->setContentsMargins(0, 0, 0, 0);
	layout2->addWidget(&arbGens[4], 0);
	arbGens[4].setMaximumHeight(232);
	arbGens[4].setMaximumWidth(600);

	QHBoxLayout* gmoogBtnLayout = new QHBoxLayout();
	gmoogBtnLayout->setContentsMargins(0, 0, 0, 0);
	QPushButton* openGm1Btn = new QPushButton("Open GM", this);
	QPushButton* openGm2Btn = new QPushButton("Open GM420", this);
	gmoogBtnLayout->addWidget(openGm1Btn);
	gmoogBtnLayout->addWidget(openGm2Btn);
	gmoogBtnLayout->addStretch(1);
	layout2->addLayout(gmoogBtnLayout);
	connect(openGm1Btn, &QPushButton::released, this, [this]() {
		openGMoogScript(1, this);
		});
	connect(openGm2Btn, &QPushButton::released, this, [this]() {
		openGMoogScript(2, this);
		});

	layout2->addWidget(&gigaMoog, 1);
	layout->addLayout(layout2, 1);
	layout->addWidget(&masterScript, 1);
	
	try {
		for (auto name : ArbGenEnum::allAgs) {
			//arbGens[(int)name].setDefault(1);
			//arbGens[(int)name].setDefault(2);
		}
		//intensityAgilent.setDefault(1);
	}
	catch (ChimeraError& err) {
		errBox("ERROR: Failed to initialize ArbGens: " + err.trace());
	}


	updateDoAoDdsNames ();
	updateVarNames ();
}

void QtScriptWindow::updateVarNames() {
	auto params = auxWin->getAllParams ();
	masterScript.highlighter->setGlobalParams(auxWin->getGlobalParams());
	masterScript.highlighter->setOtherParams (auxWin->getConfigParams());
	masterScript.highlighter->setLocalParams (masterScript.getLocalParams ());
	masterScript.highlighter->rehighlight();

	for (auto name : ArbGenEnum::allAgs) {
		arbGens[(int)name].arbGenScript.highlighter->setOtherParams(params);
		arbGens[(int)name].arbGenScript.highlighter->setLocalParams(arbGens[(int)name].arbGenScript.getLocalParams());
		arbGens[(int)name].arbGenScript.highlighter->rehighlight();
	}
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
	auto calNames = auxWin->getCalNames();

	
	masterScript.highlighter->setTtlNames(doNames);
	masterScript.highlighter->setDacNames(aoNames);
	masterScript.highlighter->setDdsNames(ddsNames);
	masterScript.highlighter->setOlNames(olNames);
	masterScript.highlighter->setCalNames(calNames);
	masterScript.highlighter->rehighlight();

	for (auto name : ArbGenEnum::allAgs) {
		arbGens[(int)name].arbGenScript.highlighter->setTtlNames(doNames);
		arbGens[(int)name].arbGenScript.highlighter->setDacNames(aoNames);
		arbGens[(int)name].arbGenScript.highlighter->setDdsNames(ddsNames);
		arbGens[(int)name].arbGenScript.highlighter->setOlNames(olNames);
		arbGens[(int)name].arbGenScript.highlighter->setCalNames(calNames);
		arbGens[(int)name].arbGenScript.highlighter->rehighlight();
	}

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

void QtScriptWindow::checkScriptSaves (){
	masterScript.checkSave (getProfile ().configLocation, mainWin->getRunInfo());
	gigaMoog.gmoogScript.checkSave(getProfile().configLocation, mainWin->getRunInfo());
	gigaMoog.gmoogScript420.checkSave(getProfile().configLocation, mainWin->getRunInfo());
	for (auto name : ArbGenEnum::allAgs) {
		arbGens[(int)name].checkSave(getProfile().configLocation, mainWin->getRunInfo());
	}
	//intensityAgilent.checkSave(getProfile().configLocation, mainWin->getRunInfo());
}

std::string QtScriptWindow::getSystemStatusString (){
	//std::string status = "Intensity Agilent:\n\t" + intensityAgilent.getDeviceIdentity();
	std::string status;
	for (auto name : ArbGenEnum::allAgs) {
		status += arbGens[(int)name].initSettings.deviceName + ":\n\t" + arbGens[(int)name].getDeviceIdentity();
		status += "\t";
		status += "Attached trigger line is \n\t\t";
		{
			status += "(" + str(arbGens[(int)name].initSettings.triggerRow) + "," + str(arbGens[(int)name].initSettings.triggerNumber) + ") ";
		}
		status += "\n";
	}
	status += "GIGAMOOG:\n\t";
	if (!GIGAMOOG_SAFEMODE) {
		status += str("GIGAMOOG System is Active at " + GIGAMOOG_IPADDRESS + " and port," + str(GIGAMOOG_IPPORT) + "\n\t");
		status += "Attached trigger line is \n\t\t";
		for (const auto& gmtrig : GM_TRIGGER_LINE) {
			status += "(" + str(gmtrig.first) + "," + str(gmtrig.second) + ") ";
		}
		status += "\n";
	}
	else {
		status += "\tGIGAMOOG System is disabled! Enable in \"constants.h\" \n";
	}

	status += "GIGAMOOG420:\n\t";
	if (!GIGAMOOG420_SAFEMODE) {
		status += str("GIGAMOOG420 System is Active at " + GIGAMOOG420_IPADDRESS + " and port," + str(GIGAMOOG420_IPPORT) + "\n\t");
		status += "Attached trigger line is \n\t\t";
		for (const auto& gmtrig : GM420_TRIGGER_LINE) {
			status += "(" + str(gmtrig.first) + "," + str(gmtrig.second) + ") ";
		}
		status += "\n";
	}
	else {
		status += "\tGIGAMOOG420 System is disabled! Enable in \"constants.h\" \n";
	}
	return status;
}

/* 
  This function retuns the names (just the names) of currently active scripts.
*/
scriptInfo<std::string> QtScriptWindow::getScriptNames (){
	scriptInfo<std::string> names;
	names.master = masterScript.getScriptName ();
	names.gmoog = gigaMoog.gmoogScript.getScriptName();
	names.gmoog420 = gigaMoog.gmoogScript420.getScriptName();

	//names.intensityAgilent = intensityAgilent.arbGenScript.getScriptName();
	return names;
}

/*
  This function returns indicators of whether a given script has been saved or not.
*/
scriptInfo<bool> QtScriptWindow::getScriptSavedStatuses (){
	scriptInfo<bool> status;
	//status.intensityAgilent = intensityAgilent.arbGenScript.savedStatus();
	status.master = masterScript.savedStatus ();
	status.gmoog = gigaMoog.gmoogScript.savedStatus();
	status.gmoog420 = gigaMoog.gmoogScript420.savedStatus();

	return status;
}

/*
  This function returns the current addresses of all files in all scripts.
*/
scriptInfo<std::string> QtScriptWindow::getScriptAddresses (){
	scriptInfo<std::string> addresses;
	//addresses.intensityAgilent = intensityAgilent.arbGenScript.getScriptPathAndName();
	addresses.master = masterScript.getScriptPathAndName ();
	addresses.gmoog = gigaMoog.gmoogScript.getScriptPathAndName();
	addresses.gmoog420 = gigaMoog.gmoogScript420.getScriptPathAndName();
	return addresses;
}

void QtScriptWindow::setIntensityDefault() 
{
	try {
		for (auto name : ArbGenEnum::allAgs) {
			arbGens[(int)name].setDefault(1);
			arbGens[(int)name].setDefault(2);
		}
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

void QtScriptWindow::updateArbGen(ArbGenEnum::name name) {
	try {
		updateConfigurationSavedStatus(false);
		arbGens[(int)name].checkSave(getProfile().configLocation, mainWin->getRunInfo());
		arbGens[(int)name].readGuiSettings();
	}
	catch (ChimeraError&) {
		throwNested("Failed to update arbGens.");
	}
}


void QtScriptWindow::newArbGenScript(ArbGenEnum::name name) 
{
	try {
		arbGens[(int)name].verifyScriptable();
		mainWin->updateConfigurationSavedStatus(false);
		arbGens[(int)name].checkSave(mainWin->getProfileSettings().configLocation, mainWin->getRunInfo());
		arbGens[(int)name].arbGenScript.newScript();
		arbGens[(int)name].arbGenScript.updateScriptNameText(mainWin->getProfileSettings().configLocation);
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}

void QtScriptWindow::openArbGenScript(ArbGenEnum::name name, IChimeraQtWindow* parent)
{
	try {
		arbGens[(int)name].verifyScriptable();
		updateConfigurationSavedStatus(false);
		arbGens[(int)name].checkSave(getProfile().configLocation, mainWin->getRunInfo());
		std::string openFileName = openWithExplorer(parent, Script::ARBGEN_SCRIPT_EXTENSION, CONFIGURATION_PATH);
		arbGens[(int)name].arbGenScript.openParentScript(openFileName, getProfile().configLocation, 
			mainWin->getRunInfo());
		arbGens[(int)name].arbGenScript.updateScriptNameText(getProfile().configLocation);
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}

void QtScriptWindow::saveArbGenScript(ArbGenEnum::name name) {
	try {
		arbGens[(int)name].verifyScriptable();
		arbGens[(int)name].arbGenScript.saveScript(getProfile().configLocation, mainWin->getRunInfo());
		arbGens[(int)name].arbGenScript.updateScriptNameText(getProfile().configLocation);
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}

void QtScriptWindow::saveArbGenScriptAs(ArbGenEnum::name name, IChimeraQtWindow* parent) {
	try {
		arbGens[(int)name].verifyScriptable();
		updateConfigurationSavedStatus(false);
		std::string extensionNoPeriod = arbGens[(int)name].arbGenScript.getExtension();
		if (extensionNoPeriod.size() == 0) {
			return;
		}
		extensionNoPeriod = extensionNoPeriod.substr(1, extensionNoPeriod.size());
		std::string newScriptAddress = saveWithExplorer(parent, extensionNoPeriod, getProfileSettings());
		arbGens[(int)name].arbGenScript.saveScriptAs(newScriptAddress, mainWin->getRunInfo());
		arbGens[(int)name].arbGenScript.updateScriptNameText(getProfile().configLocation);
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}




// just a quick shortcut.
profileSettings QtScriptWindow::getProfile (){
	return mainWin->getProfileSettings ();
}

void QtScriptWindow::windowOpenConfig(ConfigStream& configFile) {
	try {
		ConfigSystem::initializeAtDelim(configFile, "SCRIPTS");
	}
	catch (ChimeraError&) {
		reportErr("Failed to initialize configuration file at scripting window entry point \"SCRIPTS\".");
		return;
	}
	try {
		auto getlineFunc = ConfigSystem::getGetlineFunc(configFile.ver);
		std::string masterName/*, gmoogName*/;
		// order should match the windowsaveconfig
		getlineFunc(configFile, masterName);
		//getlineFunc(configFile, gmoogName);
		ConfigSystem::checkDelimiterLine(configFile, "END_SCRIPTS");
		try {
			openMasterScript(masterName);
		}
		catch (ChimeraError& err) {
			auto answer = QMessageBox::question(this, "Open Failed", "ERROR: Failed to open master script file: "
				+ qstr(masterName) + ", with error \r\n" + err.qtrace() + "\r\nAttempt to find file yourself?");
			if (answer == QMessageBox::Yes) {
				openMasterScript(openWithExplorer(nullptr, "mScript", CONFIGURATION_PATH));
			}
		}

		ConfigSystem::standardOpenConfig(configFile, gigaMoog.getDelim(), &gigaMoog);
		try {
			openGMoogScript(1, gigaMoog.scriptAddress);
		}
		catch (ChimeraError& err) {
			auto answer = QMessageBox::question(this, "Open Failed", "ERROR: Failed to open GigaMoog script file: "
				+ qstr(gigaMoog.scriptAddress) + ", with error \r\n" + err.qtrace() + "\r\nAttempt to find file yourself?");
			if (answer == QMessageBox::Yes) {
				openGMoogScript(openWithExplorer(nullptr, "gScript", CONFIGURATION_PATH));
			}
		}

		// Handle old config files with no GMOOG420 delimiter
		bool has420Section = false;
		try {
			ConfigSystem::initializeAtDelim(configFile, gigaMoog.getDelim420());
			gigaMoog.handleOpenConfig420(configFile);
			ConfigSystem::checkDelimiterLine(configFile, "END_" + gigaMoog.getDelim420());
			has420Section = true;
		}
		catch (ChimeraError&) {
			reportErr("No GMOOG420 section in this config. Re-save the config to add one.\r\n");
		}

		if (has420Section) {
			try {
				openGMoogScript(2, gigaMoog.scriptAddress420);
			}
			catch (ChimeraError& err) {
				auto answer = QMessageBox::question(this, "Open Failed", "ERROR: Failed to open GigaMoog420 script file: "
					+ qstr(gigaMoog.scriptAddress420) + ", with error \r\n" + err.qtrace() + "\r\nAttempt to find file yourself?");
				if (answer == QMessageBox::Yes) {
					openGMoogScript(2, openWithExplorer(nullptr, "gScript", CONFIGURATION_PATH));
				}
			}
		}

		for (auto name : ArbGenEnum::allAgs) {
			deviceOutputInfo info;
			ConfigSystem::stdGetFromConfig(configFile, arbGens[(int)name].getCore(), info, Version("1.0"));
			arbGens[(int)name].setOutputSettings(info);
			arbGens[(int)name].updateSettingsDisplay(getProfileSettings().configLocation, mainWin->getRunInfo());
		}


		considerScriptLocations();
	}
	catch (ChimeraError& err) {
		reportErr("Scripting Window failed to read parameters from the configuration file.\n\n" + err.qtrace());
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
		std::string openName = openWithExplorer (parent, Script::MASTER_SCRIPT_EXTENSION, CONFIGURATION_PATH);
		masterScript.openParentScript (openName, getProfile ().configLocation, mainWin->getRunInfo());
		updateConfigurationSavedStatus (false);
		masterScript.updateScriptNameText (getProfile ().configLocation);
	}
	catch (ChimeraError& err){
		reportErr ("Open Master Script Failed: " + err.qtrace () + "\r\n");
	}
}

void QtScriptWindow::openMasterScript(std::string name, bool askMove) {
	masterScript.openParentScript(name, getProfile().configLocation, mainWin->getRunInfo(), askMove);
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

void QtScriptWindow::reloadMasterFunction()
{
	try {
		masterScript.loadFunctions();
	}
	catch (ChimeraError& exception) {
		reportErr("New Master function Failed: " + exception.qtrace() + "\r\n");
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

void QtScriptWindow::newGMoogScript() { newGMoogScript(1); }
void QtScriptWindow::openGMoogScript(IChimeraQtWindow* parent) { openGMoogScript(1, parent); }
void QtScriptWindow::openGMoogScript(std::string name) { openGMoogScript(1, name); }
void QtScriptWindow::saveGMoogScript() { saveGMoogScript(1); }
void QtScriptWindow::saveGMoogScriptAs(IChimeraQtWindow* parent) { saveGMoogScriptAs(1, parent); }

void QtScriptWindow::newGMoogScript(int deviceIndex)
{
	try {
		auto profile = getProfile();
		auto run = mainWin->getRunInfo();
		auto& profileLoc = profile.configLocation;
		auto& runInfo = run;
		if (deviceIndex == 1) {
			gigaMoog.gmoogScript.checkSave(profileLoc, runInfo);
			gigaMoog.gmoogScript.newScript();
			updateConfigurationSavedStatus(false);
			gigaMoog.gmoogScript.updateScriptNameText(profileLoc);
		}
		else {
			gigaMoog.gmoogScript420.checkSave(profileLoc, runInfo);
			gigaMoog.gmoogScript420.newScript();
			updateConfigurationSavedStatus(false);
			gigaMoog.gmoogScript420.updateScriptNameText(profileLoc);
		}
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}

void QtScriptWindow::openGMoogScript(int deviceIndex, IChimeraQtWindow* parent)
{
	try {
		auto profile = getProfile();
		auto run = mainWin->getRunInfo();
		auto& profileLoc = profile.configLocation;
		auto& runInfo = run;
		if (deviceIndex == 1) {
			gigaMoog.gmoogScript.checkSave(profileLoc, runInfo);
			std::string openName = openWithExplorer(parent, Script::GMOOG_SCRIPT_EXTENSION, CONFIGURATION_PATH);
			gigaMoog.gmoogScript.openParentScript(openName, profileLoc, runInfo);
			updateConfigurationSavedStatus(false);
			gigaMoog.gmoogScript.updateScriptNameText(profileLoc);
		}
		else {
			gigaMoog.gmoogScript420.checkSave(profileLoc, runInfo);
			std::string openName = openWithExplorer(parent, Script::GMOOG_SCRIPT_EXTENSION, CONFIGURATION_PATH);
			gigaMoog.gmoogScript420.openParentScript(openName, profileLoc, runInfo);
			updateConfigurationSavedStatus(false);
			gigaMoog.gmoogScript420.updateScriptNameText(profileLoc);
		}
	}
	catch (ChimeraError& err) {
		reportErr("Open GigaMoog Script Failed: " + err.qtrace() + "\r\n");
	}
}

void QtScriptWindow::openGMoogScript(int deviceIndex, std::string name)
{
	if (deviceIndex == 1) {
		gigaMoog.gmoogScript.openParentScript(name, getProfile().configLocation, mainWin->getRunInfo());
	}
	else {
		gigaMoog.gmoogScript420.openParentScript(name, getProfile().configLocation, mainWin->getRunInfo());
	}
}

void QtScriptWindow::saveGMoogScript(int deviceIndex)
{
	if (deviceIndex == 1) {
		gigaMoog.gmoogScript.saveScript(getProfile().configLocation, mainWin->getRunInfo());
		gigaMoog.gmoogScript.updateScriptNameText(getProfile().configLocation);
	}
	else {
		gigaMoog.gmoogScript420.saveScript(getProfile().configLocation, mainWin->getRunInfo());
		gigaMoog.gmoogScript420.updateScriptNameText(getProfile().configLocation);
	}
}

void QtScriptWindow::saveGMoogScriptAs(int deviceIndex, IChimeraQtWindow* parent)
{
	try {
		std::string extensionNoPeriod;
		if (deviceIndex == 1) extensionNoPeriod = gigaMoog.gmoogScript.getExtension();
		else extensionNoPeriod = gigaMoog.gmoogScript420.getExtension();

		if (extensionNoPeriod.size() == 0) {
			return;
		}
		extensionNoPeriod = extensionNoPeriod.substr(1, extensionNoPeriod.size());
		std::string newScriptAddress = saveWithExplorer(parent, extensionNoPeriod, getProfileSettings());
		if (deviceIndex == 1) {
			gigaMoog.gmoogScript.saveScriptAs(newScriptAddress, mainWin->getRunInfo());
			gigaMoog.gmoogScript.updateScriptNameText(getProfile().configLocation);
		}
		else {
			gigaMoog.gmoogScript420.saveScriptAs(newScriptAddress, mainWin->getRunInfo());
			gigaMoog.gmoogScript420.updateScriptNameText(getProfile().configLocation);
		}
		updateConfigurationSavedStatus(false);
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}

void QtScriptWindow::saveAllScript()
{
	saveMasterScript();
	saveGMoogScript(1);
	saveGMoogScript(2);
	for (auto name : ArbGenEnum::allAgs) {
		saveArbGenScript(name);
	}
}

void QtScriptWindow::windowSaveConfig (ConfigStream& saveFile){
	scriptInfo<std::string> addresses = getScriptAddresses ();
	// order matters!
	saveFile << "SCRIPTS\n";
	saveFile << "/*Master Script Address:*/ " << addresses.master << "\n";
	//saveFile << "/*GigaMoog Script Address:*/ " << addresses.gmoog << "\n";
	saveFile << "END_SCRIPTS\n";
	gigaMoog.handleSaveConfig(saveFile);
	for (auto name : ArbGenEnum::allAgs) {
		arbGens[(int)name].handleSavingConfig(saveFile, getProfileSettings().configLocation, mainWin->getRunInfo());
	}
}

void QtScriptWindow::checkMasterSave (){
	masterScript.checkSave (getProfile ().configLocation, mainWin->getRunInfo());
	gigaMoog.gmoogScript.checkSave(getProfile().configLocation, mainWin->getRunInfo());
	gigaMoog.gmoogScript420.checkSave(getProfile().configLocation, mainWin->getRunInfo());
}

void QtScriptWindow::considerScriptLocations() {
	for (auto name : ArbGenEnum::allAgs) {
		arbGens[(int)name].arbGenScript.considerCurrentLocation(getProfile().configLocation, mainWin->getRunInfo());
	}
	masterScript.considerCurrentLocation(getProfile().configLocation, mainWin->getRunInfo());
	gigaMoog.gmoogScript.considerCurrentLocation(getProfile().configLocation, mainWin->getRunInfo());
	gigaMoog.gmoogScript420.considerCurrentLocation(getProfile().configLocation, mainWin->getRunInfo());
}

//void QtScriptWindow::updateProfile (std::string text){
//	//profileDisplay.update (text);
//}

profileSettings QtScriptWindow::getProfileSettings (){
	return mainWin->getProfileSettings ();
}

void QtScriptWindow::updateConfigurationSavedStatus (bool status){
	mainWin->updateConfigurationSavedStatus (status);
}

void QtScriptWindow::fillExpDeviceList (DeviceList& list) {
	for (auto name : ArbGenEnum::allAgs) {
		list.list.push_back(arbGens[(int)name].getCore());
	}
	list.list.push_back(gigaMoog.getCore());
	list.list.push_back(gigaMoog.getCore420());
}

std::vector<std::reference_wrapper<ArbGenSystem>> QtScriptWindow::getArbGenSystem()
{
	std::vector<std::reference_wrapper<ArbGenSystem>> ags;
	for (ArbGenSystem& ag : arbGens) {
		ags.push_back(ag);
	}
	return ags;
}

std::vector<std::reference_wrapper<ArbGenCore>> QtScriptWindow::getArbGenCore()
{
	std::vector<std::reference_wrapper<ArbGenCore>> agCores;
	for (ArbGenSystem& ag : arbGens) {
		agCores.push_back(ag.getCore());
	}
	return agCores;
}

GigaMoogCore& QtScriptWindow::getGigaMoogCore()
{
	return gigaMoog.getCore();
}

GigaMoog420Core& QtScriptWindow::getGigaMoog420Core()
{
	return gigaMoog.getCore420();
}
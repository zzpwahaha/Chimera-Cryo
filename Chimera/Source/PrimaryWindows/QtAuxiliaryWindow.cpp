#include "stdafx.h"
#include "QtAuxiliaryWindow.h"
#include <qdesktopwidget.h>
#include <PrimaryWindows/QtScriptWindow.h>
#include <PrimaryWindows/QtAndorWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>
#include <PrimaryWindows/QtMainWindow.h>
#include <ExcessDialogs/saveWithExplorer.h>
#include <ExcessDialogs/openWithExplorer.h>


#include <qlayout.h>

QtAuxiliaryWindow::QtAuxiliaryWindow (QWidget* parent) 
	: IChimeraQtWindow (parent)
	, ttlBoard (this)
	, aoSys (this)
	, aiSys(this)
	, configParamCtrl (this, "CONFIG_PARAMETERS")
	, globalParamCtrl (this, "GLOBAL_PARAMETERS")
	, dds (this, DDS_SAFEMODE)
	, olSys(this)
{	
	
	setWindowTitle ("Auxiliary Window");
}

QtAuxiliaryWindow::~QtAuxiliaryWindow () {}

bool QtAuxiliaryWindow::eventFilter (QObject* obj, QEvent* event){
	//this will trigger the quickChange in Ao/DDS to perform
	if (aoSys.eventFilter(obj, event) || dds.eventFilter(obj, event) || olSys.eventFilter(obj, event))
	{
		try 
		{
			//aoSys.forceDacs(ttlBoard.getCore(), { 0, ttlBoard.getCurrentStatus() });
		}
		catch (ChimeraError& err) {
			reportErr (err.qtrace ());
		}
		return true;
	}
	return QMainWindow::eventFilter (obj, event);
}

void QtAuxiliaryWindow::initializeWidgets (){
	statBox = new ColorBox(this, mainWin->getDevices());
	QWidget* centralWidget = new QWidget();
	setCentralWidget(centralWidget);
	//centralWidget->setStyleSheet("border: 2px solid  black; ");
	QHBoxLayout* layout = new QHBoxLayout(centralWidget);
	QPoint loc{ 0, 25 };
	try{
		QVBoxLayout* layout1 = new QVBoxLayout();
		ttlBoard.initialize (this);
		layout1->addWidget(&ttlBoard, 0);
		
		aoSys.initialize (this);
		layout1->addWidget(&aoSys, 0);

		olSys.initialize(this);
		layout1->addWidget(&olSys, 0);
		layout1->addStretch(1);

		QVBoxLayout* layout3 = new QVBoxLayout();
		globalParamCtrl.initialize (this, "GLOBAL PARAMETERS", ParameterSysType::global);
		layout3->addWidget(&globalParamCtrl, 1);

		configParamCtrl.initialize (this, "CONFIGURATION PARAMETERS", ParameterSysType::config);
		configParamCtrl.setParameterControlActive (false);
		layout3->addWidget(&configParamCtrl, 1);
		
		dds.initialize(this);
		layout3->addWidget(&dds, 0);
		//dds.initialize (this, "DDS SYSTEM");
		//layout3->addWidget(&dds, 1);
		
		optimizer.initialize (this);
		layout3->addWidget(&optimizer, 1);
		
		QVBoxLayout* layout2 = new QVBoxLayout();

		/* initialize plot controls.*/
		//aoPlots.resize (NUM_DAC_PLTS);
		//for (auto dacPltCount : range (aoPlots.size ())){
		//	std::string titleTxt;
		//	switch (dacPltCount){
		//	case 0:
		//		titleTxt = "DACs: 0-7";
		//		break;
		//	case 1:
		//		titleTxt = "DACs: 8-15";
		//		break;
		//	case 2:
		//		titleTxt = "DACs: 16-23";
		//		break;
		//	}
		//	aoPlots[dacPltCount] = new PlotCtrl (8, plotStyle::DacPlot, std::vector<int> (), titleTxt);
		//	aoPlots[dacPltCount]->init ( this);
		//	layout2->addWidget(aoPlots[dacPltCount]->getView(), 1);
		//}
		//// ttl plots are similar to aoSys.
		//ttlPlots.resize (NUM_TTL_PLTS);
		//for (auto ttlPltCount : range (ttlPlots.size ())){
		//	// currently assuming 4 ttl plots...
		//	std::string titleTxt;
		//	switch (ttlPltCount){
		//	case 0:
		//		titleTxt = "Ttls: Row A";
		//		break;
		//	case 1:
		//		titleTxt = "Ttls: Row B";
		//		break;
		//	case 2:
		//		titleTxt = "Ttls: Row C";
		//		break;
		//	case 3:
		//		titleTxt = "Ttls: Row D";
		//		break;
		//	}
		//	ttlPlots[ttlPltCount] = new PlotCtrl (16, plotStyle::TtlPlot, std::vector<int> (), titleTxt);
		//	ttlPlots[ttlPltCount]->init (this);
		//	layout2->addWidget(ttlPlots[ttlPltCount]->getView(), 1);
		//}
		aiSys.initialize(this);
		layout2->addWidget(&aiSys);
		layout2->addStretch(1);

		layout1->setContentsMargins(0, 0, 0, 0);
		layout2->setContentsMargins(0, 0, 0, 0);
		layout3->setContentsMargins(0, 0, 0, 0);
		layout->addLayout(layout1);
		layout->addLayout(layout3);
		layout->addLayout(layout2);

		DOdialog = new doChannelInfoDialog(&ttlBoard);
		AOdialog = new AoSettingsDialog(&aoSys);
		DDSdialog = new DdsSettingsDialog(&dds);
		OLdialog = new OlSettingsDialog(&olSys);
		connect(DOdialog, &doChannelInfoDialog::updateSyntaxHighLight, [this]() {this->scriptWin->updateDoAoDdsNames(); });
		connect(AOdialog, &AoSettingsDialog::updateSyntaxHighLight, [this]() {this->scriptWin->updateDoAoDdsNames(); });
		connect(DDSdialog, &DdsSettingsDialog::updateSyntaxHighLight, [this]() {this->scriptWin->updateDoAoDdsNames(); });
		connect(OLdialog, &OlSettingsDialog::updateSyntaxHighLight, [this]() {this->scriptWin->updateDoAoDdsNames(); });

	}
	catch (ChimeraError& err){
		errBox ("Failed to initialize auxiliary window properly! Trace: " + err.trace ());
		//throwNested ("FATAL ERROR: Failed to initialize Auxiliary window properly!");
	}
}

void QtAuxiliaryWindow::handleDoAoPlotData (const std::vector<std::vector<plotDataVec>>& doData,
											const std::vector<std::vector<plotDataVec>>& aoData){
	for (auto ttlPlotNum: range(ttlPlots.size())){
		ttlPlots[ttlPlotNum]->setData (doData[ttlPlotNum]);
	}
	for (auto aoPlotNum : range (aoPlots.size ())) {
		aoPlots[aoPlotNum]->setData (aoData[aoPlotNum]);
	}
}

std::vector<parameterType> QtAuxiliaryWindow::getUsableConstants (){
	// This generates a usable set of constants (mostly for "Program Now" commands") based on the current GUI settings.
	// imporantly, when running the experiment proper, the saved config settings are what is used to determine 
	// parameters, not the gui setttings.
	std::vector<parameterType> configParams = configParamCtrl.getAllConstants ();
	std::vector<parameterType> globals = globalParamCtrl.getAllParams ();
	std::vector<parameterType> params = ParameterSystem::combineParams (configParams, globals);
	ScanRangeInfo constantRange;
	constantRange.defaultInit ();
	ParameterSystem::generateKey (params, false, constantRange);
	return params;
}

void QtAuxiliaryWindow::updateOptimization (AllExperimentInput& input){
	optimizer.verifyOptInput (input);
	dataPoint resultValue = andorWin->getMainAnalysisResult ();
	auto params = optimizer.getOptParams ();
	//optimizer.updateParams ( input, resultValue, camWin->getLogger() );
	std::string msg = "Next Optimization: ";
	for (auto& param : params){
		msg += param->name + ": " + str (param->currentValue) + ";";
	}
	msg += "\r\n";
	reportStatus (qstr(msg));
}

ParameterSystem& QtAuxiliaryWindow::getGlobals (){
	return globalParamCtrl;
}

std::pair<unsigned, unsigned> QtAuxiliaryWindow::getTtlBoardSize (){
	return ttlBoard.getTtlBoardSize ();
}

void QtAuxiliaryWindow::windowSaveConfig (ConfigStream& saveFile){
	// order matters! Don't change the order here.
	configParamCtrl.handleSaveConfig (saveFile);
	ttlBoard.handleSaveConfig (saveFile);
	aoSys.handleSaveConfig (saveFile);
	dds.handleSaveConfig (saveFile);
}

void QtAuxiliaryWindow::windowOpenConfig (ConfigStream& configFile){
	try{
		ConfigSystem::standardOpenConfig (configFile, configParamCtrl.configDelim, &configParamCtrl);
		ConfigSystem::standardOpenConfig (configFile, "TTLS", &ttlBoard);
		ConfigSystem::standardOpenConfig (configFile, "DACS", &aoSys);
		aoSys.updateEdits ();
		ConfigSystem::standardOpenConfig (configFile, dds.getDelim (), &dds);
	}
	catch (ChimeraError&){
		throwNested ("Auxiliary Window failed to read parameters from the configuration file.");
	}
}


unsigned QtAuxiliaryWindow::getNumberOfDacs (){
	return aoSys.getNumberOfDacs ();
}


std::array<std::string, size_t(DOGrid::total)> QtAuxiliaryWindow::getTtlNames (){
	return ttlBoard.getCore ().getAllNames ();
}

std::array<std::string, size_t(AOGrid::total)> QtAuxiliaryWindow::getDacNames() {
	return aoSys.getCore().getNames();
}

std::array<AoInfo, size_t(AOGrid::total)> QtAuxiliaryWindow::getDacInfo (){
	return aoSys.getDacInfo ();
}

std::array<std::string, size_t(DDSGrid::total)> QtAuxiliaryWindow::getDdsNames()
{
	std::array<std::string, size_t(DDSGrid::total)> names;
	for (size_t i = 0; i < size_t(DDSGrid::total); i++)
	{
		names[i] = dds.getName(i);
	}
	return names;
}

std::array<std::string, size_t(OLGrid::total)> QtAuxiliaryWindow::getOlNames()
{
	std::array<std::string, size_t(OLGrid::total)> names;
	for (size_t i = 0; i < size_t(OLGrid::total); i++)
	{
		names[i] = olSys.getName(i);
	}
	return names;
}

std::vector<parameterType> QtAuxiliaryWindow::getAllParams (){
	std::vector<parameterType> vars = configParamCtrl.getAllParams ();
	std::vector<parameterType> vars2 = globalParamCtrl.getAllParams ();
	vars.insert (vars.end (), vars2.begin (), vars2.end ());
	return vars;
}


void QtAuxiliaryWindow::clearVariables (){
	mainWin->updateConfigurationSavedStatus (false);
	configParamCtrl.clearParameters ();
}

void QtAuxiliaryWindow::passRoundToDac (){
	mainWin->updateConfigurationSavedStatus (false);
	//aoSys.handleRoundToDac (mainWin);
}


void QtAuxiliaryWindow::setVariablesActiveState (bool activeState){
	mainWin->updateConfigurationSavedStatus (false);
	configParamCtrl.setParameterControlActive (activeState);
}


unsigned QtAuxiliaryWindow::getTotalVariationNumber (){
	return configParamCtrl.getTotalVariationNumber ();
}


void QtAuxiliaryWindow::zeroDacs (){
	try{
		mainWin->updateConfigurationSavedStatus (false);
		aoSys.zeroDacs ();
		reportStatus ("Zero'd DACs.\n");
	}
	catch (ChimeraError& exception){
		errBox(exception.trace());
		reportStatus ("Failed to Zero DACs!!!\n");
		reportErr (exception.qtrace ());
	}
}

void QtAuxiliaryWindow::zeroDds() {
	try {
		mainWin->updateConfigurationSavedStatus(false);
		dds.zeroDds();
		reportStatus("Zero'd DDSs.\n");
	}
	catch (ChimeraError& exception) {
		errBox(exception.trace());
		reportStatus("Failed to Zero DDSs!!!\n");
		reportErr(exception.qtrace());
	}
}

void QtAuxiliaryWindow::zeroOls() {
	try {
		mainWin->updateConfigurationSavedStatus(false);
		olSys.zeroOffsetLock(ttlBoard.getCore(), ttlBoard.getCurrentStatus());
		reportStatus("Default'd Offsetlocks.\n");
	}
	catch (ChimeraError& exception) {
		errBox(exception.trace());
		reportStatus("Failed to Zero OffsetLocks!!!\n");
		reportErr(exception.qtrace());
	}
}

DoSystem& QtAuxiliaryWindow::getTtlSystem (){
	return ttlBoard;
}

DoCore& QtAuxiliaryWindow::getTtlCore (){
	return ttlBoard.getCore ();
}

void QtAuxiliaryWindow::fillMasterThreadInput (ExperimentThreadInput* input){
	try	{
		input->globalParameters = globalParamCtrl.getAllParams ();
	}
	catch (ChimeraError&) {
		throwNested ("Auxiliary window failed to fill master thread input.");
	}
}

AoSystem& QtAuxiliaryWindow::getAoSys () {
	return aoSys;
}

DdsSystem& QtAuxiliaryWindow::getDdsSys() {
	return dds;
}

OlSystem& QtAuxiliaryWindow::getOlSys() {
	return olSys;
}

void QtAuxiliaryWindow::handleAbort (){
	if (optimizer.isInMiddleOfOptimizing ()){
		auto answer = QMessageBox::question (nullptr, qstr ("Save Opt?"), qstr ("Save Optimization Data?"), 
			QMessageBox::Yes | QMessageBox::No);
		if (answer == QMessageBox::Yes){
			optimizer.onFinOpt ();
		}
	}
}

void QtAuxiliaryWindow::handleMasterConfigSave (std::stringstream& configStream){
	/// ttls
	for (auto row : range(size_t(DOGrid::numOFunit))){
		for (unsigned ttlNumberInc = 0; ttlNumberInc < ttlBoard.getTtlBoardSize ().second; ttlNumberInc++){
			std::string name = ttlBoard.getName(row, ttlNumberInc);
			if (name == "")
			{
				name = str(row) + "_" + str (ttlNumberInc);
			}
			configStream << name << "\n";
			configStream << ttlBoard.getDefaultTtl (row, ttlNumberInc) << "\n";
		}
	}
	// DAC Names
	for (unsigned dacInc = 0; dacInc < aoSys.getNumberOfDacs (); dacInc++){
		std::string name = aoSys.getName (dacInc);
		std::pair<double, double> minMax = aoSys.getDacRange (dacInc);
		if (name == ""){
			// then the name hasn't been set, so create the default name
			name = "Dac" + str (dacInc);
		}
		configStream << name << "\n";
		configStream << minMax.first << " - " << minMax.second << "\n";
		configStream << aoSys.getDefaultValue (dacInc) << "\n";
		configStream << aoSys.getNote (dacInc) << "\n";
	}

	// Number of Variables
	configStream << globalParamCtrl.getCurrentNumberOfVariables () << "\n";
	/// Variables
	for (unsigned varInc : range (globalParamCtrl.getCurrentNumberOfVariables ())){
		parameterType info = globalParamCtrl.getVariableInfo (varInc);
		configStream << info.name << " ";
		configStream << info.constantValue << "\n";
		// all globals are constants, no need to output anything else.
	}
}

void QtAuxiliaryWindow::handleNormalFin () {
	try {
		/*TODO add dds*/
		aoSys.setDacStatusNoForceOut (aoSys.getCore().getFinalSnapshot ());
		ttlBoard.setTtlStatusNoForceOut (ttlBoard.getCore().getFinalSnapshot ());
	}
	catch (ChimeraError&) { /* this gets thrown if no dac events. just continue.*/ }
}


void QtAuxiliaryWindow::handleMasterConfigOpen (ConfigStream& configStream){
	ttlBoard.getCore ().resetTtlEvents ();
	ttlBoard.getCore ().prepareForce ();
	aoSys.getCore().resetDacEvents ();
	//aoSys.prepareForce ();
	for (auto row : range(size_t(DOGrid::numOFunit)))
	{
		for (unsigned ttlNumberInc : range (ttlBoard.getTtlBoardSize ().second)){
			std::string name;
			std::string defaultStatusString;
			bool defaultStatus;
			configStream >> name >> defaultStatusString;
			try {
				// In file the booleans are stored as "0" or "1".
				defaultStatus = boost::lexical_cast<int>(defaultStatusString);
			}
			catch (boost::bad_lexical_cast&){
				throwNested ("Failed to load one of the default ttl values!");
			}
			ttlBoard.setName (row, ttlNumberInc, name);
			ttlBoard.updateDefaultTtl (row, ttlNumberInc, defaultStatus);
		}
	}
	// getting aoSys.
	for (unsigned dacInc : range (aoSys.getNumberOfDacs ())){
		std::string name, defaultValueString, minString, maxString;
		double defaultValue, min, max;
		configStream >> name;
		std::string trash;
		configStream >> minString >> trash;
		if (trash != "-"){
			thrower (str ("Expected \"-\" in master config file between min and max values for variable ")
				+ name + ", dac" + str (dacInc) + ".");
		}
		configStream >> maxString;
		configStream >> defaultValueString;
		try{
			defaultValue = boost::lexical_cast<double>(defaultValueString);
			min = boost::lexical_cast<double>(minString);
			max = boost::lexical_cast<double>(maxString);
		}
		catch (boost::bad_lexical_cast&){
			throwNested ("Failed to load one of the default DAC values!");
		}
		std::string noteString = "";
		noteString = configStream.getline ();
		aoSys.setName (dacInc, name);
		aoSys.setNote (dacInc, noteString);
		aoSys.setMinMax (dacInc, min, max);
		aoSys.prepareDacForceChange (dacInc, defaultValue);
		aoSys.updateEdits ();
		aoSys.setDefaultValue (dacInc, defaultValue);
	}
	// variables.
	int varNum;
	configStream >> varNum;
	if (varNum < 0 || varNum > 1000){
		auto answer = QMessageBox::question (nullptr, qstr ("Suspicious?"), qstr ("Variable number retrieved from "
			"file appears suspicious. The number is " + str (varNum) + ". Is this accurate?"), QMessageBox::Yes
			| QMessageBox::No);
		if (answer == QMessageBox::No){
			// don't try to load anything.
			varNum = 0;
			return;
		}
	}
	// Number of Variables
	globalParamCtrl.clearParameters ();
	for (int varInc = 0; varInc < varNum; varInc++){
		parameterType tempVar;
		tempVar.constant = true;
		tempVar.overwritten = false;
		tempVar.active = false;
		double value;
		configStream >> tempVar.name >> value;
		tempVar.constantValue = value;
		tempVar.ranges.push_back ({ value, value });
		globalParamCtrl.addParameter (tempVar);
	}
	globalParamCtrl.setTableviewColumnSize ();
}

void QtAuxiliaryWindow::updateExpActiveInfo (std::vector<parameterType> expParams) {
	globalParamCtrl.setUsages (expParams);
	configParamCtrl.setUsages (expParams);
}

void QtAuxiliaryWindow::SetDacs (){
	reportStatus ("----------------------\r\nSetting Dacs... ");
	try{
		mainWin->updateConfigurationSavedStatus (false);
		//aoSys.resetDacEvents();
		//ttlBoard.resetTtlEvents();
		reportStatus("Setting Dacs...\r\n");
		aoSys.handleSetDacsButtonPress(true);
		//aoSys.forceDacs (ttlBoard.getCore (), { 0, ttlBoard.getCurrentStatus () });
		reportStatus ("Finished Setting Dacs.\r\n");
	}
	catch (ChimeraError& exception){
		errBox (exception.trace ());
		reportStatus (": " + exception.qtrace () + "\r\n");
		reportErr (exception.qtrace ());
	}
	mainWin->updateConfigurationSavedStatus(false);
}


void QtAuxiliaryWindow::SetDds() 
{
	reportStatus("----------------------\r\nSetting DDSs... ");
	try {
		mainWin->updateConfigurationSavedStatus(false);
		//aoSys.resetDacEvents();
		////ttlBoard.resetTtlEvents();
		//reportStatus("Setting Dacs...\r\n");
		dds.handleSetDdsButtonPress(true);
		//dds.setDDSs();
		//aoSys.forceDacs (ttlBoard.getCore (), { 0, ttlBoard.getCurrentStatus () });
		reportStatus("Finished Setting DDSs.\r\n");
	}
	catch (ChimeraError& exception) {
		errBox(exception.trace());
		reportStatus(": " + exception.qtrace() + "\r\n");
		reportErr(exception.qtrace());
	}
	mainWin->updateConfigurationSavedStatus(false);
}

void QtAuxiliaryWindow::SetOls()
{
	reportStatus("----------------------\r\nSetting Offsetlocks... ");
	try {
		mainWin->updateConfigurationSavedStatus(false);
		olSys.handleSetOlsButtonPress(ttlBoard.getCore(), ttlBoard.getCurrentStatus());
		reportStatus("Finished Setting Offsetlocks.\r\n");
	}
	catch (ChimeraError& exception) {
		errBox(exception.trace());
		reportStatus(": " + exception.qtrace() + "\r\n");
		reportErr(exception.qtrace());
	}
	mainWin->updateConfigurationSavedStatus(false);
}


void QtAuxiliaryWindow::ViewOrChangeTTLNames (){
	mainWin->updateConfigurationSavedStatus (false);
	//ttlInputStruct input;
	//input.ttls = &ttlBoard; 
	/*if you use above, since ttlInputStruct input is a local variable, after show(), 
	the pointer is destructed and you will have trouble in the setName function afterwards; 
	previously the diag is shown with exce() which will block the function untill it ends, 
	in which case, using local variable is fine*/
	//DoSystem* ttls = &ttlBoard;
	//doChannelInfoDialog* dialog = new doChannelInfoDialog (ttls);
	DOdialog->updateAllEdits();
	DOdialog->setStyleSheet (chimeraStyleSheets::stdStyleSheet());
	DOdialog->show();
}


void QtAuxiliaryWindow::ViewOrChangeDACNames (){
	mainWin->updateConfigurationSavedStatus (false);
	//aoInputStruct input;
	//input.aoSys = &aoSys;
	//AoSettingsDialog* dialog = new AoSettingsDialog (&input);
	AOdialog->updateAllEdits();
	AOdialog->setStyleSheet (chimeraStyleSheets::stdStyleSheet ());
	AOdialog->show();
}

void QtAuxiliaryWindow::ViewOrChangeDDSNames()
{
	DDSdialog->updateAllEdits();
	DDSdialog->setStyleSheet(chimeraStyleSheets::stdStyleSheet());
	DDSdialog->show();
}

void QtAuxiliaryWindow::ViewOrChangeOLNames()
{
	OLdialog->updateAllEdits();
	OLdialog->setStyleSheet(chimeraStyleSheets::stdStyleSheet());
	OLdialog->show();
}

std::string QtAuxiliaryWindow::getOtherSystemStatusMsg (){
	// controls are done. Report the initialization defaultStatus...
	std::string msg;
	msg += "DO System:\n";
	//if (!ttlBoard.getFtFlumeSafemode ()){
	//	msg += "\tDO System is active!\n";
	//	msg += "\t" + ttlBoard.getDoSystemInfo () + "\n";
	//	//ttlBoard.ftdi_disconnect();
	//	msg += "\t Bites Written \n";

	//}
	//else{
	//	msg += "\tCode System is disabled! Enable in \"constants.h\"\n";
	//}
	msg += "\t Zynq placeholder \n";
	msg += "\t" + std::string(__FILE__) + ", line: " + std::to_string(__LINE__) + "\n";

	msg += "Analog Out System:\n";
	if (!ANALOG_OUT_SAFEMODE){
		msg += "\tCode System is Active!\n";
		msg += "\t" + std::string(__FILE__) + "line: " + std::to_string(__LINE__) + "\n";
	}
	else{
		msg += "\tCode System is disabled! Enable in \"constants.h\"\n";
	}
	msg += "DDS System:\n";
	if (!DDS_SAFEMODE){
		msg += "\tDDS System is Active!\n";
		msg += "\t" + dds.getSystemInfo () + "\n";
		msg += "\t" + dds.getSystemInfo ();
	}
	else{
		msg += "\tDDS System is disabled! Enable in \"constants.h\"\n";
	}
	return msg;
}

std::string QtAuxiliaryWindow::getVisaDeviceStatus (){
	std::string msg;
	return msg;
}

void QtAuxiliaryWindow::fillExpDeviceList (DeviceList& list){
	//list.list.push_back (dds.getCore ());
	//list.list.push_back(olSys.getCore());
}


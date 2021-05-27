// created by Mark O. Brown
#include "stdafx.h"

#include "Scripts/Script.h"

#include "PrimaryWindows/IChimeraQtWindow.h"
#include "GeneralUtilityFunctions/cleanString.h"
#include "ParameterSystem/ParameterSystem.h"
#include "ConfigurationSystems/ConfigSystem.h"
#include "PrimaryWindows/QtAuxiliaryWindow.h"
#include "DigitalOutput/DoSystem.h"
#include "GeneralObjects/RunInfo.h"
#include <PrimaryWindows/QtMainWindow.h>
#include <ExperimentThread/ExpThreadWorker.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include "boost/lexical_cast.hpp"
#include <qcombobox.h>
#include <QInputDialog.h>

#include <qDebug>


Script::Script(IChimeraQtWindow* parent) : IChimeraSystem(parent) 
{
	isSaved = true;
	editChangeEnd = 0;
	editChangeBegin = ULONG_MAX;
	setMaximumWidth(widgetWidthMax);
}

void Script::initialize(IChimeraQtWindow* parent, std::string deviceTypeInput, std::string scriptHeader)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	deviceType = deviceTypeInput;
	ScriptableDevice devenum;
	if (deviceTypeInput == "ArbGen") {
		devenum = ScriptableDevice::ArbGen;
		extension = str (".") + ARBGEN_SCRIPT_EXTENSION;
	}
	else if (deviceTypeInput == "Master") {
		devenum = ScriptableDevice::Master;
		extension = str (".") + MASTER_SCRIPT_EXTENSION;
	}
	else {
		thrower (": Device input type not recognized during construction of script control.  (A low level bug, "
			"this shouldn't happen)");
	}
	if (scriptHeader != "")	{
		title = new QLabel (cstr (scriptHeader), parent);
		layout->addWidget(title, 0);
		//title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	}
	QHBoxLayout* layout1 = new QHBoxLayout();
	savedIndicator = new CQCheckBox ("Saved?", parent);
	savedIndicator->setEnabled(false);
	savedIndicator->setChecked (true);
	
	fileNameText = new QLabel ("", parent);
	isSaved = true;
	help = new QLabel ("?", parent);
	
	layout1->addWidget(savedIndicator, 0);
	layout1->addWidget(fileNameText, 1);
	layout1->addWidget(help, 0);
	layout1->setContentsMargins(0, 0, 0, 0);
	if (deviceType == "Master"){
		help->setToolTip ("This is a script for programming master timing for TTLs, DACs, the RSG, and the raman outputs.\n"
							"Acceptable Commands:\n"
							"-      t++\n"
							"-      t += [number] (space between = and number required)\n"
							"-      t = [number] (space between = and number required)\n"
							"-      on: [ttlName]\n"
							"-      off: [ttlName]\n"
							"-      pulseon: [ttlName] [pulseLength]\n"
							"-      pulseoff: [ttlName] [pulseLength]\n"
							"-      dac: [dacName] [voltage]\n"
							"-      dacramp: [dacName] [initValue] [finalValue] [rampTime]\n"
							"-      dacarange: [dacName] [initValue] [finalValue] [rampTime] [rampInc]\n"
							"-      daclinspace: [dacName] [initValue] [finalValue] [rampTime] [numberOfSteps]\n"
							"-      ddsamp: [ddsName] [ampValue]\n"
							"-      ddsfreq: [ddsName] [freqValue]\n"
							"-      ddslinspaceamp: [ddsName] [initValue] [finalValue] [rampTime] [numberOfSteps]\n"
							"-      ddslinspacefreq: [ddsName] [initValue] [finalValue] [rampTime] [numberOfSteps]\n"
							"-      ddsrampamp: [ddsName] [initValue] [finalValue] [rampTime]\n"
							"-      ddsrampfreq: [ddsName] [initValue] [finalValue] [rampTime]\n"
							"-      ol: [olName] [lockfreqValue]\n"
							"-      ollinspace: [olName] [initValue] [finalValue] [rampTime] [numberOfSteps]\n"
							"-      olramp: [olName] [initValue] [finalValue] [rampTime] \n"
							"-      def [functionName]([functionArguments]):\n"
							"-      call [functionName(argument1, argument2, etc...)]\n"
							"-      repeat: [numberOfTimesToRepeat]\n"
							"-           %Commands...\n"

							"-      end % (of repeat)\n"
							"-      callcppcode\n"
							"-      % marks a line as a comment. %% does the same and gives you a different color.\n"
							"-      extra white-space is generally fine and doesn't screw up analysis of the script. Format as you like.\n"
							"-      Simple Math (+-/*) is supported in the scripts as well. To insert a mathematical expresion, just \n"
							"-      add parenthesis () around the full expression");
	}
	else if (deviceType == "ArbGen"){
		help->setToolTip (">>> Scripted Agilent Waveform Help <<<\n"
						"Accepted Commands (syntax for command is encased in <>)\n"
						"- hold <val> <time(ms)> <Continuation Type> <Possibly Repeat #> <#>\n"
						"- ramp <type> <initVal> <finVal(V)> <time(ms)> <Continuation Type> <Possibly Repeat #> <#>\n"
						"- pulse <pulse type> <vOffset> <amp> <pulse-width> <time-offset (ms)> <time(ms)> <Continuation Type> <Possibly Repeat #> <#>\n"
						"- modPulse <pulse-type> <vOffset> <amp> <pulse-width> <t-offset (ms)> <mod-Freq(MHz)> <mod-Phase(Rad)> <time(ms)> <Continuation Type> <Repeat #>\n"
						"The continuation type determines what the agilent does when it reaches the end of the <time> \n"
						"argument. Accepted Values for the continuation type are:\n"
						"- repeat <requires repeat #>\n"
						"- repeatUntilTrig\n"
						"- once\n"
						"- repeatForever\n"
						"- onceWaitTrig\n"
						"Accepted ramp types are:\n"
						"- nr (no ramp)\n"
						"- lin\n"
						"- tanh\n"
						"Accepted pulse types are:\n"
						"- sech, ~ sech(time/width)\n"
						"- gaussian, width = gaussian sigma\n"
						"- lorentzian, width = FWHM (curve is normalized)\n");
	}
	else{
		help->setToolTip ("No Help available");
	}
	
	availableFunctionsCombo.combo = new CQComboBox (parent);
	loadFunctions ();
	availableFunctionsCombo.combo->setCurrentIndex (0);
	parent->connect (availableFunctionsCombo.combo, qOverload<int> (&QComboBox::activated), [this, parent]() {
		try {
			auto addr = ConfigSystem::getMasterAddressFromConfig (parent->mainWin->getProfileSettings ());
			functionChangeHandler (addr);
		}
		catch (ChimeraError & err) {
			parent->reportErr (err.qtrace ());
		}});

	//edit = new CQTextEdit ("", parent);
	edit = new CQCodeEdit(parent);
	editZoom = 0;
	edit->installEventFilter(this);
	edit->setAcceptRichText (false);
	QFont font;
	{
		font.setFamily("Consolas");
		font.setStyleHint(QFont::Monospace);
		font.setFixedPitch(true);
		font.setPointSize(11);
		edit->setFont(font);
		edit->setTabStopDistance(40);
	}


	parent->connect (edit, &QTextEdit::textChanged, [this, parent]() { updateSavedStatus (false); });
	highlighter = new SyntaxHighlighter (devenum, edit->document ());

	layout->addLayout(layout1);
	layout->addWidget(availableFunctionsCombo.combo, 0);
	layout->addWidget(edit, 1);
	
}

void Script::functionChangeHandler(std::string configPath){
	int selection = availableFunctionsCombo.combo->currentIndex( );
	if ( selection != -1 ){
		std::string text = str (availableFunctionsCombo.combo->currentText ());
		text = text.substr( 0, text.find_first_of( '(' ) );
		changeView(text, true, configPath );
	}
}



std::string Script::getScriptPath(){
	return scriptPath;
}

std::string Script::getScriptText(){
	if (!edit){
		return "";
	}
	return str(edit->toPlainText());
}

void Script::updateSavedStatus(bool scriptIsSaved){
	isSaved = scriptIsSaved;
	savedIndicator->setChecked ( scriptIsSaved );
}


std::vector<parameterType> Script::getLocalParams () {
	if (!edit) { return{}; }
	auto text = edit->toPlainText ();
	std::stringstream fileTextStream = std::stringstream (str (text));
	ScriptStream ss (fileTextStream.str ());
	auto localVars = ExpThreadWorker::getLocalParameters (ss);
	return localVars;
}

void Script::changeView(std::string viewName, bool isFunction, std::string configPath){
	if (viewName == "Parent Script"){
		loadFile(configPath);
	}
	else if (isFunction){
		loadFile(FUNCTIONS_FOLDER_LOCATION + viewName + "." + FUNCTION_EXTENSION);
	}
	else{
		// load child
		loadFile(configPath + viewName);
	}

	// colorEntireScript(params, rgbs, ttlNames, dacNames);
	// the view is fresh from a file, so it's saved.
	updateSavedStatus(true);
}


bool Script::isFunction ( ){
	int sel = availableFunctionsCombo.combo->currentIndex();
	QString text;
	if ( sel != -1 ){
		text = availableFunctionsCombo.combo->currentText();
	}
	else{
		text = "";
	}
	return text != "Parent Script";// && text != "";
}

//
void Script::saveScript(std::string configPath, RunInfo info)
{
	if (configPath == ""){
		thrower (": Please select a configuration before trying to save a script!\r\n");
	}
	if (isSaved && scriptName != ""){
		// shoudln't need to do anything
		return;
	}
	if ( isFunction() ){
		errBox( "The current view is not the parent view. Please switch to the parent view before saving to "
				"save the script, or use the save-function option to save the current function." );
		return;
	}
	if ( scriptName == "" ){
		std::string newName;
		newName = str(QInputDialog::getText (edit, "New Script Name", ("Please enter new name for the " + deviceType + " script " + scriptName + ".",
											 scriptName).c_str()));
		if (newName == ""){
			// canceled
			return;
		}
		std::string path = configPath + newName + extension;
		saveScriptAs(path, info);
	}
	if (info.running) {
		for (unsigned scriptInc = 0; scriptInc < info.currentlyRunningScripts.size(); scriptInc++) {
			if (scriptName == info.currentlyRunningScripts[scriptInc]) {
				thrower("System is currently running. You can't save over any files in use by the system while"
					" it runs, which includes the NIAWG scripts and the intensity script.");
			}
		}
	}
	auto text = edit->toPlainText();
	std::fstream saveFile(configPath + scriptName + extension, std::fstream::out);
	if (!saveFile.is_open()){
		thrower ("Failed to open script file: " + configPath + scriptName + extension);
	}
	saveFile << str(text);
	saveFile.close();
	scriptFullAddress = configPath + scriptName + extension;
	scriptPath = configPath;
	updateSavedStatus(true);
	emit notification ("Finished saving script.\n", 0);
}

//
void Script::saveScriptAs(std::string location, RunInfo info)
{
	if (location == ""){
		return;
	}
	if (info.running) {
		for (unsigned scriptInc = 0; scriptInc < info.currentlyRunningScripts.size(); scriptInc++) {
			if (scriptName == info.currentlyRunningScripts[scriptInc]) {
				thrower("System is currently running. You can't save over any files in use by the system while "
					"it runs, which includes the horizontal and vertical AOM scripts and the intensity script.");
			}
		}
	}
	auto text = edit->toPlainText();
	std::fstream saveFile(location, std::fstream::out);
	if (!saveFile.is_open()){
		thrower ("Failed to open script file: " + location);
	}
	saveFile << str(text);
	char fileChars[_MAX_FNAME];
	char dirChars[_MAX_FNAME];
	char pathChars[_MAX_FNAME];
	int myError = _splitpath_s(cstr(location), dirChars, _MAX_FNAME, pathChars, _MAX_FNAME, fileChars, _MAX_FNAME, nullptr, 0);
	scriptName = str(fileChars);
	scriptPath = str(fileChars) + str(pathChars);
	saveFile.close();
	scriptFullAddress = location;
	updateScriptNameText(location);
	updateSavedStatus(true);
	emit notification ("Finished saving script.\n", 0);
}

//
void Script::checkSave(std::string configPath, RunInfo info)
{
	if (isSaved){
		// don't need to do anything
		return;
	}
	// test first non-commented word of text to see if this looks like a function or not.
	auto text = edit->toPlainText();
	ScriptStream tempStream;
	tempStream << str(text);
	std::string word;
	tempStream >> word;
	if (word == "def"){
		auto res = QMessageBox::question (nullptr, "Save?", qstr("Current " + deviceType + " function file is unsaved. Save it?"),
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		if (res == QMessageBox::Cancel){
			thrower ("Cancel!");
		}
		else if (res == QMessageBox::No){
			return;
		}
		else if (res == QMessageBox::Yes){
			saveAsFunction();
			return;
		}
	}
	// else it's a normal script file.
	if (scriptName == ""){
		auto res = QMessageBox::question (nullptr, "Save?", qstr ("Current " + deviceType + " script file is unsaved and unnamed. Save it with a with new name?"),
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		if (res == QMessageBox::Cancel){
			thrower ("Cancel!");
		}
		else if (res == QMessageBox::No){
			return;
		}
		else if (res == QMessageBox::Yes){
			std::string newName;
			newName = str (QInputDialog::getText (edit, "New Script Name", ("Please enter new name for the " + deviceType + " script " + scriptName + ".",
				scriptName).c_str ()));
			std::string path = configPath + newName + extension;
			saveScriptAs(path, info);
			return;
		}
	}
	else{
		auto answer = QMessageBox::question (nullptr, qstr ("Save Script?"),
			qstr ("The " + deviceType + " script file is unsaved. Save it as " + scriptName + extension + "?"), 
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel );
		if (answer == QMessageBox::Cancel) {
			thrower ("Cancel!");
		}
		if (answer == QMessageBox::No) {}
		if (answer == QMessageBox::Yes) {
			saveScript(configPath, info);
		}
	}
}


//
void Script::renameScript(std::string configPath){
	if (scriptName == ""){
		// ??? don't know why I need this here.
		return;
	}
	std::string newName;
	newName = str (QInputDialog::getText (edit, "New Script Name", ("Please enter new name for the " + deviceType
		+ " script " + scriptName + ".",
		scriptName).c_str ()));
	if (newName == ""){
		// canceled
		return;
	}
	int result = MoveFile(cstr(configPath + scriptName + extension), cstr(configPath + newName + extension));
	if (result == 0){
		thrower ("Failed to rename file. (A low level bug? this shouldn't happen)");
	}
	scriptFullAddress = configPath + scriptName + extension;
	scriptPath = configPath;
}

//
void Script::deleteScript(std::string configPath){
	if (scriptName == "") {
		return;
	}
	// check to make sure:
	auto answer = QMessageBox::question (nullptr, qstr ("Delete Script?"),
		qstr ("Are you sure you want to delete the script file " + scriptName + "?"), QMessageBox::Yes | QMessageBox::No);
	if (answer == QMessageBox::No) {
		return;
	}
	int result = DeleteFile(cstr(configPath + scriptName + extension));
	if (result == 0){
		thrower ("Deleting script file failed!  (A low level bug, this shouldn't happen)");
	}
	else {
		scriptName = "";
		scriptPath = "";
		scriptFullAddress = "";
	}
}


// the differences between this and new script are that this opens the default function instead of the default script
// and that this does not reset the script config, etc. 
void Script::newFunction(){
	std::string tempName;
	tempName = DEFAULT_SCRIPT_FOLDER_PATH;
	if (deviceType == "Master"){
		tempName += "DEFAULT_FUNCTION.func";
	}
	else{
		thrower ("tried to load new function with non-master script? Only the master script supports functions"
				 " currently.");
	}
	loadFile(tempName);
	availableFunctionsCombo.combo->setCurrentIndex (-1);
}

//
void Script::newScript(){
	std::string tempName;
	tempName = DEFAULT_SCRIPT_FOLDER_PATH;
	if (deviceType == "ArbGen"){
		tempName += "DEFAULT_ARBGEN_SCRIPT.aScript";
	}
	else if (deviceType == "Master"){
		tempName += "DEFAULT_MASTER_SCRIPT.mScript";
	}	
	reset();
	loadFile(tempName, std::ios::_Openmode(std::ios::out | std::ios::trunc));
	edit->setText("%%%%%%%%%%%%%%%%% Start With 1ms %%%%%%%%%%%%%%%%% \r\nt = 1");
}


void Script::openParentScript(std::string parentScriptFileAndPath, std::string configPath, RunInfo info)
{
	if (parentScriptFileAndPath == "" || parentScriptFileAndPath == "NONE"){
		return;
	}
	char fileChars[_MAX_FNAME];
	char extChars[_MAX_EXT];
	char dirChars[_MAX_FNAME];
	char pathChars[_MAX_FNAME];
	int myError = _splitpath_s(cstr(parentScriptFileAndPath), dirChars, _MAX_FNAME, pathChars, _MAX_FNAME, fileChars, 
								_MAX_FNAME, extChars, _MAX_EXT);
	std::string extStr(extChars);
	if (deviceType == "ArbGen"){
		if (extStr != str( "." ) + ARBGEN_SCRIPT_EXTENSION){
			thrower ("Attempted to open non-agilent script from agilent script control.");
		}
	}
	else if (deviceType == "Master"){
		if (extStr != str( "." ) + MASTER_SCRIPT_EXTENSION){
			thrower ("Attempted to open non-master script from master script control!");
		}
	}
	else{
		thrower ("Unrecognized device type inside script control!  (A low level bug, this shouldn't happen).");
	}
	loadFile( parentScriptFileAndPath );
	scriptName = str(fileChars);
	scriptFullAddress = parentScriptFileAndPath;
	updateSavedStatus(true);
	std::string scriptLocation = parentScriptFileAndPath;
	std::replace (scriptLocation.begin (), scriptLocation.end (), '\\', '/');
	int sPos = scriptLocation.find_last_of ('/');
	scriptLocation = scriptLocation.substr(0, sPos);	
	if (scriptLocation + "/" != configPath && configPath != ""){
		auto answer = QMessageBox::question (nullptr, qstr ("Location?"), qstr ("The requested " + deviceType
			+ " script: " + parentScriptFileAndPath + " is not "
			"currently located in the current configuration folder. This is recommended so that "
			"scripts related to a particular configuration are reserved to that configuration "
			"folder. Copy script to current configuration folder?"), QMessageBox::Yes | QMessageBox::No);
		if (answer == QMessageBox::Yes){
			std::string scriptName = parentScriptFileAndPath.substr(sPos+1, parentScriptFileAndPath.size());
			std::string path = configPath + scriptName;
			saveScriptAs(path, info);
		}
	}
	updateScriptNameText( configPath );
	int index = availableFunctionsCombo.combo->findText ("Parent Script");
	if (index != -1) { // -1 for not found
		availableFunctionsCombo.combo->setCurrentIndex (index);
	}
}

/*
]---	This function only puts the given file on the edit for this class, it doesn't change current settings parameters. 
		It's used bare when just changing the
]-		view of the edit, while it's used with some surrounding changes for loading a new parent.
 */
void Script::loadFile(std::string pathToFile, std::ios::_Openmode flags)
{
	std::fstream openFile;
	openFile.open(pathToFile, std::ios::in | flags); /*default is open for reading*/
	if (!openFile.is_open()){
		reset();
		thrower ("Failed to open script file: " + pathToFile + ".");
	}
	std::string tempLine;
	std::string fileText;
	while (std::getline(openFile, tempLine)){
		cleanString(tempLine);
		fileText += tempLine;
	}
	// put the default into the new control.
	edit->setText(cstr(fileText));
	openFile.close();
	emit notification (qstr("Finished loading " + deviceType + " file\n"),1);
}


void Script::reset(){
	if (!availableFunctionsCombo.combo || !edit) {
		return;
	}
	int index = availableFunctionsCombo.combo->findText ("Parent Script");
	if (index != -1) { // -1 for not found
		availableFunctionsCombo.combo->setCurrentIndex (index);
	}
	scriptName = "";
	scriptPath = "";
	scriptFullAddress = "";
	updateSavedStatus(false);
	fileNameText->setText("");
	edit->setText("");
}

bool Script::savedStatus(){
	return isSaved;
}

std::string Script::getScriptPathAndName(){
	return scriptFullAddress;
}

std::string Script::getScriptName(){
	return scriptName;
}

void Script::considerCurrentLocation(std::string configPath, RunInfo info)
{
	if (scriptFullAddress.size() > 0){
		std::string scriptLocation = scriptFullAddress;
		std::replace (scriptLocation.begin (), scriptLocation.end (), '\\', '/');
		int sPos = scriptLocation.find_last_of ('/');
		scriptLocation = scriptLocation.substr (0, sPos);

		if (scriptLocation + "/" != configPath){
			auto answer = QMessageBox::question (nullptr, qstr ("Location?"), qstr ("The requested " + deviceType
				+ " script location: \"" + scriptLocation + "\" "
				"is not currently located in the current configuration folder. This is recommended"
				" so that scripts related to a particular configuration are reserved to that "
				"configuration folder. Copy script to current configuration folder?"), QMessageBox::Yes | QMessageBox::No);
			if (answer == QMessageBox::Yes) {
				std::string scriptName = scriptFullAddress.substr(sPos, scriptFullAddress.size());
				scriptFullAddress = configPath + scriptName;
				scriptPath = configPath;
				saveScriptAs(scriptFullAddress, info);
			}
		}
	}
}

std::string Script::getExtension(){
	return extension;
}

void Script::updateScriptNameText(std::string configPath){
	// there are some \\ on the endOfWord of the path by default.
	configPath = configPath.substr(0, configPath.size() - 1);
	int sPos = configPath.find_last_of('\\');
	if (sPos != -1)	{
		std::string parentFolder = configPath.substr(sPos + 1, configPath.size());
		std::string text = parentFolder + "->" + scriptName;
		fileNameText->setText(cstr(text));
	}
	else{
		fileNameText->setText(qstr(scriptName));
	}
}

void Script::setScriptText(std::string text){
	if (!edit) {
		return;
	}
	edit->setText( cstr( text ) );
}

void Script::saveAsFunction(){
	// check to make sure that the current script is defined like a function
	auto text = edit->toPlainText();
	ScriptStream stream;
	stream << str(text);
	std::string word;
	stream >> word;
	if (word != "def"){
		thrower ("Function declarations must begin with \"def\".");
	}
	std::string line;
	line = stream.getline( '\r' );
	int pos = line.find_first_of("(");
	if (pos == std::string::npos){
		thrower ("No \"(\" found in function name. If there are no arguments, use empty parenthesis \"()\"");
	}
	int initNamePos = line.find_first_not_of(" \t");
	std::string functionName = line.substr(initNamePos, line.find_first_of("("));
	if (functionName.find_first_of(" ") != std::string::npos){
		thrower ("Function name included a space! Name was" + functionName);
	}
	std::string path = FUNCTIONS_FOLDER_LOCATION + functionName + "." + FUNCTION_EXTENSION;
	FILE *file;
	fopen_s( &file, cstr(path), "r" );
	if ( !file ){
		//
	}
	else{
		emit notification ( "Overwriting function definition for function at " + qstr(path) + "...\r\n" );
		fclose ( file );
	}
	std::fstream functionFile(path, std::ios::out);
	if (!functionFile.is_open()){
		thrower ("the function file failed to open!");
	}
	functionFile << str(text);
	functionFile.close();
	// refresh this.
	loadFunctions();
	int index = availableFunctionsCombo.combo->findText (cstr (functionName));
	if (index != -1) { // -1 for not found
		availableFunctionsCombo.combo->setCurrentIndex (index);
	}
	updateSavedStatus( true );
	emit notification ("Finished saving script as a function.\n",0);
}

void Script::setEnabled ( bool enabled, bool functionsEnabled ){
	if (!availableFunctionsCombo.combo || !edit ) {
		return;
	}
	edit->setReadOnly (!enabled);
	availableFunctionsCombo.combo->setEnabled( functionsEnabled );
}


bool Script::eventFilter(QObject* obj, QEvent* event)
{
	auto aa = event->type();
	if (obj == edit && event->type() == QEvent::Wheel)
	{
		QWheelEvent* wheel = static_cast<QWheelEvent*>(event);
		if (wheel->modifiers() == Qt::ControlModifier)
		{
			if (wheel->angleDelta().y() > 0)
			{
				edit->zoomIn();
				editZoom++;
			}

			else
			{
				if (edit->currentFont().pointSize() != 1) { editZoom--; }
				edit->zoomOut();
			}

			return true;
		}
	}
	else if (obj == edit && event->type() == QEvent::KeyPress)
	{
		QKeyEvent* key = static_cast<QKeyEvent*>(event);
		if (key->modifiers() == Qt::ControlModifier)
		{
			if (key->key() == Qt::Key_0)
			{
				edit->zoomOut(editZoom);
				//editZoom > 0 ? edit->zoomOut(editZoom) : edit->zoomIn(abs(editZoom));
				editZoom = 0;
				return true;
			}
			else if (key->key() == Qt::Key_Equal)
			{
				edit->zoomIn();
				editZoom++;
				return true;
			}
			else if (key->key() == Qt::Key_Minus)
			{
				if (edit->currentFont().pointSize() != 1) { editZoom--; }
				edit->zoomOut();
				return true;
			}
			else if (key->key() == Qt::Key_Slash)
			{
				QTextCursor cur = edit->textCursor();
				const int curp = cur.position();
				const int ancp = cur.anchor();
				cur.setPosition(curp > ancp ? ancp : curp);
				cur.movePosition(QTextCursor::StartOfBlock);
				cur.setPosition(curp > ancp ? curp : ancp, QTextCursor::KeepAnchor);
				cur.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

				QString stext = cur.selection().toPlainText();
				QStringList stextlist = stext.split('\n');
				int numofComment = stextlist.length();
				bool comment = false;
				for (auto s : stextlist) {
					if (s[0] != '%') { comment = true; break; }
				}
				if (comment) {
					for (auto& s : stextlist) {
						s.insert(0, '%');
					}
				}
				else {
					for (auto& s : stextlist) {
						s = s.remove(0, 1);
					}
				}
				cur.insertText(stextlist.join('\n'));

				cur.setPosition(ancp + (ancp > curp ? (comment ? numofComment : -numofComment) : (comment ? 1 : -1)));
				cur.setPosition(curp + (ancp > curp ? (comment ? 1 : -1) : (comment ? numofComment : -numofComment)), QTextCursor::KeepAnchor);


				edit->setTextCursor(cur);
			}
			else
			{
				qDebug() << key->key();
			}

		}

	}

	return false;
}

void Script::loadFunctions(){
	availableFunctionsCombo.loadFunctions( );
}


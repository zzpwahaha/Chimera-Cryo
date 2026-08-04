#include "stdafx.h"
#include "ConfigurationSystems/Version.h"
#include "ConfigurationSystems/ConfigSystem.h"
#include <DataLogging/DataLogger.h>
#include <ExperimentThread/ExpThreadWorker.h>
#include "MicrowaveCore.h"
#include <qdebug>
#include <qelapsedtimer.h>

MicrowaveCore::MicrowaveCore(std::string delim, bool safemode, std::string port, std::pair<unsigned, unsigned> uwaveTriggerLine) : 
	configDelim(delim),
	safemode(safemode),
	uwFlume(port, safemode),
	uwaveTriggerLine(uwaveTriggerLine)
{
	if (safemode) {
		return;
	}
	std::string cmd;
	try {
		for (auto cmd : mwSetupCommands) {
			uwFlume.write(cmd);
			Sleep(5);
		}
		Sleep(1000);
		auto msg = uwFlume.query("p");
		if (msg != "1") {
			thrower("Error after initilizing setup command: PLL is not locked. Make sure you supplied a 10MHz external clock!");
		}
	}
	catch (ChimeraError& e) {
		throwNested("Error seen in initlizing Windfreak with setup commands: " + cmd);
	}
	
}

void MicrowaveCore::setTrigTime (double time) {
	triggerTime = time;
}

void MicrowaveCore::programVariation (unsigned variationNumber, std::vector<parameterType>& params, ExpThreadWorker* threadworker){
	if (!experimentActive) { return; }
	if (!experimentSettings.control || experimentSettings.list.size () == 0)	{
		// Nothing to program.
		return;
	}
	//setPmSettings ();

	std::vector<microwaveListEntry> listChannelA;
	std::vector<microwaveListEntry> listChannelB;

	for (auto entry : experimentSettings.list) {
		if (entry.channel == 0) {
			listChannelA.push_back(entry);
		}
		else {
			listChannelB.push_back(entry);
		}
	}

	QElapsedTimer etimer;
	etimer.start();
	try	{
		if (!listChannelA.empty()) {
			uwFlume.write("C0");
			sleep(5);
			if (listChannelA.size() == 1) {
				uwFlume.programSingleSetting(listChannelA[0], variationNumber);
			}
			else {
				uwFlume.programList(listChannelA, variationNumber, triggerTime);
			}
		}
		if (!listChannelB.empty()) {
			uwFlume.write("C1");
			if (listChannelB.size() == 1) {
				uwFlume.programSingleSetting(listChannelB[0], variationNumber);
			}
			else {
				uwFlume.programList(listChannelB, variationNumber, triggerTime);
			}
		}
	}
	catch (ChimeraError&)	{
		// I don't think this really happens much anymore, have fixed some small bugs in windfreak programming which
		// were probably causing this. 
		if (threadworker != nullptr) {
			emit threadworker->warn ("Failed to program windfreak first time! Trying again...");
		}
		// should probably emit a warning here. 
		try	{
			// something in the windfreak seems to need flushing at this point.
			try {
				uwFlume.query ("?");
			}
			catch (ChimeraError & ) {}
			if (!listChannelA.empty()) {
				uwFlume.write("C0");
				sleep(5);
				if (listChannelA.size() == 1) uwFlume.programSingleSetting(listChannelA[0], variationNumber);
				else uwFlume.programList(listChannelA, variationNumber, triggerTime);
			}
			if (!listChannelB.empty()) {
				uwFlume.write("C1");
				if (listChannelB.size() == 1) uwFlume.programSingleSetting(listChannelB[0], variationNumber);
				else uwFlume.programList(listChannelB, variationNumber, triggerTime);

			}
		catch (ChimeraError & ){
			throwNested ("Failed to program Windfreak!");
		}
	}
	qDebug() << ">> Programmed Microwave System in " << etimer.elapsed() << " ms";
	if (threadworker != nullptr) {
		notify({ "Windfreak list setting programmed: " + qstr(getCurrentList()), 2 }, threadworker);
	}
}

void MicrowaveCore::logSettings (DataLogger& log, ExpThreadWorker* threadworker){
	try {
		H5::Group microwaveGroup;
		try {
			microwaveGroup = log.file.createGroup("/Microwave-" + str(configDelim));
		}
		catch (H5::Exception&) {
			microwaveGroup = log.file.openGroup ("/Microwave-" + str(configDelim));
		}
		log.writeDataSet (experimentSettings.control, str ("Control"), microwaveGroup);
		unsigned count = 0;
		for (auto& listSetting : experimentSettings.list) {
			auto listElemGroup = microwaveGroup.createGroup ("List Elem #" + str (count++));
			log.writeDataSet (listSetting.frequency.expressionStr, "Frequency", listElemGroup);
			log.writeDataSet (listSetting.power.expressionStr, "Power", listElemGroup);
			log.writeDataSet (listSetting.channel, "Channel", listElemGroup);
		}
	}
	catch (H5::Exception&) {
		throwNested ("Failed to save microwave settings to H5 File!");
	}
}

std::string MicrowaveCore::queryIdentity (){
	return uwFlume.queryIdentity ();
}

void MicrowaveCore::setFmSettings (){
	uwFlume.setFmSettings ();
}

void MicrowaveCore::setPmSettings (){
	uwFlume.setPmSettings ();
}

void MicrowaveCore::calculateVariations (std::vector<parameterType>& params, ExpThreadWorker* threadworker){
	if (!experimentSettings.control) {
		return;
	}
	unsigned variations;
	if (params.size () == 0){
		variations = 1;
	}
	else{
		variations = params.front ().keyValues.size ();
	}
	for (auto freqInc : range(experimentSettings.list.size())){
		experimentSettings.list[freqInc].frequency.internalEvaluate (params, variations);
		experimentSettings.list[freqInc].power.internalEvaluate (params, variations);
	}
	notify({ qstr("Microwave List Setting: " + getCurrentList()), 1 }, threadworker);
}

std::pair<unsigned, unsigned> MicrowaveCore::getUWaveTriggerLine() {
	return uwaveTriggerLine;
}

unsigned MicrowaveCore::getNumTriggers (microwaveSettings settings){
	return settings.list.size () == 1 ? 0 : settings.list.size ();
}

double MicrowaveCore::getTriggerTime()
{
	return triggerTime;
}

microwaveSettings MicrowaveCore::getSettingsFromConfig (ConfigStream& openFile){
	microwaveSettings settings;
	auto getlineF = ConfigSystem::getGetlineFunc (openFile.ver);
	openFile >> settings.control;
	unsigned numInList = 0;
	openFile >> numInList;
	if (numInList > 100){
		auto res = QMessageBox::question (nullptr, "Suspicious...",
			"Detected suspiciously large number of microwave settings in microwave list. Number of list entries"
			" was " + qstr (numInList) + ". Is this acceptable?");
		if (res == QMessageBox::No){
			thrower ("Detected suspiciously large number of microwave settings in microwave list. Number of list entries"
					 " was " + str (numInList) + ".");
		}
	}
	settings.list.resize (numInList);
	if (numInList > 0){
		openFile.get ();
	}
	for (auto num : range (numInList)){
		getlineF (openFile, settings.list[num].frequency.expressionStr);
		getlineF (openFile, settings.list[num].power.expressionStr);
		try {
			unsigned ch = 0;
			openFile >> ch;
			settings.list[num].channel = ch;
			if (num != numInList - 1) {
				openFile.get();
			}
		}
		catch (...) {
			settings.list[num].channel = 0;
		}
	}
	return settings;
}

void MicrowaveCore::loadExpSettings (ConfigStream& stream){
	ConfigSystem::stdGetFromConfig (stream, *this, experimentSettings);
	experimentActive = experimentSettings.control;
}

std::string MicrowaveCore::getCurrentList () {
	return uwFlume.getListString ();
}

#include "stdafx.h"
#include "ArbGenCore.h"
#include "DigitalOutput/DoCore.h"
#include "Scripts/ScriptStream.h"
#include <ExperimentThread/ExpThreadWorker.h>
#include <ConfigurationSystems/ConfigSystem.h>
#include <DataLogging/DataLogger.h>
//#include <AnalogInput/CalibrationManager.h>

ArbGenCore::ArbGenCore(const arbGenSettings& settings) :
	visaFlume(settings.safemode, settings.address),
	sampleRate(settings.sampleRate),
	initSettings(settings),
	triggerRow(settings.triggerRow),
	triggerNumber(settings.triggerNumber),
	memoryLoc(settings.memoryLocation),
	configDelim(settings.configurationFileDelimiter),
	arbGenName(settings.deviceName),
	setupCommands(settings.setupCommands),
	isConnected(false)
{
	//calibrations[0].includesSqrt = false;
	calibrations[0].calibrationCoefficients = settings.calibrationCoeff;
	//calibrations[0].polynomialOrder = settings.calibrationCoeff.size();
	// could probably set these properly but in general wasn't doing this when was doing manual calibrations.
	calibrations[0].calmax = 1e6;
	calibrations[0].calmin = -1e6;

	//calibrations[1].includesSqrt = false;
	calibrations[1].calibrationCoefficients = settings.calibrationCoeff;
	//calibrations[1].polynomialOrder = settings.calibrationCoeff.size();
	calibrations[1].calmax = 1e6;
	calibrations[1].calmin = -1e6;


	try {
		visaFlume.open();
	}
	catch (ChimeraError& err) {
		errBox("Error seen while opening connection to " + arbGenName + " ArbGen:" + err.trace());
	}
}

ArbGenCore::~ArbGenCore() {
	visaFlume.close();
}

void ArbGenCore::initialize() {
	try {
		deviceInfo = visaFlume.identityQuery();
		isConnected = true;
	}
	catch (ChimeraError&) {
		deviceInfo = "Disconnected";
		isConnected = false;
	}

}

std::pair<unsigned, unsigned> ArbGenCore::getTriggerLine() {
	return { triggerRow, triggerNumber };
}

std::string ArbGenCore::getDeviceInfo() {
	return deviceInfo;
}

void ArbGenCore::analyzeArbGenScript(scriptedArbInfo& infoObj, std::vector<parameterType>& variables,
	std::string& warnings) {
	ScriptStream stream;
	ExpThreadWorker::loadArbGenScript(infoObj.fileAddress.expressionStr, stream);
	int currentSegmentNumber = 0;
	infoObj.wave.resetNumberOfTriggers();
	// Procedurally readbtn lines into segment objects.
	while (!stream.eof()) {
		int leaveTest;
		try {
			leaveTest = infoObj.wave.analyzeAgilentScriptCommand(currentSegmentNumber, stream, variables, warnings);
		}
		catch (ChimeraError&) {
			throwNested("Error seen while analyzing ArbGen script command for ArbGen " + this->configDelim);
		}
		if (leaveTest < 0) {
			thrower("IntensityWaveform.analyzeAgilentScriptCommand threw an error! Error occurred in segment #"
				+ str(currentSegmentNumber) + ".");
		}
		if (leaveTest == 1) {
			// readbtn function is telling this function to stop reading the file because it's at its end.
			break;
		}
		currentSegmentNumber++;
	}
}

std::string ArbGenCore::getDeviceIdentity() {
	std::string msg;
	try {
		msg = visaFlume.identityQuery();
	}
	catch (ChimeraError& err) {
		msg == err.trace();
	}
	if (msg == "") {
		msg = "Disconnected...\n";
	}
	return msg;
}

void ArbGenCore::setArbGen(unsigned var, std::vector<parameterType>& params, deviceOutputInfo runSettings,
	ExpThreadWorker* expWorker) {
	if (!connected()) {
		return;
	}
	//auto notify = expWorker != nullptr // if in expworker, emit notification, else no way to notify currently. 
	//	? std::function{ [expWorker](QString note, unsigned debugLevel) {emit expWorker->notification({note, debugLevel}); } }
	//	: std::function{ [expWorker] (QString note, unsigned debugLevel) { } };
	setSync(runSettings, expWorker);
	for (auto chan : range(unsigned(2))) {
		auto& channel = runSettings.channel[chan];
		auto stdNote = qstr("Programming ArbGen " + arbGenName + " Channel " + str(chan) + " ");
		try {
			switch (channel.option) {
			case ArbGenChannelMode::which::No_Control:
				break;
			case ArbGenChannelMode::which::Output_Off:
				notify({ stdNote + "Output off.\n",1 }, expWorker);
				outputOff(chan + 1);
				break;
			case ArbGenChannelMode::which::DC:
				notify({ stdNote + " DC Voltage.\n", 1 }, expWorker);
				setDC(chan + 1, channel.dc, var);
				break;
			case ArbGenChannelMode::which::Sine:
				notify({ stdNote + " Sine Wave.\n", 1 }, expWorker);
				setSine(chan + 1, channel.sine, var);
				break;
			case ArbGenChannelMode::which::Square:
				notify({ stdNote + " Square Wave.\n", 1 }, expWorker);
				setSquare(chan + 1, channel.square, var);
				break;
			case ArbGenChannelMode::which::Preloaded:
				notify({ stdNote + " Preloaded Wave.\n", 1 }, expWorker);
				setExistingWaveform(chan + 1, channel.preloadedArb);
				break;
			case ArbGenChannelMode::which::Script:
				notify({ stdNote + " Script \"" + qstr(channel.scriptedArb.fileAddress) + "\"\n", 1 }, expWorker);
				handleScriptVariation(var, channel.scriptedArb, chan + 1, params);
				setScriptOutput(var, channel.scriptedArb, chan + 1);
				break;
			default:
				thrower("Unrecognized channel " + str(chan) + " setting?!?!?!: "
					+ ArbGenChannelMode::toStr(channel.option));
			}
		}
		catch (ChimeraError& err) {
			throwNested("Error seen while programming agilent output for " + configDelim + " ArbGen channel "
				+ str(chan + 1) + ": " + err.whatBare());
		}
	}
}

//void ArbGenCore::setSync()
//{
//	try {
//		notify({ "Writing Agilent output sync option: " + qstr(runSettings.synced), 2 }, expWorker);
//		visaFlume.write("OUTPut:SYNC " + str(runSettings.synced));
//	}
//	catch (ChimeraError&) {
//		//errBox ("Failed to set agilent output synced?!");
//	}
//}

// stuff that only has to be done once.
//void ArbGenCore::prepAgilentSettings(unsigned channel) {
//	if (channel != 1 && channel != 2) {
//		thrower("Bad value for channel in prepAgilentSettings! Channel shoulde be 1 or 2.");
//	}
//	// Set timout, sample rate, filter parameters, trigger settings.
//	visaFlume.setAttribute(VI_ATTR_TMO_VALUE, 40000);
//	visaFlume.write("SOURCE1:FUNC:ARB:SRATE " + str(sampleRate));
//	visaFlume.write("SOURCE2:FUNC:ARB:SRATE " + str(sampleRate));
//}

/*
 * This function tells the agilent to use sequence # (varNum) and sets settings correspondingly.
 */
//void ArbGenCore::setScriptOutput(unsigned varNum, scriptedArbInfo scriptInfo, unsigned chan) {
//	if (scriptInfo.wave.isVaried() || varNum == 0) {
//		prepAgilentSettings(chan);
//		// check if effectively dc
//		if (scriptInfo.wave.minsAndMaxes.size() == 0) {
//			thrower("script wave min max size is zero???");
//		}
//		auto& minMaxs = scriptInfo.wave.minsAndMaxes[varNum];
//		if (fabs(minMaxs.first - minMaxs.second) < 1e-6) {
//			dcInfo tempDc;
//			tempDc.dcLevel = str(minMaxs.first);
//			tempDc.dcLevel.internalEvaluate(std::vector<parameterType>(), 1);
//			tempDc.useCal = scriptInfo.useCal;
//			setDC(chan, tempDc, 0);
//		}
//		else {
//			auto schan = "SOURCE" + str(chan);
//			// Load sequence that was previously loaded.
//			visaFlume.write("MMEM:LOAD:DATA" + str(chan) + " \"" + memoryLoc + ":\\sequence" + str(varNum) + ".seq\"");
//			visaFlume.write(schan + ":FUNC ARB");
//			visaFlume.write(schan + ":FUNC:ARB \"" + memoryLoc + ":\\sequence" + str(varNum) + ".seq\"");
//			// set the offset and then the low & high. this prevents accidentally setting low higher than high or high 
//			// higher than low, which causes agilent to throw annoying errors.
//			visaFlume.write(schan + ":VOLT:OFFSET " + str((minMaxs.first + minMaxs.second) / 2) + " V");
//			visaFlume.write(schan + ":VOLT:LOW " + str(minMaxs.first) + " V");
//			visaFlume.write(schan + ":VOLT:HIGH " + str(minMaxs.second) + " V");
//			visaFlume.write("OUTPUT" + str(chan) + " ON");
//		}
//	}
//}

//void ArbGenCore::outputOff(int channel) {
//	if (channel != 1 && channel != 2) {
//		thrower("bad value for channel inside outputOff! Channel shoulde be 1 or 2.");
//	}
//	channel++;
//	visaFlume.write("OUTPUT" + str(channel) + " OFF");
//}


bool ArbGenCore::connected() {
	return isConnected;
}

void ArbGenCore::reconnectFlume()
{
	visaFlume.open();

}


void ArbGenCore::convertInputToFinalSettings(unsigned chan, deviceOutputInfo& info, std::vector<parameterType>& params) {
	unsigned totalVariations = (params.size() == 0) ? 1 : params.front().keyValues.size();
	channelInfo& channel = info.channel[chan];
	try {
		switch (channel.option)
		{
		case ArbGenChannelMode::which::No_Control:
		case ArbGenChannelMode::which::Output_Off:
			break;
		case ArbGenChannelMode::which::DC:
			channel.dc.dcLevel.internalEvaluate(params, totalVariations);
			break;
		case ArbGenChannelMode::which::Sine:
			channel.sine.frequency.internalEvaluate(params, totalVariations);
			channel.sine.amplitude.internalEvaluate(params, totalVariations);
			break;
		case ArbGenChannelMode::which::Square:
			channel.square.frequency.internalEvaluate(params, totalVariations);
			channel.square.amplitude.internalEvaluate(params, totalVariations);
			channel.square.offset.internalEvaluate(params, totalVariations);
			break;
		case ArbGenChannelMode::which::Preloaded:
			break;
		case ArbGenChannelMode::which::Script:
			channel.scriptedArb.wave.calculateAllSegmentVariations(totalVariations, params);
			break;
		default:
			thrower("Unrecognized ArbGen Setting: " + ArbGenChannelMode::toStr(channel.option));
		}
	}
	catch (std::out_of_range&) {
		throwNested("unrecognized variable!");
	}
}

void ArbGenCore::convertInputToFinalSettings(unsigned chan, deviceOutputInfo& info)
{
	std::vector<parameterType> variables = std::vector<parameterType>();
	convertInputToFinalSettings(chan, info, variables);
}

/**
 * expects the inputted power to be in -MILI-WATTS!
 * returns set point in VOLTS
 */
double ArbGenCore::convertPowerToSetPoint(double requestedSetting, bool conversionOption,
	calResult calibration) {
	// requested setting is the voltage or power settting coming from the calibration, depending on how the agilent
	// was calibrated (power typically for tweezer powers, voltage on PD otherwise). 
	if (conversionOption) {
		// build the polynomial calibration.
		// fix this after add analog input + calibration -zzp 2021/2/24
		double setPointInVolts = requestedSetting/*CalibrationManager::calibrationFunction (requestedSetting, calibration)*/;
		return setPointInVolts;
	}
	else {
		// no conversion
		return requestedSetting;
	}
}


std::vector<std::string> ArbGenCore::getStartupCommands() {
	return setupCommands;
}

void ArbGenCore::programSetupCommands() {
	try {
		for (auto cmd : setupCommands) {
			visaFlume.write(cmd);
		}
	}
	catch (ChimeraError&) {
		throwNested("Failed to program setup commands for " + arbGenName + " ArbGen!");
	}
}


deviceOutputInfo ArbGenCore::getSettingsFromConfig(ConfigStream& file) {
	auto readFunc = ConfigSystem::getGetlineFunc(file.ver);
	deviceOutputInfo tempSettings;
	file >> tempSettings.synced;
	std::array<std::string, 2> channelNames = { "CHANNEL_1", "CHANNEL_2" };
	unsigned chanInc = 0;
	for (auto& channel : tempSettings.channel) {
		ConfigSystem::checkDelimiterLine(file, channelNames[chanInc]);
		// the extra step in all of the following is to remove the , at the end of each input.
		std::string input;
		file >> input;
		try {
			/*channel.option = file.ver < Version ("4.2") ?
				ArbGenChannelMode::which (boost::lexical_cast<int>(input) + 2) : ArbGenChannelMode::fromStr (input);*/
			channel.option = ArbGenChannelMode::fromStr(input);
		}
		catch (boost::bad_lexical_cast&) {
			throwNested("Bad channel " + str(chanInc + 1) + " option!");
		}
		std::string calibratedOption;
		file.get();
		readFunc(file, channel.dc.dcLevel.expressionStr);
		//if (file.ver > Version ("2.3")){
		file >> channel.dc.useCal;
		file.get();
		//}
		readFunc(file, channel.sine.amplitude.expressionStr);
		readFunc(file, channel.sine.frequency.expressionStr);
		file >> channel.sine.burstMode;
		//if (file.ver > Version ("2.3")){
		file >> channel.sine.useCal;
		file.get();
		//}
		readFunc(file, channel.square.amplitude.expressionStr);
		readFunc(file, channel.square.frequency.expressionStr);
		readFunc(file, channel.square.offset.expressionStr);
		file >> channel.square.burstMode;
		//if (file.ver > Version ("2.3")){
		file >> channel.square.useCal;
		file.get();
		//}
		readFunc(file, channel.preloadedArb.address.expressionStr);
		//if (file.ver > Version ("2.3")){
		file >> channel.preloadedArb.useCal;
		file.get();
		//}
		//if (file.ver >= Version ("5.9")) {
		file >> channel.preloadedArb.burstMode;
		file.get();
		//}
		readFunc(file, channel.scriptedArb.fileAddress.expressionStr);
		//if (file.ver > Version ("2.3")){
		file >> channel.scriptedArb.useCal;
		//}
		chanInc++;
	}
	return tempSettings;
}


void ArbGenCore::logSettings(DataLogger& log, ExpThreadWorker* threadworker) {
	try {
		H5::Group arbGensGroup;
		try {
			arbGensGroup = log.file.createGroup("/ArbGens");
		}
		catch (H5::Exception&) {
			arbGensGroup = log.file.openGroup("/ArbGens");
		}

		H5::Group singleArbGen(arbGensGroup.createGroup(getDelim()));
		unsigned channelCount = 1;
		log.writeDataSet(getStartupCommands(), "Startup-Commands", singleArbGen);
		for (auto& channel : expRunSettings.channel) {
			H5::Group channelGroup(singleArbGen.createGroup("Channel-" + str(channelCount)));
			std::string outputModeName = ArbGenChannelMode::toStr(channel.option);
			log.writeDataSet(outputModeName, "Output-Mode", channelGroup);
			H5::Group dcGroup(channelGroup.createGroup("DC-Settings"));
			log.writeDataSet(channel.dc.dcLevel.expressionStr, "DC-Level", dcGroup);
			H5::Group sineGroup(channelGroup.createGroup("Sine-Settings"));
			log.writeDataSet(channel.sine.frequency.expressionStr, "Frequency", sineGroup);
			log.writeDataSet(channel.sine.amplitude.expressionStr, "Amplitude", sineGroup);
			H5::Group squareGroup(channelGroup.createGroup("Square-Settings"));
			log.writeDataSet(channel.square.amplitude.expressionStr, "Amplitude", squareGroup);
			log.writeDataSet(channel.square.frequency.expressionStr, "Frequency", squareGroup);
			log.writeDataSet(channel.square.offset.expressionStr, "Offset", squareGroup);
			H5::Group preloadedArbGroup(channelGroup.createGroup("Preloaded-Arb-Settings"));
			log.writeDataSet(channel.preloadedArb.address.expressionStr, "Address", preloadedArbGroup);
			H5::Group scriptedArbSettings(channelGroup.createGroup("Scripted-Arb-Settings"));
			log.writeDataSet(channel.scriptedArb.fileAddress.expressionStr, "Script-File-Address", scriptedArbSettings);
			// TODO: load script file itself
			ScriptStream stream;
			try {
				ExpThreadWorker::loadArbGenScript(channel.scriptedArb.fileAddress.expressionStr, stream);
				log.writeDataSet(stream.str(), "ArbGen-Script-Script", scriptedArbSettings);
			}
			catch (ChimeraError&) {
				// failed to open, that's probably fine, 
				log.writeDataSet("Script Failed to load.", "ArbGen-Script-Script", scriptedArbSettings);
			}
			channelCount++;
		}
	}
	catch (H5::Exception err) {
		log.logError(err);
		throwNested("ERROR: Failed to log ArbGen parameters in HDF5 file: " + err.getDetailMsg());
	}
}

void ArbGenCore::loadExpSettings(ConfigStream& script) {
	ConfigSystem::stdGetFromConfig(script, *this, expRunSettings);
	experimentActive = (expRunSettings.channel[0].option != ArbGenChannelMode::which::No_Control
		|| expRunSettings.channel[1].option != ArbGenChannelMode::which::No_Control);
}

void ArbGenCore::calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadWorker) {
	std::string commwarnings;
	for (auto channelInc : range(2)) {
		if (expRunSettings.channel[channelInc].scriptedArb.fileAddress.expressionStr != "") {
			analyzeArbGenScript(expRunSettings.channel[channelInc].scriptedArb, params, commwarnings);
		}
	}
	convertInputToFinalSettings(0, expRunSettings, params);
	convertInputToFinalSettings(1, expRunSettings, params);
}

void ArbGenCore::setRunSettings(deviceOutputInfo newSettings) {
	// used by "program now".
	expRunSettings = newSettings;
}

void ArbGenCore::programVariation(unsigned variation, std::vector<parameterType>& params, ExpThreadWorker* threadWorker) {
	setArbGen(variation, params, expRunSettings, threadWorker);
}

void ArbGenCore::checkTriggers(unsigned variationInc, DoCore& ttls, ExpThreadWorker* threadWorker) {
	std::array<bool, 2> agMismatchVec = { false, false };
	for (auto chan : range(2)) {
		auto& agChan = expRunSettings.channel[chan];
		if (agChan.option != ArbGenChannelMode::which::Script || agMismatchVec[chan]) {
			continue;
		}
		unsigned actualTrigs = experimentActive ? ttls.countTriggers(getTriggerLine(), variationInc) : 0;
		unsigned arbGenExpectedTrigs = agChan.scriptedArb.wave.getNumTrigs();
		std::string infoString = "Actual/Expected " + getDelim() + " Triggers: "
			+ str(actualTrigs) + "/" + str(arbGenExpectedTrigs) + ".";
		if (actualTrigs != arbGenExpectedTrigs) {
			emit threadWorker->warn(qstr(
				"WARNING: ArbGen " + getDelim() + " is not getting triggered by the ttl system the same "
				"number of times a trigger command appears in the ArbGen channel " + str(chan + 1) + " script. "
				+ infoString + " First seen in variation #" + str(variationInc) + ".\r\n"));
			agMismatchVec[chan] = true;
		}
	}
}

void ArbGenCore::setAgCalibration(calResult newCal, unsigned chan) {
	if (chan != 1 && chan != 2) {
		thrower("ERROR: ArbGen channel must be 1 or 2!");
	}
	calibrations[chan - 1] = newCal;
}

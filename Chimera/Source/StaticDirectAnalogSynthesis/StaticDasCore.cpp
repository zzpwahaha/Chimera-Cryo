#include "stdafx.h"
#include "StaticDasCore.h"
#include <ExperimentThread/ExpThreadWorker.h>
#include <ConfigurationSystems/ConfigSystem.h>
#include <DataLogging/DataLogger.h>

StaticDasCore::StaticDasCore(
	const std::array<bool, size_t(StaticDASGrid::numOFunit)> safemodes,
	const std::array<std::string, size_t(StaticDASGrid::numOFunit)> ports,
	const std::array<unsigned, size_t(StaticDASGrid::numOFunit)> baudrates) :
	safemodes(safemodes),
	sdasFlumes{ StaticDASFlume(ports[0], baudrates[0], safemodes[0]) }
{
	for (auto ch : range(size_t(StaticDASGrid::numOFunit))) {
		if (safemodes[ch]) {
			return;
		}
		std::string cmd;
		try {
			for (auto cmd : dasSetupCommands) {
				sdasFlumes[ch].write(cmd);
				Sleep(1);
			}
			Sleep(10);
			auto msg = sdasFlumes[ch].query("Lock?");
			if (msg.find("not") != std::string::npos) {
				thrower("Error after initilizing setup command: PLL is not locked. Make sure the reference and PFD settings are correct and power supply is supllying enough current!");
			}
		}
		catch (ChimeraError& e) {
			throwNested("Error seen in initlizing DAS with setup commands: " + cmd);
		}
	}
}

void StaticDasCore::loadExpSettings(ConfigStream& stream)
{
	ConfigSystem::stdGetFromConfig(stream, *this, expSettings);
	experimentActive = expSettings.ctrlDAS;
}

void StaticDasCore::logSettings(DataLogger& logger, ExpThreadWorker* threadworker)
{}

void StaticDasCore::calculateVariations(std::vector<parameterType>&params, ExpThreadWorker * threadworker)
{
	if (!experimentActive && !expSettings.ctrlDAS) {
		return;
	}
	size_t totalVariations = (params.size() == 0) ? 1 : params.front().keyValues.size();
	try {
		for (auto ch : range(size_t(StaticDASGrid::total))) {
			expSettings.staticDASs[ch].assertValid(params, GLOBAL_PARAMETER_SCOPE);
			expSettings.staticDASs[ch].internalEvaluate(params, totalVariations);
			if (expSettings.staticDASs[ch].varies() && safemodes[ch / size_t(StaticDASGrid::numPERunit)]) {
				thrower("Error in varying static DAS for channel " + str(ch) +
					". The DAS is in SAFEMODE in constant.h but is varied given expression " +
					expSettings.staticDASs[ch].expressionStr);
			}
			for (auto variation : range(totalVariations)) {
				auto dasfreqVal = expSettings.staticDASs[ch].getValue(variation);
				if (!checkBound(dasfreqVal)) {
					thrower("Error in varying static DAS for channel " + str(ch) + " and variation" + str(variation) +
						". The DAS is limited to " + str(minVal) + " MHz to " + str(maxVal) +
						" MHz but is set to an outside value given expression " +
						expSettings.staticDASs[ch].expressionStr + " and its evaluation: " + str(dasfreqVal));
				}
			}
		}
	}
	catch (ChimeraError&) {
		throwNested("Failed to evaluate staticAO expression varations!");
	}
}

void StaticDasCore::programVariation(unsigned variation, std::vector<parameterType>& params, ExpThreadWorker* threadworker)
{
	if (!experimentActive && !expSettings.ctrlDAS) {
		return;
	}
	std::array<double, size_t(StaticDASGrid::total)> outputs;
	for (auto ch : range(size_t(StaticDASGrid::total))) {
		outputs[ch] = expSettings.staticDASs[ch].getValue(variation);
	}
	writeDASs(outputs);
}

StaticDASSettings StaticDasCore::getSettingsFromConfig(ConfigStream& file)
{
	StaticDASSettings tempSettings;
	auto getlineF = ConfigSystem::getGetlineFunc(file.ver);
	//file.get();
	for (auto ch : range(size_t(StaticDASGrid::total))) {
		getlineF(file, tempSettings.staticDASs[ch].expressionStr);
	}
	file >> tempSettings.ctrlDAS;
	file.get();
	return tempSettings;
}

void StaticDasCore::resetConnection()
{
	for (auto& flume : sdasFlumes) {
		flume.resetConnection();
	}
}

void StaticDasCore::directWrite(std::string cmd, int deviceId)
{
	if (deviceId > sdasFlumes.size() - 1) {
		thrower("Error in StaticDasCore::directWrite: the device id " + str(deviceId) + 
			" is larger than the actual device numeber " + str(sdasFlumes.size()) + ". This is a low level bug!");
	}
	try {
		sdasFlumes[deviceId].write(cmd);
	}
	catch (ChimeraError& e) {
		throwNested("Error seen in trying to write to Microwave System:");
	}
}

std::string StaticDasCore::directRead(int deviceId)
{
	if (deviceId > sdasFlumes.size() - 1) {
		thrower("Error in StaticDasCore::directWrite: the device id " + str(deviceId) +
			" is larger than the actual device numeber " + str(sdasFlumes.size()) + ". This is a low level bug!");
	}
	try {
		return sdasFlumes[deviceId].read();
	}
	catch (ChimeraError& e) {
		throwNested("Error seen in trying to write to Microwave System:");
	}
}

std::string StaticDasCore::getDeviceInfo()
{
	std::string info;
	for (auto& flume : sdasFlumes) {
		info += flume.query("ID") + "\n\t\t";
	}
	info.erase(info.length() - 3, 3);
	return info;
}

void StaticDasCore::setStaticDASExpSetting(StaticDASSettings tmpSetting)
{
	expSettings = tmpSetting;
}

std::string StaticDasCore::getDASCommand(double dasfreqVal, int channel)
{
	return "SOURCE " + str((channel % size_t(StaticDASGrid::numPERunit)) + 1) + ";Frequency " + str(dasfreqVal, numFreqDigits) + " MHz;";
}

void StaticDasCore::writeDASs(std::array<double, size_t(StaticDASGrid::total)> outputs)
{
	std::string command;
	for (auto ch : range(size_t(StaticDASGrid::total))) {
		command = getDASCommand(outputs[ch], ch);
		size_t unitNum = ch / size_t(StaticDASGrid::numPERunit);
		sdasFlumes[unitNum].write(command);
		if (!safemodes[unitNum]) {
			std::string recv = sdasFlumes[unitNum].read();
			std::transform(recv.begin(), recv.end(), recv.begin(), ::tolower); /*:: without namespace select from global namespce, see https://stackoverflow.com/questions/5539249/why-cant-transforms-begin-s-end-s-begin-tolower-be-complied-successfu*/
			if (recv.find("error") != std::string::npos) {
				thrower("Error in static DAS programming, from Arduino: " + recv);
			}
		}
	}
}

bool StaticDasCore::checkBound(double dasfreqVal)
{
	if (dasfreqVal > maxVal) {
		return false;
	}
	else if (dasfreqVal < minVal) {
		return false;
	}
	else {
		return true;
	}
}

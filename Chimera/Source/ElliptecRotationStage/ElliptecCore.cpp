#include "stdafx.h"
#include "ElliptecCore.h"
#include <ExperimentThread/ExpThreadWorker.h>
#include <ConfigurationSystems/ConfigSystem.h>
#include <DataLogging/DataLogger.h>
#include <boost/format.hpp>
#include <qelapsedtimer.h>
#include <qdebug.h>

ElliptecCore::ElliptecCore(bool safemode, const std::array<std::string, size_t(ElliptecGrid::numOFunit)> port) :
	safemode(safemode),
	ellFlumes{ ElliptecFlume(port[0],safemode), ElliptecFlume(port[1],safemode) }
{}

void ElliptecCore::loadExpSettings(ConfigStream& stream)
{
	ConfigSystem::stdGetFromConfig(stream, *this, expSettings);
	experimentActive = expSettings.ctrlEll;
}

void ElliptecCore::logSettings(DataLogger& logger, ExpThreadWorker* threadworker)
{}

void ElliptecCore::calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker)
{
	if (!experimentActive && !expSettings.ctrlEll) {
		return;
	}
	size_t totalVariations = (params.size() == 0) ? 1 : params.front().keyValues.size();
	try {
		for (auto ch : range(size_t(ElliptecGrid::total))) {
			expSettings.elliptecs[ch].assertValid(params, GLOBAL_PARAMETER_SCOPE);
			expSettings.elliptecs[ch].internalEvaluate(params, totalVariations);
			if (expSettings.elliptecs[ch].varies() && safemode) {
				thrower("Error in varying Elliptecs value for channel " + str(ch) +
					". The Elliptec is in SAFEMODE in constant.h but is varied given expression " +
					expSettings.elliptecs[ch].expressionStr);
			}
			for (auto variation : range(totalVariations)) {
				auto elliptecAngle = expSettings.elliptecs[ch].getValue(variation);
				if (!checkBound(elliptecAngle)) {
					thrower("Error in varying Elliptecs value for channel " + str(ch) + " and variation" + str(variation) +
						". The Ellipte is limited to " + str(minVal) + " degree to " + str(maxVal) +
						" degree but is set to an outside value given expression " +
						expSettings.elliptecs[ch].expressionStr + " and its evaluation: " + str(elliptecAngle));
				}
			}
		}
	}
	catch (ChimeraError&) {
		throwNested("Failed to evaluate staticAO expression varations!");
	}
}

void ElliptecCore::programVariation(unsigned variation, std::vector<parameterType>& params, ExpThreadWorker* threadworker)
{
	if (!experimentActive && !expSettings.ctrlEll) {
		return;
	}
	std::array<double, size_t(ElliptecGrid::total)> outputs;
	for (auto ch : range(size_t(ElliptecGrid::total))) {
		if (expSettings.elliptecs[ch].varies() || variation == 0) {
			if (expSettings.elliptecs[ch].varies() && !ELLIPTEC_CONNECTED[ch]) {
				thrower("Elliptec at channel: " + str(ch) + " is not connected but varied to be intended for scan. Double check the constant.h and whether the scan is placed at the correct location.");
			}
			outputs[ch] = expSettings.elliptecs[ch].getValue(variation);
			if (!ELLIPTEC_CONNECTED[ch]){
				continue;
			}
			writeElliptec(ch, outputs[ch]);
		}
	}
	//writeElliptecs(outputs);
}

ElliptecSettings ElliptecCore::getSettingsFromConfig(ConfigStream& file)
{
	ElliptecSettings tempSettings;
	auto getlineF = ConfigSystem::getGetlineFunc(file.ver);
	//file.get();
	for (auto ch : range(size_t(ElliptecGrid::total))) {
		getlineF(file, tempSettings.elliptecs[ch].expressionStr);
	}
	file >> tempSettings.ctrlEll;
	file.get();
	return tempSettings;
}

std::string ElliptecCore::getDeviceInfo()
{
	std::string info;
	for (auto ch : range(size_t(ElliptecGrid::total))) {
		if (!ELLIPTEC_CONNECTED[ch]) {
			continue;
		}
		info.append(ellFlumes[ch / size_t(ElliptecGrid::numPERunit)].queryIdentity(ch % size_t(ElliptecGrid::numPERunit)));
		info.append("\n\t\t");
	}
	return info;
}

void ElliptecCore::setElliptecExpSettings(ElliptecSettings tmpSetting)
{
	expSettings = tmpSetting;
}

double ElliptecCore::normalizeToRange(double angle, double min, double max)
{
	double range = max - min;
	// Bring angle into the range [0, range)
	angle = std::fmod(angle - min, range);
	if (angle < 0) {
		angle += range; // Wrap negative values
	}
	// Shift into the desired range [min, max)
	return angle + min;
}

double ElliptecCore::getElliptecPosition(unsigned channel)
{
	if (channel >= size_t(ElliptecGrid::total)) {
		thrower("Channel of Elliptec out of range in getElliptecCommand as " + str(channel));
	}
	if (safemode || !ELLIPTEC_CONNECTED[channel]) {
		return 0.0;
	}
	std::string readBack;

	unsigned maxRetries = 3;
	for (int attempt = 0; attempt < maxRetries; ++attempt) {
		try {
			readBack = ellFlumes[channel / size_t(ElliptecGrid::numPERunit)].queryWithCheck(
				str(channel % size_t(ElliptecGrid::numPERunit)) + "gp");
			break;
		}
		catch (ChimeraError& err) {
			if (err.whatBare().find("Reading is empty and timed out") != std::string::npos) {
				if (attempt == maxRetries - 1) {
					qDebug() << "ElliptecCore::getElliptecPosition: Max retries reached, operation failed!";
					throwNested("Rethrow after trying " + str(maxRetries) + " times for ElliptecCore::getElliptecPosition");
				}
				else {
					qDebug() << "ElliptecCore::getElliptecPosition: Failed due to time out: \"" + qstr(err.whatBare()) + "\" , retrying...";
				}
			}
		}
	}
	
	int angleInt = static_cast<int32_t>(std::stoul(readBack.substr(3, 8), nullptr, 16));
	return angleInt * elliptecResolutionInst;
}

std::string ElliptecCore::getElliptecCommand(unsigned channel, double angle)
{
	std::string command;
	if (channel >= size_t(ElliptecGrid::total)) {
		thrower("Channel of Elliptec out of range in getElliptecCommand as " + str(channel));
	}
	double currentAngle = getElliptecPosition(channel);
	// Calculate absolute move
	double absoluteMove = angle - currentAngle;
	// Normalize relative move to [-180, 180]
	double relativeMove = normalizeToRange(absoluteMove, -180.0, 180.0);

	unsigned angleInt = static_cast<int>(round(relativeMove / elliptecResolutionInst));
	command = (boost::format("%dmr%08X") % (channel % size_t(ElliptecGrid::numPERunit)) % angleInt).str();
	return command;
}

void ElliptecCore::writeElliptecs(std::array<double, size_t(ElliptecGrid::total)> outputs)
{
	if (safemode) {
		return;
	}
	for (auto ch : range(size_t(ElliptecGrid::total))) {
		if (!ELLIPTEC_CONNECTED[ch]) {
			continue;
		}
		writeElliptec(ch, outputs[ch]);
		Sleep(10);
	}
}

void ElliptecCore::writeElliptec(unsigned channel, double angle)
{
	if (!ELLIPTEC_CONNECTED[channel]) {
		thrower("Elliptec channel " + str(channel) + " is not connected but trying to be written. This shouldn't happen at this low level. A bug.");
	}
	QElapsedTimer timerE;
	timerE.start();
	qDebug() << " Before write" << timerE.elapsed();
	auto command = getElliptecCommand(channel, angle);
	Sleep(5);
	ellFlumes[channel / size_t(ElliptecGrid::numPERunit)].write(command);
	qDebug() << " After write" << timerE.elapsed();
	if (!safemode) {
		std::string recv = ellFlumes[channel / size_t(ElliptecGrid::numPERunit)].readWithCheck();
		qDebug() << " After read" << timerE.elapsed();
		int angleInt = static_cast<int32_t>(std::stoul(recv.substr(3, 8), nullptr, 16));
		double angle = angleInt * elliptecResolutionInst;
		emit currentAngle(channel, angle);
		qDebug() << " After emit" << timerE.elapsed();
	}

}

bool ElliptecCore::checkBound(double angle)
{
	if (angle > maxVal) {
		return false;
	}
	else if (angle < minVal) {
		return false;
	}
	else {
		return true;
	}
}

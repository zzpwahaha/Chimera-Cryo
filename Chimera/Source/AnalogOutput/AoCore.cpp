#include "stdafx.h"
#include "AoCore.h"
#include "ExperimentThread/ExpThreadWorker.h"
#include "GeneralObjects/CodeTimer.h"
#include <ExperimentMonitoringAndStatus/ExperimentSeqPlotter.h>


AoCore::AoCore() : dacTriggerTime(0.0005)
{

}

void AoCore::setNames(std::array<std::string, size_t(AOGrid::total)> namesIn)
{
	std::for_each(namesIn.begin(), namesIn.end(), [&](auto& name) {
		std::transform(name.begin(), name.end(), name.begin(), ::tolower); });
	//std::transform(names[doInc].begin(), names[doInc].end(), names[doInc].begin(), ::tolower);
	names = std::move(namesIn);
}

std::array<std::string, size_t(AOGrid::total)> AoCore::getNames()
{
	return names;
}


unsigned AoCore::getNumberSnapshots(unsigned variation) 
{
	return dacSnapshots[variation].size();
}

std::vector<std::vector<AoSnapshot>> AoCore::getSnapshots() 
{
	/* used by the unit testing suite. */
	return dacSnapshots;
}

unsigned long AoCore::getNumberEvents(unsigned variation) 
{
	return dacSnapshots[variation].size();
}





std::string AoCore::getDacSequenceMessage(unsigned variation) 
{
	std::string message;
	for (auto snap : dacSnapshots[variation]) {
		std::string time = str(snap.time, 12, true);
		message += time + ":\r\n";
		int dacCount = 0;
		for (auto val : snap.dacValues) {
			std::string volt = str(val, true);
			message += volt + ", ";
			dacCount++;
			if (dacCount % 8 == 0) {
				message += "\r\n";
			}
		}
		message += "\r\n---\r\n";
	}
	return message;
}

std::vector<std::vector<plotDataVec>> AoCore::getPlotData(unsigned var) 
{
	unsigned linesPerPlot = size_t(AOGrid::total) / ExperimentSeqPlotter::NUM_DAC_PLTS;
	std::vector<std::vector<plotDataVec>> dacData(ExperimentSeqPlotter::NUM_DAC_PLTS,
		std::vector<plotDataVec>(linesPerPlot));
	if (dacSnapshots.size() <= var) {
		thrower("Attempted to use dac data from variation " + str(var) + ", which does not exist!");
	}
	// each element of dacData should be one analog line.
	for (auto line : range(size_t(AOGrid::total))) {
		auto& data = dacData[line / linesPerPlot][line % linesPerPlot];
		data.clear();
		for (auto snapn : range(dacSnapshots[var].size())) {
			if (snapn != 0) {
				data.push_back({ dacSnapshots[var][snapn].time, double(dacSnapshots[var][snapn - 1].dacValues[line]), 0 });
			}
			data.push_back({ dacSnapshots[var][snapn].time, double(dacSnapshots[var][snapn].dacValues[line]), 0 });
		}
	}
	return dacData;
}

std::array<double, size_t(AOGrid::total)> AoCore::getFinalSnapshot()
{
	auto numVar = dacSnapshots.size();
	if (numVar != 0)
	{
		if (dacSnapshots[numVar - 1].size() != 0)
		{
			return dacSnapshots[numVar - 1].back().dacValues;
		}
	}
	thrower("No DAC Events");
}




// note that this is not directly tied to changing any "current" parameters in the AoSystem object (it of course changes a list parameter). The 
// AoSystem object "current" parameters aren't updated to reflect an experiment, so if this is called for a force out, it should be called in conjuction
// with changing "currnet" parameters in the AoSystem object.
void AoCore::setDacCommandForm(AoCommandForm command) 
{
	dacCommandFormList.push_back(command);
}

void AoCore::initializeDataObjects(unsigned cmdNum) 
{
	dacCommandFormList = std::vector<AoCommandForm>(cmdNum);

	dacCommandList.clear();
	dacSnapshots.clear();
	loadSkipDacSnapshots.clear();
	//finalFormatDacData.clear();
	//loadSkipDacFinalFormat.clear();

	dacCommandList.resize(cmdNum);
	dacSnapshots.resize(cmdNum);
	loadSkipDacSnapshots.resize(cmdNum);
	//finalFormatDacData.resize(cmdNum);
	//loadSkipDacFinalFormat.resize(cmdNum);

}


void AoCore::resetDacEvents()
{
	dacCommandFormList.clear();
	dacCommandList.clear();
	dacSnapshots.clear();
	loadSkipDacSnapshots.clear();

	//loadSkipDacFinalFormat.clear();

	initializeDataObjects(0);
}


void AoCore::calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker,
	std::vector<calResult> calibrations) 
{
	CodeTimer sTimer;
	sTimer.tick("Ao-Sys-Interpret-Start");
	unsigned variations = params.size() == 0 ? 1 : params.front().keyValues.size();
	if (variations == 0) {
		variations = 1;
	}
	/// imporantly, this sizes the relevant structures.
	dacCommandList.clear();
	dacSnapshots.clear();
	loadSkipDacSnapshots.clear();
	//finalFormatDacData.clear();
	//loadSkipDacFinalFormat.clear();

	finalDacSnapshots.clear();

	dacCommandList.resize(variations);
	dacSnapshots.resize(variations);
	loadSkipDacSnapshots.resize(variations);
	//finalFormatDacData.resize(variations);
	//loadSkipDacFinalFormat.resize(variations);

	finalDacSnapshots.resize(variations);

	bool resolutionWarningPosted = false;
	bool nonIntegerWarningPosted = false;
	sTimer.tick("After-init");
	for (auto variationInc : range(variations)) {
		if (variationInc == 0) {
			sTimer.tick("Variation-" + str(variationInc) + "-Start");
		}
		auto& cmdList = dacCommandList[variationInc];
		for (auto eventInc : range(dacCommandFormList.size())) {
			AoCommand tempEvent;
			auto& formList = dacCommandFormList[eventInc];
			tempEvent.line = formList.line;
			// Deal with time.
			if (formList.time.first.size() == 0) {
				// no variable portion of the time.
				tempEvent.time = formList.time.second;
			}
			else {
				double varTime = 0;
				for (auto variableTimeString : formList.time.first) {
					varTime += variableTimeString.evaluate(params, variationInc, calibrations);
				}
				tempEvent.time = varTime + formList.time.second;
			}
			if (variationInc == 0) {
				sTimer.tick("Time-Handled");
			}
			/// deal with command
			if (formList.commandName == "dac:") {
				/// single point.
				tempEvent.value = formList.finalVal.evaluate(params, variationInc, calibrations);
				tempEvent.endValue = tempEvent.value;
				tempEvent.rampTime = 0;

				if (variationInc == 0)/*for time tick, no effect for code*/
				{
					sTimer.tick("val-evaluated");
				}

				cmdList.push_back(tempEvent);

				if (variationInc == 0)/*for time tick, no effect for code*/
				{
					sTimer.tick("Dac:-Handled");
				}
			}
			else if (formList.commandName == "dacarange:") {
				// interpret ramp time command. I need to know whether it's ramping or not.
				double rampTime = formList.rampTime.evaluate(params, variationInc, calibrations);
				/// many points to be made.
				// convert initValue and finalValue to doubles to be used 
				double initValue, finalValue, rampInc;
				initValue = formList.initVal.evaluate(params, variationInc, calibrations);
				// deal with final value;
				finalValue = formList.finalVal.evaluate(params, variationInc, calibrations);
				// deal with ramp inc
				rampInc = formList.rampInc.evaluate(params, variationInc, calibrations);
				if (rampInc < 10.0 / pow(2, 16) && resolutionWarningPosted) {
					resolutionWarningPosted = true;
					emit threadworker->warn(cstr("Warning: ramp increment of " + str(rampInc) + " in dac command number "
						+ str(eventInc) + " is below the resolution of the aoSys (which is 10/2^16 = "
						+ str(10.0 / pow(2, 16)) + "). These ramp points are unnecessary.\r\n"));
				}
				// This might be the first not i++ usage of a for loop I've ever done... XD
				// calculate the time increment:
				int steps = int(fabs(finalValue - initValue) / rampInc + 0.5);
				double stepsFloat = fabs(finalValue - initValue) / rampInc;
				double diff = fabs(steps - fabs(finalValue - initValue) / rampInc);
				if (diff > 100 * DBL_EPSILON && nonIntegerWarningPosted) {
					nonIntegerWarningPosted = true;
					emit threadworker->warn(cstr("Warning: Ideally your spacings for a dacArange would result in a non-integer number "
						"of steps. The code will attempt to compensate by making a last step to the final value which"
						" is not the same increment in voltage or time as the other steps to take the dac to the final"
						" value at the right time.\r\n"));
				}
				double timeInc = rampTime / steps;
				double initTime = tempEvent.time;
				double currentTime = tempEvent.time;
				// handle the two directions seperately.
				if (initValue < finalValue) {
					for (double dacValue = initValue;
						(dacValue - finalValue) < -steps * 2 * DBL_EPSILON; dacValue += rampInc)
					{
						tempEvent.value = dacValue;
						tempEvent.endValue = dacValue;
						tempEvent.rampTime = 0;
						tempEvent.time = currentTime;
						cmdList.push_back(tempEvent);
						currentTime += timeInc;

					}
				}
				else
				{
					for (double dacValue = initValue;
						dacValue - finalValue > 100 * DBL_EPSILON; dacValue -= rampInc) {
						tempEvent.value = dacValue;
						tempEvent.endValue = dacValue;
						tempEvent.rampTime = 0;
						tempEvent.time = currentTime;
						cmdList.push_back(tempEvent);
						currentTime += timeInc;
					}
				}
				// and get the final value.
				tempEvent.value = finalValue;
				tempEvent.endValue = finalValue;
				tempEvent.rampTime = 0;
				tempEvent.time = initTime + rampTime;
				cmdList.push_back(tempEvent);
				if (variationInc == 0) {
					sTimer.tick("dacarange:-Handled");
				}
			}
			else if (formList.commandName == "daclinspace:") {
				// interpret ramp time command. I need to know whether it's ramping or not.
				double rampTime = formList.rampTime.evaluate(params, variationInc, calibrations);
				/// many points to be made.
				double initValue, finalValue;
				unsigned numSteps;
				initValue = formList.initVal.evaluate(params, variationInc, calibrations);
				finalValue = formList.finalVal.evaluate(params, variationInc, calibrations);
				numSteps = formList.numSteps.evaluate(params, variationInc, calibrations);
				double rampInc = (finalValue - initValue) / numSteps;
				// this warning isn't actually very useful. very rare that actually run into issues with overtaxing ao 
				// or do systems like this and these circumstances often happen when something is ramped.
				/*if ( (fabs( rampInc ) < 10.0 / pow( 2, 16 )) && !resolutionWarningPosted ){
					resolutionWarningPosted = true;
					emit threadworker->warn (cstr ("Warning: numPoints of " + str (numSteps) + " results in a ramp increment of "
						+ str (rampInc) + " is below the resolution of the aoSys (which is 10/2^16 = "
						+ str (10.0 / pow (2, 16)) + "). It's likely taxing the system to "
						"calculate the ramp unnecessarily.\r\n"));
				}*/
				// This might be the first not i++ usage of a for loop I've ever done... XD
				// calculate the time increment:
				double timeInc = rampTime / numSteps;
				double initTime = tempEvent.time;
				double currentTime = tempEvent.time;
				double val = initValue;
				// handle the two directions seperately.
				for (auto stepNum : range(numSteps))
				{
					tempEvent.value = val;
					tempEvent.endValue = val;
					tempEvent.rampTime = 0;
					tempEvent.time = currentTime;
					cmdList.push_back(tempEvent);
					currentTime += timeInc;
					val += rampInc;
				}
				// and get the final value. Just use the nums explicitly to avoid rounding error I guess.
				tempEvent.value = finalValue;
				tempEvent.endValue = finalValue;
				tempEvent.rampTime = 0;
				tempEvent.time = initTime + rampTime;
				cmdList.push_back(tempEvent);
				if (variationInc == 0) {
					sTimer.tick("daclinspace:-Handled");
				}
			}
			else if (formList.commandName == "dacramp:")
			{
				// interpret ramp time command. I need to know whether it's ramping or not.
				double rampTime = formList.rampTime.evaluate(params, variationInc);
				/// many points to be made.
				// convert initValue and finalValue to doubles to be used 
				double initValue, finalValue, numSteps;
				initValue = formList.initVal.evaluate(params, variationInc);
				// deal with final value;
				finalValue = formList.finalVal.evaluate(params, variationInc);
				// set votlage resolution to be maximum allowed by the ramp range and time
				numSteps = rampTime / DAC_TIME_RESOLUTION;
				double rampInc = (finalValue - initValue) / numSteps;
				if ((fabs(rampInc) < dacResolution) && !resolutionWarningPosted)
				{
					resolutionWarningPosted = true;
					thrower("Warning: numPoints of " + str(numSteps) + " results in a ramp increment of "
						+ str(rampInc) + " is below the resolution of the dacs (which is 20/2^16 = "
						+ str(dacResolution) + "). Ramp will not run.\r\n");
				}
				if (numSteps > DAC_RAMP_MAX_PTS) {
					thrower("Warning: numPoints of " + str(numSteps) + " is larger than the max time of the DAC ramps. Ramp will be truncated. \r\n");
				}

				double initTime = tempEvent.time;

				// for dacRamp, pass the ramp points and time directly to a single dacCommandList element
				tempEvent.value = initValue;
				tempEvent.endValue = finalValue;
				tempEvent.time = initTime;
				tempEvent.rampTime = rampTime;
				cmdList.push_back(tempEvent);
			}

			else {
				thrower("Unrecognized dac command name: " + formList.commandName);
			}
		}
	}
}






void AoCore::organizeDacCommands(unsigned variation, AoSnapshot initSnap)
{
	// each element of this is a different time (the double), and associated with each time is a vector which locates 
	// which commands were at this time, for
	// ease of retrieving all of the values in a moment.
	std::vector<std::pair<double, std::vector<AoCommand>>> timeOrganizer;
	std::vector<AoCommand> tempEvents(dacCommandList[variation]);
	// sort the events by time. using a lambda here.
	std::sort(tempEvents.begin(), tempEvents.end(),
		[](AoCommand a, AoCommand b) { return a.time < b.time; });
	for (unsigned commandInc : range(tempEvents.size()))
	{
		auto& command = tempEvents[commandInc];
		// because the events are sorted by time, the time organizer will already be sorted by time, and therefore I 
		// just need to check the back value's time. Check that the times are greater than a pico second apart. 
		// pretty arbitrary.
		if (commandInc == 0 || fabs(command.time - timeOrganizer.back().first) > 1e-9)
		{
			// new time
			std::vector<AoCommand> quickVec = { command };
			timeOrganizer.push_back({ command.time, quickVec });
		}
		else
		{
			// old time
			timeOrganizer.back().second.push_back(command);
		}
	}
	/// make the snapshots
	if (timeOrganizer.size() == 0)
	{
		// no commands, that's fine.
		return;
	}
	auto& snap = dacSnapshots[variation];
	snap.clear();

	//snap.push_back({ ZYNQ_DEADTIME,dacValuestmp });
	snap.push_back(initSnap);
	if (timeOrganizer[0].first != 0)
	{
		// then there were no commands at time 0, so just set the initial state to be exactly the original state before
		// the experiment started. I don't need to modify the first snapshot in this case, it's already set. Add a snapshot
		// here so that the thing modified is the second snapshot not the first. 
		snap.push_back(initSnap);
	}

	unsigned cnts = 0;
	for (auto& command : timeOrganizer)
	{
		if (cnts != 0)
		{
			// handle the zero case specially. This may or may not be the literal first snapshot.
			// first copy the last set so that things that weren't changed remain unchanged.
			snap.push_back(snap.back());
		}

		snap.back().time = command.first;
		for (auto& change : command.second)
		{
			// see description of this command above... update everything that changed at this time.
			snap.back().dacValues[change.line] = change.value;
			snap.back().dacEndValues[change.line] = change.endValue;
			snap.back().dacRampTimes[change.line] = change.rampTime;
		}
		cnts++;
	}
}

void AoCore::findLoadSkipSnapshots(double time, std::vector<parameterType>& variables, unsigned variation)
{
	// find the splitting time and set the loadSkip snapshots to have everything after that time.
	auto& snaps = dacSnapshots[variation];
	auto& loadSkipSnaps = loadSkipDacSnapshots[variation];
	if (snaps.size() == 0)
	{
		return;
	}
	for (auto snapshotInc : range(snaps.size() - 1))
	{
		if (snaps[snapshotInc].time < time && snaps[snapshotInc + 1].time >= time)
		{
			loadSkipSnaps = std::vector<AoSnapshot>(snaps.begin() + snapshotInc + 1, snaps.end());
			break;
		}
	}
}


void AoCore::formatDacForFPGA(UINT variation, AoSnapshot initSnap)
{
	std::array<double, size_t(AOGrid::total)> dacValuestmp = initSnap.dacValues;
	for (int i = 0; i < dacSnapshots[variation].size(); ++i)
	{
		AoSnapshot snapshotPrev;
		AoSnapshot snapshot;
		AoChannelSnapshot channelSnapshot;
		std::vector<int> channels;

		snapshot = dacSnapshots[variation][i];

		if (i == 0)
		{
			for (int j = 0; j < size_t(AOGrid::total); ++j)
			{
				if (snapshot.dacValues[j] != dacValuestmp[j] ||
					(snapshot.dacValues[j] == dacValuestmp[j] && snapshot.dacRampTimes[j] != 0)) {
					channels.push_back(j);
				}
			}
		}
		else {
			snapshotPrev = dacSnapshots[variation][i - 1];
			for (int j = 0; j < size_t(AOGrid::total); ++j)
			{
				if (snapshot.dacValues[j] != snapshotPrev.dacValues[j] ||
					snapshot.dacValues[j] != snapshotPrev.dacEndValues[j] ||
					(snapshot.dacValues[j] == snapshotPrev.dacValues[j] &&
						snapshot.dacRampTimes[j] != 0 && snapshotPrev.dacRampTimes[j] == 0))
				{
					channels.push_back(j);
				}
			}
		}

		//for each channel with a changed voltage add a dacSnapshot to the final list
		for (int channel : channels) {
			channelSnapshot.time = snapshot.time;
			channelSnapshot.channel = channel;
			channelSnapshot.dacValue = snapshot.dacValues[channel];
			channelSnapshot.dacEndValue = snapshot.dacEndValues[channel];
			channelSnapshot.dacRampTime = snapshot.dacRampTimes[channel];
			finalDacSnapshots[variation].push_back(channelSnapshot);
		}
	}
}

void AoCore::writeDacs(unsigned variation, bool loadSkip) 
{
	if (getNumberEvents(variation) != 0) {
		int tcp_connect;
		try
		{
			tcp_connect = zynq_tcp.connectTCP(ZYNQ_ADDRESS);
		}
		catch (ChimeraError& err)
		{
			tcp_connect = 1;
			thrower(err.what());
		}

		if (tcp_connect == 0)
		{
			zynq_tcp.writeDACs(finalDacSnapshots[variation]);
			zynq_tcp.disconnect();
		}
		else
		{
			thrower("connection to zynq failed. can't write DAC data\n");
		}
	}
}

void AoCore::makeFinalDataFormat(unsigned variation) 
{
	/*auto& finalNormal = finalFormatDacData[variation];
	auto& finalLoadSkip = loadSkipDacFinalFormat[variation];
	auto& normSnapshots = dacSnapshots[variation];
	auto& loadSkipSnapshots = loadSkipDacSnapshots[variation];

	for (auto& data : finalNormal) {
		data.clear();
	}
	for (auto& data : finalLoadSkip) {
		data.clear();
	}
	for (AoSnapshot snapshot : normSnapshots) {
		for (auto dacInc : range(size_t(AOGrid::total))) {
			finalNormal[dacInc / size_t(AOGrid::numPERunit)].push_back(snapshot.dacValues[dacInc]);
		}
	}
	for (AoSnapshot snapshot : loadSkipSnapshots) {
		for (auto dacInc : range(size_t(AOGrid::total))) {
			finalLoadSkip[dacInc / size_t(AOGrid::numPERunit)].push_back(snapshot.dacValues[dacInc]);
		}
	}*/

}

void AoCore::handleDacScriptCommand(AoCommandForm command, std::string name, std::vector<parameterType>& vars)
{
	if (command.commandName != "dac:" &&
		command.commandName != "dacarange:" &&
		command.commandName != "daclinspace:" &&
		command.commandName != "dacramp:")
	{
		thrower("dac commandName not recognized!");
	}
	if (!isValidDACName(name))
	{
		thrower("the name " + name + " is not the name of a dac!");
	}
	// convert name to corresponding dac line.
	command.line = getDacIdentifier(name);
	if (command.line == -1) {
		thrower("the name " + name + " is not the name of a dac!");
	}
	setDacCommandForm(command);
}

bool AoCore::isValidDACName(std::string name) 
{
	for (auto dacInc : range(size_t(AOGrid::total))) 
	{
		if (name == "dac" + str(dacInc)) {
			return true;
		}
		else if (getDacIdentifier(name) != -1) {
			return true;
		}
	}
	return false;
}

int AoCore::getDacIdentifier(std::string name) 
{
	for (auto dacInc : range(size_t(AOGrid::total)))
	{
		auto& dacName = str(names[dacInc]/*outputs[dacInc].info.name*/, 13, false, true);
		// check names set by user and check standard names which are always acceptable
		if (name == dacName || name == "dac" +
			str(dacInc / size_t(AOGrid::numPERunit)) + "_" +
			str(dacInc % size_t(AOGrid::numPERunit))/*"dac" + str ( dacInc )*/) {
			return dacInc;
		}
	}
	// not an identifier.
	return -1;
}

int AoCore::getBasicDacIdentifier(std::string name)
{
	for (auto dacInc : range(size_t(AOGrid::total)))
	{
		// check names set by user and check standard names which are always acceptable
		if (name == "dac" +
			str(dacInc / size_t(AOGrid::numPERunit)) + "_" +
			str(dacInc % size_t(AOGrid::numPERunit))/*"dac" + str ( dacInc )*/) {
			return dacInc;
		}
	}
	// not an identifier.
	return -1;
}





void AoCore::checkTimingsWork(unsigned variation) 
{
	std::vector<double> times;
	// grab all the times.
	for (auto& snapshot : dacSnapshots[variation]) {
		times.push_back(snapshot.time);
	}

	int count = 0;
	for (auto time : times) {
		int countInner = 0;
		for (auto secondTime : times) {
			// don't check against itself.
			if (count == countInner) {
				countInner++;
				continue;
			}
			// can't trigger faster than the trigger time.
			if (fabs(time - secondTime) < dacTriggerTime) {
				thrower("timings are such that the dac system would have to get triggered too fast to follow the"
					" programming! ");
			}
			countInner++;
		}
		count++;
	}
}

void AoCore::checkValuesAgainstLimits(unsigned variation, const std::array<AnalogOutput, size_t(AOGrid::total)>& outputs)
{
	for (auto line : range(size_t(AOGrid::total))) {
		for (auto snapshot : dacSnapshots[variation]) 
		{
			if (snapshot.dacValues[line] > outputs[line].info.maxVal || snapshot.dacValues[line] < outputs[line].info.minVal) 
			{
				thrower("Attempted to set Dac" + str(line) + " value outside min/max range for this line. The "
					"value was " + str(snapshot.dacValues[line]) + ", while the minimum accepted value is " +
					str(outputs[line].info.minVal) + " and the maximum value is " + str(outputs[line].info.maxVal) + ". "
					"Change the min/max if you actually need to set this value.\r\n");
			}
		}
	}
}
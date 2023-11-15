#include "stdafx.h"
#include "AoCore.h"
#include "ExperimentThread/ExpThreadWorker.h"
#include "GeneralObjects/CodeTimer.h"
#include <ExperimentMonitoringAndStatus/ExperimentSeqPlotter.h>


AoCore::AoCore() : dacTriggerTime(DAC_TIME_RESOLUTION)
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

std::vector<AoCommand> AoCore::getDacCommand(unsigned variation)
{
	if (dacCommandList.size() - 1 < variation) {
		thrower("Aquiring DAC command out of range");
	}
	return dacCommandList[variation];
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
	sizeDataStructures(cmdNum);
}

void AoCore::sizeDataStructures(unsigned variations)
{	
	// imporantly, this sizes the relevant structures.
	dacCommandList.clear();
	dacSnapshots.clear();
	loadSkipDacSnapshots.clear();
	finalDacSnapshots.clear();

	dacCommandList.resize(variations);
	dacSnapshots.resize(variations);
	loadSkipDacSnapshots.resize(variations);
	finalDacSnapshots.resize(variations);
}


void AoCore::resetDacEvents()
{
	initializeDataObjects(0);
}

void AoCore::prepareForce()
{
	// purposefully preserve dacCommandFormList, for inExpCal
	sizeDataStructures(1);
}


void AoCore::calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker,
	std::vector<calSettings>& calibrationSettings)
{
	std::vector<calResult> calibrations;
	for (auto& calset : calibrationSettings) {
		calibrations.push_back(calset.result);
	}

	CodeTimer sTimer;
	sTimer.tick("Ao-Sys-Interpret-Start");
	unsigned variations = params.size() == 0 ? 1 : params.front().keyValues.size();
	if (variations == 0) {
		variations = 1;
	}
	sizeDataStructures(variations);

	bool resolutionWarningPosted = false;
	bool nonIntegerWarningPosted = false;
	sTimer.tick("After-init");

	for (auto variationInc : range(variations)) {
		if (variationInc == 0) {
			sTimer.tick("Variation-" + str(variationInc) + "-Start");
		}
		auto& cmdList = dacCommandList[variationInc];
		for (auto eventInc : range(dacCommandFormList.size())) {
			if (variationInc == 0) {
				// clear calibration usage for each command in the first var, for later check the proper usage of calibration
				for (auto calIdx : range(calibrationSettings.size())) {
					calibrations[calIdx].currentActive = false;
				}
			}
			
			AoCommand tempEvent;
			auto& formList = dacCommandFormList[eventInc];
			tempEvent.line = formList.line;
			tempEvent.repeatId = formList.repeatId; // this set the repeatId for all steps below
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
				if (rampInc < 10.0 / pow(2, 16) && !resolutionWarningPosted) {
					resolutionWarningPosted = true;
					emit threadworker->warn(cstr("Warning: ramp increment of " + str(rampInc) + " in dac command number "
						+ str(eventInc) + " is below the resolution of the aoSys (which is 10/2^16 = "
						+ str(10.0 / pow(2, 16)) + "). These ramp points are unnecessary.\r\n"));
				}
				// This might be the first not i++ usage of a for loop I've ever done... XD
				// calculate the time increment:
				int steps = int(fabs(finalValue - initValue) / rampInc + 0.5);
				double stepsFloat = fabs(finalValue - initValue) / rampInc;
				double diff = fabs(steps - stepsFloat);
				if (diff > 100 * DBL_EPSILON && !nonIntegerWarningPosted) {
					nonIntegerWarningPosted = true;
					emit threadworker->warn(cstr("Warning: Ideally your spacings for a dacArange would result in a non-integer number "
						"of steps. The code will attempt to compensate by making a last step to the final value which"
						" is not the same increment in voltage or time as the other steps to take the dac to the final"
						" value at the right time.\r\n"));
				}
				double timeInc = rampTime / steps;
				double initTime = tempEvent.time;
				double currentTime = tempEvent.time;
				if (timeInc < DAC_TIME_RESOLUTION) {
					thrower("Warning: numPoints of " + str(steps) + " results in a ramp time steps of "
						+ str(timeInc) + " is below the time resolution of the aoSys (which is 20us)."
						" You probably want to use dacramp instead of dacarange\r\n");
				}
				// handle the two directions seperately.
				if (initValue < finalValue) {
					for (double dacValue = initValue;
						(dacValue - finalValue) < -rampInc/2 + DBL_EPSILON; dacValue += rampInc)
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
						dacValue - finalValue > rampInc/2 - DBL_EPSILON; dacValue -= rampInc) {
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
				if ( (fabs( rampInc ) < 10.0 / pow( 2, 16 )) && !resolutionWarningPosted ){
					resolutionWarningPosted = true;
					emit threadworker->warn (cstr ("Warning: numPoints of " + str (numSteps) + " results in a ramp increment of "
						+ str (rampInc) + " is below the resolution of the aoSys (which is 10/2^16 = "
						+ str (10.0 / pow (2, 16)) + ").These linspace points are unnecessary\r\n"));
				}
				// This might be the first not i++ usage of a for loop I've ever done... XD
				// calculate the time increment:
				double timeInc = rampTime / numSteps;
				double initTime = tempEvent.time;
				double currentTime = tempEvent.time;
				double val = initValue;
				if (timeInc < DAC_TIME_RESOLUTION) {
					thrower("Warning: numPoints of " + str(numSteps) + " results in a ramp time steps of "
						+ str(timeInc) + " is below the time resolution of the aoSys (which is 20us)."
						" You probably want to use dacramp instead of daclinspace\r\n");
				}

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
				if ((fabs(rampInc) < dacResolution))
				{
					emit threadworker->warn(qstr("Warning: numPoints of " + str(numSteps) + " results in a ramp increment of "
						+ str(rampInc) + " is below the resolution of the dacs (which is 20/2^16 = "
						+ str(dacResolution) + "). \r\n"));
				}
				//if (numSteps > DAC_RAMP_MAX_PTS) {
				//	thrower("Warning: numPoints of " + str(numSteps) + " is larger than the max time of the DAC ramps."
				//		" Ramp will not run. \r\n");
				//}
				unsigned numStepsInt = unsigned(numSteps + 0.5);
				if (fabs(numSteps - numStepsInt) > 100 * DBL_EPSILON) {
					thrower("Warning: numPoints of " + str(numSteps) + "resulting from ramping time = " + str(rampTime) +
						"ms, and dac timing resolution 20us, is not an interger. "
						"Ramp will not run. \r\n");
				}
				if (rampTime < DAC_TIME_RESOLUTION) {
					thrower("Warning: Ramp time of "
						+ str(rampTime) + " is below the time resolution of the aoSys (which is 20us)."
						" Ramp will not run. \r\n");
				}
				

				double initTime = tempEvent.time;

				// for dacRamp, need to check whether the end point can be reached without rounding error and then
				// pass the ramp points and time directly to a single or two dacCommandList element
				long long int codeInit = long long int((initValue / 20 + 0.5) * 65535); // ((dacval+10)/20*65535), [-10,10]->[0,65535], 65536 pts and 65535 intervals
				long long int codeFinl = long long int((finalValue / 20 + 0.5) * 65535);
				long long int incr = ((codeFinl << 16) - (codeInit << 16)) / numStepsInt; // https://stackoverflow.com/questions/7221409/is-unsigned-integer-subtraction-defined-behavior The result of a subtraction generating a negative number in an unsigned type is well-defined: //[...] A computation involving unsigned operands can never overflow, because a result that cannot be represented by the resulting unsigned integer type is reduced modulo the number that is one greater than the largest value that can be represented by the resulting type. (ISO / IEC 9899:1999 (E)§6.2.5 / 9) //As you can see, (unsigned)0 - (unsigned)1 equals - 1 modulo UINT_MAX + 1, or in other words, UINT_MAX.
				long long int res = ((codeFinl << 16) - (codeInit << 16)) % numStepsInt; // https://stackoverflow.com/questions/7594508/modulo-operator-with-negative-values, (-7/3) => -2;-2 * 3 = > -6;so a % b = > -1; (7 / -3) = > -2;- 2 * -3 = > 6;so a % b = > 1
				if (res == 0) { 
					if (incr == 0) { // so the starting point and ending point of the ramp is the same, will ignore the ramp
						tempEvent.value = initValue;
						tempEvent.endValue = finalValue;
						tempEvent.time = initTime;
						tempEvent.rampTime = 0;
						cmdList.push_back(tempEvent);
					}
					else { // no rounding error, just push back
						tempEvent.value = initValue;
						tempEvent.endValue = finalValue;
						tempEvent.time = initTime;
						tempEvent.rampTime = rampTime;
						cmdList.push_back(tempEvent);
					}
				}
				else { // handle the last ramp specifically
					tempEvent.value = initValue;
					tempEvent.endValue = ((codeFinl-(incr/0x10000)) / 65535.0) * 20.0 - 10.0;
					tempEvent.time = initTime;
					tempEvent.rampTime = rampTime - DAC_TIME_RESOLUTION;
					cmdList.push_back(tempEvent);

					tempEvent.value = tempEvent.endValue;
					tempEvent.endValue = finalValue;
					tempEvent.time += tempEvent.rampTime;
					tempEvent.rampTime = DAC_TIME_RESOLUTION;
					cmdList.push_back(tempEvent);

					tempEvent.value = finalValue;
					tempEvent.endValue = finalValue; // the purpose is to keep the value at endValue
					tempEvent.time += tempEvent.rampTime;
					tempEvent.rampTime = 0;
					cmdList.push_back(tempEvent);
				}

			}

			else {
				thrower("Unrecognized dac command name: " + formList.commandName);
			}

			if (variationInc == 0) {
				// check if the calibration usage is proper only for the first var
				for (auto calIdx : range(calibrationSettings.size())) {
					if (calibrations[calIdx].currentActive && calibrationSettings[calIdx].aoControlChannel != formList.line) {
						calibrationSettings[calIdx].usedSameChannel = false;
						emit threadworker->warn("Used calibration " + qstr(calibrations[calIdx].calibrationName) + " whose AoControlChannel is " + 
							qstr(calibrationSettings[calIdx].aoControlChannel) + ", but the calibration is used with dac channel " +
							qstr(formList.line) + ", please make sure the calibraition is used properly.\r\n", 0);
					}
				}
			}
		}
		if (variationInc == 0) {
			// check if the calibration usage is proper only for the first var
			for (auto calIdx : range(calibrationSettings.size())) {
				if (calibrations[calIdx].active && !calibrationSettings[calIdx].active) {
					thrower("Used calibration " + calibrations[calIdx].calibrationName + ", but it is not activated, please activate it and make sure the calibraition is up-to-date!\r\n", 1);
				}
			}
		}
	}

	for (size_t idx = 0; idx < calibrationSettings.size(); idx++) {
		calibrationSettings[idx].result = calibrations[idx];
	}
}


void AoCore::constructRepeats(repeatManager& repeatMgr)
{
	typedef AoCommand Command;
	/* this is to be done after ttlCommandList is filled with all variations. */
	if (dacCommandFormList.size() == 0 || dacCommandList.size() == 0) {
		thrower("No DAC Commands???");
	}

	unsigned variations = dacCommandList.size();
	repeatMgr.saveCalculationResults(); // repeatAddedTime is changed during construction, need to save and reset it before and after the construction body
	auto* repearRoot = repeatMgr.getRepeatRoot();
	auto allDescendant = repearRoot->getAllDescendant();
	if (allDescendant.empty()) {
		return; // no repeats need to handle.
	}


	// iterate through all variations
	for (auto varInc : range(variations)) {
		auto& cmds = dacCommandList[varInc];
		/*lines below should be all the same as the DoCore*/

		// Recursively add these repeat for always starting with maxDepth repeat. And also update the already constructed one to its parent layer
		// The loop will end when all commands is not associated with repeat, i.e. the maxDepth command's repeatId.repeatTreeMap is root
		while (true) {
			/*find the max depth repeated command*/
			auto maxDepthIter = std::max_element(cmds.begin(), cmds.end(), [&](const Command& a, const Command& b) {
				return (a.repeatId.repeatTreeMap.first < b.repeatId.repeatTreeMap.first); });
			Command maxDepth = *maxDepthIter;

			/*check if all command is with zero repeat. If so, exit the loop*/
			if (maxDepth.repeatId.repeatTreeMap == repeatInfoId::root) {
				break;
			}

			/*find the repeat num and the repeat added time with the unique identifier*/
			auto repeatIIter = std::find_if(allDescendant.begin(), allDescendant.end(), [&](TreeItem<repeatInfo>* a) {
				return (maxDepth.repeatId.repeatIdentifier == a->data().identifier); });
			if (repeatIIter == allDescendant.end()) {
				thrower("Can not find the ID for the repeat in the DoCommand with max depth of the tree. This is a low level bug.");
			}
			TreeItem<repeatInfo>* repeatI = *repeatIIter;
			unsigned repeatNum = repeatI->data().repeatNums[varInc];
			double repeatAddedTime = repeatI->data().repeatAddedTimes[varInc];
			/*find the parent of this repeat and record its repeatInfoId for updating the repeated ones*/
			TreeItem<repeatInfo>* repeatIParent = repeatI->parentItem();
			repeatInfoId repeatIdParent{ repeatIParent->data().identifier, repeatIParent->itemID() };
			/*collect command that need to be repeated*/
			std::vector<Command> cmdToRepeat;
			std::copy_if(cmds.begin(), cmds.end(), std::back_inserter(cmdToRepeat), [&](Command doc) {
				return (doc.repeatId.repeatIdentifier == maxDepth.repeatId.repeatIdentifier); });
			/*check if the repeated command is continuous in the cmds vector, it should be as the cmds is representing the script's order at this stage*/
			auto cmdToRepeatStart = std::search(cmds.begin(), cmds.end(), cmdToRepeat.begin(), cmdToRepeat.end(),
				[&](const Command& a, const Command& b) {
					return (a.repeatId.repeatIdentifier == b.repeatId.repeatIdentifier);
				});
			if (cmdToRepeatStart == cmds.end()) {
				thrower("The repeated command is not contiguous inside the CommandList, which is not suppose to happen.");
			}
			int cmdToRepeatStartPos = std::distance(cmds.begin(), cmdToRepeatStart);
			auto cmdToRepeatEnd = cmdToRepeatStart + cmdToRepeat.size(); // this will point to first cmd that is after those repeated one in CommandList
			/*if repeatNum is zero, delete the repeated command and reduce the time for those comand comes after the repeated commands and that is all*/
			if (repeatNum == 0) {
				cmds.erase(std::remove_if(cmds.begin(), cmds.end(), [&](Command doc) {
					return (doc.repeatId.repeatIdentifier == maxDepth.repeatId.repeatIdentifier); }), cmds.end());
				/*de-advance the time of thoses command that is later in CommandList than the repeat block*/
				cmdToRepeatEnd = cmds.begin() + cmdToRepeatStartPos; // this will point to first cmd that is after those repeated one in CommandList, since the repeated is removed, should equal to cmdToRepeatStart
				std::for_each(cmdToRepeatEnd, cmds.end(), [&](Command& doc) {
					doc.time -= repeatAddedTime; });
				continue;
			}
			/*transform the repeating commandlist to its parent repeatInfoId so that it can be repeated in its parents level*/
			// could also use this: std::transform(cmds.cbegin(), cmds.cend(), cmds.begin(), [&](Command doc) { with a return
			std::for_each(cmds.begin(), cmds.end(), [&](Command& doc) {
				if (doc.repeatId.repeatIdentifier == maxDepth.repeatId.repeatIdentifier) {
					if (doc.repeatId.placeholder) {
						doc.repeatId = repeatIdParent;
						doc.repeatId.placeholder = true; // doesn't really matter, since this will be deleted anyway.
					}
					else { doc.repeatId = repeatIdParent; }
				} });
			/*determine whether this cmdToRepeat is just a placeholder*/
			bool noPlaceholder = std::none_of(cmdToRepeat.begin(), cmdToRepeat.end(), [&](Command doc) {
				return doc.repeatId.placeholder; });
			int insertedCmdSize = 0;
			if (noPlaceholder) {
				/*start to insert the repeated 'cmdToRpeat' to end of the repeat block, after insertion, 'cmdToRepeatEnd' can not be used*/
				std::vector<Command> cmdToInsert;
				cmdToInsert.clear();
				for (unsigned repeatInc : range(repeatNum - 1)) {
					// if only repeat for once, below will be ignored, since the first repeat is already in the list
					/*transform the repeating commandlist to its parent repeatInfoId and also increment its time so that it can be repeated in its parents level*/
					std::for_each(cmdToRepeat.begin(), cmdToRepeat.end(), [&](Command& doc) {
						doc.repeatId = repeatIdParent;
						doc.time += repeatAddedTime; });
					cmdToInsert.insert(cmdToInsert.end(), cmdToRepeat.begin(), cmdToRepeat.end());
				}
				cmds.insert(cmdToRepeatEnd, cmdToInsert.begin(), cmdToInsert.end());
				insertedCmdSize = cmdToInsert.size();

			}
			else {
				/*with placeholder, check if this is the only placeholder, as it should be in most cases*/
				if (cmdToRepeat.size() > 1) {
					bool allPlaceholder = std::all_of(cmdToRepeat.begin(), cmdToRepeat.end(), [&](Command doc) {
						return doc.repeatId.placeholder; });
					if (allPlaceholder) {
						thrower("The command-to-repeat contains more than one placeholder. This shouldn't happen "
							"as Chimera will only insert one placeholder if there is no command in this repeat. "
							"This shouldn't come from nested repeat either, since Chimera will add placeholder for each level of repeats "
							"if the there is no command. And the inferior level repeat placeholder should already be deleted after previous loop"
							"A low level bug.");
					}
					else {
						thrower("The command-to-repeat contains placeholder but also other commands that is not meant for placeholding. "
							"This shouldn't happen as Chimera will only insert placeholder if there is no command in this repeat. "
							"This shouldn't come from nested repeat either, inferior level repeat placeholder should already be deleted after previous loop. "
							"A low level bug.");
					}
				}
				else {
					/*remove the placeholder for this level of repeat. Other level would have their own placeholder inserted already if needed.*/
					cmds.erase(cmds.begin() + cmdToRepeatStartPos);
					insertedCmdSize = -1; // same as '-cmdToRepeat.size()'
				}
			}
			/*advance the time of thoses command that is later in CommandList than the repeat block*/
			cmdToRepeatEnd = cmds.begin() + cmdToRepeatStartPos + cmdToRepeat.size() + insertedCmdSize;
			std::for_each(cmdToRepeatEnd, cmds.end(), [&](Command& doc) {
				doc.time += repeatAddedTime * (repeatNum - 1); });
			/*advance the time of the parent repeat, if the parent is not root*/
			if (repeatIParent != repearRoot) {
				repeatIParent->data().repeatAddedTimes[varInc] += repeatAddedTime * repeatNum;
			}
		}
	}
	repeatMgr.loadCalculationResults();
}

bool AoCore::repeatsExistInCommandForm(repeatInfoId repeatId)
{
	typedef AoCommandForm CommandForm;
	auto& cmdFormList = dacCommandFormList;
	auto cmdFormRepeated = std::find_if(cmdFormList.begin(), cmdFormList.end(),
		[&](const CommandForm& a) {
			return (a.repeatId.repeatIdentifier == repeatId.repeatIdentifier);
		});
	return (cmdFormRepeated != cmdFormList.end());
}

void AoCore::addPlaceholderRepeatCommand(repeatInfoId repeatId)
{
	typedef AoCommandForm CommandForm;
	auto& cmdFormList = dacCommandFormList;
	repeatId.placeholder = true;
	CommandForm cmdForm;
	cmdForm.commandName = "dac:";
	cmdForm.finalVal = Expression("0.0");
	cmdForm.repeatId = repeatId;
	cmdFormList.push_back(cmdForm); // should be an alert for all other commands, and it will be cleansed after advancing the time
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
		if (commandInc == 0 || fabs(command.time - timeOrganizer.back().first) > 1e-9) {
			// new time
			std::vector<AoCommand> quickVec = { command };
			timeOrganizer.push_back({ command.time, quickVec });
		}
		else {
			// old time
			timeOrganizer.back().second.push_back(command);
		}
	}
	/// make the snapshots
	if (timeOrganizer.size() == 0) {
		// no commands, that's fine.
		return;
	}
	auto& snap = dacSnapshots[variation];
	snap.clear();

	//snap.push_back({ ZYNQ_DEADTIME,dacValuestmp });
	snap.push_back(initSnap);
	if (timeOrganizer[0].first != 0) {
		// then there were no commands at time 0, so just set the initial state to be exactly the original state before
		// the experiment started. I don't need to modify the first snapshot in this case, it's already set. Add a snapshot
		// here so that the thing modified is the second snapshot not the first. 
		snap.push_back(initSnap);
	}

	unsigned cnts = 0;
	for (auto& command : timeOrganizer)
	{
		if (cnts != 0) {
			// handle the zero case specially. This may or may not be the literal first snapshot.
			// first copy the last set so that things that weren't changed remain unchanged.
			snap.push_back(snap.back());
		}

		snap.back().time = command.first;
		for (auto& change : command.second) {
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
	typedef unsigned long long l64;
	const l64 timeConv = 100000; // DIO time given in multiples of 10 ns, this is different from the ramp time, which uses timeConvDAC
	const l64 rewindTime = l64(1) << 32; // 0xFFFFFFFF + 1, correspond to 32 bit time 
	int durCounter = l64(std::llround(dacSnapshots[variation][0].time * timeConv)) / rewindTime;
	if (durCounter > 0) {// the first time stamp is larger than a rewind, shouldn't happen after organizeDAC, otherwise it is impossible to know the dac state before exp
		thrower("The DAC start time is beyond 42.9s and is started instead at " + str(dacSnapshots[variation][0].time) + ". Make sure you input a DAC command in the experiment.");
	}
	//std::array<double, size_t(AOGrid::total)> dacValuestmp = initSnap.dacValues;
	for (int i = 0; i < dacSnapshots[variation].size(); ++i)
	{
		AoSnapshot snapshotPrev;
		AoSnapshot snapshot;
		AoChannelSnapshot channelSnapshot;
		std::vector<int> channels;
		snapshot = dacSnapshots[variation][i];

		if (i == 0) {
			for (int j = 0; j < size_t(AOGrid::total); ++j)
			{
				channels.push_back(j);
			}
		}
		else {
			snapshotPrev = dacSnapshots[variation][i - 1];
			for (int j = 0; j < size_t(AOGrid::total); ++j)
			{
				if (snapshot.dacValues[j] != snapshotPrev.dacValues[j] || /*for 'dac:' command, dacValue=dacEndValue */
					snapshot.dacEndValues[j] != snapshotPrev.dacEndValues[j] || /*most of logic should be determined upto this line, e.g. const-> ramp, first line could be false but sec line will be true*/
					(snapshot.dacValues[j] == snapshotPrev.dacEndValues[j] &&
						snapshot.dacRampTimes[j] != 0 && snapshotPrev.dacRampTimes[j] == 0) /*start to ramp*/ ||
					(snapshot.dacValues[j] == snapshotPrev.dacEndValues[j] && 
						snapshot.dacRampTimes[j] == 0 && snapshotPrev.dacRampTimes[j] != 0 /*end ramp*/))
				{
					channels.push_back(j);
				}
			}
		}

		//for each channel with a changed voltage add a dacSnapshot to the final list
		for (int channel : channels) {
			while (l64(std::llround(snapshot.time * timeConv)) / rewindTime > durCounter) {
				durCounter++;
				unsigned int windTime = (l64(durCounter) * rewindTime - 3000) & l64(0xffffffff); // 3000*10ns=30us away from rewind to avoid the ramp time round up in sequencer.py
				finalDacSnapshots[variation].push_back({ DAC_REWIND[0], static_cast<double>(windTime) / timeConv, static_cast<double>(durCounter % 5),static_cast<double>(durCounter % 5),0.0 });
				finalDacSnapshots[variation].push_back({ DAC_REWIND[1], static_cast<double>(windTime) / timeConv, static_cast<double>(durCounter % 5),static_cast<double>(durCounter % 5),0.0 });
			}

			channelSnapshot.time = snapshot.time;
			channelSnapshot.channel = channel;
			channelSnapshot.dacValue = snapshot.dacValues[channel];
			channelSnapshot.dacEndValue = snapshot.dacEndValues[channel];
			channelSnapshot.dacRampTime = snapshot.dacRampTimes[channel];
			finalDacSnapshots[variation].push_back(channelSnapshot);
		}
	}
	if (finalDacSnapshots[variation].size() > maxCommandNum) {
		thrower("AO command number " + str(finalDacSnapshots[variation].size()) + " is greater than the maximum AO command that the"
			"microcontroller can accept, which is " + str(maxCommandNum));
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

// channelSnapShot[0] contains changes that need to make for dac channels, do no call this during experiment interpretation.
void AoCore::setGUIDacChange(std::vector<std::vector<AoChannelSnapshot>> channelSnapShot)
{
	prepareForce();
	dacSnapshots.resize(1); // just to make getNumberEvents happy, used in AoCore::writeDacs()
	dacSnapshots[0].resize(channelSnapShot.size());
	finalDacSnapshots = channelSnapShot;
	try {
		writeDacs(0, true);
	}
	catch (ChimeraError& e) {
		thrower("GUI sending data to DAC failed: \r\n" + e.trace());
	}
	
	//int tcp_connect;
	//try {
	//	tcp_connect = zynq_tcp.connectTCP(ZYNQ_ADDRESS);
	//}
	//catch (ChimeraError& err) {
	//	tcp_connect = 1;
	//	thrower(err.what());
	//}
	//Sleep(100);
	//if (tcp_connect == 0) {
	//	zynq_tcp.writeCommand("trigger");
	//	zynq_tcp.disconnect();
	//}
	//else {
	//	thrower("connection to zynq failed. can't write DAC data\n");
	//}
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
		auto dacName = str(names[dacInc]/*outputs[dacInc].info.name*/, 13, false, true);
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
			auto tt = (time - secondTime);
			auto tmp = abs(time - secondTime);
			auto tmp1 = dacTriggerTime - 1e6 * DBL_EPSILON;
			auto t = tmp < tmp1;
			if (fabs(time - secondTime) < dacTriggerTime  - 1e6 * DBL_EPSILON) {
				thrower("timings are such that the dac system would have to get triggered too fast to follow the"
					" programming! Where the dacTriggerTime is " + str(dacTriggerTime) + ". And the criterion is dacTriggerTime - 1e6* DBL_EPSILON = " + str(dacTriggerTime - 1e6 * DBL_EPSILON) + "\n"
					"For variation " + str(variation) + " between time " + str(secondTime) + " and time " + str(time) + ", whose time difference is " + str(fabs(time - secondTime)));
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


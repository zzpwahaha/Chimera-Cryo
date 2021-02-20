#include "stdafx.h"
#include "OlCore.h"

#include <qDebug>
#include <qelapsedtimer.h>

OlCore::OlCore(bool safemode)
	: qtFlume(safemode, "COM3")
{
	
}

OlCore::~OlCore()
{
	qtFlume.close();
}


bool OlCore::isValidOLName(std::string name)
{
	for (unsigned olInc = 0; olInc < size_t(OLGrid::total); olInc++)
	{
		if (getOLIdentifier(name) != -1) {
			return true;
		}
	}
	return false;
}

int OlCore::getOLIdentifier(std::string name)
{
	for (unsigned olInc = 0; olInc < size_t(OLGrid::total); olInc++)
	{
		// check names set by user.
		std::transform(names[olInc].begin(), names[olInc].end(), names[olInc].begin(), ::tolower);
		if (name == names[olInc])
		{
			return olInc;
		}
		// check standard names which are always acceptable.
		if (name == "ol" +
			str(olInc / size_t(OLGrid::numPERunit)) + "_" +
			str(olInc % size_t(OLGrid::numPERunit)))
		{
			return olInc;
		}
	}
	// not an identifier.
	return -1;
}

void OlCore::setNames(std::array<std::string, size_t(OLGrid::total)> namesIn)
{
	names = std::move(namesIn);
}

std::string OlCore::getName(int olNumber)
{
	return names[olNumber];
}

std::array<std::string, size_t(OLGrid::total)> OlCore::getName()
{
	return names;
}

/**********************************************************************************************/
void OlCore::logSettings(DataLogger& log, ExpThreadWorker* threadworker)
{
}

void OlCore::loadExpSettings(ConfigStream& stream)
{
}

void OlCore::normalFinish()
{

}

void OlCore::errorFinish()
{
}


void OlCore::resetOLEvents()
{
	olCommandFormList.clear();
	olCommandList.clear();
	olSnapshots.clear();
	olChannelSnapshots.clear();
}

void OlCore::initializeDataObjects(unsigned variationNum)
{
	olCommandFormList.clear();
	olCommandList.clear();
	olSnapshots.clear();
	olChannelSnapshots.clear();

	olCommandFormList.resize(variationNum);
	olCommandList.resize(variationNum);
	olSnapshots.resize(variationNum);
	olChannelSnapshots.resize(variationNum);
}

void OlCore::setOLCommandForm(OlCommandForm command)
{
	olCommandFormList.push_back(command);
	// you need to set up a corresponding trigger to tell the dacs to change the output at the correct time. 
	// This is done later on interpretation of ramps etc.
}

void OlCore::handleOLScriptCommand(OlCommandForm command, std::string name, std::vector<parameterType>& vars)
{
	if (command.commandName != "ol:" &&
		command.commandName != "ollinspace:" &&
		command.commandName != "olramp:")
	{
		thrower("ERROR: offsetlock commandName not recognized!");
	}
	if (!isValidOLName(name))
	{
		thrower("ERROR: the name " + name + " is not the name of a offsetlock!");
	}
	// convert name to corresponding dac line.
	command.line = getOLIdentifier(name);
	if (command.line == -1)
	{
		thrower("ERROR: the name " + name + " is not the name of a offsetlock!");
	}
	setOLCommandForm(command);
}


void OlCore::calculateVariations(std::vector<parameterType>& variables, ExpThreadWorker* threadworker)
{
	unsigned variations;
	variations = variables.front().keyValues.size();
	if (variations == 0)
	{
		variations = 1;
	}
	/// imporantly, this sizes the relevant structures.
	olCommandList = std::vector<std::vector<OlCommand>>(variations);
	olSnapshots = std::vector<std::vector<OlSnapshot>>(variations);
	olChannelSnapshots = std::vector<std::vector<OlChannelSnapshot>>(variations);

	bool resolutionWarningPosted = false;
	bool nonIntegerWarningPosted = false;

	for (unsigned variationInc = 0; variationInc < variations; variationInc++)
	{
		for (unsigned eventInc = 0; eventInc < olCommandFormList.size(); eventInc++)
		{
			OlCommand tempEvent;
			tempEvent.line = olCommandFormList[eventInc].line;
			// Deal with time.
			if (olCommandFormList[eventInc].time.first.size() == 0)
			{
				// no variable portion of the time.
				tempEvent.time = olCommandFormList[eventInc].time.second;
			}
			else
			{
				double varTime = 0;
				for (auto variableTimeString : olCommandFormList[eventInc].time.first)
				{
					varTime += variableTimeString.evaluate(variables, variationInc);
				}
				tempEvent.time = varTime + olCommandFormList[eventInc].time.second;
			}

			if (olCommandFormList[eventInc].commandName == "ol:")
			{
				/// single point.
				////////////////
				// deal with amp
				tempEvent.value = olCommandFormList[eventInc].initVal.evaluate(variables, variationInc);
				tempEvent.endValue = tempEvent.value;
				tempEvent.numSteps = 1;
				tempEvent.rampTime = OL_TIME_RESOLUTION;
				olCommandList[variationInc].push_back(tempEvent);
			}
			else if (olCommandFormList[eventInc].commandName == "ollinspace:")
			{
				// interpret ramp time command. I need to know whether it's ramping or not.
				double rampTime = olCommandFormList[eventInc].rampTime.evaluate(variables, variationInc);
				/// many points to be made.
				// convert initValue and finalValue to doubles to be used 
				double initValue, finalValue;
				int numSteps;
				initValue = olCommandFormList[eventInc].initVal.evaluate(variables, variationInc);
				// deal with final value;
				finalValue = olCommandFormList[eventInc].finalVal.evaluate(variables, variationInc);
				// deal with numPoints
				numSteps = olCommandFormList[eventInc].numSteps.evaluate(variables, variationInc);
				double rampInc = (finalValue - initValue) / double(numSteps);
				if ((fabs(rampInc) < olFreqResolution) && !resolutionWarningPosted)
				{
					resolutionWarningPosted = true;
					thrower("Warning: numPoints of " + str(numSteps) + " results in an amplitude ramp increment of "
						+ str(rampInc) + " is below the resolution of the offset lock (which is " + str(50) + "MHz/2^25 = "
						+ str(olFreqResolution) + "). \r\n");
				}
				double timeInc = rampTime / double(numSteps);
				double initTime = tempEvent.time;
				double currentTime = tempEvent.time;
				double val = initValue;

				for (auto stepNum : range(numSteps))
				{
					tempEvent.value = val;
					tempEvent.time = currentTime;
					tempEvent.endValue = val;
					tempEvent.rampTime = OL_TIME_RESOLUTION;
					tempEvent.numSteps = 1;
					olCommandList[variationInc].push_back(tempEvent);
					currentTime += timeInc;
					val += rampInc;
				}
				 //and get the final amp.
				tempEvent.value = finalValue;
				tempEvent.time = initTime + rampTime;
				tempEvent.endValue = finalValue;
				tempEvent.rampTime = OL_TIME_RESOLUTION;
				tempEvent.numSteps = 1;
				olCommandList[variationInc].push_back(tempEvent);
			}
			else if (olCommandFormList[eventInc].commandName == "olramp:")
			{
				double rampTime = olCommandFormList[eventInc].rampTime.evaluate(variables, variationInc);
				// convert initValue and finalValue to doubles to be used 
				double initValue, finalValue, numSteps;
				initValue = olCommandFormList[eventInc].initVal.evaluate(variables, variationInc);
				// deal with final value;
				finalValue = olCommandFormList[eventInc].finalVal.evaluate(variables, variationInc);
				// set votlage resolution to be maximum allowed by the ramp range and time
				numSteps = rampTime / OL_TIME_RESOLUTION;
				double rampInc = (finalValue - initValue) / numSteps;
				if ((fabs(rampInc) < olFreqResolution) && !resolutionWarningPosted)
				{
					resolutionWarningPosted = true;
					thrower("Warning: numPoints of " + str(numSteps) + " results in a ramp increment of "
						+ str(rampInc) + " is below the frequency resolution of the offsetlock (which is 50/2^25 = "
						+ str(olFreqResolution) + "). Ramp will not run.\r\n");
				}
				if (numSteps > 0xffffffff) {
					thrower("Warning: numPoints of " + str(numSteps) +
						" is larger than the max num of steps of the offsetlock ramps. Ramp will not run. \r\n");
				}

				double initTime = tempEvent.time;

				// for olramp, pass the ramp points and time directly to a single olCommandList element
				tempEvent.value = initValue;
				tempEvent.endValue = finalValue;
				tempEvent.time = initTime;
				tempEvent.rampTime = rampTime;
				tempEvent.numSteps = unsigned int(numSteps);
				olCommandList[variationInc].push_back(tempEvent);
			}
			else
			{
				thrower("ERROR: Unrecognized offsetlock command name: " + olCommandFormList[eventInc].commandName);
			}
		}
	}
}


void OlCore::organizeOLCommands(unsigned variation, std::string& warning)
{
	// timeOrganizer: each element of this is a different time (the double), and associated with each time is a vector which locates 
	// which commands were at this time, for ease of retrieving all of the values in a moment.
	
	std::vector<OlCommand> tempEvents(olCommandList[variation]);
	// sort the events by time. using a lambda here.
	std::sort(tempEvents.begin(), tempEvents.end(),
		[](OlCommand a, OlCommand b) {return a.time < b.time; });
	//timeOrganizer.clear();
	//timeOrganizer.push_back({ ZYNQ_DEADTIME, std::vector<OlCommand>{} });
	//for (unsigned short i = 0; i < size_t(OLGrid::total); i++) {
	//	timeOrganizer.back().second.push_back( OlCommand{ i,ZYNQ_DEADTIME,status[i],status[i],0,-1 } );
	//}


	for (unsigned short channel = 0; channel < size_t(OLGrid::total); channel++)
	{
		auto& timeOgzer = timeOrganizer[channel];
		timeOgzer.clear();
		//timeOgzer.push_back({ ZYNQ_DEADTIME, OlCommand{channel,ZYNQ_DEADTIME,status[channel],status[channel],1,OL_TIME_RESOLUTION} });

		for (unsigned commandInc = 0; commandInc < tempEvents.size(); commandInc++)
		{
			if (tempEvents[commandInc].line != channel) { continue; }
			// for each channel with a freq gap, bridge it by adding command at the end of last command
			const auto& tE = tempEvents[commandInc];
			if (timeOgzer.empty()) {
				timeOgzer.push_back({ tE.time, tE });
			}
			else {
				if (fabs(timeOgzer.back().second.endValue - tE.value) > 2 * DBL_EPSILON)
				{
					
					warning += "Warning from OffsetLock: output(" + OlCore::getName(tE.line)
						+ "), the frequency between two consecutive event point t=" + str(timeOgzer.back().first, 3)
						+ ", endfreq=" + str(timeOgzer.back().second.endValue, 3) + " and t=" + str(tE.time, 6) + ", startfreq="
						+ str(tE.value, 3) + " are not the same. Have bridged it with a command inserted at t="
						+ str(tE.time + OL_TIME_RESOLUTION, 3) + "\r\n";

					timeOgzer.push_back({ tE.time,
						OlCommand{channel,tE.time,timeOgzer.back().second.endValue,tE.value,1,OL_TIME_RESOLUTION} });
				}
				// because the events are sorted by time, the time organizer will already be sorted by time, and therefore I 
				// just need to check the back value's time.
				double timediff = (tE.time - timeOgzer.back().first); //had better be positive, otherwise indicating a bridge is inserted perviously
				if (timediff >= OL_TIME_RESOLUTION - 2 * DBL_EPSILON)
				{
					timeOgzer.push_back({ tE.time, tE });
				}
				else
				{
					if (timediff < -OL_TIME_RESOLUTION - 2 * DBL_EPSILON) {
						thrower("Error in timeOrganizer, the time seems not sorted. A low level bug.");
					}
					// time that cannot be resolved by offset lock, complain it if it is of the same line
					warning += "Warning from OffsetLock: output(" + OlCore::getName(tE.line)
						+ "), the time between two consecutive event point t=" + str(timeOgzer.back().first, 3) + " and t="
						+ str(tE.time, 3) + " is below the offset lock resolution: " + str(OL_TIME_RESOLUTION, 3)
						+ "ms. Have make the later time exact " + str(OL_TIME_RESOLUTION, 3) + "ms away\r\n";
					timeOgzer.push_back({ timeOgzer.back().first + OL_TIME_RESOLUTION, tE });
				}
			}

		}
	}
	//for (unsigned commandInc = 0; commandInc < tempEvents.size(); commandInc++)
	//{
	//	// because the events are sorted by time, the time organizer will already be sorted by time, and therefore I 
	//	// just need to check the back value's time.
	//	double timediff = fabs(tempEvents[commandInc].time - timeOrganizer.back().first);
	//	if (commandInc == 0 || timediff >= OL_TIME_RESOLUTION/*2 * DBL_EPSILON*/)
	//	{
	//		timeOrganizer.push_back({ tempEvents[commandInc].time,
	//								std::vector<OlCommand>({ tempEvents[commandInc] }) });
	//	}
	//	else if (timediff < OL_TIME_RESOLUTION)
	//	{
	//		// time that cannot be resolved by offset lock, complain it if it is of the same line
	//		bool exist = false;
	//		for (auto& cmd : timeOrganizer.back().second) {
	//			if (tempEvents[commandInc].line == cmd.line) {
	//				exist = true;
	//			}
	//		}
	//		if (exist) {
	//			timeOrganizer.push_back({ timeOrganizer.back().first + OL_TIME_RESOLUTION,
	//								std::vector<OlCommand>({ tempEvents[commandInc] }) });
	//			warning += "Warning from OffsetLock: output(" + OlCore::getName(tempEvents[commandInc].line)
	//				+ "), the time between two consecutive event point t=" + str(timeOrganizer.back().first, 3) + "and t="
	//				+ str(tempEvents[commandInc].time, 3) + " is below the offset lock resolution: " + str(OL_TIME_RESOLUTION, 3) 
	//				+ "ms. Have make the later time exact " + str(OL_TIME_RESOLUTION, 3) + "away";
	//		}
	//		else {
	//			// really is a new time for that line
	//			if (timediff < 2 * DBL_EPSILON) {
	//				// old time
	//				timeOrganizer.back().second.push_back(tempEvents[commandInc]);
	//			}
	//			else {
	//				//new time
	//				timeOrganizer.push_back({ tempEvents[commandInc].time,
	//								std::vector<OlCommand>({ tempEvents[commandInc] }) });
	//			}
	//		}
	//	}
	//}

	///// make the snapshots
	//if (timeOrganizer.size() == 0)
	//{
	//	// no commands, that's fine.
	//	return;
	//}


}

void OlCore::makeFinalDataFormat(unsigned variation, DoCore& doCore)
{
	OlChannelSnapshot channelSnapshot;
	//for each channel with a changed freq, bridge it by adding command at the end of last command
	for (unsigned short channel = 0; channel < size_t(OLGrid::total); channel++)
	{
		auto& timeOgzer = timeOrganizer[channel];
		for (unsigned commandInc = 0; commandInc < timeOgzer.size(); commandInc++)
		{
			auto& to = timeOgzer[commandInc];
			channelSnapshot.val = to.second.value;
			channelSnapshot.endVal = to.second.endValue;
			channelSnapshot.rampTime = to.second.rampTime;
			channelSnapshot.numSteps = to.second.numSteps;

			channelSnapshot.time = to.first;
			channelSnapshot.channel = to.second.line;

			olChannelSnapshots[variation].push_back(channelSnapshot);

			/*within timeOrganizer[i], the time are the same*/
			doCore.ttlPulseDirect(OL_TRIGGER_LINE[channel].first , OL_TRIGGER_LINE[channel].second, 
				channelSnapshot.time, OL_TRIGGER_TIME, variation);
		}
	}

}

//void OlCore::standardExperimentPrep(unsigned variation, DoCore& doCore, std::string& warning)
//{
//	organizeOLCommands(variation, warning);
//	makeFinalDataFormat(variation, doCore);
//}


void OlCore::writeOLs(unsigned variation)
{
	qtFlume.getPort().clear();
	//unsigned channel, steps;
	//double time, start, stop, ramptime;
	std::string buffCmd;
	for (auto& channelSnap : olChannelSnapshots[variation])
	{
		buffCmd += "(" + str(channelSnap.channel) + "," + str(channelSnap.val, numFreqDigits) + ","
			+ str(channelSnap.endVal, numFreqDigits) + "," + str(channelSnap.numSteps) + ","
			+ str(channelSnap.rampTime, numTimeDigits) + ")";
	}
	buffCmd += "e";
	QElapsedTimer timer;
	timer.start();
	std::string recv = qtFlume.query(buffCmd);
	qDebug() << tmp << qstr(buffCmd) << "Total time:" << timer.elapsed()/**/ <<"ms";
	if (recv.empty()) {
		thrower("Nothing feeded back from Teensy after writing, something might be wrong with it." + recv);
		qDebug() << tmp << qstr("Empty return");
	}
	else {
		qDebug() << tmp << qstr(recv);
		std::transform(recv.begin(), recv.end(), recv.begin(), ::tolower); /*:: without namespace select from global namespce, see https://stackoverflow.com/questions/5539249/why-cant-transforms-begin-s-end-s-begin-tolower-be-complied-successfu*/
		if (recv.find("Error") != std::string::npos) {
			thrower("Error in offset lock programming, from Teensy: " + recv + "\r\nNote each number can only be of 13 chars long");
		}
	}
	tmp++;
}

void OlCore::OLForceOutput(std::array<double,size_t(OLGrid::total)> status, DoCore& doCore, DOStatus dostatus)
{
	resetOLEvents();
	initializeDataObjects(1);
	for (unsigned short inc = 0; inc < size_t(OLGrid::total); inc++)
	{
		olChannelSnapshots[0].push_back({ inc,0.1,status[inc],status[inc],1,1 });
	}
	writeOLs(0);
	doCore.FPGAForcePulse(dostatus, OL_TRIGGER_LINE, OL_TRIGGER_TIME);
	
}
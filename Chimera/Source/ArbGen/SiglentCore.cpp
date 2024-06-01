#include "stdafx.h"
#include "SiglentCore.h"

SiglentCore::SiglentCore(const arbGenSettings& settings) :
	ArbGenCore(settings)
{

}

SiglentCore::~SiglentCore() {
	visaFlume.close();
}


// stuff that only has to be done once.
void SiglentCore::prepArbGenSettings(unsigned channel) {
	if (channel != 1 && channel != 2) {
		thrower("Bad value for channel in prepAgilentSettings! Channel shoulde be 1 or 2.");
	}
	// Set timout, sample rate, filter parameters, trigger settings.
	visaFlume.setAttribute(VI_ATTR_TMO_VALUE, 40000);
	/*once write this command, cannot set SWEEP or BURST anymore, have to turn to DDS and set them and then turn it back to TARB*/
	visaFlume.write("C1:SRATE MODE,TARB,VALUE," + str(sampleRate));
	visaFlume.write("C2:SRATE MODE,TARB,VALUE," + str(sampleRate));
}

/*
 * This function tells the agilent to use sequence # (varNum) and sets settings correspondingly.
 */
void SiglentCore::setScriptOutput(unsigned varNum, scriptedArbInfo scriptInfo, unsigned chan) {
	if (scriptInfo.wave.isVaried() || varNum == 0) {
		prepArbGenSettings(chan);
		// check if effectively dc
		if (scriptInfo.wave.minsAndMaxes.size() == 0) {
			thrower("script wave min max size is zero???");
		}
		auto& minMaxs = scriptInfo.wave.minsAndMaxes[varNum];
		if (fabs(minMaxs.first - minMaxs.second) < 1e-6) {
			dcInfo tempDc;
			tempDc.dcLevel = str(minMaxs.first);
			std::vector<parameterType> var = std::vector<parameterType>();
			tempDc.dcLevel.internalEvaluate(var, 1);
			tempDc.useCal = scriptInfo.useCal;
			setDC(chan, tempDc, 0);
		}
		else {
			auto schan = "C" + str(chan);
			visaFlume.write(schan + ":ARWV NAME,wave" + str(varNum));
			visaFlume.write(schan + ":BSWV WVTP,ARB");
			visaFlume.write(schan + ":BSWV OFST," + str((minMaxs.first + minMaxs.second) / 2) + "V");
			visaFlume.write(schan + ":BSWV LLEV," + str(minMaxs.first) + "V");
			visaFlume.write(schan + ":BSWV HLEV," + str(minMaxs.second) + "V");
			programBurstMode(chan, true);


			//visaFlume.write(schan + ":OUTPut ON");
			outputOn(chan);
		}
	}
}


void SiglentCore::outputOff(int channel) {
	if (channel != 1 && channel != 2) {
		thrower("bad value for channel inside outputOff! Channel shoulde be 1 or 2.");
	}
	//channel++;
	visaFlume.write("C" + str(channel) + ":OUTPut OFF");
}

void SiglentCore::outputOn(int channel)
{
	if (channel != 1 && channel != 2) {
		thrower("bad value for channel inside outputOn! Channel shoulde be 1 or 2.");
	}
	//channel++;
	std::string outputOn;
	visaFlume.query("C" + str(channel) + ":OUTP?", outputOn, "%t");
	if (outputOn.find("OUTP ON") == std::string::npos) {
		visaFlume.write("C" + str(channel) + ":OUTPut ON");
	}
}


void SiglentCore::setSync(const deviceOutputInfo& runSettings, ExpThreadWorker* expWorker)
{
	try {
		notify({ "Writing Siglent output sync option: " + qstr(runSettings.synced), 2 }, expWorker);
		visaFlume.write("C1:SYNC " + (runSettings.synced) ? "ON" : "OFF");
		/*default sync to CH1, siglent can also sync with mod, see if we need zzp 20210228*/
	}
	catch (ChimeraError&) {
		thrower("Failed to set Siglent output synced, check connection as well as Siglent command syntax in c++ code");
		//errBox ("Failed to set agilent output synced?!");
	}
}

void SiglentCore::setPolarity(int channel, bool polarityInverted, ExpThreadWorker* expWorker)
{
	if (channel != 1 && channel != 2) {
		thrower("Bad value for channel inside setDC! Channel shoulde be 1 or 2.");
	}
	//try {
	//	visaFlume.write("C" + str(channel) + ":INVT " + (polarityInverted ? "ON" : "OFF"));
	//}
	//catch (ChimeraError&) {
	//	throwNested("Seen while programming output polarity for channel " + str(channel) + " (1-indexed).");
	//}
}

void SiglentCore::setDC(int channel, dcInfo info, unsigned var) {
	if (channel != 1 && channel != 2) {
		thrower("Bad value for channel inside setDC! Channel shoulde be 1 or 2.");
	}
	try {
		visaFlume.write("C" + str(channel) + ":BSWV WVTP,DC,OFST,"
			+ str(convertPowerToSetPoint(info.dcLevel.getValue(var), info.useCal, calibrations[channel - 1])) 
			+ "V");
	}
	catch (ChimeraError&) {
		throwNested("Seen while programming DC for channel " + str(channel) + " (1-indexed).");
	}
}


void SiglentCore::setExistingWaveform(int channel, preloadedArbInfo info) {
	if (channel != 1 && channel != 2) {
		thrower("Bad value for channel in setExistingWaveform! Channel shoulde be 1 or 2.");
	}
	visaFlume.write("C" + str(channel) + ":ARWV NAME," + info.address.expressionStr);
	programBurstMode(channel, info.burstMode);
	//visaFlume.write("C" + str(channel) + ":OUTPut ON");
	outputOn(channel);
}


void SiglentCore::programBurstMode(int channel, bool burstOption) 
{
	std::string sStr = "C" + str(channel);
	if (burstOption) 
	{
		std::string arbMode;
		visaFlume.query(sStr + ":SRATE?", arbMode, "%t");
		bool TARB = false;
		if (arbMode.find("TARB") != std::string::npos) {
			TARB = true;
		}
		/*must change to DDS mode to be able to change burst state*/
		visaFlume.write(sStr + ":SRATE MODE,DDS");
		
		visaFlume.write(sStr + ":BTWV STATE,ON");
		visaFlume.write(sStr + ":BTWV TRSR,EXT");
		visaFlume.write(sStr + ":BTWV GATE_NCYC,NCYC");
		visaFlume.write(sStr + ":BTWV EDGE,RISE");
		visaFlume.write(sStr + ":BTWV TIME,1"); /*Value of Ncycle number*/
		visaFlume.write(sStr + ":BTWV STPS,0");
		visaFlume.write(sStr + ":BTWV DLAY,0");

		if (TARB) {
			visaFlume.write(sStr + ":SRATE MODE,TARB");
			//thrower("Agilent can not set Burst in TrueArb mode. Have changed the mode to DDS.");
		}

	}
	else {
		visaFlume.write(sStr + ":BTWV STATE,OFF");
	}
}

void SiglentCore::programNonArbBurstMode(int channel, bool burstOption)
{
	std::string sStr = "C" + str(channel);
	if (burstOption)
	{
		std::string arbMode;
		visaFlume.query(sStr + ":SRATE?", arbMode, "%t");
		bool TARB = false;
		if (arbMode.find("TARB") != std::string::npos) {
			TARB = true;
			/*must change to DDS mode to be able to change burst state*/
			visaFlume.write(sStr + ":SRATE MODE,DDS");
		}

		visaFlume.write(sStr + ":BTWV STATE,ON");
		//visaFlume.write(sStr + ":BTWV GATE_NCYC,NCYC");
		//visaFlume.write(sStr + ":BTWV DLAY,0"); // This need to be changed when gating is set to NCYC
		visaFlume.write(sStr + ":BTWV GATE_NCYC,GATE");
		visaFlume.write(sStr + ":BTWV TRSR,EXT");
		visaFlume.write(sStr + ":BTWV EDGE,RISE");
		visaFlume.write(sStr + ":BTWV PLRT,POS");
		visaFlume.write(sStr + ":BTWV STPS,0");

		if (TARB) {
			//visaFlume.write(sStr + ":SRATE MODE,TARB");
			thrower("Agilent can not set Burst in TrueArb mode. Have changed the mode to DDS.");
		}

	}
	else {
		visaFlume.write(sStr + ":BTWV STATE,OFF");
	}
}


// set the agilent to output a square wave.
void SiglentCore::setSquare(int channel, squareInfo info, unsigned var) {
	if (channel != 1 && channel != 2) {
		thrower("Bad Value for Channel in setSquare! Channel shoulde be 1 or 2.");
	}
	try {
		visaFlume.write("C" + str(channel) + ":BSWV WVTP,SQUARE"
			",FRQ," + str(info.frequency.getValue(var) * 1000) +"HZ"
			",AMP," + str(convertPowerToSetPoint(info.amplitude.getValue(var), info.useCal, calibrations[channel - 1])) + "VPP"
			",OFST," + str(convertPowerToSetPoint(info.offset.getValue(var), info.useCal, calibrations[channel - 1])) + "V"
			",DUTY," + str(info.dutyCycle.getValue(var)) + 
			",PHSE," + str(info.phase.getValue(var)));
		programNonArbBurstMode(channel, info.burstMode);
		if (info.burstMode) {
			visaFlume.write("C" + str(channel) + +":BTWV CARR,PHSE," + str(info.phase.getValue(var)));
			visaFlume.write("C" + str(channel) + +":BTWV STPS," + str(info.phase.getValue(var))); // both works the same way
		}
		outputOn(channel);
	}
	catch (ChimeraError&) {
		throwNested("Seen while programming Square Wave for channel " + str(channel) + " (1-indexed).");
	}
}


void SiglentCore::setSine(int channel, sineInfo info, unsigned var) {
	if (channel != 1 && channel != 2) {
		thrower("Bad value for channel in setSine! Channel shoulde be 1 or 2.");
	}
	try {
		visaFlume.write("C" + str(channel) + ":BSWV WVTP,SINE"
			",FRQ," + str(info.frequency.getValue(var) * 1000) + "HZ"
			",AMP," + str(convertPowerToSetPoint(info.amplitude.getValue(var), info.useCal, calibrations[channel - 1])) + "VPP"
			",OFST,0V"
			",PHSE," + str(info.phase.getValue(var)));
		programNonArbBurstMode(channel, info.burstMode);
		outputOn(channel);
		//visaFlume.write("C" + str(channel) + ":BTWV CARR,WVTP,SINE"
		//	",FRQ," + str(info.frequency.getValue(var) * 1000) + "HZ"
		//	",AMP," + str(convertPowerToSetPoint(info.amplitude.getValue(var), info.useCal, calibrations[channel - 1])) + "VPP"
		//	",OFST,0V,PHSE,0"); // this also works

	}
	catch (ChimeraError& e) {
		throwNested("Seen while programming Sine Wave for channel " + str(channel) + " (1-indexed). \r\n" + e.trace());
	}

}

/**
 * This function tells the agilent to put out the DC default waveform.
 */
void SiglentCore::setDefault(int channel) {
	try {
		// turn it to the default voltage...
		std::string setPointString = str(convertPowerToSetPoint(SIGLENT_DEFAULT_POWER, true, calibrations[channel - 1]));
		visaFlume.write("C" + str(channel) + ":BSWV WVTP,DC,OFST," + setPointString + "V");
		outputOff(channel);
	}
	catch (ChimeraError&) {
		throwNested("Seen while programming default voltage.");
	}
}

void SiglentCore::handleScriptVariation(unsigned variation, scriptedArbInfo& scriptInfo, unsigned channel,
	std::vector<parameterType>& params) {
	prepArbGenSettings(channel);
	/**********make sure be in DDS mode to change burst and sweep***********/
	programSetupCommands();
	if (scriptInfo.wave.isVaried() || variation == 0) 
	{
		unsigned totalSegmentNumber = scriptInfo.wave.getSegmentNumber();
		// Loop through all segments
		for (auto segNumInc : range(totalSegmentNumber)) {
			// Use that information to writebtn the data.
			try {
				scriptInfo.wave.calSegmentData(segNumInc, sampleRate, variation);
			}
			catch (ChimeraError&) {
				throwNested("IntensityWaveform.calSegmentData threw an error! Error occurred in segment #"
					+ str(totalSegmentNumber));
			}
		}
		// order matters.
		// loop through again and calc/normalize/writebtn values.
		scriptInfo.wave.convertPowersToVoltages(scriptInfo.useCal, calibrations[channel - 1]);
		scriptInfo.wave.calcMinMax();
		scriptInfo.wave.minsAndMaxes.resize(variation + 1);
		scriptInfo.wave.minsAndMaxes[variation].second = scriptInfo.wave.getMaxVolt();
		scriptInfo.wave.minsAndMaxes[variation].first = scriptInfo.wave.getMinVolt();
		scriptInfo.wave.normalizeVoltages();

		prepArbGenSettings(channel);
		std::string& totalSeq = scriptInfo.wave.getTotalSequence();
		totalSeq = "C" + str(channel) + ":WVDT WVNM,wave" + str(variation) + ",WAVEDATA,";
		for (unsigned segNumInc : range(totalSegmentNumber)) 
		{
			totalSeq += compileAndReturnDataSendString(scriptInfo, segNumInc, variation,
				totalSegmentNumber, channel);
		}
		//compileSequenceString(scriptInfo, totalSegmentNumber, variation, channel, variation);
		// submit the sequence
		visaFlume.write(totalSeq/*scriptInfo.wave.returnSequenceString()*/);

		//std::string tmp;
		//visaFlume.query("WVDT? USER,wave0", tmp);
	}
}


/*
 * This function takes the data points (that have already been converted and normalized) and puts them into a string
 * for the agilent to readbtn. segNum: this is the segment number that this data is for
 * varNum: This is the variation number for this segment (matters for naming the segments)
 * totalSegNum: This is the number of segments in the waveform (also matters for naming)
 * For siglent, cannot do sequencing. Have to stitch all seq together.
 */
std::string SiglentCore::compileAndReturnDataSendString(scriptedArbInfo& scriptInfo, int segNum, int varNum, int totalSegNum, unsigned chan) {
	std::vector<Segment> waveformSegments = scriptInfo.wave.getWaveformSegments();

	// must get called after data conversion
	std::string tempSendString;
	//tempSendString = "C" + str(chan) + ":WVDT WVNM,wave" + str(segNum + totalSegNum * varNum) + ",WAVEDATA,";
	unsigned numData = waveformSegments[segNum].returnDataSize();
	double scale = (1 << 15) - 1; //upper,lower of 16bit signed interger
	for (unsigned sendDataInc = 0; sendDataInc < numData; sendDataInc++) 
	{
		/*note the format is small endian for siglent*/
		signed short data = static_cast<signed short>(scale * waveformSegments[segNum].returnDataVal(sendDataInc));
		unsigned char hi = data >> 8;
		unsigned char lo = data & 0xff;
		tempSendString += static_cast<char>(data & 0xff);
		tempSendString += static_cast<char>(data >> 8);
		//tempSendString += ",";
	}
	/*the coma for the last data will be handled after write all seq*/
	return tempSendString;
}


/*
* This function compiles the sequence string which tells the agilent what waveforms to output when and with what trigger control. The sequence is stored
* as a part of the class.  Not used for siglent zzp20210301
*/
void SiglentCore::compileSequenceString(scriptedArbInfo& scriptInfo, int totalSegNum, int sequenceNum, unsigned channel, unsigned varNum) {
	std::vector<Segment> waveformSegments = scriptInfo.wave.getWaveformSegments();
	std::string& totalSequence = scriptInfo.wave.getTotalSequence();

	//std::string tempSequenceString, tempSegmentInfoString;
	//// Total format is  #<n><n digits><sequence name>,<arb name1>,<repeat count1>,<play control1>,<marker mode1>,<marker point1>,<arb name2>,<repeat count2>,
	//// <play control2>, <marker mode2>, <marker point2>, and so on.
	//tempSequenceString = "SOURce" + str(channel) + ":DATA:SEQ #";
	//tempSegmentInfoString = "sequence" + str(sequenceNum) + ",";
	//if (totalSegNum == 0) {
	//	thrower("No segments in agilent waveform???\r\n");
	//}
	//for (int segNumInc = 0; segNumInc < totalSegNum; segNumInc++) {
	//	tempSegmentInfoString += "segment" + str(segNumInc + totalSegNum * sequenceNum) + ",";
	//	tempSegmentInfoString += str(waveformSegments[segNumInc].getInput().repeatNum.getValue(varNum)) + ",";
	//	tempSegmentInfoString += SegmentEnd::toStr(waveformSegments[segNumInc].getInput().continuationType) + ",";
	//	tempSegmentInfoString += "highAtStart,4,";
	//}
	//// remove final comma.
	//tempSegmentInfoString.pop_back();
	//totalSequence = tempSequenceString + str((str(tempSegmentInfoString.size())).size())
	//	+ str(tempSegmentInfoString.size()) + tempSegmentInfoString;
}


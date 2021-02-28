// created by Mark O. Brown
#pragma once
#include "Segment.h"
#include "SegmentEnd.h"
#include <string>
#include <vector>
//#include <AnalogInput/calInfo.h>
#include <AnalogOutput/calInfo.h>

class ScriptStream;
/**
* The class ScriptedAgilentWaveform contains all of the information and handling relevant for the entire intensity waveform that gets programmed to the Andor.
* This includes a vector of segments which contain segment-specific information. The functions and variabels relevant for this class are:
*/

class ScriptedArbGenWaveform 
{
public:
	ScriptedArbGenWaveform();
	bool analyzeAgilentScriptCommand( int segNum, ScriptStream& script, std::vector<parameterType>& params,
		std::string& warnings);
	void calSegmentData( int SegNum, unsigned long sampleRate, unsigned varNum);
	std::string returnSequenceString();
	bool isVaried();
	void calculateAllSegmentVariations( unsigned totalNumVariations, std::vector<parameterType>& variables);
	void convertPowersToVoltages (bool useCal, calResult calibration);
	void normalizeVoltages();
	void calcMinMax();
	double getMaxVolt();
	double getMinVolt();
	unsigned long getSegmentNumber();
	std::vector<std::pair<double, double>> minsAndMaxes;
	unsigned long getNumTrigs( );
	void resetNumberOfTriggers( );

	const std::vector<Segment>& getWaveformSegments() { return waveformSegments; }
	std::string& getTotalSequence() { return totalSequence; }

	//virtual std::string compileAndReturnDataSendString(int segNum, int varNum, int totalSegNum, unsigned chan) = 0;
	//virtual void compileSequenceString(int totalSegNum, int sequenceNum, unsigned channel, unsigned varNum) = 0;
	
protected:
	unsigned numberOfTriggers;
	std::vector<Segment> waveformSegments;
	double maxVolt;
	double minVolt;
	int segmentNum;
	std::string totalSequence;
};

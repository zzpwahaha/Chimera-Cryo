// created by Mark O. Brown
#pragma once

#include "ScriptedArbGenWaveform.h"
#include "ArbGenChannelMode.h"
#include "ArbGenSettings.h"
#include "ParameterSystem/Expression.h"
#include <string>
#include <array>

//class Agilent;

struct minMaxDoublet{
	double min;
	double max;
};


struct generalArbGenOutputInfo{
	bool useCal = false;
};

struct scriptedArbInfo : public generalArbGenOutputInfo {
	Expression fileAddress = "";
	ScriptedArbGenWaveform wave;
};


struct dcInfo : public generalArbGenOutputInfo {
	Expression dcLevel;
};


struct squareInfo : public generalArbGenOutputInfo {
	Expression frequency;
	Expression amplitude;
	Expression offset;
	// not used yet // ?!?!
	Expression dutyCycle;
	Expression phase;
};


struct sineInfo : public generalArbGenOutputInfo {
	Expression frequency;
	Expression amplitude;
};


struct preloadedArbInfo : public generalArbGenOutputInfo {
	// The only reason at this time to make this an expression instead of a normal string is to make sure it gets 
	// outputted to the config file correctly in case it's empty. 
	Expression address = "";
	bool burstMode = false;
};


struct channelInfo{
	ArbGenChannelMode::which option = ArbGenChannelMode::which::No_Control;
	dcInfo dc;
	sineInfo sine;
	squareInfo square;
	preloadedArbInfo preloadedArb;
	scriptedArbInfo scriptedArb;
};


struct deviceOutputInfo{
	// first ([0]) is channel 1, second ([1]) is channel 2.
	std::array<channelInfo, 2> channel;
	bool synced = false;
};


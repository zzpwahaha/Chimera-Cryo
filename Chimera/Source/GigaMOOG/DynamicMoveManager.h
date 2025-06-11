#pragma once
#include <Scripts/ScriptStream.h>
#include <GigaMOOG/MessageSender.h>
#include <Rearrangement/rearrangeLUT.h>
#include <Rearrangement/rearrangeStructures.h>
#include <Rearrangement/rearrangeParameters.h>


class DynamicMoveManager
{
public:
	// THIS CLASS IS NOT COPYABLE.
	DynamicMoveManager& operator=(const DynamicMoveManager&) = delete;
	DynamicMoveManager(const DynamicMoveManager&) = delete;
	DynamicMoveManager() {};
	bool analyzeMoogScript(std::string word, ScriptStream& currentMoogScript, MessageSender& ms, std::vector<parameterType>& variables, unsigned variation);
	void writeRearrangeMoves(moveSequence input, MessageSender& ms, unsigned variation);
	//moveSequence getRearrangeMoves(std::string rearrangeType);
	rearrangeParameters getRearrangeParameters();
	bool isMoveActive();
	void updataParameterForVariation(unsigned variation);

private:
	// write load for move with LUT
	void writeLoad(MessageSender& ms, unsigned variation);
	void writeMoveOff(MessageSender& ms);
	void checkTotalPower();

public:
	const unsigned MAX_XTONES = 39; // could be changed to 48 if using more tones for rearrangement
	const unsigned MAX_YTONES = 13; // could be changed to 48 if using more tones for rearrangement
	const double MAX_XPOWER = 282000; // see 20250603 increase power to 1.5W, 2dB ch0, 3dB ch 1
	const double MAX_YPOWER = 260000;
private:
	bool moveActive;
	rearrangeLUT moveLUT;
	rearrangeParameters moveParam;

};


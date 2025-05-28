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
	void writeRearrangeMoves(moveSequence input, MessageSender& ms);
	//moveSequence getRearrangeMoves(std::string rearrangeType);
	rearrangeParameters getRearrangeParameters();
	bool isMoveActive();

private:
	// write load for move with LUT
	void writeLoad(MessageSender& ms);
	void writeMoveOff(MessageSender& ms);

public:
	const unsigned MAX_XTONES = 38; // could be changed to 48 if using more tones for rearrangement
	const unsigned MAX_YTONES = 24; // could be changed to 48 if using more tones for rearrangement
	const unsigned TONES_REPEAT = 4; // repeat certain tones by this fold. Power is increased by TONES_REPEAT^2

private:
	bool moveActive;
	rearrangeLUT moveLUT;
	rearrangeParameters moveParam;

};


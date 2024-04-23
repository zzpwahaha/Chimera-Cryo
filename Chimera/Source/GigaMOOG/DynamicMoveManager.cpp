#include "stdafx.h"
#include "DynamicMoveManager.h"
#include <GigaMOOG/MemoryController.h>

bool DynamicMoveManager::analyzeMoogScript(std::string word, ScriptStream& currentMoogScript, MessageSender& ms, std::vector<parameterType>& variables, unsigned variation)
{
	if (word != "rearrange") {
		return false;
	}

	Expression ampStepNew, freqStepNew, ampStepPaintNew, freqStepPaintNew, xoff, yoff, yPaintStartExpr, yPaintEndExpr, scrunchSpacingExpression;
	std::string tmp, initAOX, initAOY, filterAOX, filterAOY;
	currentMoogScript >> moveParam.rearrangeMode;
	auto rearrangeMode = moveParam.rearrangeMode;
	if (rearrangeMode != "scrunchx" && rearrangeMode != "scrunchy" && rearrangeMode != "scrunchxy"
		&& rearrangeMode != "scrunchyx" && rearrangeMode != "centerscrunchyx" && rearrangeMode != "tetris") {
		thrower("Invalid rearrangement mode. Valid options are scrunchx, scrunchy, scrunchxy, scrunchyx, centerscrunchyx, and tetris.");
	}

	currentMoogScript >> scrunchSpacingExpression;
	moveParam.scrunchSpacing = scrunchSpacingExpression.evaluate(variables, variation);

	currentMoogScript >> ampStepNew;
	currentMoogScript >> freqStepNew;

	moveParam.ampStepMag = std::round(ampStepNew.evaluate(variables, variation));
	if (moveParam.ampStepMag > 134217727 || moveParam.ampStepMag < 0) {
		thrower("Warning: gmoog amplitude step out of range [-134217728, 134217727]. Need to be positive.");
	}

	moveParam.freqStepMag = round(freqStepNew.evaluate(variables, variation));
	if (moveParam.freqStepMag > 511 || moveParam.freqStepMag < 0) {
		thrower("Warning: gmoog frequency step out of range [-512, 511]. Need to be positive.");
	}

	currentMoogScript >> tmp;
	if (tmp == "xoffset") {
		currentMoogScript >> xoff;
		moveParam.xOffsetManual = xoff.evaluate(variables, variation);
	}
	else {
		thrower("Error: must first specify x frequency offset.");
	}

	currentMoogScript >> tmp;
	if (tmp == "yoffset") {
		currentMoogScript >> yoff;
		moveParam.yOffsetManual = yoff.evaluate(variables, variation);
	}
	else {
		thrower("Error: must first specify y frequency offset.");
	}

	currentMoogScript >> tmp;
	auto& initialPositionsX = moveParam.initialPositionsX;
	auto& nTweezerX = moveParam.nTweezerX;
	if (tmp == "initx") {
		currentMoogScript >> initAOX;
		initialPositionsX.clear();
		nTweezerX = 0;
		for (auto& ch : initAOX) { //convert string to boolean vector
			if (ch == '0') {
				initialPositionsX.push_back(0);
			}
			else if (ch == '1') {
				initialPositionsX.push_back(1);
				nTweezerX++;
			}
			else { thrower("Error: non-boolean target value."); }
		}
	}
	else {
		thrower("Error: must first specify initial x values.");
	}

	currentMoogScript >> tmp;
	auto& initialPositionsY = moveParam.initialPositionsY;
	auto& nTweezerY = moveParam.nTweezerY;
	if (tmp == "inity") {
		currentMoogScript >> initAOY;
		initialPositionsY.clear();
		nTweezerY = 0;
		for (auto& ch : initAOY) { //convert string to boolean vector
			if (ch == '0') {
				initialPositionsY.push_back(0);
			}
			else if (ch == '1') {
				initialPositionsY.push_back(1);
				nTweezerY++;
			}
			else { thrower("Error: non-boolean target value."); }
		}
	}
	else {
		thrower("Error: must first specify initial y values.");
	}

	auto& initialPositions = moveParam.initialPositions;
	initialPositions.clear();
	for (auto tweezerBoolY : initialPositionsY) {
		for (auto tweezerBoolX : initialPositionsX) {
			initialPositions.push_back(tweezerBoolX * tweezerBoolY);
		}
	}


	if (initialPositionsX.size() != moveLUT.getXDim() || initialPositionsY.size() != moveLUT.getYDim()) {
		thrower("Error: initial positions (" + str(initialPositionsX.size()) + ", " + str(initialPositionsY.size()) +
			") must match tweezer look up table size: (" + str(moveLUT.getXDim()) + ", " + str(moveLUT.getYDim()) + ").");
	}

	currentMoogScript >> tmp;
	auto& filterPositionsX = moveParam.filterPositionsX;
	auto& nFilterTweezerX = moveParam.nFilterTweezerX;
	if (tmp == "filterx") {
		currentMoogScript >> filterAOX;
		filterPositionsX.clear();
		nFilterTweezerX = 0;
		for (auto& ch : filterAOX) { //convert string to boolean vector
			if (ch == '0') {
				filterPositionsX.push_back(0);
			}
			else if (ch == '1') {
				filterPositionsX.push_back(1);
				nFilterTweezerX++;
			}
			else { thrower("Error: non-boolean target value."); }
		}
	}
	else {
		thrower("Error: must first specify filter x values.");
	}

	currentMoogScript >> tmp;
	auto& filterPositionsY = moveParam.filterPositionsY;
	auto& nFilterTweezerY = moveParam.nFilterTweezerY;
	if (tmp == "filtery") {
		currentMoogScript >> filterAOY;
		filterPositionsY.clear();
		nFilterTweezerY = 0;
		for (auto& ch : filterAOY) { //convert string to boolean vector
			if (ch == '0') {
				filterPositionsY.push_back(0);
			}
			else if (ch == '1') {
				filterPositionsY.push_back(1);
				nFilterTweezerY++;
			}
			else { thrower("Error: non-boolean target value."); }
		}
	}
	else {
		thrower("Error: must first specify filter y values.");
	}

	currentMoogScript >> tmp;
	auto& targetPositions = moveParam.targetPositions;
	auto& targetNumber = moveParam.targetNumber;
	if (tmp == "targetstart") {
		targetPositions.clear();
		targetNumber = 0;

		for (size_t i = 0; i < moveLUT.getYDim(); i++) {
			currentMoogScript >> tmp;
			for (auto& ch : tmp) { //convert string to boolean vector
				if (ch == '0') {
					targetPositions.push_back(0);
				}
				else if (ch == '1') {
					targetPositions.push_back(1);
					targetNumber += 1; //count total desired atom number
				}
				else { thrower("Error: non-boolean target value."); }
			}
			if (tmp.size() != moveLUT.getXDim()) { thrower("Error: invalid target dimensions"); }
		}

		currentMoogScript >> tmp;
		if (tmp != "targetend") {
			thrower("Error: invalid target dimensions");
		}
	}
	else {
		thrower("Error: must specify target locations.");
	}
	
	// force the end of file so that does not read anything else 
	while (!(currentMoogScript.peek() == EOF) || word != "__end__") {
		word = "";
		currentMoogScript >> word;
	}
	writeLoad(ms);
	return true;
}

void DynamicMoveManager::writeRearrangeMoves(moveSequence input, MessageSender& ms)
{
	//Important: this function automatically writes the terminator and sends the data
	unsigned nMoves = input.nMoves();

	MemoryController memoryDAC0;
	MemoryController memoryDAC1;

	if (nMoves > 256 / 3) {
		thrower("ERROR: too many moves for gmoog buffer");
	}
	writeMoveOff(ms);

	const double ampStepMag = moveParam.ampStepMag;
	const double freqStepMag = moveParam.freqStepMag;
	size_t nx, ny;
	double phase, amp, freq, ampPrev, freqPrev;
	int ampstep, freqstep;

	//step 0: turn off all load tones.
	for (unsigned channel = 0; channel < MAX_XTONES; channel++) {
		if (channel < moveParam.nTweezerX) {
			size_t hardwareChannel = (channel * 8) % 48 + (channel * 8) / 48;
			memoryDAC0.moveChannel(hardwareChannel / 8);
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC0).channel(hardwareChannel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0)
				.instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(0).FTWIncr(0).phaseJump(1);;
			ms.enqueue(m);
		}
	}
	for (unsigned channel = 0; channel < MAX_YTONES; channel++) {
		if (channel < moveParam.nTweezerY) {
			size_t hardwareChannel = (channel * 8) % 48 + (channel * 8) / 48;
			memoryDAC1.moveChannel(hardwareChannel / 8);
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC1).channel(hardwareChannel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0)
				.instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(0).FTWIncr(0).phaseJump(1);;
			ms.enqueue(m);
		}
	}

	for (size_t stepID = 0; stepID < nMoves; stepID++) {
		nx = input.moves[stepID].nx();
		ny = input.moves[stepID].ny();

		//Get most hardware efficient channels to use. Also handle tripling up of tones.
		std::vector<int> hardwareChannelsDAC0;
		std::vector<int> hardwareChannelsDAC1;
		if (ny == 1) {
			hardwareChannelsDAC0 = memoryDAC0.getNextChannels(nx);
			hardwareChannelsDAC1 = memoryDAC1.getNextChannels(3);
		}
		else if (nx == 1) {
			hardwareChannelsDAC0 = memoryDAC0.getNextChannels(3);
			hardwareChannelsDAC1 = memoryDAC1.getNextChannels(ny);
		}
		else {
			hardwareChannelsDAC0 = memoryDAC0.getNextChannels(nx);
			hardwareChannelsDAC1 = memoryDAC1.getNextChannels(ny);
		}

		//step 1: ramp up tones at initial locations and phases
		for (int channel = 0; channel < MAX_XTONES; channel++) {
			if (ny > 1 && nx == 1 && channel < 3) {
				//Triple up tones if only a single tone on, assuming y axis not already tripled.
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freq = moveLUT.getFreqX(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);
				amp = moveLUT.getAmpX(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0)
					.instantFTW(1).ATWIncr(ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);
				ms.enqueue(m);
			}
			else if (ny != 0 && nx != 0 && channel < nx) {
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freq = moveLUT.getFreqX(input.moves[stepID].startAOX[channel], input.moves[stepID].startAOY[0]);
				amp = moveLUT.getAmpX(input.moves[stepID].startAOX[channel], input.moves[stepID].startAOY[0]);
				phase = fmod(180 * pow(channel + 1, 2) / nx, 360); //this assumes comb of even tones, imperfect, but also short duration so not super critical, and fast.
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(phase)
					.instantFTW(1).ATWIncr(ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);;
				ms.enqueue(m);
			}
		}

		for (int channel = 0; channel < MAX_YTONES; channel++) {
			if (nx != 0 && ny == 1 && channel < 3) {
				//Triple up tones if only a single tone on. 
				size_t hardwareChannel = hardwareChannelsDAC1[channel];
				freq = moveLUT.getFreqY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);
				amp = moveLUT.getAmpY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0)
					.instantFTW(1).ATWIncr(ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);;
				ms.enqueue(m);
			}
			else if (nx != 0 && ny != 0 && channel < ny) {
				size_t hardwareChannel = hardwareChannelsDAC1[channel];
				freq = moveLUT.getFreqY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[channel]);
				amp = moveLUT.getAmpY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[channel]);
				phase = fmod(180 * pow(channel + 1, 2) / ny, 360);
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(phase)
					.instantFTW(1).ATWIncr(ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);;
				ms.enqueue(m);
			}
		}

		//step 2: ramp to new locations
		for (int channel = 0; channel < MAX_XTONES; channel++) {
			if (ny > 1 && nx == 1 && channel < 3) {
				//Triple up tones if only a single tone on.
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freqPrev = moveLUT.getFreqX(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);
				ampPrev = moveLUT.getAmpX(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);

				freq = moveLUT.getFreqX(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[0]);
				amp = moveLUT.getAmpX(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[0]);

				ampstep = (amp < ampPrev) ? -ampStepMag : ampStepMag; //Change sign of steps appropriately.
				freqstep = (freq < freqPrev) ? -freqStepMag : freqStepMag;

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0)
					.instantFTW(0).ATWIncr(ampstep).stepSequenceID(3 * stepID + 1 + 1).FTWIncr(freqstep).phaseJump(0);;
				ms.enqueue(m);
			}
			else if (ny != 0 && nx != 0 && channel < nx) {
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freqPrev = moveLUT.getFreqX(input.moves[stepID].startAOX[channel], input.moves[stepID].startAOY[0]);
				ampPrev = moveLUT.getAmpX(input.moves[stepID].startAOX[channel], input.moves[stepID].startAOY[0]);

				freq = moveLUT.getFreqX(input.moves[stepID].endAOX[channel], input.moves[stepID].endAOY[0]);
				amp = moveLUT.getAmpX(input.moves[stepID].endAOX[channel], input.moves[stepID].endAOY[0]);

				ampstep = (amp < ampPrev) ? -ampStepMag : ampStepMag; //Change sign of steps appropriately.
				freqstep = (freq < freqPrev) ? -freqStepMag : freqStepMag;

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0)
					.instantFTW(0).ATWIncr(ampstep).stepSequenceID(3 * stepID + 1 + 1).FTWIncr(freqstep).phaseJump(0);;
				ms.enqueue(m);
			}
		}

		for (int channel = 0; channel < MAX_YTONES; channel++) {
			if (nx != 0 && ny == 1 && channel < 3) {
				//Triple up tones if only a single tone on. 
				size_t hardwareChannel = hardwareChannelsDAC1[channel];

				freqPrev = moveLUT.getFreqY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);
				ampPrev = moveLUT.getAmpY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);

				freq = moveLUT.getFreqY(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[0]);
				amp = moveLUT.getAmpY(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[0]);

				ampstep = (amp < ampPrev) ? -ampStepMag : ampStepMag; //Change sign of steps appropriately.
				freqstep = (freq < freqPrev) ? -freqStepMag : freqStepMag;

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0)
					.instantFTW(0).ATWIncr(ampstep).stepSequenceID(3 * stepID + 1 + 1).FTWIncr(freqstep).phaseJump(0);;
				ms.enqueue(m);
			}
			else if (nx != 0 && ny != 0 && channel < ny) {
				size_t hardwareChannel = hardwareChannelsDAC1[channel];

				freqPrev = moveLUT.getFreqY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[channel]);
				ampPrev = moveLUT.getAmpY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[channel]);

				freq = moveLUT.getFreqY(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[channel]);
				amp = moveLUT.getAmpY(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[channel]);

				ampstep = (amp < ampPrev) ? -ampStepMag : ampStepMag; //Change sign of steps appropriately.
				freqstep = (freq < freqPrev) ? -freqStepMag : freqStepMag;

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0)
					.instantFTW(0).ATWIncr(ampstep).stepSequenceID(3 * stepID + 1 + 1).FTWIncr(freqstep).phaseJump(0);;
				ms.enqueue(m);
			}
		}

		//step 3: ramp all tones to 0
		for (int channel = 0; channel < MAX_XTONES; channel++) {
			if (ny > 1 && nx == 1 && channel < 3)  {
				//Triple up tones if only a single tone on.
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freq = moveLUT.getFreqX(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[0]);
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(0.01).phaseDegrees(0)
					.instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 2 + 1).FTWIncr(0).phaseJump(0);;
				ms.enqueue(m);
			}
			else if (ny != 0 && nx != 0 && channel < nx) {
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freq = moveLUT.getFreqX(input.moves[stepID].endAOX[channel], input.moves[stepID].endAOY[0]);
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(0.01).phaseDegrees(0)
					.instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 2 + 1).FTWIncr(0).phaseJump(0);;
				ms.enqueue(m);
				//Has trouble with ramping to 0 amp for some reason - set to ~1 LSB = 100/65535.
			}
		}

		for (int channel = 0; channel < MAX_YTONES; channel++) {
			if (nx != 0 && ny == 1 && channel < 3) {
				//Triple up tones if only a single tone on. 
				size_t hardwareChannel = hardwareChannelsDAC1[channel];
				freq = moveLUT.getFreqY(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[0]);
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(0.01).phaseDegrees(0)
					.instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 2 + 1).FTWIncr(0).phaseJump(0);;
				ms.enqueue(m);
			}
			else if (nx != 0 && ny != 0 && channel < ny) {
				size_t hardwareChannel = hardwareChannelsDAC1[channel];
				freq = moveLUT.getFreqY(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[channel]);
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(0.01).phaseDegrees(0)
					.instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 2 + 1).FTWIncr(0).phaseJump(0);;
				ms.enqueue(m);
			}
		}
	}

	//additional snapshot ramping down all channels - unclear why needed, but prevents extra trigger issues.

	for (unsigned channel = 0; channel < 48; channel++) {
		Message m0 = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC0).channel(channel)
			.setting(MessageSetting::MOVEFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0)
			.instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * (nMoves - 1) + 2 + 2).FTWIncr(0).phaseJump(1);;
		ms.enqueue(m0);
	
		Message m1 = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC1).channel(channel)
			.setting(MessageSetting::MOVEFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0)
			.instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * (nMoves - 1) + 2 + 2).FTWIncr(0).phaseJump(1);;
		ms.enqueue(m1);
	}
}

void DynamicMoveManager::writeLoad(MessageSender& ms)
{
	//Since writeLoad always called before rearrange, just do auto tweezer offset here.
	//if (autoTweezerOffsetActive) {
	//	xOffset = xOffsetManual + xOffsetAuto;
	//	yOffset = yOffsetManual + yOffsetAuto;
	//}
	//else {
	//	xOffset = xOffsetManual;
	//	yOffset = yOffsetManual;
	//}

	moveParam.xOffset = moveParam.xOffsetManual;
	moveParam.yOffset = moveParam.yOffsetManual;

	//Write load settings based on initXY

	size_t iTweezerX = 0;
	size_t iMaskX = 0;
	for (auto const& channelBool : moveParam.initialPositionsX) {
		double phase, amp, freq;
		if (iTweezerX > MAX_XTONES) {
			thrower("For safety, maximum number of x tones is limited to " + str(MAX_XTONES) + " in rearrangement mode");
		}
		if (channelBool) {
			size_t hardwareChannel = (iTweezerX * 8) % 48 + (iTweezerX * 8) / 48;
			phase = fmod(180 * pow(iTweezerX + 1, 2) / moveParam.nTweezerX, 360); //this assumes comb of even tones.
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC0).channel(hardwareChannel)
				.setting(MessageSetting::LOADFREQUENCY)
				.frequencyMHz(moveLUT.getFreqX(iMaskX, 0)).amplitudePercent(moveLUT.getAmpX(iMaskX, 0)).phaseDegrees(phase);
			ms.enqueue(m);
			iTweezerX++;
		}
		iMaskX++;
	}

	size_t iTweezerY = 0;
	size_t iMaskY = 0;
	for (auto const& channelBool : moveParam.initialPositionsY) {
		double phase, amp, freq;
		if (iTweezerY > MAX_YTONES) {
			thrower("For safety, maximum number of y tones is limited to " + str(MAX_YTONES) + " in rearrangement mode");
		}
		if (channelBool) {
			size_t hardwareChannel = (iTweezerY * 8) % 48 + (iTweezerY * 8) / 48;
			phase = fmod(180 * pow(iTweezerY + 1, 2) / moveParam.nTweezerY, 360); //this assumes comb of even tones.
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC1).channel(hardwareChannel)
				.setting(MessageSetting::LOADFREQUENCY)
				.frequencyMHz(moveLUT.getFreqY(0, iMaskY)).amplitudePercent(moveLUT.getAmpY(0, iMaskY)).phaseDegrees(phase);
			ms.enqueue(m);
			iTweezerY++;
		}
		iMaskY++;
	}
}

void DynamicMoveManager::writeMoveOff(MessageSender& ms)
{
	//REMINDER: gmoog memory is in blocks of 8 channels, not necessary to clear every channel.
	for (int stepID = 0; stepID < 256; stepID++) {
		for (unsigned channel = 0; channel < 6; channel++) {
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC0).channel(channel * 8)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0)
				.instantFTW(1).ATWIncr(0).stepSequenceID(stepID).FTWIncr(0).phaseJump(0);
			ms.enqueue(m);
		}

		for (unsigned channel = 0; channel < 6; channel++) {
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC1).channel(channel * 8)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0)
				.instantFTW(1).ATWIncr(0).stepSequenceID(stepID).FTWIncr(0).phaseJump(0);
			ms.enqueue(m);
		}

		//TODO: put back in after programming rate fixed, and when using both rails.
		//for (unsigned channel = 0; channel < 48; channel++) {
		//	Message m = Message::make().destination(MessageDestination::KA007)
		//		.DAC(MessageDAC::DAC2).channel(channel)
		//		.setting(MessageSetting::MOVEFREQUENCY)
		//		.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0)
		//		.instantFTW(1).ATWIncr(0).stepSequenceID(stepID).FTWIncr(0).phaseJump(0);
		//	ms.enqueue(m);
		//}
		//for (unsigned channel = 0; channel < 48; channel++) {
		//	Message m = Message::make().destination(MessageDestination::KA007)
		//		.DAC(MessageDAC::DAC3).channel(channel)
		//		.setting(MessageSetting::MOVEFREQUENCY)
		//		.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0)
		//		.instantFTW(1).ATWIncr(0).stepSequenceID(stepID).FTWIncr(0).phaseJump(0);
		//	ms.enqueue(m);
		//}
	}
}


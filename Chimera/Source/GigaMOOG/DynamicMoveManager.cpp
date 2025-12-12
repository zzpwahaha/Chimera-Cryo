#include "stdafx.h"
#include "DynamicMoveManager.h"
#include <GigaMOOG/MemoryController.h>

bool DynamicMoveManager::analyzeMoogScript(std::string word, ScriptStream& currentMoogScript, MessageSender& ms, std::vector<parameterType>& variables, unsigned variation)
{
	if (word != "rearrange") {
		moveActive = false;
		return false;
	}
	if (variation == 0) {
		moveLUT.refreshLUT();
		moveParam.xOffsetManual.clear();
		moveParam.yOffsetManual.clear();
		// DOES NOT SUPPORT VARIATION FOR NOW EXCEPT X/YOFFSET
		moveActive = true;
	}

	Expression ampStepNew, freqStepNew, ampStepPaintNew, freqStepPaintNew, repeatX, repeatY, xoff, yoff, yPaintStartExpr, yPaintEndExpr, scrunchSpacingExpression;
	std::string tmp, loadAOX, loadAOY, initAOX, initAOY, filterAOX, filterAOY;
	currentMoogScript >> moveParam.rearrangeMode;
	auto rearrangeMode = moveParam.rearrangeMode;
	if (rearrangeMode != "scrunchx" && rearrangeMode != "scrunchy" && rearrangeMode != "scrunchxy"
		&& rearrangeMode != "centerscrunchx" && rearrangeMode != "centerscrunchy"
		&& rearrangeMode != "scrunchyx" && rearrangeMode != "centerscrunchyx" 
		&& rearrangeMode!="scrunchxtarget" && rearrangeMode != "tetris"
		&& rearrangeMode != "tweezer1dinittest") {
		thrower("Invalid rearrangement mode. Valid options are scrunchx, scrunchy, scrunchxy, scrunchyx, centerscrunchyx, and tetris.");
	}

	currentMoogScript >> scrunchSpacingExpression;
	if (scrunchSpacingExpression.varies()) {
		thrower("Error: Variation in variable " + scrunchSpacingExpression.expressionStr + " is not allowed in rearrangement(gigamoog) script for now.");
	}
	moveParam.scrunchSpacing = scrunchSpacingExpression.evaluate(variables, variation);

	currentMoogScript >> ampStepNew;
	currentMoogScript >> freqStepNew;

	if (ampStepNew.varies()) {
		thrower("Error: Variation in variable " + ampStepNew.expressionStr + " is not allowed in rearrangement(gigamoog) script for now.");
	}
	moveParam.ampStepMag = std::round(ampStepNew.evaluate(variables, variation));
	if (moveParam.ampStepMag > 134217727 || moveParam.ampStepMag < 0) {
		thrower("Warning: gmoog amplitude step out of range [-134217728, 134217727]. Need to be positive.");
	}

	if (freqStepNew.varies()) {
		thrower("Error: Variation in variable " + freqStepNew.expressionStr + " is not allowed in rearrangement(gigamoog) script for now.");
	}
	moveParam.freqStepMag = round(freqStepNew.evaluate(variables, variation));
	if (moveParam.freqStepMag > 511 || moveParam.freqStepMag < 0) {
		thrower("Warning: gmoog frequency step out of range [-512, 511]. Need to be positive.");
	}

	currentMoogScript >> tmp;
	if (tmp == "singlex_repeatx") {
		currentMoogScript >> repeatX;
		if (repeatX.varies()) {
			thrower("Error: Variation in variable " + repeatX.expressionStr + " is not allowed in rearrangement(gigamoog) script for now.");
		}
		moveParam.singlexRepeatX = static_cast<unsigned>(std::round(repeatX.evaluate(variables, variation)));
	}
	else {
		thrower("Error: must first specify number of repeats for tones in X axis when single X tone is one for move.");
	}

	currentMoogScript >> tmp;
	if (tmp == "singlex_repeaty") {
		currentMoogScript >> repeatY;
		if (repeatY.varies()) {
			thrower("Error: Variation in variable " + repeatY.expressionStr + " is not allowed in rearrangement(gigamoog) script for now.");
		}
		moveParam.singlexRepeatY = static_cast<unsigned>(std::round(repeatY.evaluate(variables, variation)));
	}
	else {
		thrower("Error: must first specify number of repeats for tones in Y axis when single X tone is one for move.");
	}

	currentMoogScript >> tmp;
	if (tmp == "singley_repeatx") {
		currentMoogScript >> repeatX;
		if (repeatX.varies()) {
			thrower("Error: Variation in variable " + repeatX.expressionStr + " is not allowed in rearrangement(gigamoog) script for now.");
		}
		moveParam.singleyRepeatX = static_cast<unsigned>(std::round(repeatX.evaluate(variables, variation)));
	}
	else {
		thrower("Error: must first specify number of repeats for tones in X axis when single Y tone is one for move.");
	}

	currentMoogScript >> tmp;
	if (tmp == "singley_repeaty") {
		currentMoogScript >> repeatY;
		if (repeatY.varies()) {
			thrower("Error: Variation in variable " + repeatY.expressionStr + " is not allowed in rearrangement(gigamoog) script for now.");
		}
		moveParam.singleyRepeatY = static_cast<unsigned>(std::round(repeatY.evaluate(variables, variation)));
	}
	else {
		thrower("Error: must first specify number of repeats for tones in Y axis when single Y tone is one for move.");
	}

	currentMoogScript >> tmp;
	if (tmp == "xoffset") {
		currentMoogScript >> xoff;
		if (xoff.varies()) {
			//thrower("Error: Variation in variable " + xoff.expressionStr + " is not allowed in rearrangement(gigamoog) script for now.");
		}
		moveParam.xOffsetManual.push_back(xoff.evaluate(variables, variation));
	}
	else {
		thrower("Error: must first specify x frequency offset.");
	}

	currentMoogScript >> tmp;
	if (tmp == "yoffset") {
		currentMoogScript >> yoff;
		if (yoff.varies()) {
			//thrower("Error: Variation in variable " + yoff.expressionStr + " is not allowed in rearrangement(gigamoog) script for now.");
		}
		moveParam.yOffsetManual.push_back(yoff.evaluate(variables, variation));
	}
	else {
		thrower("Error: must first specify y frequency offset.");
	}

	currentMoogScript >> tmp;
	auto& loadPositionsX = moveParam.loadPositionsX;
	auto& nTweezerLoadX = moveParam.nTweezerLoadX;
	if (tmp == "loadx") {
		currentMoogScript >> loadAOX;
		loadPositionsX.clear();
		nTweezerLoadX = 0;
		for (auto& ch : loadAOX) { //convert string to boolean vector
			if (ch == '0') {
				loadPositionsX.push_back(0);
			}
			else if (ch == '1') {
				loadPositionsX.push_back(1);
				nTweezerLoadX++;
			}
			else { thrower("Error: non-boolean target value."); }
		}
	}
	else {
		thrower("Error: must first specify load x values.");
	}

	currentMoogScript >> tmp;
	auto& loadPositionsY = moveParam.loadPositionsY;
	auto& nTweezerLoadY = moveParam.nTweezerLoadY;
	if (tmp == "loady") {
		currentMoogScript >> loadAOY;
		loadPositionsY.clear();
		nTweezerLoadY = 0;
		for (auto& ch : loadAOY) { //convert string to boolean vector
			if (ch == '0') {
				loadPositionsY.push_back(0);
			}
			else if (ch == '1') {
				loadPositionsY.push_back(1);
				nTweezerLoadY++;
			}
			else { thrower("Error: non-boolean target value."); }
		}
	}
	else {
		thrower("Error: must first specify load y values.");
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
	checkTotalPower();
	writeLoad(ms, variation);
	//writeMoveOff(ms); // clear move sequence in gigamoog, no matter whether will rearrange or not (depending on loaded atom number)
	return true;
}

void DynamicMoveManager::writeRearrangeMoves(moveSequence input, MessageSender& ms, unsigned variation)
{
	// Write load settings so that tweezers can be reset immediately after moves.
	// This is important since in the rep-first setting, there is no programVariation to set the Load for gigamoog
	//writeLoad(ms, variation);

	unsigned nMoves = input.nMoves();

	MemoryController memoryDAC0;
	MemoryController memoryDAC1;

	if (nMoves > 256 / 3 || nMoves > 256 / 3) {
		thrower("ERROR: too many moves for gmoog buffer");
	}
	writeMoveOff(ms);

	const double ampStepMag = moveParam.ampStepMag;
	const double freqStepMag = moveParam.freqStepMag;
	size_t nx, ny;
	double phase, amp, freq, ampPrev, freqPrev;
	int ampstep, freqstep;

	//step 0: turn off all load tones.
	auto numChannelX = 48; //moveParam.nTweezerX * moveParam.repeatX;
	for (unsigned channel = 0; channel < numChannelX && channel < MAX_XTONES; channel++) {
		size_t hardwareChannel = (channel * 8) % 48 + (channel * 8) / 48;
		memoryDAC0.moveChannel(hardwareChannel / 8);
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC0).channel(hardwareChannel)
			.setting(MessageSetting::MOVEFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0)
			.instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(0).FTWIncr(0).phaseJump(1);;
		ms.enqueue(m);
	}
	auto numChannelY = 48; // moveParam.nTweezerY* moveParam.repeatY;
	for (unsigned channel = 0; channel < numChannelY && channel < MAX_YTONES; channel++) {
		size_t hardwareChannel = (channel * 8) % 48 + (channel * 8) / 48;
		memoryDAC1.moveChannel(hardwareChannel / 8);
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC1).channel(hardwareChannel)
			.setting(MessageSetting::MOVEFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0)
			.instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(0).FTWIncr(0).phaseJump(1);;
		ms.enqueue(m);
		//std::cout << "setmove " << 0/*stepSequenceID*/ << " DAC1 "
		//	<< hardwareChannel/*channel*/ << " " << 1/*instantFTW*/ << " " << 1/*phaseJump*/ << " "
		//	<< 0.01/*amplitudePercent*/ << " " << -ampStepMag/*ATWIncr*/ << " "
		//	<< 0/*frequencyMHz*/ << " " << 0/*FTWIncr*/ << " "
		//	<< 0/*phaseDegrees*/ << std::endl;
	}

	for (size_t stepID = 0; stepID < nMoves; stepID++) {
		nx = input.moves[stepID].nx();
		ny = input.moves[stepID].ny();
		if (nx == 0 || ny == 0) {
			//thrower("Error in writeRearrangeMoves: seeing zero number of moves!");
			continue;
		}
		auto [repeatX, repeatY] = moveParam.getRepeatXY(nx, ny);

		//Get most hardware efficient channels to use. Also handle tripling up of tones.
		std::vector<int> hardwareChannelsDAC0 = memoryDAC0.getNextChannels(nx * repeatX);
		std::vector<int> hardwareChannelsDAC1 = memoryDAC0.getNextChannels(ny * repeatY);

		//step 1: ramp up tones at initial locations and phases
		for (int channel = 0; channel < nx * repeatX && channel < MAX_XTONES; channel++) {
			int logicalChannel = channel / repeatX;
			size_t hardwareChannel = hardwareChannelsDAC0[channel];

			freq = moveLUT.getFreqX(input.moves[stepID].startAOX[logicalChannel], input.moves[stepID].startAOY[0]);
			amp = moveLUT.getAmpX(input.moves[stepID].startAOX[logicalChannel], input.moves[stepID].startAOY[0]);
			phase = fmod(180 * pow(logicalChannel + 1, 2) / nx, 360); //this assumes comb of even tones, imperfect, but also short duration so not super critical, and fast.

			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC0).channel(hardwareChannel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(phase)
				.instantFTW(1).ATWIncr(ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);;
			ms.enqueue(m);
		}
		for (int channel = 0; channel < ny * repeatY && channel < MAX_YTONES; channel++) {
			int logicalChannel = channel / repeatY;
			size_t hardwareChannel = hardwareChannelsDAC1[channel];

			freq = moveLUT.getFreqY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[logicalChannel]);
			amp = moveLUT.getAmpY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[logicalChannel]);
			phase = fmod(180 * pow(logicalChannel + 1, 2) / ny, 360);

			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC1).channel(hardwareChannel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(phase)
				.instantFTW(1).ATWIncr(ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);;
			ms.enqueue(m);
			//std::cout << "setmove " << 3 * stepID + 1/*stepSequenceID*/ << " DAC1 "
			//	<< hardwareChannel/*channel*/ << " " << 1/*instantFTW*/ << " " << 1/*phaseJump*/ << " "
			//	<< amp/*amplitudePercent*/ << " " << ampStepMag/*ATWIncr*/ << " "
			//	<< freq/*frequencyMHz*/ << " " << 0/*FTWIncr*/ << " "
			//	<< phase/*phaseDegrees*/ << std::endl;
		}

		//step 2: ramp to new locations
		for (int channel = 0; channel < nx * repeatX && channel < MAX_XTONES; channel++) {
			int logicalChannel = channel / repeatX;
			size_t hardwareChannel = hardwareChannelsDAC0[channel];

			freqPrev = moveLUT.getFreqX(input.moves[stepID].startAOX[logicalChannel], input.moves[stepID].startAOY[0]);
			ampPrev = moveLUT.getAmpX(input.moves[stepID].startAOX[logicalChannel], input.moves[stepID].startAOY[0]);

			freq = moveLUT.getFreqX(input.moves[stepID].endAOX[logicalChannel], input.moves[stepID].endAOY[0]);
			amp = moveLUT.getAmpX(input.moves[stepID].endAOX[logicalChannel], input.moves[stepID].endAOY[0]);

			ampstep = (amp < ampPrev) ? -ampStepMag : ampStepMag; //Change sign of steps appropriately.
			freqstep = (freq < freqPrev) ? -freqStepMag : freqStepMag;

			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC0).channel(hardwareChannel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0)
				.instantFTW(0).ATWIncr(ampstep).stepSequenceID(3 * stepID + 1 + 1).FTWIncr(freqstep).phaseJump(0);;
			ms.enqueue(m);
		}
		for (int channel = 0; channel < ny * repeatY && channel < MAX_YTONES; channel++) {
			int logicalChannel = channel / repeatY;
			size_t hardwareChannel = hardwareChannelsDAC1[channel];

			freqPrev = moveLUT.getFreqY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[logicalChannel]);
			ampPrev = moveLUT.getAmpY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[logicalChannel]);

			freq = moveLUT.getFreqY(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[logicalChannel]);
			amp = moveLUT.getAmpY(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[logicalChannel]);

			ampstep = (amp < ampPrev) ? -ampStepMag : ampStepMag; //Change sign of steps appropriately.
			freqstep = (freq < freqPrev) ? -freqStepMag : freqStepMag;

			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC1).channel(hardwareChannel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0)
				.instantFTW(0).ATWIncr(ampstep).stepSequenceID(3 * stepID + 2).FTWIncr(freqstep).phaseJump(0);
			ms.enqueue(m);
			//std::cout << "setmove " << 3 * stepID + 2/*stepSequenceID*/ << " DAC1 "
			//	<< hardwareChannel/*channel*/ << " " << 0/*instantFTW*/ << " " << 0/*phaseJump*/ << " "
			//	<< amp/*amplitudePercent*/ << " " << ampstep/*ATWIncr*/ << " "
			//	<< freq/*frequencyMHz*/ << " " << freqstep/*FTWIncr*/ << " "
			//	<< 0/*phaseDegrees*/ << std::endl;
		}

		//step 3: ramp all tones to 0
		for (int channel = 0; channel < nx * repeatX && channel < MAX_XTONES; channel++) {
			int logicalChannel = channel / repeatX;
			size_t hardwareChannel = hardwareChannelsDAC0[channel];
			freq = moveLUT.getFreqX(input.moves[stepID].endAOX[logicalChannel], input.moves[stepID].endAOY[0]);
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC0).channel(hardwareChannel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(freq).amplitudePercent(0.01).phaseDegrees(0)
				.instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 3).FTWIncr(0).phaseJump(0);;
			ms.enqueue(m);
			//Has trouble with ramping to 0 amp for some reason - set to ~1 LSB = 100/65535.
		}
		for (int channel = 0; channel < ny * repeatY && channel < MAX_YTONES; channel++) {
			int logicalChannel = channel / repeatY;
			size_t hardwareChannel = hardwareChannelsDAC1[channel];
			freq = moveLUT.getFreqY(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[logicalChannel]);
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC1).channel(hardwareChannel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(freq).amplitudePercent(0.01).phaseDegrees(0)  // near-zero amp (~1 LSB)
				.instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 3).FTWIncr(0).phaseJump(0);
			ms.enqueue(m);
			//std::cout << "setmove " << 3 * stepID + 3/*stepSequenceID*/ << " DAC1 "
			//	<< hardwareChannel/*channel*/ << " " << 1/*instantFTW*/ << " " << 0/*phaseJump*/ << " "
			//	<< 0.01/*amplitudePercent*/ << " " << -ampStepMag/*ATWIncr*/ << " "
			//	<< freq/*frequencyMHz*/ << " " << 0/*FTWIncr*/ << " "
			//	<< 0/*phaseDegrees*/ << std::endl;
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
		//std::cout << "setmove " << 3 * (nMoves - 1) + 2 + 2/*stepSequenceID*/ << " DAC1 "
		//	<< channel/*channel*/ << " " << 1/*instantFTW*/ << " " << 1/*phaseJump*/ << " "
		//	<< 0.01/*amplitudePercent*/ << " " << -ampStepMag/*ATWIncr*/ << " "
		//	<< 0/*frequencyMHz*/ << " " << 0/*FTWIncr*/ << " "
		//	<< 0/*phaseDegrees*/ << std::endl;
	}
}

void DynamicMoveManager::writeLoad(MessageSender& ms, unsigned variation)
{
	updataParameterForVariation(variation);
	auto [repeatX, repeatY] = moveParam.getRepeatXY(moveParam.nTweezerLoadX, moveParam.nTweezerLoadY);
	
	//Write load settings based on loadXY
	size_t iTweezerX = 0, iMaskX = 0;
	for (bool channelBool : moveParam.loadPositionsX) {
		if (iTweezerX >= MAX_XTONES / repeatX) {
			thrower("For safety, maximum number of x tones is limited to " + str(MAX_XTONES) + " in rearrangement mode");
		}
		if (channelBool) {
			double phase = fmod(180 * pow(iTweezerX + 1, 2) / moveParam.nTweezerX, 360); //this assumes comb of even tones.
			for (size_t r = 0; r < repeatX; r++) {
				size_t toneIdx = iTweezerX * repeatX + r;
				size_t hardwareChannel = (toneIdx * 8) % 48 + (toneIdx * 8) / 48;
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::LOADFREQUENCY)
					.frequencyMHz(moveLUT.getFreqX(iMaskX, 0))
					.amplitudePercent(moveLUT.getAmpX(iMaskX, 0))
					.phaseDegrees(phase);
				ms.enqueue(m);
				std::cout << "set DAC0 " << hardwareChannel << " " << moveLUT.getAmpX(iMaskX, 0) << " " << moveLUT.getFreqX(iMaskX, 0) << " " << phase << std::endl;
			}
			iTweezerX++;
		}
		iMaskX++;
	}

	size_t iTweezerY = 0, iMaskY = 0;
	for (bool channelBool : moveParam.loadPositionsY) {
		if (iTweezerY >= MAX_YTONES / repeatY) {
			thrower("Exceeded MAX_YTONES (" + str(MAX_YTONES) + ") in rearrangement mode");
		}
		if (channelBool) {
			double phase = fmod(180 * pow(iTweezerY + 1, 2) / moveParam.nTweezerY, 360);
			for (size_t r = 0; r < repeatY; ++r) {
				size_t toneIdx = iTweezerY * repeatY + r;
				size_t hardwareChannel = (toneIdx * 8) % 48 + (toneIdx * 8) / 48;
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::LOADFREQUENCY)
					.frequencyMHz(moveLUT.getFreqY(0, iMaskY))
					.amplitudePercent(moveLUT.getAmpY(0, iMaskY))
					.phaseDegrees(phase);
				ms.enqueue(m);
				std::cout << "set DAC1 " << hardwareChannel << " " << moveLUT.getAmpY(0, iMaskY) << " " << moveLUT.getFreqY(0, iMaskY) << " " << phase << std::endl;
			}
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

void DynamicMoveManager::checkTotalPower()
{
	auto [repeatX, repeatY] = moveParam.getRepeatXY(moveParam.nTweezerLoadX, moveParam.nTweezerLoadY);
	size_t iLoadTweezerX = 0, iLoadMaskX = 0;
	double totalLoadPowerX = 0.0, maxLoadPowerX = 0.0;
	for (bool channelBool : moveParam.loadPositionsX) {
		if (iLoadTweezerX >= MAX_XTONES / repeatX) {
			thrower("For safety, maximum number of x tones is limited to " + str(MAX_XTONES) + " in rearrangement mode");
		}
		if (channelBool) {
			totalLoadPowerX += repeatX * repeatX * moveLUT.getAmpX(iLoadMaskX, 0) * moveLUT.getAmpX(iLoadMaskX, 0);
			iLoadTweezerX++;
		}
		maxLoadPowerX += repeatX * repeatX * moveLUT.getAmpX(iLoadMaskX, 0) * moveLUT.getAmpX(iLoadMaskX, 0);
		iLoadMaskX++;
	}
	std::cout << "DynamicMoveManager::checkTotalPower: Total  power in X axis: " << str(totalLoadPowerX) << ", maximum power in X axis: " << str(maxLoadPowerX) << std::endl;

	size_t iLoadTweezerY = 0, iLoadMaskY = 0;
	double totalLoadPowerY = 0.0, maxLoadPowerY = 0.0;
	for (bool channelBool : moveParam.loadPositionsY) {
		if (iLoadTweezerY >= MAX_YTONES / repeatY) {
			thrower("For safety, maximum number of Y tones is limited to " + str(MAX_YTONES) + " in rearrangement mode");
		}
		if (channelBool) {
			totalLoadPowerY += repeatY * repeatY * moveLUT.getAmpY(0, iLoadMaskY) * moveLUT.getAmpY(0, iLoadMaskY);
			iLoadTweezerY++;
		}
		maxLoadPowerY += repeatY * repeatY * moveLUT.getAmpY(0, iLoadMaskY) * moveLUT.getAmpY(0, iLoadMaskY);
		iLoadMaskY++;
	}
	std::cout << "DynamicMoveManager::checkTotalPower: Total  power in Y axis: " << str(totalLoadPowerY) << ", maximum power in Y axis: " << str(maxLoadPowerY) << std::endl;

	//size_t iTweezerX = 0, iMaskX = 0;
	//double totalPowerX = 0.0, maxPowerX = 0.0;
	//for (bool channelBool : moveParam.initialPositionsX) {
	//	if (iTweezerX >= MAX_XTONES / moveParam.repeatX) {
	//		thrower("For safety, maximum number of x tones is limited to " + str(MAX_XTONES) + " in rearrangement mode");
	//	}
	//	if (channelBool) {
	//		totalPowerX += moveParam.repeatX * moveParam.repeatX * moveLUT.getAmpX(iMaskX, 0) * moveLUT.getAmpX(iMaskX, 0);
	//		iTweezerX++;
	//	}
	//	maxPowerX += moveParam.repeatX * moveParam.repeatX * moveLUT.getAmpX(iMaskX, 0) * moveLUT.getAmpX(iMaskX, 0);
	//	iMaskX++;
	//}
	//std::cout << "DynamicMoveManager::checkTotalPower: Total  power in X axis: " << str(totalPowerX) << ", maximum power in X axis: " << str(maxPowerX) << std::endl;

	//size_t iTweezerY = 0, iMaskY = 0;
	//double totalPowerY = 0.0, maxPowerY = 0.0;
	//for (bool channelBool : moveParam.initialPositionsY) {
	//	if (iTweezerY >= MAX_YTONES / moveParam.repeatY) {
	//		thrower("For safety, maximum number of Y tones is limited to " + str(MAX_YTONES) + " in rearrangement mode");
	//	}
	//	if (channelBool) {
	//		totalPowerY += moveParam.repeatY * moveParam.repeatY * moveLUT.getAmpY(iMaskY, 0) * moveLUT.getAmpY(iMaskY, 0);
	//		iTweezerY++;
	//	}
	//	maxPowerY += moveParam.repeatY * moveParam.repeatY * moveLUT.getAmpY(iMaskY, 0) * moveLUT.getAmpY(iMaskY, 0);
	//	iMaskY++;
	//}
	//std::cout << "DynamicMoveManager::checkTotalPower: Total  power in Y axis: " << str(totalPowerY) << ", maximum power in Y axis: " << str(maxPowerY) << std::endl;

	if (totalLoadPowerX > 1.1 * MAX_XPOWER) {
		thrower("Maximum power for the grid in the X axis is " + str(totalLoadPowerX) + ", and is greater than 1.5W."
			" If you believe it is fine, please change the alert threshold.");
	}
	if (totalLoadPowerY > 1.1 * MAX_YPOWER) {
		thrower("Maximum power for the grid in the Y axis is " + str(totalLoadPowerY) + ", and is greater than 1.5W."
			" If you believe it is fine, please change the alert threshold.");
	}

}

rearrangeParameters DynamicMoveManager::getRearrangeParameters()
{
	return moveParam;
}

bool DynamicMoveManager::isMoveActive()
{
	return moveActive;
}

void DynamicMoveManager::updataParameterForVariation(unsigned variation)
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
	if (moveActive) {
		moveParam.xOffset = moveParam.xOffsetManual[variation];
		moveParam.yOffset = moveParam.yOffsetManual[variation];
		moveLUT.setOffset(moveParam.xOffset, moveParam.yOffset);
	}
}

std::pair<std::vector<double>, std::vector<double>> DynamicMoveManager::getRawLUTs()
{
	return {moveLUT.getRawATWLUT(), moveLUT.getRawFTWLUT() };
}

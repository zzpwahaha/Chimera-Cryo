#pragma once
#include <string>
#include <vector>

struct rearrangeParameters {
	std::string rearrangeMode;
	int scrunchSpacing;
	int ampStepMag/* = 134217727*/;
	int freqStepMag/* = 511*/;

	double xOffset, yOffset;
	double xOffsetManual, yOffsetManual;

	unsigned nTweezerX, nTweezerY, nFilterTweezerX, nFilterTweezerY;
	std::vector<bool> initialPositionsX, initialPositionsY, initialPositions,
		filterPositionsX, filterPositionsY;
	unsigned targetNumber;
	std::vector<unsigned char> targetPositions;
};
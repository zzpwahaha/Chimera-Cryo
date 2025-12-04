#pragma once
#include <string>
#include <vector>

struct rearrangeParameters {
	std::string rearrangeMode;
	int scrunchSpacing;
	int ampStepMag/* = 134217727*/;
	int freqStepMag/* = 511*/;

	double xOffset, yOffset;
	std::vector<double> xOffsetManual, yOffsetManual;
	unsigned repeatX, repeatY;

	unsigned nTweezerLoadX, nTweezerLoadY, nTweezerX, nTweezerY, nFilterTweezerX, nFilterTweezerY;
	std::vector<bool> loadPositionsX, loadPositionsY, initialPositionsX, initialPositionsY, initialPositions,
		filterPositionsX, filterPositionsY;
	unsigned targetNumber;
	std::vector<unsigned char> targetPositions;
};
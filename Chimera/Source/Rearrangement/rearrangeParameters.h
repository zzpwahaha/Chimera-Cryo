#pragma once
#include <string>
#include <vector>

struct rearrangeParameters {
	unsigned rearrangeRound;
	std::vector<std::string> rearrangeMode;
	int scrunchSpacing;
	int ampStepMag/* = 134217727*/;
	int freqStepMag/* = 511*/;
	std::vector<int> ampStepMags, freqStepMags;

	double xOffset, yOffset;
	std::vector<double> xOffsetManual, yOffsetManual;
	unsigned singlexRepeatX, singlexRepeatY; // number of repeat when the move involves single x tone
	unsigned singleyRepeatX, singleyRepeatY; // number of repeat when the move involves single y tone

	unsigned nTweezerLoadX, nTweezerLoadY, nTweezerX, nTweezerY, nFilterTweezerX, nFilterTweezerY, nPaintTweezerX, nPaintTweezerY;
	std::vector<bool> loadPositionsX, loadPositionsY, initialPositionsX, initialPositionsY, initialPositions,
		filterPositionsX, filterPositionsY, paintPositionsX, paintPositionsY;
	std::vector<std::pair<int, int>> filterSegementsX, filterSegementsY;
	unsigned targetNumber;
	std::vector<unsigned char> targetPositions;
	inline std::pair<unsigned, unsigned> getRepeatXY(unsigned char nx, unsigned char ny) {
		unsigned x_val = (nx == 1) ? singlexRepeatX : singleyRepeatX;
		unsigned y_val = (ny == 1) ? singleyRepeatY : singlexRepeatY;
		return { x_val, y_val };
	};
};
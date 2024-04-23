#pragma once
#include <vector>

struct moveSingle {
	std::vector<signed char> startAOX; //negative values allow for special handling, e.g. removing atoms.
	std::vector<signed char> startAOY;
	std::vector<signed char> endAOX;
	std::vector<signed char> endAOY;

	inline unsigned char nx() {
		return startAOX.size();
	}; //number of active tweezers for AOD X
	inline unsigned char ny() {
		return startAOY.size();
	}; //number of active tweezers for AOD Y
};

struct moveSequence {
	std::vector<moveSingle> moves;
	inline unsigned char nMoves() {
		return moves.size();
	}
};

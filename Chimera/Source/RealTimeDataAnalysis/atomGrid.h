// created by Mark O. Brown
#pragma once

#include "GeneralObjects/coordinate.h"

struct atomGrid{
	coordinate gridOrigin = coordinate(0,0);
	unsigned long pixelSpacingX = 0;
	unsigned long pixelSpacingY = 0;
	unsigned long includedPixelX = 0; // =x, means include the rect of edge length of 2x+1 on this dimension, centered on the specified pixel.
	unsigned long includedPixelY = 0;
	// in atoms
	unsigned long width = 0;
	unsigned long height = 0;
	unsigned long numAtoms ( ){
		return width * height;
	}
	bool useFile = false;
	std::string fileName = std::string("");
	std::vector<std::vector<coordinate>> atomLocs; // the first layer is atom label, the second is the coords for each individual atom. Note those coords can be more than one for the purpose of maixmizing the SNR.
	static void loadGridFile(atomGrid& grid);
	static const std::string GRID_FILE_EXTENSION;
};


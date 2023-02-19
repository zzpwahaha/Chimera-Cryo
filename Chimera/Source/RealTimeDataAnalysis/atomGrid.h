// created by Mark O. Brown
#pragma once

#include "GeneralObjects/coordinate.h"

struct atomGrid{
	coordinate gridOrigin;
	unsigned long pixelSpacingX;
	unsigned long pixelSpacingY;
	unsigned long includedPixelX; // =x, means include the rect of edge length of 2x+1 on this dimension, centered on the specified pixel.
	unsigned long includedPixelY;
	// in atoms
	unsigned long width;
	unsigned long height;
	unsigned long numAtoms ( ){
		return width * height;
	}
};


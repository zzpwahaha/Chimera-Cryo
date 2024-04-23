#include "stdafx.h"
#include "rearrangeLUT.h"
#include <Python/cnpy.h>

void rearrangeLUT::refreshLUT()
{
	//load LUTs from .npy file
	cnpy::NpyArray arrAmpLUT = cnpy::npy_load(TWEEZER_AMPLITUDE_LUT_FILE_ADDRESS);
	std::vector<double> ampLUT = arrAmpLUT.as_vec<double>(); //load LUT as a flattened list of floats (row major)
	cnpy::NpyArray arrFreqLUT = cnpy::npy_load(TWEEZER_FREQUENCY_LUT_FILE_ADDRESS);
	std::vector<double> freqLUT = arrFreqLUT.as_vec<double>(); // (row major)
	//cnpy::NpyArray arrPaintAmpLUT = cnpy::npy_load(TWEEZER_PAINT_AMPLITUDE_LUT_FILE_LOCATION);
	//std::vector<double> paintAmpLUT = arrPaintAmpLUT.as_vec<double>(); //load LUT as a flattened list of floats (row major)
	//cnpy::NpyArray arrPaintMaskLUT = cnpy::npy_load(TWEEZER_PAINT_MASK_LUT_FILE_LOCATION);
	//std::vector<double> paintMaskLUT = arrPaintMaskLUT.as_vec<double>(); //load LUT as a flattened list of floats (row major)

	if (arrAmpLUT.shape[0] != arrFreqLUT.shape[0] ||
		arrAmpLUT.shape[1] != arrFreqLUT.shape[1] ||
		arrAmpLUT.shape[2] != arrFreqLUT.shape[2]) {
		thrower("Dimensions of amplitude LUT (" + str(arrAmpLUT.shape[0]) + ", " + str(arrAmpLUT.shape[1]) + ", " + str(arrAmpLUT.shape[2]) + "), "
			" and frequency LUT (" + str(arrFreqLUT.shape[0]) + ", " + str(arrFreqLUT.shape[1]) + ", " + str(arrFreqLUT.shape[2]) + "), do not match.");
	}

	xDim = arrAmpLUT.shape[0];
	yDim = arrAmpLUT.shape[1]; 
	//xDimPaint = arrPaintAmpLUT.shape[0];
	//yDimPaint = arrPaintAmpLUT.shape[1];
	//xDimPaintMask = arrPaintMaskLUT.shape[0];
	//yDimPaintMask = arrPaintMaskLUT.shape[1];

	ATW_LUT.clear();
	FTW_LUT.clear();
	for (auto amp : ampLUT) {
		ATW_LUT.push_back(amp);
		//ATW_LUT.push_back(getATW(amp));
	}

	for (auto freq : freqLUT) {
		FTW_LUT.push_back(freq);
		//FTW_LUT.push_back(getFTW(freq)); //TODO: switch LUTs back to tuning words for speed, after fixing the message builder nonsense.
	}
}

void rearrangeLUT::setOffset(double xOffset, double yOffset)
{
	this->xOffset = xOffset;
	this->yOffset = yOffset;
}

double rearrangeLUT::getFreqX(int xIndex, int yIndex)
{
	yIndex = (yIndex < 0) ? 0 : yIndex;
	if (xIndex == -1) { //special handling for atom removal
		return 60 + xOffset;
	}
	else if (xIndex == -2) {
		return 300 + xOffset;
	}
	else if (xIndex < xDim && xIndex >= 0 && yIndex < yDim && yIndex >= 0) {
		return FTW_LUT[2 * yDim * xIndex + 2 * yIndex + 0] + xOffset;
	}
	else {
		thrower("Invalid LUT index: (" + str(xIndex) + ", " + str(yIndex) + ") while looking up for FreqX.");
	}
}

double rearrangeLUT::getFreqY(int xIndex, int yIndex)
{
	xIndex = (xIndex < 0) ? 0 : xIndex;
	if (yIndex == -1) {//special handling for atom removal 
		return 60 + yOffset;
	}
	else if (yIndex == -2) {
		return 300 + yOffset;
	}
	else if (xIndex < xDim && xIndex >= 0 && yIndex < yDim && yIndex >= 0) {
		return FTW_LUT[2 * yDim * xIndex + 2 * yIndex + 1] + yOffset;
	}
	else {
		thrower("Invalid LUT index: (" + str(xIndex) + ", " + str(yIndex) + ") while looking up for FreqY.");
	}
}

double rearrangeLUT::getAmpX(int xIndex, int yIndex)
{
	yIndex = (yIndex < 0) ? 0 : yIndex;
	if (xIndex == -1 || xIndex == -2) { //special handling for atom removal
		return ATW_LUT[2 * yDim * 0 + 2 * yIndex + 0];
	}
	else if (xIndex >= 0 && yIndex >= 0) {
		return ATW_LUT[2 * yDim * xIndex + 2 * yIndex + 0];
	}
	else {
		thrower("Invalid LUT index: (" + str(xIndex) + ", " + str(yIndex) + ") while looking up for AmpX.");
	}
}

double rearrangeLUT::getAmpY(int xIndex, int yIndex)
{
	xIndex = (xIndex < 0) ? 0 : xIndex;
	if (yIndex == -1 || yIndex == -2) { //special handling for atom removal
		return ATW_LUT[2 * yDim * xIndex + 2 * 0 + 1];
	}
	else if (xIndex < xDim && xIndex >= 0 && yIndex < yDim && yIndex >= 0) {
		return ATW_LUT[2 * yDim * xIndex + 2 * yIndex + 1];
	}
	else {
		thrower("Invalid LUT index: (" + str(xIndex) + ", " + str(yIndex) + ") while looking up for AmpY.");
	}
}

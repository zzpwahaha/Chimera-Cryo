#include "stdafx.h"
#include "rearrangeLUT.h"
#include <algorithm>
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

	if (arrFreqLUT.shape[0] > 127 || arrFreqLUT.shape[1] > 127) {
		thrower("Dimensions of amplitude LUT (" + str(arrAmpLUT.shape[0]) + ", " + str(arrAmpLUT.shape[1]) + ", " + str(arrAmpLUT.shape[2]) + "), "
			" and frequency LUT (" + str(arrFreqLUT.shape[0]) + ", " + str(arrFreqLUT.shape[1]) + ", " + str(arrFreqLUT.shape[2]) + "), are greater than 127, which exceeds the LUT limited size. "
			"Need to change the type for startAOX, startAOY, endAOX, endAOY.");
	}
	xDim = arrAmpLUT.shape[1];
	yDim = arrAmpLUT.shape[0]; 
	freqSpacingX = abs(freqLUT[2 * xDim * 0/*yIndex*/ + 2 * 1/*xIndex*/ + 0] - freqLUT[2 * xDim * 0/*yIndex*/ + 2 * 0/*xIndex*/ + 0]);
	freqSpacingY = abs(freqLUT[2 * xDim * 1/*yIndex*/ + 2 * 0/*xIndex*/ + 1] - freqLUT[2 * xDim * 0/*yIndex*/ + 2 * 0/*xIndex*/ + 1]);
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

//double rearrangeLUT::getFreqX(int xIndex, int yIndex) {
//	yIndex = (yIndex < 0) ? 0 : yIndex; 
//	if (xIndex == -1) { //special handling for atom removal 
//		return 85 + xOffset;
//	}
//	else if (xIndex == -2) { 
//		return 107 + xOffset; 
//	}
//	else if (xIndex < xDim && xIndex >= 0 && yIndex < yDim && yIndex >= 0) { 
//		return roundToTwoDecimalPlaces(FTW_LUT[2 * xDim * yIndex + 2 * xIndex + 0] + xOffset); 
//	}
//	else { 
//		thrower("Invalid LUT index: (" + str(xIndex) + ", " + str(yIndex) + ") while looking up for FreqX."); 
//	}
//}

double rearrangeLUT::getFreqX(int xIndex, int yIndex)
{
	int y = std::clamp(yIndex, 0, static_cast<int>(yDim) - 1);
	// In-range LUT lookup
	if (xIndex < xDim && xIndex >= 0) {
		return roundToTwoDecimalPlaces(FTW_LUT[2 * xDim * y + 2 * xIndex + 0] + xOffset);
	}
	// ---------- X extrapolation ----------
	if (xIndex < 0) {
		// extrapolate from left boundary but physically the tones is on the larger frequency side, hence the minus sign
		double f0 = FTW_LUT[2 * xDim * y + 2 * 0 + 0];
		return roundToTwoDecimalPlaces(f0 - xIndex * freqSpacingX + xOffset);
	}
	else {
		// extrapolate from right boundary but physically the tones is on the lower frequency side, hence the minus sign
		double fMax = FTW_LUT[2 * xDim * y + 2 * (xDim - 1) + 0];
		return roundToTwoDecimalPlaces(fMax - (xIndex - (xDim - 1)) * freqSpacingX + xOffset);
	}
}

//double rearrangeLUT::getFreqY(int xIndex, int yIndex)
//{
//	xIndex = (xIndex < 0) ? 0 : xIndex;
//	if (yIndex == -1) {//special handling for atom removal 
//		return 75 + yOffset;
//	}
//	else if (yIndex == -2) {
//		return 125 + yOffset;
//	}
//	else if (xIndex < xDim && xIndex >= 0 && yIndex < yDim && yIndex >= 0) {
//		return roundToTwoDecimalPlaces(FTW_LUT[2 * xDim * yIndex + 2 * xIndex + 1] + yOffset);
//	}
//	else {
//		thrower("Invalid LUT index: (" + str(xIndex) + ", " + str(yIndex) + ") while looking up for FreqY.");
//	}
//}

double rearrangeLUT::getFreqY(int xIndex, int yIndex)
{
	// Clamp X (FreqY depends on Y; X is just a selector)
	int x = std::clamp(xIndex, 0, static_cast<int>(xDim) - 1);
	// In-range LUT lookup
	if (yIndex >= 0 && yIndex < yDim) {
		return roundToTwoDecimalPlaces(FTW_LUT[2 * xDim * yIndex + 2 * x + 1] + yOffset);
	}
	// ---------- Y extrapolation ----------
	if (yIndex < 0) {
		// extrapolate from bottom boundary
		double f0 = FTW_LUT[2 * xDim * 0 + 2 * x + 1];
		return roundToTwoDecimalPlaces(f0 + yIndex * freqSpacingY + yOffset);
	}
	else {
		// extrapolate from top boundary
		double fMax = FTW_LUT[2 * xDim * (yDim - 1) + 2 * x + 1];
		return roundToTwoDecimalPlaces(fMax + (yIndex - (yDim - 1)) * freqSpacingY + yOffset);
	}
}

//double rearrangeLUT::getAmpX(int xIndex, int yIndex)
//{
//	yIndex = (yIndex < 0) ? 0 : yIndex;
//	if (xIndex == -1 || xIndex == -2) { //special handling for atom removal
//		return ATW_LUT[2 * xDim * yIndex + 2 * 0 + 0];
//	}
//	else if (xIndex >= 0 && yIndex >= 0) {
//		return ATW_LUT[2 * xDim * yIndex + 2 * xIndex + 0];
//	}
//	else {
//		thrower("Invalid LUT index: (" + str(xIndex) + ", " + str(yIndex) + ") while looking up for AmpX.");
//	}
//}

double rearrangeLUT::getAmpX(int xIndex, int yIndex)
{
	// Clamp yIndex to valid range
	int y = std::clamp(yIndex, 0, static_cast<int>(yDim) - 1);
	// Clamp xIndex to valid range (use edge values if out of bounds)
	int x = std::clamp(xIndex, 0, static_cast<int>(xDim) - 1);
	return ATW_LUT[2 * xDim * y + 2 * x + 0];
}

//double rearrangeLUT::getAmpY(int xIndex, int yIndex)
//{
//	xIndex = (xIndex < 0) ? 0 : xIndex;
//	if (yIndex == -1 || yIndex == -2) { //special handling for atom removal
//		return ATW_LUT[2 * xDim * 0 + 2 * xIndex + 1];
//	}
//	else if (xIndex < xDim && xIndex >= 0 && yIndex < yDim && yIndex >= 0) {
//		return ATW_LUT[2 * xDim * yIndex + 2 * xIndex + 1];
//	}
//	else {
//		thrower("Invalid LUT index: (" + str(xIndex) + ", " + str(yIndex) + ") while looking up for AmpY.");
//	}
//}

double rearrangeLUT::getAmpY(int xIndex, int yIndex)
{
	// Clamp xIndex to valid range
	int x = std::clamp(xIndex, 0, static_cast<int>(xDim) - 1);
	// Clamp yIndex to valid range (use edge values if out of bounds)
	int y = std::clamp(yIndex, 0, static_cast<int>(yDim) - 1);
	return ATW_LUT[2 * xDim * y + 2 * x + 1];
}

std::vector<double> rearrangeLUT::getRawATWLUT()
{
	return ATW_LUT;
}

std::vector<double> rearrangeLUT::getRawFTWLUT()
{
	return FTW_LUT;
}

double rearrangeLUT::roundToTwoDecimalPlaces(double value)
{
	return value;
	//return std::round(value * 100.0) / 100.0;
}

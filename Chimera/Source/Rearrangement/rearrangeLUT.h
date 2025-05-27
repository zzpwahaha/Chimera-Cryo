#pragma once

class rearrangeLUT
{
public:
	void refreshLUT();
	void setOffset(double xOffset, double yOffset);
	unsigned getXDim() { return xDim; };
	unsigned getYDim() { return yDim; };
	double getFreqX(int xIndex, int yIndex);
	double getFreqY(int xIndex, int yIndex);
	double getAmpX(int xIndex, int yIndex);
	double getAmpY(int xIndex, int yIndex);

private:

public:
	const std::string TWEEZER_AMPLITUDE_LUT_FILE_ADDRESS = str(CODE_ROOT) + "\\TweezerLUT\\ampLUT.npy";
	const std::string TWEEZER_FREQUENCY_LUT_FILE_ADDRESS = str(CODE_ROOT)+ "\\TweezerLUT\\freqLUT.npy";


private:
	unsigned xDim, yDim;
	double xOffset, yOffset;
	std::vector<double> ATW_LUT, FTW_LUT;
};


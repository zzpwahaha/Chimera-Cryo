#pragma once
#include <string>
#include <vector>
#include <ArbGen/whichAg.h>
#include <AnalogInput/BSplineFit.h>


struct calBase {
	static std::string dblVecToString (std::vector<double> ctrls) {
		std::string ctrlString;
		for (auto val : ctrls) {
			ctrlString += str (val, 8, true) + " ";
		}
		return ctrlString;
	}
};

struct calResult : public calBase {
	std::string calibrationName = "";
	bool active = false; // true if used in experiment
	bool currentActive = false; // true if used in the evaluate for the particular command, used only in expthread for locating which cal is used for eval.
	//unsigned polynomialOrder = 0;
	std::vector<double> ctrlVals;
	std::vector<double> resVals;
	std::vector<double> calibrationCoefficients;
	//bool includesSqrt=true;
	double calmin = 0;
	double calmax = 0;
	unsigned orderBSpline = 0;
	unsigned nBreak = 0;
	BSplineFit bsfit;
	void fillCalibrationResult() {
		calibrationCoefficients = bsfit.getfittedPara();
		auto bsPara = bsfit.getBSplinePara();
		orderBSpline = bsPara[0];
		nBreak = bsPara[2];
	}
	std::string stringRepr () const {
		return calibrationName + " Calibration:"
			+ "\n================================"
			+ "\nControl Values: " + dblVecToString(ctrlVals)
			+ "\nResult Values: " + dblVecToString(resVals)
			+ "\nCalibration Coefficients: " + dblVecToString(calibrationCoefficients)
			+ "\nBSpline order: " + str(orderBSpline)
			+ "\nBSpline number of break points: " + str(nBreak)
			+ "\nCalibration result Min/Max: " + str (calmin, 3) + "/" + str (calmax, 3);
	}
};

struct calSettings : public calBase {
	bool active = false;
	bool usedSameChannel = true; // if the cal is used in the experiment script with the same output channel as in the calibration
	unsigned int aiInChan = 0; // this is the number on the AI box, 0-15 -> A0,B0,A1,B1,...A7,B7
	unsigned int aoControlChannel = 0;
	bool useAg = false;
	ArbGenEnum::name whichAg;
	unsigned agChannel = 1;
	QString ctrlPtString;
	// in % from old value
	bool calibrated = false;
	std::vector<std::pair<unsigned, unsigned> > ttlConfig;
	std::vector<std::pair<unsigned, double>> aoConfig;
	bool currentlyCalibrating = false;
	unsigned int avgNum = 100;
	// importantly, this value reflects the setting for the next calibration, whereas the version in the result 
	// refers to whether that particular result includes the sqrt. 
	//bool includeSqrt = true;
	calResult result;
	calResult historicalResult;
};

Q_DECLARE_METATYPE (calSettings)
Q_DECLARE_METATYPE (std::vector<calSettings>)
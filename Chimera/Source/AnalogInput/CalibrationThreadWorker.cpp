#include <stdafx.h>
#include <AnalogInput/CalibrationManager.h>
#include <AnalogInput/CalibrationThreadWorker.h>
#include <AnalogInput/calInfo.h>
#include "AnalogInput/AiSystem.h"
#include <AnalogInput/CalibrationThreadWorker.h>
#include <AnalogInput/PolynomialFit.h>
#include "AnalogOutput/AoSystem.h"
#include "DigitalOutput/DoSystem.h"
#include "ParameterSystem/ParameterSystem.h"
#include "ConfigurationSystems/Version.h"
#include <CustomQtControls/AutoNotifyCtrls.h>
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <ArbGen/ArbGenCore.h>
#include <Python/NewPythonHandler.h>
#include <Plotting/PlotCtrl.h>


CalibrationThreadWorker::CalibrationThreadWorker (CalibrationThreadInput input_) {
	
	input = input_;
}

CalibrationThreadWorker::~CalibrationThreadWorker () {
}

void CalibrationThreadWorker::runAll () {
	unsigned count = 0;
	// made this asynchronous to facilitate updating gui while 
	emit updateBoxColor("Green", "AI-SYSTEM");
	for (auto& cal : input.calibrations) {
		try {
			calibrate (cal, count);
		}
		catch (ChimeraError & e) {
			emit error (qstr (e.trace ()));
			// but continue to try the other ones. 
		}
		Sleep (200);
		count++;
	}
	input.ttls->zeroBoard ();
	input.ao->zeroDacs (/*input.ttls->getCore (), { 0, input.ttls->getCurrentStatus () }*/);
	emit updateBoxColor("Gray", "AI-SYSTEM");
}

void CalibrationThreadWorker::calibrate (calSettings& cal, unsigned which) {
	if (!cal.active) {
		return;
	}
	auto& result = cal.result;
	emit startingNewCalibration (cal);
	emit notification(qstr("Running Calibration " + result.calibrationName + ".\n"));
	cal.currentlyCalibrating = true;
	//cal.result.includesSqrt = cal.includeSqrt;
	input.ttls->zeroBoard ();
	input.ao->zeroDacs ();
	for (auto dac : cal.aoConfig) {
		input.ao->setSingleDac (dac.first, dac.second);
	}
	for (auto ttl : cal.ttlConfig) {
		auto& outputs = input.ttls->getDigitalOutputs();
		outputs(ttl.first, ttl.second).set(true, false); //TODO: check compatibility for row and col
	}
	input.ttls->getCore().FPGAForceOutput(input.ttls->getCurrentStatus());
	Sleep (100); // give some time for the lasers to settle..
	result.resVals.clear ();
	unsigned aiNum = cal.aiInChan;
	unsigned aoNum = cal.aoControlChannel;
	result.ctrlVals = CalibrationManager::calPtTextToVals (cal.ctrlPtString);
	for (auto calPoint : result.ctrlVals) {
		try {
			if (cal.useAg) {
				auto& ag = input.arbGens[int(cal.whichAg)].get();
				dcInfo tempInfo;
				tempInfo.dcLevel = str (calPoint);
				tempInfo.dcLevel.internalEvaluate (std::vector<parameterType> (), 1);
				ag.setDC (cal.agChannel, tempInfo, 0);
				ag.outputOn(cal.agChannel);
			}
			else {
				input.ao->setSingleDac (aoNum, calPoint);
			}
		}
		catch (ChimeraError & err) {
			emit error (err.qtrace ());
		}
		Sleep(100); // give some time for the analog output to change and settle..
		// try maxTries to read before yell out error
		int count = 0;
		int maxTries = 3;
		while (true) {
			try {
				result.resVals.push_back(input.ai->getSingleChannelValue(aiNum, cal.avgNum));
				break;
			}
			catch (ChimeraError& e) {
				if (++count == maxTries) throw e;
			}
		}

		emit newCalibrationDataPoint (QPointF (calPoint, result.resVals.back ()));
		Sleep (20);
	}

	try {
		if (cal.useAg) {
			auto& ag = input.arbGens[int(cal.whichAg)].get();
			ag.outputOn(cal.agChannel);
		}
	}
	catch (ChimeraError& err) {
		emit error(err.qtrace());
	}

	CalibrationManager::determineCalMinMax (cal.result);
	cal.currentlyCalibrating = false;
	std::ofstream file (str(CODE_ROOT)+"\\Data-Analysis-Code\\CalibrationValuesFile.txt");
	if (!file.is_open ()) {
		thrower ("Failed to Write Calibration Results!");
	}
	for (auto num : range (result.resVals.size ())) {
		file << result.ctrlVals[num] << " " << result.resVals[num] << "\n";
	}
	file.close ();
	//result.calibrationCoefficients = input.pythonHandler->runCalibrationFits (cal, input.parentWin);
	//PolynomialFit<cal.includeSqrt, 5> fitworker;
	//std::vector<double> initP(1 + result.polynomialOrder + 2 * result.includesSqrt, 0.0);
	//initP[0] = result.calmin;
	//if (result.includesSqrt) {
	//	initP[result.polynomialOrder + 2 * result.includesSqrt] = result.ctrlVals[0] - 0.5;
	//}
	//PolynomialFit fitWorker(result.ctrlVals.size(),  result.resVals.data(), result.ctrlVals.data(),
	//	result.polynomialOrder, result.includesSqrt,initP);
	//fitWorker.solve_system();
	//result.calibrationCoefficients = fitWorker.fittedPara();
	result.bsfit.initialize(result.ctrlVals.size(), result.resVals, result.ctrlVals, 
		result.orderBSpline, result.nBreak);
	result.bsfit.solve_system();
	result.fillCalibrationResult();
	//result.includesSqrt = cal.includeSqrt;
	//calibrationTable->repaint ();
	cal.calibrated = true;
	if (cal.useAg) {
		auto& ag = input.arbGens[int(cal.whichAg)].get();
		ag.setAgCalibration (result, cal.agChannel);
	}
	emit calibrationChanged ();
	emit finishedCalibration (cal);
}

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
#include <Plotting/PlotInfo.h>


CalibrationThreadWorker::CalibrationThreadWorker (CalibrationThreadInput input_) {
	
	input = input_;
}

CalibrationThreadWorker::~CalibrationThreadWorker () {
}

void CalibrationThreadWorker::runAll () {
	unsigned count = 0;
	// made this asynchronous to facilitate updating gui while 
	emit updateBoxColor("Green", "AI-SYSTEM");
	DOStatus doInitStatus = input.ttls->getCurrentStatus();
	auto aoInitStatus = input.ao->getDacValues();
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
	input.ao->setDacStatus(aoInitStatus);
	Sleep(50);
	input.ttls->setTtlStatus(doInitStatus);
	emit updateBoxColor("Gray", "AI-SYSTEM");
	emit mainProcessFinish();
}

void CalibrationThreadWorker::calibrate (calSettings& cal, unsigned which) {
	if (!cal.active) {
		cal.calibrated = false;
		return;
	}
	emit startingNewCalibration(cal);
	emit notification(qstr("Running Calibration " + cal.result.calibrationName + ".\n"));
	cal.calibrated = false;
	//auto& result = cal.result;
	calResult result(cal.result); // copy cal.result in case the calibration failed, in which case it should/will fall back to the old calibration value

	//cal.result.includesSqrt = cal.includeSqrt;
	input.ao->zeroDacs ();
	Sleep(50);
	input.ttls->zeroBoard(); // ZZP 02/12/2023 - need ttls to trigger dac change in zynq, same as bellow
	Sleep(50);
	for (auto dac : cal.aoConfig) {
		input.ao->setSingleDac (dac.first, dac.second);
		input.ttls->getCore().FPGAForceOutput(input.ttls->getCurrentStatus()); // ZZP 02/12/2023 - update ttls since now dac/dds gui update will always need separate TTL update as zynq trigger
		Sleep(50);
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
		if (cal.useAg) {
			auto& ag = input.arbGens[int(cal.whichAg)].get();
			dcInfo tempInfo;
			tempInfo.dcLevel = str (calPoint);
			std::vector<parameterType> var = std::vector<parameterType>();
			tempInfo.dcLevel.internalEvaluate (var, 1);
			ag.setDC (cal.agChannel, tempInfo, 0);
			ag.outputOn(cal.agChannel);
		}
		else {
			input.ao->setSingleDac (aoNum, calPoint);
			input.ttls->getCore().FPGAForceOutput(input.ttls->getCurrentStatus()); // ZZP 02/12/2023 - need ttls to trigger dac change in zynq, same as bellow
			Sleep(50);
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

	try {
		result.bsfit.initialize(result.ctrlVals.size(), result.resVals, result.ctrlVals,
			result.orderBSpline, result.nBreak);
		result.bsfit.solve_system();
		result.fillCalibrationResult();
	}
	catch (...) {
		throwNested("B-Spline fitting failed, looks like the data is not valid, please calibrate it again.");
	}

	//result.includesSqrt = cal.includeSqrt;
	cal.result = result;
	cal.calibrated = true;
	if (cal.useAg) {
		auto& ag = input.arbGens[int(cal.whichAg)].get();
		ag.setAgCalibration (result, cal.agChannel);
	}
	emit calibrationChanged ();
	emit finishedCalibration (cal);
}

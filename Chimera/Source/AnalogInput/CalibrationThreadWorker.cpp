#include <stdafx.h>
#include <AnalogInput/CalibrationManager.h>
#include <AnalogInput/CalibrationThreadWorker.h>
#include <AnalogInput/calInfo.h>
#include "AnalogInput/AiSystem.h"
#include <AnalogInput/CalibrationThreadWorker.h>
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
}

void CalibrationThreadWorker::calibrate (calSettings& cal, unsigned which) {
	if (!cal.active) {
		return;
	}
	auto& result = cal.result;
	emit startingNewCalibration (cal);
	emit notification(qstr("Running Calibration " + result.calibrationName + ".\n"));
	cal.currentlyCalibrating = true;
	input.ttls->zeroBoard ();
	input.ao->zeroDacs ();
	for (auto dac : cal.aoConfig) {
		input.ao->setSingleDac (dac.first, dac.second);
	}
	for (auto ttl : cal.ttlConfig) {
		auto& outputs = input.ttls->getDigitalOutputs();
		outputs(ttl.first, ttl.second).set(true); //TODO: check compatibility for row and col
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
			}
			else {
				input.ao->setSingleDac (aoNum, calPoint);
			}
		}
		catch (ChimeraError & err) {
			emit error (err.qtrace ());
		}
		result.resVals.push_back (input.ai->getSingleChannelValue (aiNum, cal.avgNum));
		emit newCalibrationDataPoint (QPointF (calPoint, result.resVals.back ()));
		Sleep (20);
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
	result.calibrationCoefficients = input.pythonHandler->runCalibrationFits (cal, input.parentWin);
	result.includesSqrt = cal.includeSqrt;
	//calibrationTable->repaint ();
	cal.calibrated = true;
	if (cal.useAg) {
		auto& ag = input.arbGens[int(cal.whichAg)].get();
		ag.setAgCalibration (result, cal.agChannel);
	}
	emit calibrationChanged ();
	emit finishedCalibration (cal);
}

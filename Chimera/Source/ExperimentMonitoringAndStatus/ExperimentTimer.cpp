// created by Mark O. Brown
#include "stdafx.h"
#include "ExperimentTimer.h"
#include "PrimaryWindows/QtAndorWindow.h"
#include <qprogressbar.h>

void ExperimentTimer::initialize( IChimeraQtWindow* parent ){
	this->setParent(parent);
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	timeDisplay = new QLabel ("%", this);
	layout->addWidget(timeDisplay);
	/// PROGRESS BARS
	// subseries progress bar
	QVBoxLayout* layout1 = new QVBoxLayout(this);
	layout1->setContentsMargins(0, 0, 0, 0);
	long timerWidth = 550 * 2;
	repetitionProgress = new QProgressBar (this);
	repetitionProgress->setRange (0, 10000);
	// series progress bar display
	variationProgress = new QProgressBar (this);
	variationProgress->setRange (0, 10000);
	layout1->addWidget(repetitionProgress);
	layout1->addWidget(variationProgress);
	layout->addLayout(layout1);
}

void ExperimentTimer::update(unsigned __int64 currentExpNumber, unsigned __int64 repsPerVariation, unsigned __int64 numberOfVariations, unsigned picsPerRep, bool repFirst)
{
	int totalExperiments = repsPerVariation * numberOfVariations;
	int minAverageNumber = 10;
	int repetitionPosition, variationPosition;
	if (repFirst) {
		repetitionPosition = int(((currentExpNumber % repsPerVariation) + 1) * 10000.0 / repsPerVariation);
		variationPosition = int((currentExpNumber / repsPerVariation + 1) * 10000.0 / numberOfVariations);
	}
	else {
		repetitionPosition = int(((currentExpNumber / numberOfVariations) + 1) * 10000.0 / repsPerVariation);
		variationPosition = int((currentExpNumber % numberOfVariations + 1) * 10000.0 / numberOfVariations);
	}
	repetitionProgress->setValue(repetitionPosition);
	variationProgress->setValue(variationPosition);
	if (currentExpNumber == 0)
	{
		firstTime = GetTickCount64();
		timeDisplay->setText( "Estimating Time..." );
	}
	else if (currentExpNumber < totalExperiments)
	{
		long long thisTime = GetTickCount64();
		double averageTime = (thisTime - firstTime) / double(currentExpNumber);
		// in seconds...
		int timeLeft = int((repsPerVariation * numberOfVariations - currentExpNumber) * averageTime / 1000);
		int hours = timeLeft / 3600;
		int minutes = (timeLeft % 3600) / 60;
		int seconds = (timeLeft % 3600) % 60;
		std::string timeString = "";
		timeString += str(hours) + ":";
		if (minutes < 10) {
			timeString += "0" + str(minutes);
		}
		else {
			timeString += str(minutes);
		}
		if (hours == 0 && minutes < 5) {
			if (seconds < 10) {
				timeString += ":0" + str(seconds);
			}
			else {
				timeString += ":" + str(seconds);
			}
		}
		timeDisplay->setText(cstr(timeString));
	}
	else
	{
		timeDisplay->setText( "FIN!" );
	}
}


void ExperimentTimer::setTimerDisplay(std::string newText)
{
	timeDisplay->setText( cstr(newText) );
}
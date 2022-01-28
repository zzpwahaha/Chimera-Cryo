// created by Mark O. Brown
#include "stdafx.h"
#include <algorithm>
#include <numeric>
#include "PictureStats.h"
#include <qlayout.h>


// as of right now, the position of this control is not affected by the mode or the trigger mode.
void PictureStats::initialize( IChimeraQtWindow* parent )
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	pictureStatsHeader = new QLabel ("Raw Counts", parent);
	repetitionIndicator = new QLabel ("Repetition ?/?", parent);
	layout->addWidget(pictureStatsHeader);
	layout->addWidget(repetitionIndicator);
	/// Picture labels ////////////////////////////////////////////////////////////
	QGridLayout* layout1 = new QGridLayout(this);
	layout1->setContentsMargins(0, 0, 0, 0);
	collumnHeaders[0] = new QLabel ("Pic:", parent);
	int inc = 0;
	for (auto& control : picNumberIndicators)
	{
		inc++;
		control = new QLabel (cstr ("#" + str (inc) + ":"), parent);
		layout1->addWidget(control, inc, 0);
	}
	/// Max Count 
	collumnHeaders[1] = new QLabel ("Max:", parent);
	inc = 0;
	for (auto& control : maxCounts) {
		inc++;
		control = new QLabel ("-", parent);
		layout1->addWidget(control, inc, 1);
	}
	/// Min Counts
	collumnHeaders[2] = new QLabel ("Min:", parent);
	inc = 0;
	for (auto& control : minCounts) {
		inc++;
		control = new QLabel ("-", parent);
		layout1->addWidget(control, inc, 2);
	}
	/// Average Counts
	collumnHeaders[3] = new QLabel ("Avg:", parent);
	inc = 0;
	for (auto& control : avgCounts) {
		inc++;
		control = new QLabel ("-", parent);
		layout1->addWidget(control, inc, 3);
	}
	/// Selection Counts
	collumnHeaders[4] = new QLabel ("Avg:", parent);
	inc = 0;
	for (auto& control : selCounts) {
		inc++;
		control = new QLabel ("-", parent);
		layout1->addWidget(control, inc, 4);
	}
	for (auto idx : range(collumnHeaders.size())) {
		layout1->addWidget(collumnHeaders[idx], 0, idx);
	}
	layout->addLayout(layout1);
}


void PictureStats::reset(){
	for (auto& control : maxCounts)	{
		control->setText("-");
	}
	for (auto& control : minCounts)	{
		control->setText ("-");
	}
	for (auto& control : avgCounts)	{
		control->setText ("-");
	}
	for (auto& control : selCounts){
		control->setText ("-");
	}
	repetitionIndicator->setText ( "Repetition ---/---" );
}


void PictureStats::updateType(std::string typeText){
	displayDataType = typeText;
	pictureStatsHeader->setText (cstr(typeText));
}


statPoint PictureStats::getMostRecentStats ( ){
	return mostRecentStat;
}


std::pair<int, int> PictureStats::update ( Matrix<long> image, unsigned imageNumber, coordinate selectedPixel, 
										   int currentRepetitionNumber, int totalRepetitionCount ){
	repetitionIndicator->setText ( cstr ( "Repetition " + str ( currentRepetitionNumber ) + "/"
												+ str ( totalRepetitionCount ) ) );
	if ( image.size ( ) == 0 ){
		// hopefully this helps with stupid imaging bug...
		return { 0,0 };
	}
	statPoint currentStatPoint;
	currentStatPoint.selv = image ( selectedPixel.row, selectedPixel.column );
	currentStatPoint.minv = 65536;
	currentStatPoint.maxv = 1;
	// for all pixels... find the max and min of the picture.
	for (auto pixel : image)	{
		try	{
			if ( pixel > currentStatPoint.maxv ){
				currentStatPoint.maxv = pixel;
			}
			if ( pixel < currentStatPoint.minv ){
				currentStatPoint.minv = pixel;
			}
		}
		catch ( std::out_of_range& ){
			// I haven't seen this error in a while, but it was a mystery when we did.
			errBox ( "ERROR: caught std::out_of_range while updating picture statistics! experimentImagesInc = "
					 + str ( imageNumber ) + ", pixelInc = " + str ( "NA" ) + ", image.size() = " + str ( image.size ( ) )
					 + ". Attempting to continue..." );
			return { 0,0 };
		}
	}
	currentStatPoint.avgv = std::accumulate ( image.data.begin ( ), image.data.end ( ), 0.0 ) / image.size ( );

	if ( displayDataType == RAW_COUNTS ){
		maxCounts[ imageNumber ]->setText ( cstr ( currentStatPoint.maxv, 1 ) );
		minCounts[ imageNumber ]->setText ( cstr ( currentStatPoint.minv, 1 ) );
		selCounts[ imageNumber ]->setText ( cstr ( currentStatPoint.selv, 1 ) );
		avgCounts[ imageNumber ]->setText ( cstr ( currentStatPoint.avgv, 5 ) );
		mostRecentStat = currentStatPoint;
	}
	else if ( displayDataType == CAMERA_PHOTONS ){
		statPoint camPoint;
		//double selPhotons, maxPhotons, minPhotons, avgPhotons;
		//if (eEMGainMode)
		if ( false ){
			camPoint = (currentStatPoint - convs.EMGain200BackgroundCount ) * convs.countToCameraPhotonEM200;
		}
		else{
			camPoint = ( currentStatPoint - convs.conventionalBackgroundCount ) * convs.countToCameraPhoton;
		}
		maxCounts[ imageNumber ]->setText ( cstr ( camPoint.maxv, 1 ) );
		minCounts[ imageNumber ]->setText ( cstr ( camPoint.minv, 1 ) );
		selCounts[ imageNumber ]->setText ( cstr ( camPoint.selv, 1 ) );
		avgCounts[ imageNumber ]->setText ( cstr ( camPoint.avgv, 1 ) );
		mostRecentStat = camPoint;
	}
	else if ( displayDataType == ATOM_PHOTONS ){
		statPoint atomPoint;
		//if (eEMGainMode)
		if ( false ){
			atomPoint = ( currentStatPoint - convs.EMGain200BackgroundCount ) * convs.countToScatteredPhotonEM200;
		}
		else{
			atomPoint = ( currentStatPoint - convs.conventionalBackgroundCount ) * convs.countToScatteredPhoton;
		}
		maxCounts[ imageNumber ]->setText ( cstr ( atomPoint.maxv, 1 ) );
		minCounts[ imageNumber ]->setText ( cstr ( atomPoint.minv, 1 ) );
		selCounts[ imageNumber ]->setText ( cstr ( atomPoint.selv, 1 ) );
		avgCounts[ imageNumber ]->setText ( cstr ( atomPoint.avgv, 1 ) );
		mostRecentStat = atomPoint;
	}	
	return { currentStatPoint.minv, currentStatPoint.maxv };
}
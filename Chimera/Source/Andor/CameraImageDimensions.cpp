// created by Mark O. Brown
#include "stdafx.h"
#include "CameraImageDimensions.h"
#include "PrimaryWindows/QtAndorWindow.h"
#include "PrimaryWindows/QtMainWindow.h"
#include "AndorCameraSettingsControl.h"
#include <PrimaryWindows/QtAndorWindow.h>
#include <boost/lexical_cast.hpp>

ImageDimsControl::ImageDimsControl (std::string whichCam) : camType (whichCam) {
	isReady = false;
}

void ImageDimsControl::initialize(/*CQComboBox* binningCombo, */IChimeraQtWindow* parent) {
	QGridLayout* layout = new QGridLayout(this);
	leftText = new QLabel ("Left", parent);
	rightText = new QLabel ("Right (/2048)", parent);
	horBinningText = new QLabel ("H. Bin", parent);
	bottomLabel = new QLabel ("Bottom (/2048)", parent);
	topLabel = new QLabel ("Top", parent);
	vertBinningText = new QLabel ("V. Bin", parent);
	leftEdit = new CQLineEdit ("1", parent);
	rightEdit = new CQLineEdit ("5", parent);
	horBinningEdit = new CQLineEdit ("1", parent);
	bottomEdit = new CQLineEdit ("1", parent);
	topEdit = new CQLineEdit ("5", parent);
	vertBinningEdit = new CQLineEdit ("1", parent);
	
	//binningLabel = new QLabel("Bin. Opts", parent);


	int idx = 0;
	for (auto l : { leftText ,rightText ,horBinningText,bottomLabel,topLabel,vertBinningText, /*binningLabel*/}) {
		layout->addWidget(l, 0, idx);
		idx++;
	}
	idx = 0;
	for (auto l : { leftEdit ,rightEdit ,horBinningEdit,bottomEdit,topEdit,vertBinningEdit, }) {
		layout->addWidget(l, 1, idx);
		idx++;
	}
	//layout->addWidget(binningCombo, 1, idx);
	//binningComboCopy = binningCombo;
}

void ImageDimsControl::saveParams (ConfigStream& saveFile, imageParameters params) {
	saveFile << "\nCAMERA_IMAGE_DIMENSIONS"
			 << "\n/*Left:*/ " << params.left
			 << "\n/*Right:*/ " << params.right
			 << "\n/*H-Bin:*/ " << params.horizontalBinning
			 << "\n/*Bottom:*/ " << params.bottom
			 << "\n/*Top:*/ " << params.top
			 << "\n/*V-Bin:*/ " << params.verticalBinning
			 << "\nEND_CAMERA_IMAGE_DIMENSIONS\n";
}

//void ImageDimsControl::setBinningMode(AndorBinningMode::mode mode)
//{
//	binningMode = mode;
//}

void ImageDimsControl::handleSave(ConfigStream& saveFile ){
	saveParams (saveFile, readImageParameters());
}

imageParameters ImageDimsControl::getImageDimSettingsFromConfig (ConfigStream& configFile){
	imageParameters params;
	configFile >> params.left;
	configFile >> params.right;
	configFile >> params.horizontalBinning;
	configFile >> params.bottom;
	configFile >> params.top;
	configFile >> params.verticalBinning;
	return params;
}

void ImageDimsControl::handleOpen(ConfigStream& openFile){
	ConfigSystem::checkDelimiterLine( openFile, "CAMERA_IMAGE_DIMENSIONS" );
	imageParameters params = getImageDimSettingsFromConfig ( openFile );
	setImageParametersFromInput( params );
}


imageParameters ImageDimsControl::readImageParameters(){
	// in case called before initialized
	imageParameters params;
	if (!leftEdit)	{
		return params;
	}
	// set all of the image parameters
	try	{
		params.left = boost::lexical_cast<int>( str(leftEdit->text ()) );
	}
	catch ( boost::bad_lexical_cast&) {
		isReady = false;
		throwNested ( "Left border argument not an integer!\r\n" );
	}
	try	{
		params.right = boost::lexical_cast<int>( str(rightEdit->text()) );
	}
	catch ( boost::bad_lexical_cast&) {
		isReady = false;
		throwNested ( "Right border argument not an integer!\r\n" );
	}
	//
	try	{
		params.bottom = boost::lexical_cast<int>( str(bottomEdit->text()) );
	}
	catch ( boost::bad_lexical_cast&) {
		isReady = false;
		throwNested ( "Top border argument not an integer!\r\n" );
	}
	//
	try	{
		params.top = boost::lexical_cast<int>( str(topEdit->text()) );
	}
	catch ( boost::bad_lexical_cast&) {
		isReady = false;
		throwNested ( "Bottom border argument not an integer!\r\n" );
	}
	//int sel = binningComboCopy->currentIndex();
	//auto binningMode = AndorBinningMode::fromStr(str(binningComboCopy->currentText()));
	try	{
		params.horizontalBinning = boost::lexical_cast<int>( str(horBinningEdit->text()) );
		//params.horizontalBinning = AndorBinningMode::toInt(binningMode);
	}
	catch ( boost::bad_lexical_cast&) {
		isReady = false;
		throwNested ( "Horizontal binning argument not an integer!\r\n" );
	}
	try	{
		params.verticalBinning = boost::lexical_cast<int>( str(vertBinningEdit->text()) );
		//params.horizontalBinning = AndorBinningMode::toInt(binningMode);
	}
	catch ( boost::bad_lexical_cast&) {
		isReady = false;
		throwNested ( "Vertical binning argument not an integer!\r\n" );
	}
	// Check Image parameters
	try	{
		params.checkConsistency ( camType );
	}
	catch ( ChimeraError& )
	{
		isReady = false;
		throw;
	}

	// made it through successfully.
	isReady = true;
	return params;
}


/*
 * I forget why I needed a second function for this.
 */
void ImageDimsControl::setImageParametersFromInput( imageParameters param ){
	// set all of the image parameters
	leftEdit->setText( qstr(param.left ) );
	rightEdit->setText (qstr (param.right ) );
	bottomEdit->setText (qstr (param.bottom ) );
	topEdit->setText (qstr (param.top ) );
	horBinningEdit->setText (qstr (param.horizontalBinning ) );
	vertBinningEdit->setText (qstr (param.verticalBinning ) );
	// Check Image parameters
	try{
		param.checkConsistency(camType);
	}
	catch ( ChimeraError ){
		isReady = false;
		throw;
	}
	// made it through successfully.
	isReady = true;
}


bool ImageDimsControl::checkReady(){
	if (isReady){
		return true;
	}
	else{
		return false;
	}
}

void ImageDimsControl::updateEnabledStatus (bool viewRunning) {
	leftEdit->setEnabled(!viewRunning);
	rightEdit->setEnabled (!viewRunning);
	horBinningEdit->setEnabled (!viewRunning);
	bottomEdit->setEnabled (!viewRunning);
	topEdit->setEnabled (!viewRunning);
	vertBinningEdit->setEnabled (!viewRunning);
}

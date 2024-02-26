// created by Mark O. Brown
#include "stdafx.h"
#include "ATMCD32D.h"
#include "Andor/AndorCameraCore.h"
#include "Andor/AndorTriggerModes.h"
#include "Andor/AndorRunMode.h"
#include "ConfigurationSystems/ConfigSystem.h"
#include "MiscellaneousExperimentOptions/Repetitions.h"
#include <Andor/AndorCameraThreadWorker.h>
#include <Andor/AndorCameraThreadImageGrabber.h>
#include <PrimaryWindows/QtMainWindow.h>
#include <PrimaryWindows/QtAndorWindow.h>
#include <ExperimentThread/ExpThreadWorker.h>
#include <qdebug.h>
#include <qthread.h>
#include <chrono>
#include <process.h>
#include <algorithm>
#include <numeric>
#include <random>

std::string AndorCameraCore::getSystemInfo(){
	std::string info;
	// can potentially get more info from this.
	//AndorCapabilities capabilities;
	//getCapabilities( capabilities );
	info += "Camera Model: " + flume.getHeadModel() + "\n\t";
	int num; 
	flume.getSerialNumber(num);
	info += "Camera Serial Number: " + str(num) + "\n";
	return info;
}

AndorRunSettings AndorCameraCore::getSettingsFromConfig (ConfigStream& configFile){
	AndorRunSettings tempSettings; 
	tempSettings.imageSettings = ConfigSystem::stdConfigGetter ( configFile, "CAMERA_IMAGE_DIMENSIONS", 
																  ImageDimsControl::getImageDimSettingsFromConfig);
	ConfigSystem::initializeAtDelim (configFile, "CAMERA_SETTINGS");
	configFile >> tempSettings.controlCamera;
	configFile.get ();
	std::string txt = configFile.getline ();
	tempSettings.triggerMode = AndorTriggerMode::fromStr (txt);
	//configFile >> tempSettings.emGainModeIsOn;
	//configFile >> tempSettings.emGainLevel;
	txt = configFile.getline ();
	tempSettings.acquisitionMode = AndorRunModes::fromStr(txt);
	txt = configFile.getline();
	tempSettings.gainMode= AndorGainMode::fromStr(txt);
	//if (txt == AndorRunModes::toStr (AndorRunModes::mode::Video) || txt == "Video Mode"){
	//	tempSettings.acquisitionMode = AndorRunModes::mode::Video;
	//	tempSettings.repetitionsPerVariation = INT_MAX;
	//}
	//else if (txt == AndorRunModes::toStr (AndorRunModes::mode::Kinetic) || txt == "Kinetic Series Mode"){
	//	tempSettings.acquisitionMode = AndorRunModes::mode::Kinetic;
	//}
	//else if (txt == AndorRunModes::toStr (AndorRunModes::mode::Accumulate) || txt == "Accumulate Mode")	{
	//	tempSettings.acquisitionMode = AndorRunModes::mode::Accumulate;
	//}
	//else{
	//	thrower ("ERROR: Unrecognized camera mode: " + txt);
	//}
	configFile >> tempSettings.frameRate;
	//configFile >> tempSettings.kineticCycleTime;
	//configFile >> tempSettings.accumulationTime;
	//configFile >> tempSettings.accumulationNumber;
	configFile >> tempSettings.temperatureSetting;
	unsigned numExposures = 0;
	configFile >> numExposures;
	tempSettings.exposureTimes.resize (numExposures);
	for (auto& exp : tempSettings.exposureTimes)	{
		configFile >> exp;
	}
	configFile >> tempSettings.continuousMode;
	configFile >> tempSettings.picsPerRepetition;
	//configFile >> tempSettings.horShiftSpeedSetting;
	//configFile >> tempSettings.vertShiftSpeedSetting;
	return tempSettings;
} 


AndorCameraCore::AndorCameraCore( bool safemode_opt ) : safemode( safemode_opt ), flume( safemode_opt ){
	//runSettings.emGainModeIsOn = false;
	flume.initialize( );
	//flume.setBaselineClamp( 1 );
	//flume.setBaselineOffset( 0 );
	//flume.setDMAParameters( 1, 0.0001f );

	//Set the camera to continuously acquires frames 
	flume.setBool(L"SpuriousNoiseFilter", AT_FALSE);
	flume.setEnumString(L"CycleMode", L"Continuous");
	flume.setEnumString(L"TriggerSource", L"D-Type Connector");
	flume.setEnumString(L"PixelEncoding", L"Mono16");

	flume.setEnumString(L"FrameGenMode", L"Off");
	//Set the camera AUX out to show if any pixel row is being exposed
	flume.setEnumString(L"IOSelector", L"Aux Out 1");
	flume.setEnumString(L"AuxiliaryOutSource", L"FireAny");
	flume.setEnumString(L"IOSelector", L"Aux Out 2");
	flume.setEnumString(L"AuxOutSourceTwo", L"ExternalShutterControl");
	flume.setEnumString(L"FanSpeed", L"Off");



	H5::H5File fp(PLOT_FILES_SAVE_LOCATION + "\\test_data" + "\\test_20230213.hdf5", H5F_ACC_RDONLY);
	H5::DataSet dset = fp.openDataSet("/default");
	H5::DataSpace dspace = dset.getSpace();
	hsize_t dims[3];
	hsize_t rank = dspace.getSimpleExtentDims(dims, NULL);
	// Define the memory dataspace
	hsize_t dimsm[2] = { dims[1] , dims[2] };
	H5::DataSpace memspace(2, dimsm);
	// Initialize hyperslabs
	hsize_t dataCount[3] = { 1, dims[1], dims[2] };
	hsize_t dataOffset[3] = { 0, 0, 0 };
	const hsize_t memCount[2] = { dims[1], dims[2] };
	const hsize_t memOffset[2] = { 0, 0 };
	memspace.selectHyperslab(H5S_SELECT_SET, memCount, memOffset);
	dspace.selectHyperslab(H5S_SELECT_SET, dataCount, dataOffset);
	std::vector<long> tmp(dims[1]*dims[2]);
	try {
		dset.read(tmp.data(), H5::PredType::NATIVE_LONG, memspace, dspace);

	}
	catch (H5::Exception& err) {
		FILE* pFile;
		// note the "w", so this file is constantly overwritten.
		fopen_s(&pFile, "TempH5Log.txt", "w");
		if (pFile != 0) {
			err.printErrorStack(pFile);
			fclose(pFile);
		}
		std::ifstream readFile("TempH5Log.txt");
		if (!readFile) {
			thrower("Failed to get full HDF5 Error! Read file failed to open?!?");
		}
		std::stringstream buffer;
		buffer << readFile.rdbuf();
	}
}

AndorCameraCore::~AndorCameraCore()
{
}

void AndorCameraCore::initializeClass(IChimeraQtWindow* parent, chronoTimes* imageTimes){
	threadExpectingAcquisition = false;

	threadWorkerInput.Andor = this;
	threadWorkerInput.imageTimes = imageTimes;
	// begin the camera wait thread.
	AndorCameraThreadWorker* worker = new AndorCameraThreadWorker (&threadWorkerInput);
	QThread* workerThread = new QThread;
	worker->moveToThread (workerThread);
	parent->mainWin->connect (worker, &AndorCameraThreadWorker::notify,
							  parent->mainWin, &QtMainWindow::handleNotification);
	//parent->andorWin->connect (worker, &AndorCameraThreadWorker::pictureTaken,
	//						   parent->andorWin, &QtAndorWindow::onCameraProgress);
	parent->andorWin->connect(worker, &AndorCameraThreadWorker::error,
		parent->andorWin, &QtAndorWindow::reportErr);

	parent->mainWin->connect (workerThread, &QThread::started, worker, &AndorCameraThreadWorker::process);
	parent->mainWin->connect (workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
	parent->mainWin->connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
	workerThread->start (QThread::TimeCriticalPriority);


	threadGrabberInput.picBufferQueue = &threadWorkerInput.picBufferQueue;
	threadGrabberInput.Andor = this;
	threadGrabberInput.imageTimes = imageTimes;
	// begin the camera image grabber thread.
	AndorCameraThreadImageGrabber* grabber = new AndorCameraThreadImageGrabber(&threadGrabberInput);
	QThread* grabberThread = new QThread;
	grabber->moveToThread(grabberThread);
	parent->andorWin->connect(grabber, &AndorCameraThreadImageGrabber::pictureGrabbed,
		parent->andorWin, &QtAndorWindow::onCameraProgress);
	parent->andorWin->connect(grabber, &AndorCameraThreadImageGrabber::pauseExperiment,
		parent->mainWin, &QtMainWindow::pauseExperiment);
	parent->mainWin->connect(grabber, &AndorCameraThreadImageGrabber::notify,
		parent->mainWin, &QtMainWindow::handleNotification);
	parent->andorWin->connect(grabber, &AndorCameraThreadImageGrabber::error,
		parent->andorWin, &QtAndorWindow::reportErr);

	parent->mainWin->connect(grabberThread, &QThread::started, grabber, &AndorCameraThreadImageGrabber::process);
	parent->mainWin->connect(grabberThread, &QThread::finished, grabberThread, &QObject::deleteLater);
	parent->mainWin->connect(grabberThread, &QThread::finished, grabber, &QObject::deleteLater);
	grabberThread->start(QThread::TimeCriticalPriority);
}

ThreadsafeQueue<NormalImage>* AndorCameraCore::getGrabberQueue()
{
	return &threadGrabberInput.imageQueue;
}

void AndorCameraCore::updatePictureNumber( unsigned __int64 newNumber ){
	currentPictureNumber = newNumber;
}

/* 
 * pause the camera thread which watches the camera for pictures
 */
void AndorCameraCore::pauseThread(){
	// andor should not be taking images anymore at this point.
	threadExpectingAcquisition = false;
}

/*
 * this should get called when the camera finishes running. right now this is very simple.
 */
void AndorCameraCore::onFinish(){
	//threadInput.signaler.notify_all();
	try {
		flume.abortAcquisition();
	}
	catch (ChimeraError& e) {
		thrower("Error in aborting andor camera acquisition \r\n" + e.trace());
	}


	//Free the allocated buffers 
	for (int i = 0; i < acqBuffers.size(); i++) {
		//delete[] acqBuffers[i];
		acqBuffers[i].clear();
		tempImageBuffers[i] = nullptr;
	}
	cameraIsRunning = false;
	threadExpectingAcquisition = false;
}

void AndorCameraCore::setCalibrating( bool cal ){
	calInProgress = cal;
}

bool AndorCameraCore::isCalibrating( ){
	return calInProgress;
}

/*
 * Get whatever settings the camera is currently using in it's operation, assuming it's operating.
 */
AndorRunSettings AndorCameraCore::getAndorRunSettings(){
	return runSettings;
}

void AndorCameraCore::setSettings(AndorRunSettings settingsToSet){
	runSettings = settingsToSet;
}

void AndorCameraCore::setAcquisitionMode(){
	flume.setAcquisitionMode(int(runSettings.acquisitionMode));
}

std::vector<std::string> AndorCameraCore::getVertShiftSpeeds () {
	auto numSpeeds = flume.getNumberVSSpeeds ();
	std::vector<std::string> speeds (numSpeeds);
	for (auto speedNum : range(numSpeeds)) {
		speeds[speedNum] = str(flume.getVSSpeed (speedNum),2);
	}
	return speeds;
}

std::vector<std::string> AndorCameraCore::getHorShiftSpeeds () {
	auto numSpeeds = flume.getNumberHSSpeeds ();
	std::vector<std::string> speeds (numSpeeds);
	for (auto speedNum : range (numSpeeds)) {
		speeds[speedNum] = str(flume.getHSSpeed (1,0,speedNum),2);
	}
	return speeds;
}

void AndorCameraCore::waitForAcquisition(unsigned long long pictureNumber, unsigned int timeout)
{
	flume.waitBuffer(&tempImageBuffers[pictureNumber % numberOfImageBuffers], &bufferSize, timeout);
}

void AndorCameraCore::queueBuffers(unsigned long long pictureNumber)
{
	flume.queueBuffer(acqBuffers[pictureNumber % numberOfAcqBuffers].data(), bufferSize);
}

/* 
 * Large function which initializes a given camera image run.
 */
void AndorCameraCore::armCamera( double& minKineticCycleTime ){
	if (safemode) {
		//return;
	}
	if (cameraIsRunning) {
		qDebug() << "Camera is running in sequence with picture number" << currentPictureNumber << ". Pass AndorCameraCore::armCamera from deviceProgramVariaton";
		return;
	}
	/// Set a bunch of parameters.
	// Set to 1 MHz readout rate in both cases
	//flume.setADChannel(1);
	//if (runSettings.emGainModeIsOn)	{
	//	flume.setHSSpeed(0, runSettings.horShiftSpeedSetting);
	//	qDebug () << "Horizontal Shift Speed: " << flume.getHSSpeed (1, 0, runSettings.horShiftSpeedSetting);
	//}
	//else {
	//	flume.setHSSpeed(1, runSettings.horShiftSpeedSetting);
	//	qDebug () << "Horizontal Shift Speed: " << flume.getHSSpeed (1, 1, runSettings.horShiftSpeedSetting);
	//}
	//flume.setVSSpeed (runSettings.vertShiftSpeedSetting);
	//qDebug () << "Vertical Shift Speed: " << flume.getVSSpeed (runSettings.vertShiftSpeedSetting);
	setAcquisitionMode();
	//setReadMode();
	if (runSettings.triggerMode != AndorTriggerMode::mode::ExternalExposure) {
		setExposures(0);
		qDebug() << "Set ExpRunningExposure to" << runSettings.exposureTimes[0];
		qDebug() << "Now exposure time is" << runSettings.exposureTime;
	}
	if (runSettings.triggerMode == AndorTriggerMode::mode::Internal) {
		setFrameRate();
	}
	//setCameraBinningMode();
	setImageParametersToCamera();
	// Set Mode-Specific Parameters
	if (runSettings.acquisitionMode == AndorRunModes::mode::Single){
		//setFrameTransferMode();
		//setKineticCycleTime ();
		/// TODO: set this properly
	}
	else if (runSettings.acquisitionMode == AndorRunModes::mode::Kinetic){
		//setKineticCycleTime();
		//setScanNumber();
		// set this to 1.
		//setNumberAccumulations(true);
		//setFrameTransferMode ( );
	}
	else if (runSettings.acquisitionMode == AndorRunModes::mode::Accumulate){
		//setAccumulationCycleTime();
		//setNumberAccumulations(false);
	}
	//setGainMode();
	setCameraTriggerMode();
	setCameraGainMode();


	currentPictureNumber = 0;
	cameraIsRunning = true;

	AT_64 ImageSizeBytes = 0;
	flume.getInt(L"Image Size Bytes", &ImageSizeBytes);
	bufferSize = static_cast<int>(ImageSizeBytes);

	for (int i = 0; i < numberOfAcqBuffers; i++) {
		acqBuffers[i].clear();
		acqBuffers[i].resize(bufferSize);
		tempImageBuffers[i] = nullptr;
	}
	queueBuffers(0);

	flume.setEnumString(L"CycleMode", L"Continuous");
	flume.setEnumString(L"TriggerSource", L"D-Type Connector");
	flume.setEnumString(L"PixelEncoding", L"Mono16");
	flume.setEnumString(L"IOSelector", L"Aux Out 1");
	flume.setEnumString(L"AuxiliaryOutSource", L"FireAny");
	flume.setEnumString(L"IOSelector", L"Aux Out 2");
	flume.setEnumString(L"AuxOutSourceTwo", L"ExternalShutterControl");

	flume.startAcquisition();


	/// Do some plotting stuffs
	// get the min time after setting everything else.
	//minKineticCycleTime = getMinKineticCycleTime( );

	 // remove the spurious wakeup check.
	threadExpectingAcquisition = true;
	// notify the thread that the experiment has started..
	threadWorkerInput.picBufferQueue.clear();
	threadGrabberInput.imageQueue.clear();
	threadWorkerInput.signaler.notify_all();
	threadGrabberInput.signaler.notify_all();
	
}

void AndorCameraCore::preparationChecks () {
	//try {
	//	auto res = flume.checkForNewImages ();
	//	// success?!?
	//	thrower ("In preparation section, looks like there are already images in the camera???");
	//}
	//catch (ChimeraError & err) {
	//	// the expected result is throwing NO_NEW_DATA. 
	//	if (err.whatBare () != flume.getErrorMsg (DRV_NO_NEW_DATA)) {
	//		try {
	//			flume.andorErrorChecker (flume.queryStatus ());
	//		}
	//		catch (ChimeraError & err2) {
	//			throwNested ("Error seen while checking for new images: " + err.trace ()
	//				+ ", Camera Status:" + err2.trace () + ", Camera is running bool: " + str (cameraIsRunning));
	//		}
	//		throwNested ("Error seen while checking for new images: " + err.trace ()
	//			+ ", Camera Status: DRV_SUCCESS, Camera is running bool: " + str (cameraIsRunning));
	//	}
	//}
}


/* 
 * This function checks for new pictures as indicated by the image buffer tempImageBuffers[currentPictureNumber % numberOfImageBuffers], 
 * and shapes them into the array which holds all of the pictures for a given experiment cycle, except continuousMode.
 */
std::vector<Matrix<long>> AndorCameraCore::acquireImageData (){
	try	{
		// each image processed from the call from imageGrabber thread
		int experimentPictureNumber = (currentPictureNumber) % runSettings.picsPerRepetition;
		if (runSettings.continuousMode) {
			experimentPictureNumber = 0;
		}
		if (experimentPictureNumber == 0){
			repImages.clear ();
		}
		repImages.push_back(Matrix<long>());

		auto& imSettings = runSettings.imageSettings;
		Matrix<long> tempImage(imSettings.heightBinned(), imSettings.widthBinned(), 0);
		repImages[experimentPictureNumber] = Matrix<long>(imSettings.heightBinned(), imSettings.widthBinned(), 0);
		if (!safemode){
			try	{
				auto bufferNumber = currentPictureNumber % numberOfImageBuffers;
				AT_64 Stride, Width, Height;
				flume.getInt(L"AOIStride", &Stride);
				flume.getInt(L"AOIWidth", &Width);
				flume.getInt(L"AOIHeight", &Height);
				if (tempImageBuffers[bufferNumber] == nullptr) {
					if (cameraIsRunning == true) {
						throwNested("Andor camera image buffer is empty. Lowlevel bug???");
					}
					else {
						qDebug() << "AndorCameraCore::acquireImageData: Andor camera aborted, tried to retrieve the last image but the buffer is already cleaned. Nothing to worry about";
					}
				}
				for (AT_64 Row = 0; Row < Height; Row++) {
					//Cast the raw image buffer to a 16-bit array. 
					//...Assumes the PixelEncoding is 16-bit. 
					unsigned short* ImagePixels = reinterpret_cast<unsigned short*>(tempImageBuffers[bufferNumber]);
					if (ImagePixels == nullptr) {
						thrower("Andor image pointer is null. This should not happen during experiment but could happen when abort.");
					}
					//Process each pixel in a row as normal 
					for (AT_64 Pixel = 0; Pixel < Width; Pixel++) {
						tempImage (Row, Pixel) = ImagePixels[Pixel];
					} //Use Stride to get the memory location of the next row. 
					tempImageBuffers[bufferNumber] += Stride;
				}
			}
			catch (ChimeraError &e)	{
				// let the blank image roll through to keep the image numbers going sensibly. // ??? WTF zzp 20220913
				throwNested ("Error while calling getOldestImage.\n"+e.trace());
			}
			// immediately rotate
			for (auto imageVecInc : range(repImages[experimentPictureNumber].size ())){
				//repImages[experimentPictureNumber].data[imageVecInc] = tempImage.data[((imageVecInc
				//	% imSettings.width ()) + 1) * imSettings.height () - imageVecInc / imSettings.width () - 1];
				repImages[experimentPictureNumber].data[imageVecInc] = tempImage.data[imageVecInc];
			}
		}
		else{
			//for (auto imageVecInc : range (repImages[experimentPictureNumber].size ()))	{
			//	std::random_device rd;
			//	std::mt19937 e2 (rd ());
			//	std::normal_distribution<> dist (180, 20);
			//	std::normal_distribution<> dist2 (350, 100);
			//	tempImage.data[imageVecInc] = dist (e2) + 10;
			//	if (((imageVecInc / imSettings.widthBinned ()) % 2 == 1) && ((imageVecInc % imSettings.widthBinned()) % 2 == 1)){
			//		// can have an atom here.
			//		if (unsigned (rand ()) % 300 > imageVecInc + 50){
			//			// use the exposure time and em gain level 
			//			tempImage.data[imageVecInc] += runSettings.exposureTimes[experimentPictureNumber] * 1e3 * dist2 (e2);
			//			//if (runSettings.emGainModeIsOn)
			//			//{
			//			//	tempImage.data[imageVecInc] *= runSettings.emGainLevel;
			//			//}
			//		}
			//	}
			//}
			//auto& ims = runSettings.imageSettings;
			//for (auto rowI : range (repImages[experimentPictureNumber].getRows ()))	{
			//	for (auto colI : range (repImages[experimentPictureNumber].getCols ()))	{
			//		repImages[experimentPictureNumber] (rowI, colI) = tempImage (tempImage.getRows()-colI-1, rowI);
			//	}
			//}
			H5::H5File fp(PLOT_FILES_SAVE_LOCATION + "\\test_data" + "\\test_20230213.hdf5", H5F_ACC_RDONLY);
			H5::DataSet dset = fp.openDataSet("/default");
			H5::DataSpace dspace = dset.getSpace();
			hsize_t dims[3];
			hsize_t rank = dspace.getSimpleExtentDims(dims, NULL);
			// Define the memory dataspace
			hsize_t dimsm[2] = { dims[1] , dims[2] };
			H5::DataSpace memspace(2, dimsm);
			// Initialize hyperslabs
			hsize_t dataCount[3] = { 1, dims[1], dims[2] };
			hsize_t dataOffset[3] = { currentPictureNumber % dims[0], 0, 0 };
			const hsize_t memCount[2] = { dims[1], dims[2] };
			const hsize_t memOffset[2] = { 0, 0 };
			memspace.selectHyperslab(H5S_SELECT_SET, memCount, memOffset);
			dspace.selectHyperslab(H5S_SELECT_SET, dataCount, dataOffset);
			dset.read(repImages[experimentPictureNumber].data.data(), H5::PredType::NATIVE_LONG, memspace, dspace);
		}
		return repImages;
	}
	catch (ChimeraError &){
		throwNested ("Error Seen in acquireImageData.");
	}
}


// sets this based on internal settings object.
void AndorCameraCore::setCameraTriggerMode(){
	//std::string errMsg;
	//int trigType;
	//if (runSettings.triggerMode == AndorTriggerMode::mode::Internal){
	//	trigType = 0;
	//}
	//else if (runSettings.triggerMode == AndorTriggerMode::mode::External){
	//	trigType = 1;
	//}
	//else if (runSettings.triggerMode == AndorTriggerMode::mode::StartOnTrigger){
	//	trigType = 6;
	//}
	std::string trgmode = AndorTriggerMode::toStr(runSettings.triggerMode);
	flume.setEnumString(L"TriggerMode", w_str(trgmode.c_str()).c_str());
}

void AndorCameraCore::setCameraGainMode()
{
	wchar_t tmp[1024];
	int enumidx;
	flume.getEnumIndex(L"PixelEncoding", &enumidx);
	flume.getEnumStringByIndex(L"PixelEncoding", enumidx, tmp, 1024);
	qDebug() << "Get the PxielEncoding before set the gain mode" << QString::fromWCharArray(tmp);

	std::string gainmode = AndorGainMode::toStr(runSettings.gainMode);
	flume.setEnumString(L"GainMode", w_str(gainmode.c_str()).c_str());

	flume.getEnumIndex(L"GainMode", &enumidx);
	flume.getEnumStringByIndex(L"GainMode", enumidx, tmp, 1024);
	qDebug() << "Get the GainMode after set the gain mode" << QString::fromWCharArray(tmp);

	flume.getEnumIndex(L"PixelEncoding", &enumidx);
	flume.getEnumStringByIndex(L"PixelEncoding", enumidx, tmp, 1024);
	qDebug() << "Get the PxielEncoding after set the gain mode" << QString::fromWCharArray(tmp);

	flume.setEnumString(L"PixelEncoding", L"Mono16"); // prevent from andor auto changing pixel encoding, see sdk3 manual 4.6

	flume.getEnumIndex(L"PixelEncoding", &enumidx);
	flume.getEnumStringByIndex(L"PixelEncoding", enumidx, tmp, 1024);
	qDebug() << "Get the PxielEncoding after set the gain mode after set the encoding to Mono16" << QString::fromWCharArray(tmp);

}

void AndorCameraCore::setCameraBinningMode()
{
	std::string binningmode = AndorBinningMode::toStr(runSettings.binningMode);
	flume.setEnumString(L"AOIBinning", w_str(binningmode.c_str()).c_str());
}


void AndorCameraCore::setTemperature(){
	// Get the current temperature
	if (runSettings.temperatureSetting < -45 || runSettings.temperatureSetting > 25){
		auto answer = QMessageBox::question(nullptr, "Temperature Warning!", "Warning: The selected temperature is "
			"outside the \"normal\" temperature range of the camera (-60 through 25 C). Proceed anyways?");
		if (answer == QMessageBox::No){
			return;
		}
	}
	// Proceedure to initiate cooling
	changeTemperatureSetting( false );
}


//void AndorCameraCore::setReadMode(){
//	flume.setReadMode(runSettings.readMode);
//}


void AndorCameraCore::setExposures(int expoIdx){
	//if (runSettings.exposureTimes.size() > 0 && runSettings.exposureTimes.size() <= 16){
	//	flume.setRingExposureTimes(runSettings.exposureTimes.size(), runSettings.exposureTimes.data());
	//}
	//else{
	//	thrower ("ERROR: Invalid size for vector of exposure times, value of " + str(runSettings.exposureTimes.size()) + ".");
	//}
	flume.setFloat(L"ExposureTime", runSettings.exposureTimes[expoIdx]);
	double expo;
	flume.getFloat(L"ExposureTime", &expo);
	runSettings.exposureTime = expo;
	runSettings.exposureTimes[expoIdx] = expo;
}

void AndorCameraCore::setExpRunningExposure(unsigned long long pictureNumber)
{
	if (runSettings.triggerMode != AndorTriggerMode::mode::ExternalExposure) {
		setExposures(pictureNumber % expRunSettings.picsPerRepetition); // set exposure for the next image
		qDebug() << "Set ExpRunningExposure to" << runSettings.exposureTimes[pictureNumber % expRunSettings.picsPerRepetition];
		qDebug() << "Now exposure time is" << runSettings.exposureTime;
	}
}


void AndorCameraCore::setImageParametersToCamera(){
	auto& im = runSettings.imageSettings;
	if (((im.top - im.bottom + 1) % im.verticalBinning) != 0) {
		qDebug() << "(bottom - top + 1) % vertical binning must be 0!";
		thrower("(bottom - top + 1) % vertical binning must be 0!");
	}
	if (((im.right - im.left + 1) % im.horizontalBinning) != 0) {
		qDebug () << "(right - left + 1) % horizontal binning must be 0!";
		thrower("(right - left + 1) % horizontal binning must be 0!");
	}
	flume.setImage(im.horizontalBinning, im.verticalBinning, im.left, im.right, im.bottom, im.top);
	//flume.setImage( im.verticalBinning, im.horizontalBinning, im.bottom, im.top,  im.left, im.right );
}

double AndorCameraCore::setFrameRate()
{
	flume.setFloat(L"FrameRate", runSettings.frameRate);
	double fr;
	flume.getFloat(L"FrameRate", &fr);
	runSettings.frameRate = fr;
	return fr;
}

//void AndorCameraCore::setKineticCycleTime(){
//	flume.setKineticCycleTime(runSettings.kineticCycleTime);
//}

//void AndorCameraCore::setScanNumber()
//{
//	if (runSettings.totalPicsInExperiment() == 0 && runSettings.totalPicsInVariation() != 0){
//		// all is good. The first variable has not been set yet.
//	}
//	else if (runSettings.totalPicsInVariation() == 0){
//		thrower ("ERROR: Scan Number Was Zero.\r\n");
//	}
//	else{
//		//flume.setNumberKinetics(int(runSettings.totalPicsInExperiment()));
//		flume.setNumberKinetics ( int ( runSettings.totalPicsInVariation ( ) ) );
//	}
//}


//void AndorCameraCore::setFrameTransferMode(){
//	flume.setFrameTransferMode(runSettings.frameTransferMode);
//}


/*
 * exposures should be initialized to be the correct size. Nothing else matters for the inputs, they get 
 * over-written.
 * throws exception if fails
 */
void AndorCameraCore::checkAcquisitionTimings(float& kinetic, float& accumulation, std::vector<float>& exposures)
{
	if ( exposures.size ( ) == 0 ){
		return;
	}
	float tempExposure, tempAccumTime, tempKineticTime;
	float * timesArray = nullptr;
	std::string errMsg;
	if (safemode)
	{
		// if in safemode initialize this stuff to the values to be outputted.
		if (exposures.size() > 0){
			tempExposure = exposures[0];
		}
		else{
			tempExposure = 0;
		}
		tempAccumTime = accumulation;
		tempKineticTime = kinetic;
	}
	else{
		tempExposure = 0;
		tempAccumTime = 0;
		tempKineticTime = 0;
	}
	// It is necessary to get the actual times as the system will calculate the
	// nearest possible time. eg if you set exposure time to be 0, the system
	// will use the closest value (around 0.01s)
	timesArray = new float[exposures.size()];
	if (!safemode){
		flume.getAcquisitionTimes(tempExposure, tempAccumTime, tempKineticTime);
		flume.getAdjustedRingExposureTimes(int(exposures.size()), timesArray);
	}
	else {
		for (unsigned exposureInc = 0; exposureInc < exposures.size(); exposureInc++){
			timesArray[exposureInc] = exposures[exposureInc];
		}
	}
	// Set times
	if (exposures.size() > 0){
		for (unsigned exposureInc = 0; exposureInc < exposures.size(); exposureInc++){
			exposures[exposureInc] = timesArray[exposureInc];
		}
		delete[] timesArray;
	}
	accumulation = tempAccumTime;
	kinetic = tempKineticTime;
}

//void AndorCameraCore::setAccumulationCycleTime(){
//	flume.setAccumulationCycleTime(runSettings.accumulationTime);
//}
//
//void AndorCameraCore::setNumberAccumulations(bool isKinetic){
//	std::string errMsg;
//	if (isKinetic){
//		// right now, kinetic series mode always has one accumulation. could add this feature later if desired to do 
//		// both kinetic and accumulation. Not sure there's actually much of a reason to use accumulations. 
//		//setNumberAccumulations(true); // ???
//		flume.setAccumulationNumber(1);
//	}
//	else{
//		// ???
//		// setNumberAccumulations(false); // ???
//		flume.setAccumulationNumber(runSettings.accumulationNumber);
//	}
//}


//void AndorCameraCore::setGainMode(){
//	if (!runSettings.emGainModeIsOn){
//		// Set Gain
//		int numGain;
//		flume.getNumberOfPreAmpGains(numGain);
//		flume.setPreAmpGain(2);
//		float myGain;
//		flume.getPreAmpGain(2, myGain);
//		// 1 is for conventional gain mode.
//		flume.setOutputAmplifier(1);
//	}
//	else{
//		// 0 is for em gain mode.
//		flume.setOutputAmplifier(0);
//		flume.setPreAmpGain(2);
//		if (runSettings.emGainLevel > 300){
//			flume.setEmGainSettingsAdvanced(1);
//		}
//		else{
//			flume.setEmGainSettingsAdvanced(0);
//		}
//		flume.setEmCcdGain(runSettings.emGainLevel);
//	}
//}


void AndorCameraCore::changeTemperatureSetting(bool turnTemperatureControlOff){
	int minimumAllowedTemp, maximumAllowedTemp;
	// the default, in case the program is in safemode.
	minimumAllowedTemp = -45;
	maximumAllowedTemp = 25;
	// check if temp is in valid range
	flume.getTemperatureRange(minimumAllowedTemp, maximumAllowedTemp);
	if (runSettings.temperatureSetting < minimumAllowedTemp || runSettings.temperatureSetting > maximumAllowedTemp)	{
		thrower ("ERROR: Temperature is out of range\r\n");
	}
	else{
		// if it is in range, switch on cooler and set temp
		if (turnTemperatureControlOff == false)	{
			flume.temperatureControlOn();
		}
		else{
			flume.temperatureControlOff();
		}
	}

	if (turnTemperatureControlOff == false){
		flume.setTemperature(runSettings.temperatureSetting);
	}
	else{
		thrower ("Temperature Control has been turned off.\r\n");
	}
}

AndorTemperatureStatus AndorCameraCore::getTemperature ( ){
	AndorTemperatureStatus stat;
	stat.temperatureSetting = getAndorRunSettings().temperatureSetting;
	try{
		if (ANDOR_SAFEMODE) { stat.andorRawMsg = "SAFEMODE"; }
		else {
			auto msgCode = flume.getTemperature (stat.temperature);
			stat.andorRawMsg = flume.getErrorMsg (msgCode);
		}
		// if not stable this won't get changed.
		if (stat.andorRawMsg != "DRV_ACQUIRING") {
			mostRecentTemp = stat.temperature;
			stat.colorCode = QColor("chocolate");
		}
		if (stat.andorRawMsg == "DRV_TEMPERATURE_STABILIZED") {
			stat.msg = "Temperature has stabilized at " + str (stat.temperature) + " (C)";
			stat.colorCode = QColor("lawngreen");
		}
		else if (stat.andorRawMsg == "DRV_TEMPERATURE_NOT_REACHED") {
			stat.msg = "Set temperature not yet reached. Current temperature is " + str (stat.temperature) + " (C)";
			stat.colorCode = QColor("hotpink");
		}
		else if (stat.andorRawMsg == "DRV_TEMPERATURE_NOT_STABILIZED") {
			stat.msg = "Temperature of " + str (stat.temperature) + " (C) reached but not stable.";
			stat.colorCode = QColor("orange");
		}
		else if (stat.andorRawMsg == "DRV_TEMPERATURE_DRIFT") {
			stat.msg = "Temperature had stabilized but has since drifted. Temperature: " + str (stat.temperature);
			stat.colorCode = QColor("gold");
		}
		else if (stat.andorRawMsg == "DRV_TEMPERATURE_OFF") {
			stat.msg = "Temperature control is off. Temperature: " + str (stat.temperature);
			stat.colorCode = QColor("sienna");
		}
		else if (stat.andorRawMsg == "DRV_ACQUIRING") {
			// doesn't change color of temperature control. This way the color of the control represents the state of
			// the temperature right before the acquisition started, so that you can tell if you remembered to let it
			// completely stabilize or not.
			stat.msg = "Camera is Acquiring data. No updates are available. \r\nMost recent temperature: "
				+ str (mostRecentTemp);
			stat.colorCode = QColor("chocolate");
		}
		else if (stat.andorRawMsg == "SAFEMODE") {
			stat.msg = "Device is running in Safemode... No Real Temperature Data is available.";
			stat.colorCode = QColor("lightgreen");
		}
		else {
			stat.msg = "Unexpected Temperature Message: " + stat.andorRawMsg + ". Temperature: "
				+ str (stat.temperature);
			stat.colorCode = QColor("indianred");
		}
	}
	catch ( ChimeraError& ){
		throwNested ("Failed to get temperature from andor?!");
	}
	return stat;
}

int AndorCameraCore::queryStatus ( ){
	return flume.queryStatus ( );
}

bool AndorCameraCore::isRunning ( ){
	return cameraIsRunning;
}

//double AndorCameraCore::getMinKineticCycleTime ( ){
//	// get the currently set kinetic cycle time.
//	float minKineticCycleTime, dummy1, dummy2;
//	flume.setKineticCycleTime ( 0 );
//	flume.getAcquisitionTimes ( dummy1, dummy2, minKineticCycleTime );
//
//	// re-set whatever's currently in the settings.
//	setKineticCycleTime ( );
//	return minKineticCycleTime;
//}

double AndorCameraCore::getMaxFrameRate()
{
	double maxFrameR;
	flume.getFloatMax(L"FrameRate", &maxFrameR);
	return maxFrameR;
}

void AndorCameraCore::setIsRunningState ( bool state ){
	cameraIsRunning = state;
}

void AndorCameraCore::abortAcquisition ( ){
	flume.abortAcquisition ( );
}

void AndorCameraCore::logSettings (DataLogger& log, ExpThreadWorker* threadworker){
	try	{
		if (!experimentActive)	{
			if (threadworker != nullptr) {
				emit threadworker->notification (qstr ("Not logging Andor info!\n"), 0);
			}
			H5::Group andorGroup (log.file.createGroup ("/Andor:Off"));
			return;
		}
		// in principle there are some other low level settings or things that aren't used very often which I could include 
		// here. I'm gonna leave this for now though.
		H5::Group andorGroup (log.file.createGroup ("/Andor"));
		hsize_t rank1[] = { 1 };
		// pictures. These are permanent members of the class for speed during the writing process.	
		if (expRunSettings.acquisitionMode == AndorRunModes::mode::Single) {
			hsize_t setDims[] = { unsigned __int64 (expRunSettings.totalPicsInExperiment ()), expRunSettings.imageSettings.heightBinned(),
				expRunSettings.imageSettings.widthBinned() };
			hsize_t picDims[] = { 1, expRunSettings.imageSettings.heightBinned(), expRunSettings.imageSettings.widthBinned() };
			log.AndorPicureSetDataSpace = H5::DataSpace (3, setDims);
			log.AndorPicDataSpace = H5::DataSpace (3, picDims);
			log.AndorPictureDataset = andorGroup.createDataSet ( "Pictures", H5::PredType::NATIVE_LONG, 
																 log.AndorPicureSetDataSpace);
			log.currentAndorPicNumber = 0;
		}
		else {
			/*
				hsize_t setDims[] = { 0, settings.imageSettings.width (), settings.imageSettings.height () };
				hsize_t picDims[] = { 1, settings.imageSettings.width (), settings.imageSettings.height () };
				AndorPicureSetDataSpace = H5::DataSpace (3, setDims);
				AndorPicDataSpace = H5::DataSpace (3, picDims);
				AndorPictureDataset = andorGroup.createDataSet ("Pictures: N/A", H5::PredType::NATIVE_LONG, AndorPicureSetDataSpace);
			*/
		}
		if (log.AndorPicureSetDataSpace.getId () == -1) {
			thrower ("Failed to initialize AndorPicureSetDataSpace???");
		}
		if (log.AndorPicDataSpace.getId () == -1) {
			thrower ("Failed to initialize AndorPicDataSpace???");
		}
		if (log.AndorPictureDataset.getId () == -1) {
			thrower ("Failed to initialize AndorPictureDataset???");
		}
		log.andorDataSetShouldBeValid = true;
		log.writeDataSet (AndorRunModes::toStr(expRunSettings.acquisitionMode), "Camera-Mode", andorGroup);
		log.writeDataSet (expRunSettings.exposureTimes, "Exposure-Times", andorGroup);
		log.writeDataSet (AndorTriggerMode::toStr (expRunSettings.triggerMode), "Trigger-Mode", andorGroup);
		log.writeDataSet (AndorGainMode::toStr(expRunSettings.gainMode), "Gain-Mode", andorGroup);
		//log.writeDataSet (expRunSettings.emGainModeIsOn, "EM-Gain-Mode-On", andorGroup);
		//if (expRunSettings.emGainModeIsOn) {
		//	log.writeDataSet (expRunSettings.emGainLevel, "EM-Gain-Level", andorGroup);
		//}
		//else{
		//	log.writeDataSet (-1, "NA:EM-Gain-Level", andorGroup);
		//}
		// image settings
		H5::Group imageDims = andorGroup.createGroup ("Image-Dimensions");
		log.writeDataSet (expRunSettings.imageSettings.top, "Top", imageDims);
		log.writeDataSet (expRunSettings.imageSettings.bottom, "Bottom", imageDims);
		log.writeDataSet (expRunSettings.imageSettings.left, "Left", imageDims);
		log.writeDataSet (expRunSettings.imageSettings.right, "Right", imageDims);
		log.writeDataSet (expRunSettings.imageSettings.horizontalBinning, "Horizontal-Binning", imageDims);
		log.writeDataSet (expRunSettings.imageSettings.verticalBinning, "Vertical-Binning", imageDims);
		log.writeDataSet (expRunSettings.temperatureSetting, "Temperature-Setting", andorGroup);
		log.writeDataSet (expRunSettings.picsPerRepetition, "Pictures-Per-Repetition", andorGroup);
		log.writeDataSet (expRunSettings.repetitionsPerVariation, "Repetitions-Per-Variation", andorGroup);
		log.writeDataSet (expRunSettings.totalVariations, "Total-Variation-Number", andorGroup);
	}
	catch (H5::Exception err){
		if (threadworker != nullptr) {
			emit threadworker->notification (qstr ("Failed to log andor info!!\n"), 0);
		}
		log.logError (err);
		throwNested ("ERROR: Failed to log andor parameters in HDF5 file: " + err.getDetailMsg ());
	}
}

void AndorCameraCore::loadExpSettings (ConfigStream& stream){
	ConfigSystem::stdGetFromConfig (stream, *this, expRunSettings);
	expRunSettings.repetitionsPerVariation = ConfigSystem::stdConfigGetter (stream, "REPETITIONS", 
																			 Repetitions::getSettingsFromConfig);
	mainOptions mainOpts = ConfigSystem::stdConfigGetter(stream, "MAIN_OPTIONS",
														MainOptionsControl::getSettingsFromConfig);
	expRunSettings.repFirst = mainOpts.repetitionFirst;
	expAnalysisSettings = ConfigSystem::stdConfigGetter (stream, "DATA_ANALYSIS", 
														  DataAnalysisControl::getAnalysisSettingsFromFile); 
	experimentActive = expRunSettings.controlCamera;
}

void AndorCameraCore::calculateVariations (std::vector<parameterType>& params, ExpThreadWorker* threadworker){
	expRunSettings.totalVariations = (params.size () == 0 ? 1 : params.front ().keyValues.size ());;
	expRunSettings.variationShuffleIndex = params.front().shuffleIndex;
	if (experimentActive){
		setSettings (expRunSettings);
		emit threadworker->prepareAndor (&expRunSettings, expAnalysisSettings);
	}
}

void AndorCameraCore::programVariation (unsigned variationInc, std::vector<parameterType>& params, ExpThreadWorker* threadworker){
	if (experimentActive && (!threadworker->getAbortStatus())){
		double kinTime;
		armCamera (kinTime);
	}
}

std::pair<unsigned, unsigned> AndorCameraCore::getCurrentRepVarNumber(unsigned int currentPicNumber)
{
	unsigned currentRepNumber, currentVarNumber;
	if (expRunSettings.repFirst) {
		currentVarNumber = (currentPicNumber / expRunSettings.picsPerRepetition) / expRunSettings.repetitionsPerVariation;
		currentRepNumber = (currentPicNumber / expRunSettings.picsPerRepetition) % expRunSettings.repetitionsPerVariation;
	}
	else {
		currentRepNumber = (currentPicNumber / expRunSettings.picsPerRepetition) / expRunSettings.totalVariations;
		currentVarNumber = (currentPicNumber / expRunSettings.picsPerRepetition) % expRunSettings.totalVariations;
	}
	if (expRunSettings.variationShuffleIndex.size() < currentVarNumber) {
		thrower("AndorCamera variationShuffleIndex size" + str(expRunSettings.variationShuffleIndex.size()) + " is smaller than currentVarNumber" + str(currentVarNumber) + "A low level bug!");
	}
	return std::pair<unsigned, unsigned>(currentRepNumber, expRunSettings.variationShuffleIndex[currentVarNumber]);
}

void AndorCameraCore::normalFinish (){
}

void AndorCameraCore::errorFinish (){
	try	{
		//abortAcquisition ();
		onFinish();
	}
	catch (ChimeraError &) { /*Probably just idle.*/ }
}

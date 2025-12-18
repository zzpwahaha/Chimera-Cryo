#include "stdafx.h"
#include "Andor/AndorCameraThreadImageGrabber.h"
#include <Andor/AndorCameraCore.h>
#include <qdebug.h>
#include <qelapsedtimer.h>

AndorCameraThreadImageGrabber::AndorCameraThreadImageGrabber(cameraThreadImageGrabberInput* input_)
	:input(input_) {};

AndorCameraThreadImageGrabber::~AndorCameraThreadImageGrabber() {}

void AndorCameraThreadImageGrabber::prepareCruncherExit()
{
	input->Andor->threadExpectingAcquisition = false;
	*input->cruncherThreadActive = false;
	// this will invoke cruncherThread and it will then detect cruncherThreadActive=false and therefore quit (-1 is not being recongnized)
	input->imageQueue.push({ {-1ULL, 0, 0}, Matrix<long>() });
}


void AndorCameraThreadImageGrabber::process() 
{
	const int debugPicsPerRep = 2;
	unsigned long long pictureNumber = 0;
	std::unique_lock<std::mutex> lock(input->runMutex);
	while (!input->Andor->cameraThreadExitIndicator) {
		if (true/*!input->Andor->safemode*/) {
			// wait until/unless camera is ready to take images. The signaler should be waked before worker's so that the grabber can wait on 'pop'
			while (!input->Andor->threadExpectingAcquisition) {
				input->signaler.wait(lock);
				input->imageQueue.clear();
				pictureNumber = 0;
			}
			auto popPictureNumber = input->picBufferQueue->pop();
			auto timerE = QElapsedTimer();
			timerE.start();
			if (popPictureNumber % debugPicsPerRep == 0) {
				(*input->imageGrabTimes).push_back(std::chrono::high_resolution_clock::now());
				flog << "From Grabber thread: get image number" << pictureNumber << " at " << std::chrono::duration_cast<std::chrono::nanoseconds>(input->imageGrabTimes->back() - input->imageTimes->back()).count() / 1e6 << " ms relative to worker thread" << fendl;
			}
			if (!input->Andor->cameraIsRunning) {
				// aborted by user and get rewake-ed up by worker thread
				prepareCruncherExit();
				continue;
			}
			if (popPictureNumber != pictureNumber) {
				thrower("AndorCameraThreadImageGrabber error: \r\n The pop-ed pictureNumber from WorkerThread is: " + str(popPictureNumber)
					+ ", but the expected pictureNumber is: " + str(pictureNumber)
					+ ". This is a low-level error as no other thread should be able to pop the picBufferQueue and the order should be FIFO.", 0);
			}
			input->Andor->updatePictureNumber(pictureNumber);
			flog << "void AndorCameraThreadImageGrabber::process: about to acquireImageData at " << timerE.elapsed() << " ms" << fendl;
			try {
				auto images = input->Andor->acquireImageData();
				auto repVar = input->Andor->getCurrentRepVarNumber(pictureNumber);
				AndorRunSettings curSettings = input->Andor->getAndorRunSettings();
				size_t currentActivePicNum = curSettings.continuousMode ? 0 : pictureNumber % curSettings.picsPerRepetition;
				// push to imageQueue which will invoke the atomCruncher thread
				flog << "void AndorCameraThreadImageGrabber::process: finished acquireImageData at " << timerE.elapsed() << " ms" << fendl;
				input->imageQueue.push({ {pictureNumber, repVar.first, repVar.second}, images[currentActivePicNum] });
				emit pictureGrabbed({ {pictureNumber, repVar.first, repVar.second}, images[currentActivePicNum] });
			}
			catch (ChimeraError& err) {
				emit error(err.qtrace(), 0);
				emit pauseExperiment();
				prepareCruncherExit();
				continue;
			}
			pictureNumber++;
			if (pictureNumber == input->Andor->runSettings.totalPicsInExperiment()) {
				// get the last picture. acquisition is over 
				prepareCruncherExit();
			}
		}
		else {
			
		}
	}


}

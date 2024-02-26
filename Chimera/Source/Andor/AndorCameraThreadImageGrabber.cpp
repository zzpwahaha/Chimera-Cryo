#include "stdafx.h"
#include "Andor/AndorCameraThreadImageGrabber.h"
#include <Andor/AndorCameraCore.h>

AndorCameraThreadImageGrabber::AndorCameraThreadImageGrabber(cameraThreadImageGrabberInput* input_)
	:input(input_) {};

AndorCameraThreadImageGrabber::~AndorCameraThreadImageGrabber() {};

void AndorCameraThreadImageGrabber::process() 
{
	unsigned long long pictureNumber = 0;
	std::unique_lock<std::mutex> lock(input->runMutex);
	while (!input->Andor->cameraThreadExitIndicator) {
		// wait until/unless camera is ready to take images. The signaler should be waked before worker's so that the grabber can wait on 'pop'
		while (!input->Andor->threadExpectingAcquisition) {
			input->signaler.wait(lock);
		}
		auto popPictureNumber = input->picBufferQueue->pop();
		if (popPictureNumber != pictureNumber) {
			emit error("AndorCameraThreadImageGrabber error: \r\n The pop-ed pictureNumber from WorkerThread is: " + qstr(popPictureNumber)
				+ ", but the expected pictureNumber is: " + qstr(pictureNumber)
				+ ". This is a low-level error as no other thread should be able to pop the picBufferQueue and the order should be FIFO.", 0);
		}
		input->Andor->updatePictureNumber(pictureNumber);
		try {
			auto images = input->Andor->acquireImageData();
			auto repVar = input->Andor->getCurrentRepVarNumber(pictureNumber);
			AndorRunSettings curSettings = input->Andor->getAndorRunSettings();
			size_t currentActivePicNum = curSettings.continuousMode ? 0 : pictureNumber % curSettings.picsPerRepetition;
			// push to imageQueue which will invoke the atomCruncher thread
			input->imageQueue.push({ {pictureNumber, repVar.first, repVar.second}, images[currentActivePicNum] });
			emit pictureGrabbed({ {pictureNumber, repVar.first, repVar.second}, images[currentActivePicNum] });
		}
		catch (ChimeraError& err) {
			emit error(err.qtrace(), 0);
			emit pauseExperiment();
			return;
		}
		pictureNumber++;
	}


}

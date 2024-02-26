#include "stdafx.h"
#include "Andor/AndorCameraThreadWorker.h"
#include <Andor/AndorCameraCore.h>
#include <qdebug.h>

AndorCameraThreadWorker::AndorCameraThreadWorker (cameraThreadWorkerInput* input_){
	input = input_;
}

AndorCameraThreadWorker::~AndorCameraThreadWorker () {
}

void AndorCameraThreadWorker::process (){
	//... I'm not sure what this lock is doing here... why not inside while loop?
	int safeModeCount = 0;
	unsigned long long pictureNumber = 0;
	bool armed = false;
	std::unique_lock<std::timed_mutex> lock (input->runMutex, std::chrono::milliseconds (1000));
	if (!lock.owns_lock ()) {
		errBox ("ERROR: ANDOR IMAGING THREAD FAILED TO LOCK THE RUN MUTEX! IMAGING THREAD CLOSING!");
	}
	while (!input->Andor->cameraThreadExitIndicator){
		/*
		 * wait until unlocked. this happens when data is started.
		 * the first argument is the lock.  The when the lock is locked, this function just sits and doesn't use cpu,
		 * unlike a while(gGlobalCheck){} loop that waits for gGlobalCheck to be set. The second argument here is a
		 * lambda, more or less a quick inline function that doesn't in this case have a name. This handles something
		 * called spurious wakeups, which are weird and appear to relate to some optimization things from the quick
		 * search I did. Regardless, I don't fully understand why spurious wakeups occur, but this protects against
		 * them.
		 */
		 // Also, anytime this gets locked, the count should be reset.
		 // input->signaler.wait( lock, [input]() { return input->expectingAcquisition; } ); // equivalent to code below, check first before lock
		while (!input->Andor->threadExpectingAcquisition) {
			input->signaler.wait (lock);
		}
		if (!input->Andor->safemode){
			try	{
				if (pictureNumber == input->Andor->runSettings.totalPicsInExperiment() && armed) {
					// get the last picture. acquisition is over 					
					// make sure the thread waits when it hits the condition variable.
					input->Andor->threadExpectingAcquisition = false;
					pictureNumber = 0;
					armed = false;
				}
				else{
					while (true) {
						try {
							input->Andor->waitForAcquisition(pictureNumber, 1000);
						}
						catch (ChimeraError& e) {
							if (e.whatBare() == "AT_ERR_TIMEDOUT") {
								if (input->Andor->cameraIsRunning) {
									qDebug() << "input->Andor->waitForAcquisition time out for 1000ms, camera still running, will re-wait for acquisition";
									continue;
								}
								else {
									qDebug() << "input->Andor->waitForAcquisition time out for 1000ms, camera NOT running, will abort and reset pic counter";
									input->Andor->threadExpectingAcquisition = false;
									pictureNumber = 0;
									armed = false;
								}
							}
							else {
								emit error("AndorThreadWorker error: \r\n input->Andor->waitForAcquisition error \r\n" + e.qtrace(), 0);
							}
						}
						break;
					}
					if (pictureNumber % 2 == 0) {
						(*input->imageTimes).push_back (std::chrono::high_resolution_clock::now ());
					}
					qDebug() << "From Worker thread: get image number" << pictureNumber << " at " << (*input->imageTimes).back().time_since_epoch().count() - (*input->imageTimes)[0].time_since_epoch().count();
					armed = true;
					if (!input->Andor->cameraIsRunning) {
						// aborted by user
						input->Andor->threadExpectingAcquisition = false;
						pictureNumber = 0;
						armed = false;
						qDebug() << "CameraThreadWorker aborted from user by awakening it again from the waitForAcquisition";
					}
					else {
						input->picBufferQueue.push(pictureNumber); // this will wake Grabber thread to grab image from memory with buffer identified by pictureNumber
						input->Andor->queueBuffers(pictureNumber + 1); // immediately requeue buffer for next image reading
						input->Andor->setExpRunningExposure(pictureNumber + 1); // set exposure, probably should only use external exposure so do not need to worry about time delay in this function
						//emit pictureTaken (pictureNumber);
						pictureNumber++;
					}
				}
			}
			catch (ChimeraError&) {
				//...? When does this happen? not sure why this is here...
			}
		}
		else { // safemode
			// simulate an actual wait.
			Sleep (500);
			qDebug() << "Andor safemode debug: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			if (pictureNumber % 2 == 0) {
				(*input->imageTimes).push_back (std::chrono::high_resolution_clock::now ());
			}
			if (input->Andor->cameraIsRunning && safeModeCount < input->Andor->runSettings.totalPicsInExperiment ()) {
				if (true/*input->Andor->runSettings.acquisitionMode == AndorRunModes::mode::Kinetic*/) {
					if (input->Andor->isCalibrating ()) {
						//input->comm->sendCameraCalProgress (safeModeCount);
					}
					else {
						emit pictureTaken (safeModeCount);
						//input->comm->sendCameraProgress (safeModeCount);
					}
					safeModeCount++;
				}
				else {
					if (input->Andor->isCalibrating ()) {
						//input->comm->sendCameraCalProgress (1);
					}
					else {
						emit pictureTaken (1);
						//input->comm->sendCameraProgress (1);
					}
				}
			}
			else{
				input->Andor->cameraIsRunning = false;
				safeModeCount = 0;
				if (input->Andor->isCalibrating ()) {
					//input->comm->sendCameraCalFin ();
				}
				else {
					emit acquisitionFinished ();
					//input->comm->sendCameraFin ();
				}
				input->Andor->threadExpectingAcquisition = false;
			}
		}
	}
}


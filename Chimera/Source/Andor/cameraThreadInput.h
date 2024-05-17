#pragma once

#include <memory>
#include <vector>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <GeneralObjects/ThreadsafeQueue.h>
#include <GeneralObjects/Queues.h>

class AndorCameraCore;

struct cameraThreadWorkerInput {
	std::timed_mutex runMutex;
	std::condition_variable_any signaler;
	// This thread-safe queue is storing the picture number for the pcitures to be read from memory 
	// More detailly, the buffer is return-ed in waitForAcquisition as tempImageBuffers[(pictureNumber) % numberOfImageBuffers]), therefore pictureNumber can be used to retrieve this buffer
	// This queue is constantly being tried to get pop-ed by the imageGrabber, which get sigaled after a successful pop to grab the image from Andor
	ThreadsafeQueue<unsigned long long>	picBufferQueue;
	// Andor is set to this in the constructor of the andor camera.
	AndorCameraCore* Andor;
	std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>>* imageTimes;
};

struct cameraThreadImageGrabberInput {
	std::mutex runMutex;
	std::condition_variable_any signaler;
	ThreadsafeQueue<unsigned long long>* picBufferQueue;
	// This thread-safe queue is being pop-ed by atomCrucherThread as atomCruncherInput is holding a pointer to this
	ThreadsafeQueue<NormalImage> imageQueue;
	AndorCameraCore* Andor;
	std::atomic<bool>* cruncherThreadActive; // for terminating cruncherThread when acquisition is stopped either by user or normal finish
	std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>>* imageTimes;
};
#pragma once
#include "GeneralObjects/IDeviceCore.h"
#include "FrameObserver.h"
#include "MakoWrapper.h"
#include "MakoSettingControl.h"
#include "CMOSSetting.h"

class MakoCameraCore : public IDeviceCore
{
public:
	// THIS CLASS IS NOT COPYABLE.
	MakoCameraCore& operator=(const MakoCameraCore&) = delete;
	MakoCameraCore(const MakoCameraCore&) = delete;

	MakoCameraCore(std::string ip, bool SAFEMODE);
	~MakoCameraCore();
	
	std::string getDelim() override { return configDelim; }
	void loadExpSettings(ConfigStream& stream) override {};
	void logSettings(DataLogger& logger, ExpThreadWorker* threadworker) override {};
	void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker) override {};
	void programVariation(unsigned variation, std::vector<parameterType>& params,
		ExpThreadWorker* threadworker) override {};
	void normalFinish() override {};
	void errorFinish() override {};
	

	void initializeVimba();
	void validateCamera(const CameraPtrVector& Cameras);

	void releaseBuffer();

	void checkDisplayInterval();

	bool isStreamingAvailable();

	void prepareCapture();

	void startCapture();

	void stopCapture();

	void setROI(int width, int height, int offsetx, int offsety);

	void resetFullROI();

	void updateCurrentSettings();



	std::string CameraName() { return cameraName; }
	MakoSettingControl& getMakoCtrl() { return makoCtrl; }
	FrameObserver* getFrameObs() { return frameObs.get(); }
	CameraPtr& getCameraPtr() { return cameraPtr; }
	MakoSettings getRunningSettings() { return runSettings; }


public:
    std::string configDelim = "MAKO_CAMERA_SETTINGS";
	const unsigned int BUFFER_COUNT = 7;

private:
	VimbaSystem& m_VimbaSystem;
	//remember to setParent in the gui class
	MakoSettingControl makoCtrl;
	SP_DECL(FrameObserver) frameObs;
	std::string cameraIP;

	CameraPtr cameraPtr;
	std::string cameraName;

	MakoSettings runSettings;
	MakoSettings expRunSettings;
    

};


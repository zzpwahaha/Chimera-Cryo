#pragma once
#include "GeneralObjects/IDeviceCore.h"
#include "FrameObserver.h"
#include "ImageCalculatingThread.h"
#include "MakoWrapper.h"

class MakoCameraCore :
    public IDeviceCore
{
	
	std::string getDelim() override { return configDelim; }
	void loadExpSettings(ConfigStream& stream) override;
	void logSettings(DataLogger& logger, ExpThreadWorker* threadworker) override;
	void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker) override;
	void programVariation(unsigned variation, std::vector<parameterType>& params,
		ExpThreadWorker* threadworker) override {};
	void normalFinish() override {};
	void errorFinish() override {};
	



public:
    std::string configDelim = "MAKO_CAMERA_SETTINGS";

private:
	FrameObserver frameObs;
	ImageCalculatingThread imgCThread;
    
};


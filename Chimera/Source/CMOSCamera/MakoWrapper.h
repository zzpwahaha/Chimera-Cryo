#pragma once
#include "VimbaCPP/Include/VimbaSystem.h"
#include <VimbaCPP/Include/ICameraListObserver.h>

using namespace AVT::VmbAPI;
using AVT::VmbAPI::CameraPtrVector;

/*use this class to keep an eye on the connection of camera in case it get plugged/unplugged after Chimera started*/
class CameraObserver : public QObject, public AVT::VmbAPI::ICameraListObserver
{
	Q_OBJECT
public:
	CameraObserver(void) {};
	~CameraObserver(void) {};
	virtual void CameraListChanged(AVT::VmbAPI::CameraPtr pCam, AVT::VmbAPI::UpdateTriggerType reason) 
	{
		thrower("FATAL ERROR: Mako camera list changed! Check camera connection, either plugged in or disconnected one or several cameras!"
		"\n This should not happen.");
	};

protected:
private:

signals:
	void updateDeviceList(void);
};

typedef AVT::VmbAPI::shared_ptr<CameraObserver> QtCameraObserverPtr;



class MakoWrapper
{
public:
	// THIS CLASS IS NOT COPYABLE.
	MakoWrapper& operator=(const MakoWrapper&) = delete;
	MakoWrapper(const MakoWrapper&) = delete;

	MakoWrapper();
	~MakoWrapper();

	static InterfacePtr getInterfaceByID(VimbaSystem& vsys, std::string sInterfaceID);
	static FeaturePtr getInterfaceFeatureByName(InterfacePtr interfaceP, std::string featurename);
	static std::string getFeatureValue(FeaturePtr featPtr);
	static void setIntegerValue(FeaturePtr featPtr, long long val);
	static void setFloatingValue(FeaturePtr featPtr, double dValue);
	static std::string getFeatureInformation(FeaturePtr featPtr);
	static bool isEventFeature(FeaturePtr pFeature);


	void initializeVimba();
	void searchCameras(const CameraPtrVector& Cameras);
	VmbErrorType getCameraDisplayName(const CameraPtr& camera, std::string& sDisplayName);
	VmbErrorType getIPAddress(const AVT::VmbAPI::CameraPtr& camera, std::string& sIPAdress);
private:
	VimbaSystem& m_VimbaSystem;
	std::array<std::string, MAKO_NUMBER> cameraNames;
	std::vector<CameraPtr> cameraPtrs;


};


#include "stdafx.h"
#include "MakoCameraCore.h"
#include <GeneralObjects/IChimeraSystem.h>
#include "ConfigurationSystems/ConfigSystem.h"
#include <ExperimentThread/ExpThreadWorker.h>
#include "MiscellaneousExperimentOptions/Repetitions.h"
#include "MiscellaneousExperimentOptions/MainOptionsControl.h"
#include <DataLogging/DataLogger.h>
#include <qdebug.h>

MakoCameraCore::MakoCameraCore(CameraInfo camInfo, IChimeraSystem* parent)
    : m_VimbaSystem(AVT::VmbAPI::VimbaSystem::GetInstance())
    , makoCtrl()
    , camInfo(camInfo)
    , pDeviceObs(new CameraObserver())
{
    if (camInfo.safemode) {
        return;
    }
    try {
        initializeVimba();
        SP_SET(frameObs, new FrameObserver(cameraPtr));
        makoCtrl.initialize(qstr(cameraName), cameraPtr); // cameraName is initialized in initializeVimba->validateCamera
    }
    catch (ChimeraError& e) {
        throwNested("Error in initializing MAKO Vimba control");
    }

    connect(&makoCtrl, &MakoSettingControl::resetFPS, [this]() {
        SP_ACCESS(frameObs)->resetFrameCounter(false); });

    connect(pDeviceObs.get(), &CameraObserver::deviceListChanged, parent, [this, parent](QString err) {
        emit parent->warning(err, 0); });

}

MakoCameraCore::~MakoCameraCore()
{
    qDebug() << "dctor of mako core";
    releaseBuffer();
    cameraPtr->Close();
    m_VimbaSystem.Shutdown();
    qDebug() << "finish dctor of mako core";
}

void MakoCameraCore::logSettings(DataLogger& log, ExpThreadWorker* threadworker)
{
    try {
        if (!experimentActive) {
            H5::Group makoGroup(log.file.createGroup("/Mako:Off"));
            return;
        }
        H5::Group makoGroup;
        try {
            makoGroup = log.file.openGroup("/Mako");
        }
        catch (H5::Exception&) {
            makoGroup = log.file.createGroup("/Mako");
        }
        H5::Group makoSubGroup(makoGroup.createGroup(CameraInfo::toStr(camInfo.camName)));
        hsize_t rank1[] = { 1 };
        // pictures. These are permanent members of the class for speed during the writing process.	
        hsize_t setDims[] = { unsigned __int64(expRunSettings.totalPictures()), expRunSettings.dims.height(),
                               expRunSettings.dims.width() };
        hsize_t picDims[] = { 1, expRunSettings.dims.height(), expRunSettings.dims.width() };
        log.MakoPicureSetDataSpace[camInfo.camName] = H5::DataSpace(3, setDims);
        log.MakoPicDataSpace[camInfo.camName] = H5::DataSpace(3, picDims);
        log.MakoPictureDataset[camInfo.camName] = makoSubGroup.createDataSet("Pictures", H5::PredType::NATIVE_LONG,
            log.MakoPicureSetDataSpace[camInfo.camName]);
        log.currentMakoPicNumber[camInfo.camName] = 0;
        //log.writeDataSet(BaslerAcquisition::toStr(expRunSettings.acquisitionMode), "Camera-Mode", baslerGroup);
        //log.writeDataSet(BaslerAutoExposure::toStr(expRunSettings.exposureMode), "Exposure-Mode", baslerGroup);
        log.writeDataSet(expRunSettings.exposureTime, "Exposure-Time", makoSubGroup);
        log.writeDataSet(MakoTrigger::toStr(expRunSettings.triggerMode), "Trigger-Mode", makoSubGroup);
        // image settings
        H5::Group imageDims = makoSubGroup.createGroup("Image-Dimensions");
        log.writeDataSet(expRunSettings.dims.top, "Top", imageDims);
        log.writeDataSet(expRunSettings.dims.bottom, "Bottom", imageDims);
        log.writeDataSet(expRunSettings.dims.left, "Left", imageDims);
        log.writeDataSet(expRunSettings.dims.right, "Right", imageDims);
        log.writeDataSet(expRunSettings.dims.horizontalBinning, "Horizontal-Binning", imageDims);
        log.writeDataSet(expRunSettings.dims.verticalBinning, "Vertical-Binning", imageDims);
        log.writeDataSet(expRunSettings.frameRate, "Frame-Rate", makoSubGroup);
        log.writeDataSet(expRunSettings.rawGain, "Raw-Gain", makoSubGroup);
    }
    catch (H5::Exception err) {
        log.logError(err);
        throwNested("ERROR: Failed to log basler parameters in HDF5 file: " + err.getDetailMsg());
    }
}

// to enable emitting imageReadyForExp in ImageCalculatingThread
void MakoCameraCore::loadExpSettings(ConfigStream& stream) {
    if (camInfo.safemode) {
        return;
    }
    ConfigSystem::stdGetFromConfig(stream, *this, expRunSettings, Version("1.0"));
    expRunSettings.repsPerVar = ConfigSystem::stdConfigGetter(stream, "REPETITIONS",
        Repetitions::getSettingsFromConfig);
    mainOptions mainOpts = ConfigSystem::stdConfigGetter(stream, "MAIN_OPTIONS",
        MainOptionsControl::getSettingsFromConfig);
    expRunSettings.repFirst = mainOpts.repetitionFirst;
    experimentActive = expRunSettings.expActive;
    if (experimentActive) {
        emit makoStarted();
    }
}

void MakoCameraCore::calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker)
{
    if (camInfo.safemode) {
        return;
    }
    expRunSettings.variations = (params.size() == 0 ? 1 : params.front().keyValues.size());
    expRunSettings.variationShuffleIndex = params.front().shuffleIndex;
    makoCtrl.setSettings(expRunSettings);
    runSettings = expRunSettings;    
    if (experimentActive) {
        emit threadworker->prepareMako(&expRunSettings, camInfo);//Qt::BlockingQueuedConnection
    }
    
}

// to disable emitting imageReadyForExp in ImageCalculatingThread
void MakoCameraCore::normalFinish()
{
    emit makoFinished();
}

void MakoCameraCore::errorFinish()
{
    emit makoFinished();
}

MakoSettings MakoCameraCore::getSettingsFromConfig(ConfigStream& configFile)
{
    MakoSettings newSettings;
    configFile >> newSettings.expActive;
    std::string test;
    try {
        configFile >> test;
        newSettings.dims.left = boost::lexical_cast<int>(test);
        configFile >> test;
        newSettings.dims.top = boost::lexical_cast<int>(test);
        configFile >> test;
        newSettings.dims.right = boost::lexical_cast<int>(test);
        configFile >> test;
        newSettings.dims.bottom = boost::lexical_cast<int>(test);
    }
    catch (boost::bad_lexical_cast&) {
        throwNested("Mako control failed to convert dimensions recorded in the config file "
            "to integers");
    }
    configFile >> newSettings.dims.horizontalBinning;
    configFile >> newSettings.dims.verticalBinning;
    configFile >> newSettings.exposureTime;
    configFile >> newSettings.frameRate;
    configFile >> newSettings.rawGain;
    configFile >> newSettings.picsPerRep;
    std::string txt;
    configFile >> txt;
    newSettings.triggerMode = MakoTrigger::fromStr(txt);
    return newSettings;
}

void MakoCameraCore::initializeVimba()
{
    CameraPtrVector     currentListedCameras;
    VmbErrorType error = m_VimbaSystem.Startup();
    if (VmbErrorSuccess != error)
    {
        thrower("Startup failed, Error: " + str(Helper::mapReturnCodeToString(error)));
        return;
    }
    try
    {
        CameraPtrVector tmpCameras;
        error = m_VimbaSystem.GetCameras(tmpCameras);
        /*if (tmpCameras.size() != MAKO_NUMBER) {
            thrower("Less number of cMOS cameras than expected. Expect" + str(MAKO_NUMBER) + " but only get" + str(tmpCameras.size()));
        }*/
        if (VmbErrorSuccess == error)
        {
            validateCamera(tmpCameras);
            if (VmbErrorSuccess != error)
            {
                thrower("RegisterCameraListObserver Failed, Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
            }
        }
        else
        {
            thrower("could not get camera list,  Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
        }

    }
    catch (ChimeraError& e)
    {
        throwNested(str("Mako <constructor> Exception: "));
    }
}

void MakoCameraCore::validateCamera(const CameraPtrVector& Cameras)
{
    QMap <QString, QString>     ifTypeMap;
    InterfacePtrVector          ifPtrVec;
    std::string                 sInterfaceID;
    VmbErrorType                error;
    /* list all interfaces found by VmbAPI */
    error = m_VimbaSystem.GetInterfaces(ifPtrVec);
    if (VmbErrorSuccess != error)
    {
        thrower("GetInterfaces Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
        return;
    }
    /* check type of Interfaces and complain if there is types other than GigE*/
    for (unsigned int i = 0; i < ifPtrVec.size(); i++)
    {
        error = ifPtrVec.at(i)->GetID(sInterfaceID);
        if (VmbErrorSuccess != error) {
            thrower("GetID <Interface " + str(i) + " Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
            continue;
        }

        VmbInterfaceType    ifType = VmbInterfaceUnknown;
        VmbErrorType        errorGetType = ifPtrVec.at(i)->GetType(ifType);
        if (VmbErrorSuccess != errorGetType) {
            thrower("GetType <Interface " + str(i) + " Failed, Error: " + str(Helper::mapReturnCodeToString(errorGetType)));
            continue;
        }

        switch (ifType)
        {
        case VmbInterfaceEthernet:
            ifTypeMap[qstr(sInterfaceID)] = "GigE";
            break;
        case VmbInterfaceCL:
            break; // ignore the CL
        case VmbInterfaceUsb:
            break; // ignore the USB
        default:
            thrower("Error in search Mako camera, found camera type other than GigE (and CL and USB)");
        }
    }

    std::string allIPs;
    /*gather info for all listed camera and compare to the expected one from constant.h*/
    for (unsigned int i = 0; i < Cameras.size(); i++)
    {
        /*check ip address*/
        std::string ipaddress;
        error = MakoWrapper::getIPAddress(Cameras[i], ipaddress);
        if (VmbErrorSuccess != error)
        {
            thrower("GetIPAddress error for camera " + str(i) + "Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
            continue;
        }
        allIPs += ipaddress.append(" ");
        if (ipaddress.find(camInfo.ip) != std::string::npos) {
            /*register camera list observer*/
            cameraPtr = Cameras[i];
            error = m_VimbaSystem.RegisterCameraListObserver(pDeviceObs);

            /*get camera name*/
            std::string displayName;
            error = MakoWrapper::getCameraDisplayName(Cameras[i], displayName);
            if (VmbErrorSuccess != error)
            {
                thrower("GetDisplayName error for camera " + str(i) + "Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
                continue;
            }
            cameraName = displayName;

            /*check access type*/
            VmbAccessModeType accessType = VmbAccessModeType::VmbAccessModeNone;
            error = Cameras[i]->GetPermittedAccess(accessType);
            if (VmbErrorSuccess == error) {
                if (!(accessType & VmbAccessModeType::VmbAccessModeFull)) {
                    thrower("Camera do not support full access mode, check if there is running program that grab the camera.");
                }
            }
            else {
                thrower("Error in GetPermittedAccess. Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
            }

            /*open camera*/
            VmbError_t error = cameraPtr->Open(VmbAccessModeFull);
            if (VmbErrorSuccess != error) {
                thrower("Fatal Error, Failed to open Mako camera " + str(cameraName) + "with error:" + str(error)
                    + " " + str(Helper::mapReturnCodeToString(error)));
                return;
            }
            return;
        }
    }
    thrower("Mako validate Error: Did not find the camera IP in the current cameras. Want " + camInfo.ip +
        ", but the the listed IPs are " + allIPs);
}

void MakoCameraCore::releaseBuffer()
{
    frameObs->Stopping();
    //m_pImgCThread->StopProcessing();
    VmbError_t error = cameraPtr->EndCapture();
    if (VmbErrorSuccess == error)
        error = cameraPtr->FlushQueue();
    if (VmbErrorSuccess == error)
        error = cameraPtr->RevokeAllFrames();

}

void MakoCameraCore::checkDisplayInterval()
{
    FeaturePtr pFeatMode;

    if (VmbErrorSuccess == cameraPtr->GetFeatureByName("AcquisitionMode", pFeatMode))
    {
        std::string sValue("");
        if (VmbErrorSuccess == pFeatMode->GetValue(sValue))
        {
            /* display all received frames for SingleFrame and MultiFrame mode or if the user wants to have it */
            if (0 == sValue.compare("SingleFrame") || 0 == sValue.compare("MultiFrame"))
                SP_ACCESS(frameObs)->ImageProcessThreadPtr()->LimitFrameRate(false);
            /* display frame in a certain interval to save CPU consumption for continuous mode */
            else
                SP_ACCESS(frameObs)->ImageProcessThreadPtr()->LimitFrameRate(true);
        }
    }
    cameraPtr->GetFeatureByName("TriggerMode", pFeatMode);
    std::string trigmode("");
    if (VmbErrorSuccess == pFeatMode->GetValue(trigmode))
    if (VmbErrorSuccess == cameraPtr->GetFeatureByName("TriggerSource", pFeatMode))
    {
        std::string sValue("");
        if (VmbErrorSuccess == pFeatMode->GetValue(sValue))
        {
            /* display all received frames for fixed rate mode or hardware trigger*/
            if (0 == trigmode.compare("On") && (0 == sValue.compare("FixedRate") || 0 == sValue.compare("Line1")))
                SP_ACCESS(frameObs)->ImageProcessThreadPtr()->LimitFrameRate(false);
            else
                SP_ACCESS(frameObs)->ImageProcessThreadPtr()->LimitFrameRate(true);
        }
    }
}

bool MakoCameraCore::isStreamingAvailable()
{
    AVT::VmbAPI::FeaturePtr pStreamIDFeature;
    cameraPtr->GetFeatureByName("StreamID", pStreamIDFeature);
    return (NULL == pStreamIDFeature) ? false : true;
}

void MakoCameraCore::prepareCapture()
{
    FeaturePtr pFeature;
    VmbInt64_t nPayload = 0;
    QVector <FramePtr> frames;
    VmbError_t error = cameraPtr->GetFeatureByName("PayloadSize", pFeature);
    VmbUint32_t nCounter = 0;
    if (VmbErrorSuccess != error)
    {
        thrower(str(QTime::currentTime().toString("hh:mm:ss:zzz")) + "\t" + " GetFeatureByName [PayloadSize] Failed! Error: "
            + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
        return;
    }

    error = pFeature->GetValue(nPayload);
    if (VmbErrorSuccess != error)
    {
        thrower(str(QTime::currentTime().toString("hh:mm:ss:zzz")) + "\t" + " GetValue [PayloadSize] Failed! Error: "
            + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
        return;
    }

    frames.resize(BUFFER_COUNT);
    bool bIsStreamingAvailable = isStreamingAvailable();
    if (bIsStreamingAvailable)
    {
        for (int i = 0; i < frames.size(); i++)
        {
            try {
                frames[i] = FramePtr(new Frame(nPayload));
                nCounter++;
            }
            catch (std::bad_alloc&) {
                frames.resize((VmbInt64_t)(nCounter * 0.7));
                break;
            }
            /*this is the key part to set the frame thread start to receive signal*/
            frameObs->Starting();
            /*the start() will do a lot of overhead to create the thread and it eventually call run()*/

            error = frames[i]->RegisterObserver(frameObs);
            if (VmbErrorSuccess != error)
            {
                thrower(str(QTime::currentTime().toString("hh:mm:ss:zzz")) + "\t" + " RegisterObserver frame[" + str(i) + "] Failed! Error: "
                    + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
            }
        }

        for (int i = 0; i < frames.size(); i++)
        {
            error = cameraPtr->AnnounceFrame(frames[i]);
            if (VmbErrorSuccess != error)
            {
                thrower(str(QTime::currentTime().toString("hh:mm:ss:zzz")) + "\t" + " AnnounceFrame frame[" + str(i) + "] Failed! Error: "
                    + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
            }
        }
    }

    error = cameraPtr->StartCapture();
    if (VmbErrorSuccess != error)
    {
        std::string sMessage = " StartCapture Failed! Error: ";
        thrower(str(QTime::currentTime().toString("hh:mm:ss:zzz")) + "\t" + sMessage + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
    }
    
    if (bIsStreamingAvailable)
    {
        for (int i = 0; i < frames.size(); i++)
        {
            error = cameraPtr->QueueFrame(frames[i]);
            if (VmbErrorSuccess != error)
            {
                thrower(str(QTime::currentTime().toString("hh:mm:ss:zzz")) + "\t" + " QueueFrame [" + str(i) + " Failed! Error: "
                    + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
            }
        }
    }

}

void MakoCameraCore::startCapture()
{
    FeaturePtr pFeat;
    VmbError_t error = cameraPtr->GetFeatureByName("AcquisitionStart", pFeat);
    FeaturePtr pFeatFormat;
    auto error2 = cameraPtr->GetFeatureByName("PixelFormat", pFeatFormat);
    QString Pixformat = qstr(MakoWrapper::getFeatureValue(pFeatFormat));
    int nResult2 = Pixformat.compare("Mono12Packed");
    if ((VmbErrorSuccess == error) && (VmbErrorSuccess == error2) && (0 != nResult2))
    {
        SP_ACCESS(frameObs)->resetFrameCounter(true);
        error = pFeat->RunCommand();
        if (VmbErrorSuccess == error) {
            return;
        }
        else {
            thrower(str(QTime::currentTime().toString("hh:mm:ss:zzz")) + "\t" + "Start capture Failed! Error: "
                + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
        }
    }
    else {
        thrower("Failed to get FeatureName: AcquisitionStart \n"
            " or FormatName, or tried to use PACKED12, which is not supported in Chimera");
    }
}

void MakoCameraCore::stopCapture() 
{
    FeaturePtr pFeat;
    VmbError_t error = cameraPtr->GetFeatureByName("AcquisitionStop", pFeat);
    if (VmbErrorSuccess == error)
    {
        SP_ACCESS(frameObs)->resetFrameCounter(true);
        error = pFeat->RunCommand();
        if (VmbErrorSuccess == error) {
            return;
        }
        else {
            thrower(str(QTime::currentTime().toString("hh:mm:ss:zzz")) + "\t" + "Start capture Failed! Error: "
                + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
        }
    }
    else {
        thrower("Failed to get FeatureName: AcquisitionStop ");
    }
}

std::array<unsigned, 4> MakoCameraCore::getROIIncrement()
{
    FeaturePtr pFeat;
    VmbInt64_t xw, yw, ox, oy;
    if (VmbErrorSuccess == cameraPtr->GetFeatureByName("Width", pFeat))
    {
        VmbErrorType error = pFeat->GetIncrement(xw);
        if (VmbErrorSuccess != error)
        {
            thrower("Failed to get width increment " + str(xw) + ", and error: " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
        }
    }
    if (VmbErrorSuccess == cameraPtr->GetFeatureByName("Height", pFeat))
    {
        VmbErrorType error = pFeat->GetIncrement(yw);
        if (VmbErrorSuccess != error)
        {
            thrower("Failed to get Height increment " + str(yw) + ", and error: " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
        }
    }
    if (VmbErrorSuccess == cameraPtr->GetFeatureByName("OffsetX", pFeat))
    {
        VmbErrorType error = pFeat->GetIncrement(ox);
        if (VmbErrorSuccess != error)
        {
            thrower("Failed to get OffsetX increment " + str(ox) + ", and error: " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
        }
    }
    if (VmbErrorSuccess == cameraPtr->GetFeatureByName("OffsetY", pFeat))
    {
        VmbErrorType error = pFeat->GetIncrement(oy);
        if (VmbErrorSuccess != error)
        {
            thrower("Failed to get OffsetY increment " + str(oy) + ", and error: " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
        }
    }
    return std::array<unsigned, 4>({ unsigned(xw), unsigned(yw), unsigned(ox), unsigned(oy) });// xwidth, ywidth, offsetx, offsety
}

void MakoCameraCore::setROI(int width, int height, int offsetx, int offsety)
{
    FeaturePtr pFeat;
    if (VmbErrorSuccess == cameraPtr->GetFeatureByName("Width", pFeat))
    {
        VmbErrorType error = pFeat->SetValue(width);
        if (VmbErrorSuccess != error)
        {
            thrower("Failed to set ROI with width " + str(width) + ", and error: " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
            return;
        }
    }
    if (VmbErrorSuccess == cameraPtr->GetFeatureByName("Height", pFeat))
    {
        VmbErrorType error = pFeat->SetValue(height);
        if (VmbErrorSuccess != error)
        {
            thrower("Failed to set ROI with height " + str(height) + ", and error: " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
            return;
        }
    }
    if (VmbErrorSuccess == cameraPtr->GetFeatureByName("OffsetX", pFeat))
    {
        VmbErrorType error = pFeat->SetValue(offsetx);
        if (VmbErrorSuccess != error)
        {
            thrower("Failed to set ROI with offsetx " + str(offsetx) + ", and error: " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
            return;
        }
    }
    if (VmbErrorSuccess == cameraPtr->GetFeatureByName("OffsetY", pFeat))
    {
        VmbErrorType error = pFeat->SetValue(offsety);
        if (VmbErrorSuccess != error)
        {
            thrower("Failed to set ROI with offsety " + str(offsety) + ", and error: " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
            return;
        }
    }
}

void MakoCameraCore::resetFullROI()
{
    FeaturePtr pFeat;
    if (VmbErrorSuccess == cameraPtr->GetFeatureByName("OffsetX", pFeat))
    {
        VmbErrorType error = pFeat->SetValue(0);
        if (VmbErrorSuccess != error)
        {
            thrower("Failed to set ROI with OffsetX " + str(0) + ", and error: " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
            return;
        }
    }
    if (VmbErrorSuccess == cameraPtr->GetFeatureByName("OffsetY", pFeat))
    {
        VmbErrorType error = pFeat->SetValue(0);
        if (VmbErrorSuccess != error)
        {
            thrower("Failed to set ROI with OffsetY " + str(0) + ", and error: " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
            return;
        }
    }
    VmbInt64_t  maxVal = 0;
    if (VmbErrorSuccess == cameraPtr->GetFeatureByName("HeightMax", pFeat))
    {
        if (VmbErrorSuccess == pFeat->GetValue(maxVal))
        {
            if (VmbErrorSuccess == cameraPtr->GetFeatureByName("Height", pFeat))
            {
                VmbErrorType error = pFeat->SetValue(maxVal);
                if (VmbErrorSuccess != error)
                {
                    thrower("Failed to set ROI with height " + str(maxVal) + ", and error: " + str(error) +
                        " " + str(Helper::mapReturnCodeToString(error)));
                    return;
                }
            }
        }
    }

    if (VmbErrorSuccess == cameraPtr->GetFeatureByName("WidthMax", pFeat))
    {
        if (VmbErrorSuccess == pFeat->GetValue(maxVal))
        {
            if (VmbErrorSuccess == cameraPtr->GetFeatureByName("Width", pFeat))
            {
                VmbErrorType error = pFeat->SetValue(maxVal);
                if (VmbErrorSuccess != error)
                {
                    thrower("Failed to set ROI with width " + str(maxVal) + ", and error: " + str(error) +
                        " " + str(Helper::mapReturnCodeToString(error)));
                    return;
                }
            }
        }
    }

}

void MakoCameraCore::updateCurrentSettings()
{
    makoCtrl.updateCurrentSettings();
    runSettings = makoCtrl.getCurrentSettings();
}

void MakoCameraCore::setExpActive(bool active)
{
    runSettings.expActive = active;
    expRunSettings.expActive = active;
    makoCtrl.setExpActive(active);
}

void MakoCameraCore::setPicsPerRep(int picsperrep)
{
    runSettings.picsPerRep = picsperrep;
    expRunSettings.picsPerRep = picsperrep;
    makoCtrl.setPicsPerRep(picsperrep);
}

//void MakoCameraCore::setCurrentRepVarNumber(size_t rep, size_t var)
//{
//    currentRepNumber = rep;
//    currentVarNumber = var;
//}

std::pair<size_t, size_t> MakoCameraCore::getCurrentRepVarNumber(unsigned int currentPicNumber)
{
    size_t currentRepNumber, currentVarNumber;
    if (expRunSettings.repFirst) {
        currentVarNumber = (currentPicNumber / expRunSettings.picsPerRep) / expRunSettings.repsPerVar;
        currentRepNumber = (currentPicNumber / expRunSettings.picsPerRep) % expRunSettings.repsPerVar;
    }
    else {
        currentRepNumber = (currentPicNumber / expRunSettings.picsPerRep) / expRunSettings.variations;
        currentVarNumber = (currentPicNumber / expRunSettings.picsPerRep) % expRunSettings.variations;
    }
    if (expRunSettings.variationShuffleIndex.size() < currentVarNumber) {
        thrower("MakoCamera variationShuffleIndex size" + str(expRunSettings.variationShuffleIndex.size()) + " is smaller than currentVarNumber" + str(currentVarNumber) + "A low level bug!");
    }
    return std::pair<size_t, size_t>(currentRepNumber, expRunSettings.variationShuffleIndex[currentVarNumber]);
}

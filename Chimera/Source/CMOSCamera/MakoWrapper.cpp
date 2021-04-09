#include "stdafx.h"
#include "MakoWrapper.h"
#include <CMOSCamera/Helper.h>
#include <qendian.h>

MakoWrapper::MakoWrapper()
	: m_VimbaSystem(AVT::VmbAPI::VimbaSystem::GetInstance())
{
}

MakoWrapper::~MakoWrapper()
{
    m_VimbaSystem.Shutdown();
}

InterfacePtr MakoWrapper::getInterfaceByID(VimbaSystem& vsys, std::string sInterfaceID)
{
    InterfacePtr interfacePtr;
    VmbErrorType error = vsys.GetInterfaceByID(sInterfaceID.c_str(), interfacePtr);
    if (VmbErrorSuccess != error) {
        thrower("GetType <Interface> by ID Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
    }
    return interfacePtr;
}

FeaturePtr MakoWrapper::getInterfaceFeatureByName(InterfacePtr interfaceP, std::string featurename)
{
    FeaturePtr featurePtr; 
    VmbErrorType error = interfaceP->GetFeatureByName(featurename.c_str(), featurePtr);
    if (VmbErrorSuccess != error) {
        thrower("GetType <Feature> by feature name Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
    }
    return featurePtr;
}

std::string MakoWrapper::getFeatureValue(FeaturePtr featPtr)
{
    VmbInt64_t  nValue64 = 0;
    double      dValue = 0;
    bool        bValue = false;
    std::string stdValue;

    std::string sValue;

    VmbFeatureDataType dataType = VmbFeatureDataUnknown;
    VmbError_t error = featPtr->GetDataType(dataType);

    if (VmbErrorSuccess == error)
    {
        switch (dataType)
        {
        case VmbFeatureDataInt:
            error = featPtr->GetValue(nValue64);
            if (VmbErrorSuccess == error){ 
                sValue = str(nValue64); 
            }
            else{ 
                thrower("Get feature value int Failed, Error: " + str(Helper::mapReturnCodeToString(error))); 
            }
            break;

        case VmbFeatureDataFloat:
            error = featPtr->GetValue(dValue);
            if (VmbErrorSuccess == error) {
                sValue = str(dValue);
            }
            else {
                thrower("Get feature value double Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
            }
            break;

        case VmbFeatureDataEnum:
            error = featPtr->GetValue(stdValue);
            if (VmbErrorSuccess == error) {
                sValue = stdValue;
            }
            else {
                thrower("Get feature value string Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
            }
            break;

        case VmbFeatureDataString:
            error = featPtr->GetValue(stdValue);
            if (VmbErrorSuccess == error) {
                sValue = stdValue;
            }
            else {
                thrower("Get feature value string Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
            }
            break;

        case VmbFeatureDataBool:
            error = featPtr->GetValue(bValue);
            if (VmbErrorSuccess == error) {
                bValue ? sValue = "true" : sValue = "false";
            }
            else {
                thrower("Get feature value string bool Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
            }
        case VmbFeatureDataCommand:
            sValue = "[COMMAND]";
            break;

        case VmbFeatureDataRaw:
            sValue = "Click here to open";
            break;
        default: break;
        }
    }
    else {
        thrower("Get feature type Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
    }
    return sValue;
}


void MakoWrapper::setIntegerValue(FeaturePtr featPtr, long long val)
{
    long long min, max;
    VmbErrorType error;
    error = featPtr->GetRange(min, max);
    if (VmbErrorSuccess != error) {
        thrower("get feature range Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
    }
    if (val < min) { val = min; }
    if (val > max) { val = max; }

    error = featPtr->SetValue(val);
    if (VmbErrorSuccess != error) {
        thrower("Set int feature value Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
    }
}

void MakoWrapper::setFloatingValue(FeaturePtr featPtr, double dValue)
{
    double min, max;
    VmbErrorType error;
    error = featPtr->GetRange(min, max);
    if (VmbErrorSuccess != error) {
        thrower("get feature range Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
    }
    if (dValue < min) { dValue = min; }
    if (dValue > max) { dValue = max; }
    error = featPtr->SetValue(dValue);
    if (VmbErrorSuccess != error) {
        thrower("Set double feature value Failed, Error: " + str(Helper::mapReturnCodeToString(error)));
    }
}

void MakoWrapper::initializeVimba()
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
        if (tmpCameras.size() != MAKO_NUMBER) {
            thrower("Less number of cMOS cameras than expected. Expect" + str(MAKO_NUMBER) + " but only get" + str(tmpCameras.size()));
        }
        cameraPtrs = tmpCameras;
        if (VmbErrorSuccess == error)
        {
            searchCameras(tmpCameras);
            QtCameraObserverPtr pDeviceObs(new CameraObserver());
            error = m_VimbaSystem.RegisterCameraListObserver(pDeviceObs);
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
    catch (const ChimeraError& e)
    {
        throwNested(str("Mako <constructor> Exception: ") + e.what());
    }
}

void MakoWrapper::searchCameras(const CameraPtrVector& Cameras)
{
    QMap <QString, QString>     ifTypeMap;
    InterfacePtrVector          ifPtrVec;
    std::string                 sInterfaceID;
    VmbErrorType                error;
    /* list all interfaces found by VmbAPI */
    error = m_VimbaSystem.GetInterfaces(ifPtrVec);
    if (VmbErrorSuccess != error)
    {
        thrower("GetInterfaces Failed, Error: "+str(Helper::mapReturnCodeToString(error)));
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
        default: 
            thrower("Error in search Mako camera, found camera type other than GigE");
        }
    }
    
    /*gather info for all listed camera and compare to the expected one from constant.h*/
    for (unsigned int i = 0; i < Cameras.size(); i++)
    {
        /*get camera name*/
        std::string displayName;
        error = getCameraDisplayName(Cameras[i], displayName);
        if (VmbErrorSuccess != error)
        {
            thrower("GetDisplayName error for camera " + str(i) + "Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
            continue;
        }
        cameraNames[i] = displayName;

        /*check ip address*/
        std::string ipaddress;
        error = getIPAddress(Cameras[i], ipaddress);
        if (VmbErrorSuccess != error)
        {
            thrower("GetIPAddress error for camera " + str(i) + "Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
            continue;
        }

        /*check access type*/
        VmbAccessModeType accessType = VmbAccessModeType::VmbAccessModeNone;
        error = Cameras[i]->GetPermittedAccess(accessType);
        if (VmbErrorSuccess == error) {
            if (accessType != VmbAccessModeType::VmbAccessModeFull) {
                thrower("Camera do not support full access mode, check if there is running program that grab the camera.");
            }
        }
        else {
            thrower("Error in GetPermittedAccess. Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
        }
    }

}

std::string MakoWrapper::getFeatureInformation(FeaturePtr featPtr)
{
    std::string sInformation;
    VmbInt64_t nMin = 0, nMax = 0, nInc = 0;
    double dMin = 0, dMax = 0, dInc = 0;

    std::string sFeatureName;
    featPtr->GetName(sFeatureName);
    sInformation += "<b>FEATURE NAME:</b> " + sFeatureName + "<br/>";

    VmbFeatureVisibilityType visibility = VmbFeatureVisibilityUnknown;
    featPtr->GetVisibility(visibility);
    sInformation += "<b>VISIBILITY:</b> ";
    switch (visibility)
    {
    case VmbFeatureVisibilityUnknown:   sInformation.append("UNKNOWN<br/>");   break;
    case VmbFeatureVisibilityBeginner:  sInformation.append("BEGINNER<br/>");  break;
    case VmbFeatureVisibilityExpert:    sInformation.append("EXPERT<br/>");    break;
    case VmbFeatureVisibilityGuru:      sInformation.append("GURU<br/>");      break;
    case VmbFeatureVisibilityInvisible: sInformation.append("INVISIBLE<br/>"); break;
    default: sInformation.append("N/A<br/>"); break;
    }

    /* get feature type and type-specific info */
    VmbFeatureDataType dataType = VmbFeatureDataUnknown;
    VmbFeatureFlagsType flags;
    VmbError_t error = featPtr->GetDataType(dataType);
    if (VmbErrorSuccess == error)
    {
        switch (dataType)
        {
        case VmbFeatureDataInt:
        {
            /* only show range and increment for integer features that might change */
            if ((VmbErrorSuccess == featPtr->GetFlags(flags)) && ((((VmbFeatureFlagsVolatile | VmbFeatureFlagsWrite | VmbFeatureFlagsModifyWrite) & flags) != 0) || (VmbFeatureFlagsRead == flags)))
            {
                if (VmbErrorSuccess == featPtr->GetRange(nMin, nMax))
                    sInformation += "<b>TYPE:</b> Integer<br/><b>MINIMUM:</b> " + str(nMin) + "<br/><b>MAXIMUM:</b> " + str(nMax) + "<br/>";
                if ((VmbErrorSuccess == featPtr->GetIncrement(nInc)) && (1 != nInc))
                    sInformation += "<b>INTERVAL:</b> " + str(nInc) + "<br/>";
            }
            break;
        }
        case VmbFeatureDataFloat:
        {
            /* only show range and increment for float features that might change */
            VmbFeatureFlagsType flags;
            if ((VmbErrorSuccess == featPtr->GetFlags(flags)) && ((((VmbFeatureFlagsVolatile | VmbFeatureFlagsWrite | VmbFeatureFlagsModifyWrite) & flags) != 0) || (VmbFeatureFlagsRead == flags)))
            {
                if (VmbErrorSuccess == featPtr->GetRange(dMin, dMax))
                    sInformation += "<b>TYPE:</b> Float<br/><b>MINIMUM:</b> " + str(QString::number(dMin, 'g', 9)) + "<br/><b>MAXIMUM:</b> " + str(QString::number(dMax, 'g', 12)) + "<br/>";

                if (VmbErrorSuccess == featPtr->GetIncrement(dInc))
                    sInformation += "<b>INTERVAL:</b> " + str(QString::number(dInc, 'f', 10)) + "<br/>";
            }
            break;
        }
        case VmbFeatureDataEnum:    sInformation.append("<b>TYPE:</b> Enumeration<br/>"); break;
        case VmbFeatureDataString:  sInformation.append("<b>TYPE:</b> String<br/>");      break;
        case VmbFeatureDataBool:    sInformation.append("<b>TYPE:</b> Boolean<br/>");     break;
        case VmbFeatureDataCommand: sInformation.append("<b>TYPE:</b> Command<br/>");     break;
        case VmbFeatureDataRaw:     sInformation.append("<b>TYPE:</b> Raw<br/>");     break;
        default: break;
        }
    }

    std::string sCategory;
    featPtr->GetCategory(sCategory);
    sInformation += "<b>CATEGORY:</b> " + sCategory + "<br/>";

    FeaturePtrVector           featPtrVec;

    sInformation += "<br/><b>AFFECTED FEATURE(S):</b> ";
    featPtr->GetAffectedFeatures(featPtrVec);
    for (unsigned int i = 0; i < featPtrVec.size(); i++)
    {
        std::string sName;
        featPtrVec.at(i)->GetName(sName);

        if (0 == i)
            sInformation.append("<br/>");

        sInformation += sName;
        if (i + 1 != featPtrVec.size())
        {
            sInformation.append(", ");
            if (0 == ((i + 1) % 4) && (i != 0))
                sInformation.append("<br/>");
        }
    }

    if (0 == featPtrVec.size())
        sInformation += "N/A";

    sInformation.append("<br/>");
    return sInformation;
}

bool MakoWrapper::isEventFeature(FeaturePtr pFeature)
{
    std::string sCategory;
    if (!SP_ISNULL(pFeature)
        && VmbErrorSuccess == SP_ACCESS(pFeature)->GetCategory(sCategory)
        && std::strstr(sCategory.c_str(), "/EventID"))
    {
        return true;
    }
    else
    {
        return false;
    }
}



/*below two functions are directly copied from Vimba example and I do not want to mess up with it (even though it is definitly optimizable)*/
VmbErrorType MakoWrapper::getCameraDisplayName(const CameraPtr& camera, std::string& sDisplayName)
{
    VmbErrorType  error;
    std::string sID;
    std::string sSN;

    error = camera->GetModel(sDisplayName);
    if (VmbErrorSuccess != error) {
        thrower("GetModel Failed, Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
        return error;
    }
    else
    {
        error = camera->GetSerialNumber(sSN);
        if (VmbErrorSuccess != error)
        {
            thrower("GetSerialNumber Failed, Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
            return error;
        }
        else
        {
            error = camera->GetID(sID);
            if (VmbErrorSuccess != error)
            {
                thrower("GetID Failed, Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
                return error;
            }
            else
            {
                std::string sDisplayNameEnding;

                sDisplayNameEnding.append("-");
                sDisplayNameEnding.append(sSN);

                if (0 != sSN.compare(sID))
                {
                    sDisplayNameEnding.append("(");
                    sDisplayNameEnding.append(sID);
                    sDisplayNameEnding.append(")");
                }

                std::string sLegacyDisplayName = sDisplayName + sDisplayNameEnding;

                VmbInterfaceType cameraIFType;
                error = camera->GetInterfaceType(cameraIFType);

                if (VmbErrorSuccess != error) {
                    thrower("GetInterfaceType Failed, Error: " + str(error) + " " + str(Helper::mapReturnCodeToString(error)));
                    return error;
                }

                // camera interface type is GigE. update display name with IP address 
                if (VmbInterfaceEthernet == cameraIFType)
                {
                    // lookup the IP address of the camera         
                    std::string sIPAddress;
                    error = getIPAddress(camera, sIPAddress);

                    // replace the model ID with the IP address
                    if (VmbErrorSuccess == error && !sIPAddress.empty())
                    {
                        QString sTempDisplayName = QString::fromStdString(sDisplayName);
                        QRegExp regExp("\\(([^)]+)\\)");

                        // to account for cameras without model name in parenthesis
                        if (-1 == regExp.indexIn(sTempDisplayName))
                        {
                            sDisplayName.append(sIPAddress);
                        }
                        else
                        {
                            sTempDisplayName.replace(regExp, qstr(sIPAddress));
                            sDisplayName = sTempDisplayName.toStdString();
                        }

                        sDisplayName.append(sDisplayNameEnding);
                    }
                }
                // other camera interface types use legacy naming convention
                else
                {
                    sDisplayName = sLegacyDisplayName;
                }
            }
        }
    }
    return error;
}

VmbErrorType MakoWrapper::getIPAddress(const AVT::VmbAPI::CameraPtr& camera, std::string& sIPAdress)
{
    VmbErrorType error;
    std::string sCameraID, sInterfaceID, sDeviceID;
    InterfacePtr   pInterface;
    FeaturePtr     pSelectorFeature, pSelectedDeviceID, pSelectedIPAddress;
    VmbInt64_t                  nMinRange, nMaxRange, nIP;
    VmbInt32_t                  nIP_32;

    // get the camera ID
    error = camera->GetID(sCameraID);
    if (VmbErrorSuccess == error)
    {
        // get the interface ID
        error = camera->GetInterfaceID(sInterfaceID);
        if (VmbErrorSuccess == error)
        {
            // get a pointer to the interface
            error = m_VimbaSystem.GetInterfaceByID(sInterfaceID.c_str(), pInterface);
            if (VmbErrorSuccess == error)
            {
                // open the interface 
                error = pInterface->Open();
                if (VmbErrorSuccess == error)
                {
                    // get the device selector
                    error = pInterface->GetFeatureByName("DeviceSelector", pSelectorFeature);
                    if (VmbErrorSuccess == error)
                    {
                        // get the range of the available devices 
                        error = pSelectorFeature->GetRange(nMinRange, nMaxRange);

                        // check for negative range in case requested feature contains no items
                        if (VmbErrorSuccess == error && nMaxRange >= 0)
                        {
                            // get the device ID pointer
                            error = pInterface->GetFeatureByName("DeviceID", pSelectedDeviceID);
                            if (VmbErrorSuccess == error)
                            {
                                // get IP addresses of all cameras connected to interface
                                error = pInterface->GetFeatureByName("GevDeviceIPAddress", pSelectedIPAddress);
                                if (VmbErrorSuccess == error)
                                {
                                    // find the IP address of the desired camera 
                                    for (VmbInt64_t intNo = nMinRange; intNo <= nMaxRange; ++intNo)
                                    {
                                        error = pSelectorFeature->SetValue(intNo);
                                        if (VmbErrorSuccess == error)
                                        {
                                            error = pSelectedDeviceID->GetValue(sDeviceID);
                                            if (VmbErrorSuccess == error)
                                            {
                                                if (0 == sDeviceID.compare(sCameraID))
                                                {
                                                    error = pSelectedIPAddress->GetValue(nIP);
                                                    if (VmbErrorSuccess == error)
                                                    {
                                                        nIP_32 = static_cast<VmbInt32_t>(nIP);

                                                        // format IP address string
                                                        sIPAdress = str(QString("(%1)").arg(Helper::IPv4ToString(qFromBigEndian(nIP_32), true)));

                                                        // close the interface
                                                        error = pInterface->Close();
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            sIPAdress.clear();
                            error = VmbErrorNotFound;
                        }

                    }
                }
            }
        }
    }

    return error;
}

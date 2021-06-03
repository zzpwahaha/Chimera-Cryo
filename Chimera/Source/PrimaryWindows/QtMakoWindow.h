#pragma once
#include <QMainWindow>
#include <QTimer>
#include "ConfigurationSystems/ProfileIndicator.h"
#include "ConfigurationSystems/profileSettings.h"
#include "ExperimentThread/ExperimentThreadInput.h"
#include "IChimeraQtWindow.h"
#include <CMOSCamera/MakoCamera.h>


class QtMakoWindow : public IChimeraQtWindow 
{
    Q_OBJECT

public:
    explicit QtMakoWindow(QWidget* parent = nullptr);
    ~QtMakoWindow();

    void initializeWidgets();
    void windowOpenConfig(ConfigStream& configFile);
    void windowSaveConfig(ConfigStream& newFile);
    void fillExpDeviceList(DeviceList& list);
    std::string getSystemStatusString();
public slots: //will this work without qslot? NO zzp 2021/06/01
    void prepareWinForAcq(MakoSettings* settings, CameraInfo caminfo);


public:
    const CameraInfo camInfo1{ CameraInfo::name::Mako1,MAKO_IPADDRS[0],MAKO_DELIMS[0],MAKO_SAFEMODE[0] };
    const CameraInfo camInfo2{ CameraInfo::name::Mako2,MAKO_IPADDRS[1],MAKO_DELIMS[1],MAKO_SAFEMODE[1] };

private:
    std::array<MakoCamera, MAKO_NUMBER> cam;


};

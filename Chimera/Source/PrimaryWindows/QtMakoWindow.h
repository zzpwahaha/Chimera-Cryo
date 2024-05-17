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
    static const unsigned WINDOW_MAKO_NUMBER = CameraInfo::WINDOW_MAKO_NUMBER;

    explicit QtMakoWindow(std::array<CameraInfo, WINDOW_MAKO_NUMBER> camInfos, QWidget* parent = nullptr);
    ~QtMakoWindow();

    void initializeWidgets();
    void windowOpenConfig(ConfigStream& configFile);
    void windowSaveConfig(ConfigStream& newFile);
    void fillExpDeviceList(DeviceList& list);
    void fillMasterThreadInput(ExperimentThreadInput* input) override {};
    std::string getSystemStatusString();
    MakoCamera* getMakoCam(int idx) { return &cam[idx]; }
public slots: //will this work without qslot? NO zzp 2021/06/01
    void prepareWinForAcq(MakoSettings* settings, CameraInfo caminfo);
    void CMOSChkFinished(); // used for counting all cmos camera to finish and close the hdf5 file

public:
    const std::array<CameraInfo, WINDOW_MAKO_NUMBER> camInfos;
    
private:
    std::array<MakoCamera, WINDOW_MAKO_NUMBER> cam;


};

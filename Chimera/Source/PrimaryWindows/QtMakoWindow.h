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
    void windowOpenConfig(ConfigStream& configFile) {};
    void windowSaveConfig(ConfigStream& newFile) {};
    void fillExpDeviceList(DeviceList& list) {};

private:
    std::array<MakoCamera, MAKO_NUMBER> cam;

};


#pragma once
#include "LowLevel/constants.h"
#include "GeneralObjects/IChimeraSystem.h"
#include "ConfigurationSystems/Version.h"
#include "ConfigurationSystems/ConfigStream.h"
#include "PrimaryWindows/IChimeraQtWindow.h"
#include <QLabel.h>
#include <qcheckbox.h>
#include <array>
#include "MakoCameraCore.h"
#include "ImageCalculatingThread.h"
#include <GeneralImaging/PictureViewer.h>

class MakoCamera : public IChimeraSystem 
{
    Q_OBJECT
public:
	// THIS CLASS IS NOT COPYABLE.
	MakoCamera& operator=(const MakoCamera&) = delete;
	MakoCamera(const MakoCamera&) = delete;
	
	MakoCamera(CameraInfo camInfo, IChimeraQtWindow* parent);
    ~MakoCamera();

    void initialize();

	void releaseBuffer();

	void acquisitionStartStopFromCtrler(const QString& sThisFeature);

    void acquisitionStartStopFromAction();

    void setCurrentScreenROI();

    void resetFullROI(bool notStartReStart = false);

    void manualSaveImage();

    void updateStatusBar();

    void prepareForExperiment();

    MakoCameraCore& getMakoCore() { return core; };
    const CameraInfo& getCameraInfo() { return camInfo; }

public slots:
    void handleExpImage(QVector<double> img, int width, int height);



private:
    void initPlotContextMenu();
    void handleStatusButtonClicked(QString featName);


private:
    // this order matters since the ctor will initialize core first and then viewer and finally imgCThread
    MakoCameraCore core;
	PictureViewer viewer;
    ImageCalculatingThread imgCThread;
    CameraInfo camInfo;

    QDialog* makoCtrlDialog;
    QFileDialog* saveFileDialog;

	bool isCamRunning;

    unsigned int currentRepNumber;
    bool isExpRunning;

    QCheckBox*                          m_expActive;
	QLabel*                             m_OperatingStatusLabel;
    QPushButton*                        m_ImageSizeButtonH;
    QPushButton*                        m_ImageSizeButtonW;
    //QPushButton*                        m_FormatButton;
    QPushButton*                        m_TrigOnOffButton;
    QPushButton*                        m_TrigSourceButton;
    //QPushButton*                        m_FramerateButton;
    //QLabel*                             m_FramesLabel;
    QLabel*                             m_CursorScenePosLabel;
    QPushButton*                        m_ExposureTimeButton;
    QPushButton*                        m_CameraGainButton;

	QAction*                            m_aStartStopCap;
    QAction*                            m_aManualCscale;
    //QString                             currentFormat;

};

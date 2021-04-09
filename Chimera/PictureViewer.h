#pragma once
#include <QWidget>
#include <qdialog.h>
#include <QString>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qmenu.h>
#include "../3rd_Party/qcustomplot/qcustomplot.h"
#include <GeneralImaging/RangeSlider.h>

class PictureViewer :  public QWidget
{
    Q_OBJECT
    QDialog*                            m_DiagController;
    QDialog*                            m_DiagInfomation;
    QDialog*                            m_DiagRSlider;
    RangeSlider*                        m_RSliderV;
    QSpinBox*                           m_upperSB;
    QSpinBox*                           m_lowerSB;
    QString                             m_sSliderFormat;


    QLabel*                             m_OperatingStatusLabel;
    QPushButton*                        m_FormatButton;
    QPushButton*                        m_ImageSizeButtonH;
    QPushButton*                        m_ImageSizeButtonW;
    QPushButton*                        m_FramerateButton;
    QLabel*                             m_FramesLabel;
    QLabel*                             m_CursorScenePosLabel;
    QPushButton*                        m_ExposureTimeButton;
    QPushButton*                        m_CameraGainButton;

    QSharedPointer<QCustomPlot>         m_QCP;
    QCPAxisRect*                        m_QCPcenterAxisRect;
    QCPAxisRect*                        m_QCPbottomAxisRect;
    QCPAxisRect*                        m_QCPleftAxisRect;
    QSharedPointer<QCPColorMap>         m_colorMap;
    QSharedPointer<QCPGraph>            m_bottomGraph;
    QSharedPointer<QCPGraph>            m_leftGraph;
    QCPColorScale*                      m_colorScale;
    QVector<QCPColorGradient>           m_colorgradient;
    QMap<int, QString>                  m_cmapMap;
    QComboBox*                          m_cmapCombo;
    QDialog*                            m_dCScale;

    QCPItemTracer*                      m_QCPtracerbottom;
    QCPItemText*                        m_QCPtraceTextbottom;
    QCPItemTracer*                      m_QCPtracerleft;
    QCPItemText*                        m_QCPtraceTextleft;

    QMenu*                              m_ContextMenu;
    QAction*                            m_aStartStopCap;
    QAction*                            m_aDiagCtrler;
    QAction*                            m_aDiagInfo;
    QAction*                            m_aSetCurrScrROI;
    QAction*                            m_aResetFullROI;
    QAction*                            m_aPlotTracer;
    QAction*                            m_aPlotFitter;
    QAction*                            m_aPlotFitter2D;
    QAction*                            m_aCscale;
    QAction*                            m_aManualCscale;
    QAction*                            m_aSaveCamSetting;
    QAction*                            m_aLoadCamSetting;
    QAction*                            m_aSaveImg;
    QAction*                            m_aCamlist;
    QAction*                            m_aDisconnect;

    /* Save Image Option */
    QString                             m_SaveFileDir;
    QString                             m_SelectedExtension;
    QFileDialog*                        m_saveFileDialog; // save an image

    unsigned int                        m_FrameBufferCount;

public:
    PictureViewer(QWidget* parent = 0, Qt::WindowFlags flag = 0, QString sID = " ");
    ~PictureViewer();



private slots:
    
 
    void on_ActionFreerun_triggered();
    void on_ActionSaveCameraSettings_triggered();
    void on_ActionLoadCameraSettings_triggered();
    void on_ActionSaveAs_triggered();

    /* custom */
    void OnShowContextMenu(const QPoint& pt);
    void onAcquisitionStartStop(const QString& sThisFeature); //this is for the command from tree controller
    void onImageCalcStartStop(bool);

    void onSetDescription(const QString& sDesc);
    void onimageReadyFromCalc();
    
    void onSetEventMessage(const QStringList& sMsg);
    void onSetCurrentFPS(const QString& sFPS);
    void onSetFrameCounter(const unsigned int& nNumberOfFrames);
    void onFeedLogger(const QString& sMessage);
    void onResetFPS();

    void onSetMousePosInCMap(QMouseEvent* event);
    void SetCurrentScreenROI();
    void ResetFullROI(bool notStartReStart = false);
    void updateExposureTime();
    void updateCameraGain();

    void loadColorCSV();

};


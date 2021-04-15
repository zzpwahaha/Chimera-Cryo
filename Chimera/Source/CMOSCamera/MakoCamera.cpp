#include "stdafx.h"
#include "MakoCamera.h"
#include <qdialog.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qaction.h>

MakoCamera::MakoCamera(std::string ip, bool SAFEMODE, IChimeraQtWindow* parent)
    : IChimeraSystem(parent)
    , core(ip, SAFEMODE)
    , viewer(core.CameraName(), this)
    , imgCThread(SP_DECL(FrameObserver)(core.getFrameObs()), core, SAFEMODE,
        viewer.plot(), viewer.cmap(), viewer.bottomPlot(), viewer.leftPlot())
    , saveFileDialog(nullptr)
{
    
	//connect()

	// context menu of viewer

}

MakoCamera::~MakoCamera()
{
    //releaseBuffer();
}

void MakoCamera::initialize()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    /*image title*/
    QLabel* namelabel = new QLabel(qstr(core.CameraName()));
    namelabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    layout->addWidget(namelabel, 0);
    /*status bar*/
    QStatusBar* statusbar1 = new QStatusBar(this);
    QStatusBar* statusbar2 = new QStatusBar(this);
    m_OperatingStatusLabel = new QLabel(" Ready ", this);
    m_TrigOnOffButton = new QPushButton("Trig:On/Off", this);
    m_TrigSourceButton = new QPushButton("TrigSrc", this);
    m_ImageSizeButtonH = new QPushButton("sizeH", this);
    m_ImageSizeButtonW = new QPushButton("sizeW", this);
    QWidget* imageSizeBtn = new QWidget();
    QHBoxLayout* imageSizeBtnLayout = new QHBoxLayout(imageSizeBtn);
    imageSizeBtnLayout->setMargin(0);
    imageSizeBtnLayout->addWidget(m_ImageSizeButtonH);
    imageSizeBtnLayout->addWidget(m_ImageSizeButtonW);
    QLabel* framesLabel = new QLabel("frame#", this);
    QPushButton* framerateButton = new QPushButton("FPS", this);
    m_CursorScenePosLabel = new QLabel("pos", this);
    m_ExposureTimeButton = new QPushButton("exposure", this);
    m_CameraGainButton = new QPushButton("gain", this);
    statusbar1->addWidget(m_OperatingStatusLabel);
    statusbar1->addWidget(imageSizeBtn);
    statusbar1->addWidget(m_TrigOnOffButton);
    statusbar1->addWidget(m_TrigSourceButton);
    statusbar1->addWidget(m_CursorScenePosLabel);
    statusbar2->addWidget(m_ExposureTimeButton);
    statusbar2->addWidget(m_CameraGainButton);
    statusbar2->addWidget(framesLabel);
    statusbar2->addWidget(framerateButton);

    m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0,0, 0); color: rgb(255,255,255)");
    for (auto& btn : { m_ImageSizeButtonH ,m_ImageSizeButtonW,m_CameraGainButton,m_ExposureTimeButton,
        framerateButton,m_TrigOnOffButton,m_TrigSourceButton })
        btn->setStyleSheet("border: none; color: rgb(128, 89, 255); font: 10pt");
    
    layout->addWidget(statusbar1, 0);
    layout->addWidget(statusbar2, 0);
    viewer.setMinimumSize(800, 800);
    layout->addWidget(&viewer, 1);
    //layout->addStretch(1);

    QRect rec = QApplication::desktop()->screenGeometry();
    this->setMaximumSize(rec.width() / 2, rec.height());

    //connect(m_FormatButton, &QPushButton::clicked, this, [this]() {
    //    core.getMakoCtrl().updateRegisterFeature();
    //    QList<QStandardItem*> tmp = core.getMakoCtrl().controllerModel()->findItems("PixelFormat", Qt::MatchRecursive | Qt::MatchWrap);
    //    if (!tmp.isEmpty()) { core.getMakoCtrl().onClicked(tmp.at(0)->index().siblingAtColumn(1)); }
    //    });
    connect(m_TrigOnOffButton, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("TriggerMode"); });
    connect(m_TrigSourceButton, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("TriggerSource"); });
    connect(m_ImageSizeButtonH, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("Height"); });
    connect(m_ImageSizeButtonW, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("Width"); });
    connect(m_CameraGainButton, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("Gain"); });
    connect(m_ExposureTimeButton, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("ExposureTimeAbs"); });
    connect(framerateButton, &QPushButton::clicked, this, [this]() {
        handleStatusButtonClicked("AcquisitionFrameRateAbs"); });

    connect(core.getFrameObs(), &FrameObserver::setCurrentFPS, this,
        [this, framerateButton](const QString& sFPS) {
            framerateButton->setText(" FPS: " + sFPS + " "); });
    connect(core.getFrameObs(), &FrameObserver::setFrameCounter, this,
        [this, framesLabel](unsigned int nNumberOfFrames) {
            framesLabel->setText("Frames: " + qstr(nNumberOfFrames) + " "); });
        

    /*image calc thread*/
    connect(&imgCThread, &ImageCalculatingThread::imageReadyForPlot, this, [this]() {
        viewer.renderImgFromCalcThread(m_aManualCscale->isChecked());
        imgCThread.mutex().lock();
        //updateStatusBar();
        QMouseEvent event(QMouseEvent::None, imgCThread.mousePos(), Qt::NoButton, 0, 0);
        viewer.onSetMousePosInCMap(&event, m_CursorScenePosLabel);
        imgCThread.mutex().unlock(); });

    connect(viewer.plot(), &QCustomPlot::mouseMove, this, [this](QMouseEvent* mouseEvn) {
        imgCThread.updateMousePos(mouseEvn); });
    connect(viewer.bottomAxes()->axis(QCPAxis::atTop), qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
        this, [this](QCPRange range) {
            auto [ox, oy] = imgCThread.offsetXY();
            viewer.bottomAxes()->axis(QCPAxis::atBottom)->setRange(range - ox); });
    connect(viewer.leftAxes()->axis(QCPAxis::atRight), qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
        this, [this](QCPRange range) {
            auto [ox, oy] = imgCThread.offsetXY();
            viewer.leftAxes()->axis(QCPAxis::atLeft)->setRange(range - oy); });

    connect(&imgCThread, &ImageCalculatingThread::currentFormat, this, [this](QString format) {
        if (0 == currentFormat.compare(format)) return;
        if (0 == format.compare("Mono12")) {
            viewer.rangeSlider()->setRange(0, 4095);
            viewer.rangeSlider()->upperSpinBox()->setRange(0, 4095);
            viewer.rangeSlider()->lowerSpinBox()->setRange(0, 4095);
            currentFormat = format;
        }
        else if (0 == format.compare("Mono8")) {
            viewer.rangeSlider()->setRange(0, 255);
            viewer.rangeSlider()->upperSpinBox()->setRange(0, 255);
            viewer.rangeSlider()->lowerSpinBox()->setRange(0, 255);
            currentFormat = format;
        }
        else {
            emit error("I am curious how on earth do you get format other than Mono8/12");
        } });


    /*set up mako controller gui*/
    //core.getMakoCtrl().setParent(this);
    makoCtrlDialog = new QDialog(this);
    makoCtrlDialog->setModal(false);
    makoCtrlDialog->setWindowTitle("Controller for " + qstr(core.CameraName()));
    QWidget* widgetTree = new QWidget(makoCtrlDialog);
    core.getMakoCtrl().initializeWidget(widgetTree);
    QVBoxLayout* diagLayout = new QVBoxLayout(makoCtrlDialog);
    makoCtrlDialog->setLayout(diagLayout);
    diagLayout->addWidget(widgetTree);

    //imgCThread.setParent(this);

    // basic viewer function, mouse position and double click
    connect(viewer.plot(), &QCustomPlot::mouseMove, [this](QMouseEvent* event) {
        viewer.onSetMousePosInCMap(event, m_CursorScenePosLabel);
        if (!isCamRunning) { viewer.plot()->replot(); } });

    connect(viewer.plot(), &QCustomPlot::mouseDoubleClick, this, [this]() {
        updateStatusBar();
        imgCThread.setDefaultView();
        viewer.plot()->replot(); });

    initPlotContextMenu();
}

void MakoCamera::handleStatusButtonClicked(QString featName)
{
    try {
        core.getMakoCtrl().updateRegisterFeature();
        updateStatusBar();
        QList<QStandardItem*> tmp = core.getMakoCtrl().controllerModel()->findItems(featName, Qt::MatchRecursive | Qt::MatchWrap);
        if (!tmp.isEmpty()) {
            core.getMakoCtrl().onClicked(tmp.at(0)->index().siblingAtColumn(1));
        }
    }
    catch (ChimeraError& e) {
        emit error(qstr("Error in handle the MakoStatusButton") + e.what());
    }
    
}


void MakoCamera::initPlotContextMenu()
{
    this->setContextMenuPolicy(Qt::CustomContextMenu);

    m_aStartStopCap = new QAction("&Streaming");
    viewer.contextMenu()->addAction(m_aStartStopCap);
    m_aStartStopCap->setCheckable(true);
    m_aStartStopCap->setEnabled(core.isStreamingAvailable());
    connect(m_aStartStopCap, &QAction::triggered, [this]() {acquisitionStartStopFromAction(); });

    QAction* aDiagCtrler = new QAction("Con&troller");
    viewer.contextMenu()->addAction(aDiagCtrler);
    connect(aDiagCtrler, &QAction::triggered, this, [this]() {makoCtrlDialog->show(); });

    viewer.contextMenu()->addSeparator();

    QAction* aSetCurrScrROI = new QAction("SetCurrentRO&I");
    viewer.contextMenu()->addAction(aSetCurrScrROI);
    connect(aSetCurrScrROI, &QAction::triggered, [this]() {setCurrentScreenROI(); });

    QAction* aResetFullROI = new QAction("ResetFullROI");
    viewer.contextMenu()->addAction(aResetFullROI);
    connect(aResetFullROI, &QAction::triggered, [this]() {resetFullROI(); });


    QAction* aPlotTracer = new QAction("Tracer");
    aPlotTracer->setCheckable(true);
    aPlotTracer->setChecked(true);
    viewer.contextMenu()->addAction(aPlotTracer);
    connect(aPlotTracer, &QAction::triggered, this, [this, aPlotTracer]() {
        for (auto& tra : { viewer.bottomTracer(),viewer.leftTracer() })
        {
            aPlotTracer->isChecked() ? tra->setVisible(true) : tra->setVisible(false);
        }
        for (auto& tra : { viewer.bottomTracerText(),viewer.leftTracerText() })
        {
            aPlotTracer->isChecked() ? tra->setVisible(true) : tra->setVisible(false);
        }
        viewer.plot()->replot(); });

    QAction* aPlotFitter = new QAction("Fitting");
    aPlotFitter->setCheckable(true);
    aPlotFitter->setChecked(true);
    viewer.contextMenu()->addAction(aPlotFitter);
    connect(aPlotFitter, &QAction::triggered, this, [this, aPlotFitter]() {
        for (auto& fitgraph : { viewer.plot()->graph(2),viewer.plot()->graph(3) })
        {
            aPlotFitter->isChecked() ? fitgraph->setVisible(true) : fitgraph->setVisible(false);
        }
        for (auto& ax : { viewer.leftAxes()->axis(QCPAxis::atBottom),viewer.bottomAxes()->axis(QCPAxis::atLeft) })
        {
            aPlotFitter->isChecked() ? 0 : ax->setLabel(" ");
        }
        aPlotFitter->isChecked() ? imgCThread.toggleDoFitting(true) : imgCThread.toggleDoFitting(false);
        viewer.plot()->replot(); });

    QAction* aPlotFitter2D = new QAction("Fitting2D");
    aPlotFitter2D->setCheckable(true);
    aPlotFitter2D->setChecked(false);
    viewer.contextMenu()->addAction(aPlotFitter2D);
    connect(aPlotFitter2D, &QAction::triggered, this, [this, aPlotFitter2D]() {
        auto hairCurve = reinterpret_cast<QCPCurve*>(viewer.plot()->axisRect(1)->plottables().at(1));
        auto parametric = reinterpret_cast<QCPCurve*>(viewer.plot()->axisRect(1)->plottables().at(2));
        for (auto& fitgraph : { hairCurve,parametric })
        {
            aPlotFitter2D->isChecked() ? fitgraph->setVisible(true) : fitgraph->setVisible(false);
        }
        for (auto& ax : { viewer.centerAxes()->axis(QCPAxis::atTop) })
        {
            aPlotFitter2D->isChecked() ? 0 : ax->setLabel(" ");
        }
        aPlotFitter2D->isChecked() ? imgCThread.toggleDoFitting2D(true) : imgCThread.toggleDoFitting2D(false);
        viewer.plot()->replot(); });


    QAction* aCscale = new QAction("Color Scale");
    viewer.contextMenu()->addAction(aCscale);
    connect(aCscale, &QAction::triggered, this, [this]() {
        viewer.manualColorScaleDlg()->move(QCursor::pos());
        viewer.manualColorScaleDlg()->show(); });


    m_aManualCscale = new QAction("Manual Color Scale");
    m_aManualCscale->setCheckable(true);
    m_aManualCscale->setChecked(false);
    viewer.initManualColorRangeAction(m_aManualCscale);

    viewer.contextMenu()->addSeparator();
    QAction* aSaveImg = new QAction("Save Image");
    viewer.contextMenu()->addAction(aSaveImg);
    connect(aSaveImg, &QAction::triggered, [this]() {manualSaveImage(); });
}

void MakoCamera::releaseBuffer()
{
    core.releaseBuffer();
    imgCThread.StopProcessing();
}

void MakoCamera::acquisitionStartStopFromCtrler(const QString& sThisFeature)
{
    /* this is intended to stop and start the camera again since PixelFormat, Height and Width have been changed while camera running
     * ignore this when the changing has been made while camera not running */
    if ((0 == sThisFeature.compare("AcquisitionStart")) && !isCamRunning)
    {
        m_aStartStopCap->setChecked(true);
        acquisitionStartStopFromAction(); // only start if originally is not running, ignore if already running
    }
    else if (sThisFeature.contains("AcquisitionStop") && isCamRunning)
    {
        m_aStartStopCap->setChecked(false);
        acquisitionStartStopFromAction(); // only stop if originally is not running, ignore if already running
        if (!m_aStartStopCap->isEnabled())
            m_aStartStopCap->setEnabled(core.isStreamingAvailable());
    }
}

void MakoCamera::acquisitionStartStopFromAction()
{
    core.checkDisplayInterval();
    updateStatusBar();
    /* ON */
    if (m_aStartStopCap->isChecked())
    {
        try {
            core.prepareCapture();
            core.startCapture();
        }
        catch (ChimeraError& e) {
            isCamRunning = false;
            imgCThread.StopProcessing();
            m_OperatingStatusLabel->setText("Error");
            m_OperatingStatusLabel->setStyleSheet("background-color: rgb(196,0, 0); color: rgb(255,255,255)");
            m_aStartStopCap->setChecked(false);
            emit IChimeraSystem::error(qstr("Failed to start! \n") + e.what());
            return;
        }
        // Do some GUI-related preparations before really starting (to avoid timing problems)
        m_OperatingStatusLabel->setText(" Running... ");
        m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0,128, 0); color: rgb(255,255,255)");

        isCamRunning = true;
        imgCThread.StartProcessing();
    }
    /* OFF */
    else
    {
        try {
            core.stopCapture();
        }
        catch (ChimeraError& e) {
            m_OperatingStatusLabel->setText("Error");
            m_OperatingStatusLabel->setStyleSheet("background-color: rgb(196,0, 0); color: rgb(255,255,255)");
            emit IChimeraSystem::error(qstr("Failed to stop! \n") + e.what());
            return;
        }
        m_OperatingStatusLabel->setText(" Ready ");
        m_OperatingStatusLabel->setStyleSheet("background-color: rgb(0, 0, 0); color: rgb(255,255,255)");
        
        isCamRunning = false;
        releaseBuffer();
    }
}


void MakoCamera::setCurrentScreenROI()
{
    auto [maxh, maxw] = core.getMakoCtrl().getMaxImageSize();
    QCPRange xr = viewer.centerAxes()->axis(QCPAxis::atBottom)->range();
    QCPRange yr = viewer.centerAxes()->axis(QCPAxis::atLeft)->range();
    if (xr.lower > 0 && xr.upper < maxw && yr.lower>0 && yr.upper < maxh)
    {
        int xlower = 2 * std::floor(xr.lower / 2);
        int xupper = 2 * std::ceil(xr.upper / 2);
        int ylower = 2 * std::floor(yr.lower / 2);
        int yupper = 2 * std::ceil(yr.upper / 2);
        xupper += (xupper - xlower) % 4 == 0 ? 0 : 2;
        yupper += (yupper - ylower) % 4 == 0 ? 0 : 2;
        int xw = (xupper - xlower) > 2 ? xupper - xlower : 4;
        int yw = (yupper - ylower) > 2 ? yupper - ylower : 4;
        
        resetFullROI(true);
        try {
            acquisitionStartStopFromCtrler("AcquisitionStop");
            Sleep(5);//give some time for it to shut down
            /*first reset the value to full*/
            core.setROI(xw, yw, xlower, ylower);
            acquisitionStartStopFromCtrler("AcquisitionStart");
        }
        catch (ChimeraError& e) {
            emit IChimeraSystem::error("Error in setting CMOS ROI \n" + qstr(e.what()));
        }
        
        Sleep(50);
    }
    else {
        emit IChimeraSystem::warning("Warning in setting the ROI in CMOS camera, the values are out of bound");
    }

}

void MakoCamera::resetFullROI(bool notStartReStart)
{
    if (!notStartReStart)
    {
        acquisitionStartStopFromCtrler("AcquisitionStop");
    }
    core.resetFullROI();
    if (!notStartReStart)
    {
        acquisitionStartStopFromCtrler("AcquisitionStart");
    }
}

void MakoCamera::manualSaveImage()
{
    // make a copy of the images before save-as dialog appears (image can change during time dialog open)
    QVector<double> imgSave;
    imgSave = std::move(imgCThread.rawImageDefinite());
    //imgSave = QVector<double>(m_pImgCThread->rawImage().begin(), m_pImgCThread->rawImage().end());
    auto [imgWidth, imgHeight] = imgCThread.WidthHeight();


    QString     fileExtension;
    bool        isImageAvailable = true;

    if (imgSave.isEmpty())
    {
        isImageAvailable = false;
    }
    else
    {
        if (nullptr != saveFileDialog)
        {
            delete saveFileDialog;
            saveFileDialog = nullptr;
        }

        fileExtension = "*.pdf ;; *.csv";
        saveFileDialog = new QFileDialog(this, "Save Image", qstr(DATA_SAVE_LOCATION), fileExtension);
        saveFileDialog->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint);
        saveFileDialog->selectNameFilter("*.csv");
        saveFileDialog->setViewMode(QFileDialog::Detail);
        saveFileDialog->setAcceptMode(QFileDialog::AcceptSave);

        if (saveFileDialog->exec())
        {   //OK
            QString selectedExtension = saveFileDialog->selectedNameFilter();
            QString saveFileDir = saveFileDialog->directory().absolutePath();
            QStringList files = saveFileDialog->selectedFiles();

            if (!files.isEmpty())
            {
                QString fileName = files.at(0);

                bool saved = false;

                if (!fileName.endsWith(selectedExtension.section('*', -1)))
                {
                    fileName.append(selectedExtension);
                }
                if (0 == selectedExtension.compare("*.pdf"))
                {
                    saved = viewer.plot()->savePdf(fileName, 0, 0, QCP::epNoCosmetic);
                }
                else if (0 == selectedExtension.compare("*.csv"))
                {
                    QFile file(fileName);
                    if (file.open(QIODevice::ReadWrite))
                    {
                        QTextStream stream(&file);
                        for (size_t i = 0; i < imgHeight; i++)
                        {
                            for (size_t j = 0; j < imgWidth; j++)
                            {
                                stream << imgSave[i * imgWidth + j];
                                j == imgWidth - 1 ? stream << "," : stream << endl;
                            }
                        }
                    }
                    saved = true;
                }

                if (true == saved)
                {
                    QMessageBox::information(this, tr("Vimba Viewer"), tr("Image: ") + fileName + tr(" saved successfully"));
                }
                else
                {
                    QMessageBox::warning(this, tr("Vimba Viewer"), tr("Error saving image"));
                }

            }
        }
    }

    if (!isImageAvailable)
    {
        QMessageBox::warning(this, tr("Vimba Viewer"), tr("No image to save"));
    }
}

void MakoCamera::updateStatusBar()
{
    core.updateCurrentSettings();
    MakoSettings ms = core.getRunningSettings();

    //m_FormatButton->setText("Pixel Format: " + qstr(imgCThread.format()) + " ");
    //auto [w, h] = imgCThread.WidthHeight();
    //QMouseEvent event(QMouseEvent::None, imgCThread.mousePos(), Qt::NoButton, 0, 0);
    //viewer.onSetMousePosInCMap(&event, m_CursorScenePosLabel);
    //imgCThread.updateExposureTime();
    //imgCThread.updateCameraGain();
    m_ImageSizeButtonH->setText("Size H: " + qstr(ms.dims.height()));
    m_ImageSizeButtonW->setText(",W: " + qstr(ms.dims.width()) + " ");
    m_ExposureTimeButton->setText("Exposure time (ms): " + qstr(ms.exposureTime / 1.0e3, 3));
    m_CameraGainButton->setText("Gain (dB): " + qstr(ms.rawGain, 0));
    m_TrigOnOffButton->setText(ms.trigOn ? "Trig: On" : "Trig: Off");
    m_TrigSourceButton->setText("TrigSource: " + qstr(MakoTrigger::toStr(ms.triggerMode)));
}

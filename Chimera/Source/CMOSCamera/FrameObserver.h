#pragma once

#include <QObject>
#include <QImage>
#include <QVector>
#include <QTime>
#include <QThread>
#include <QStringList>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>

#include "Helper.h"
#include "ImageProcessingThread.h"
#include <VimbaCPP/Include/IFrameObserver.h>
#include <VimbaCPP/Include/Frame.h>
#include <VimbaCPP/Include/Camera.h>

/* Number of frames in use to count the first FPS since start*/
const unsigned int MAX_FRAMES_TO_COUNT = 50;

using AVT::VmbAPI::CameraPtr;
using AVT::VmbAPI::FramePtr;

class FrameObserver : public QObject, public AVT::VmbAPI::IFrameObserver
{
    Q_OBJECT

    public:
            void Stopping();
            void Starting();
            FrameObserver ( CameraPtr pCam );
           ~FrameObserver ();
            virtual void FrameReceived          ( const FramePtr frame );
            void resetFrameCounter              ( bool bIsRestart );
            const QSharedPointer<ImageProcessingThread>& ImageProcessThreadPtr() const { return m_pImageProcessingThread; }
       
    private:
            bool setFrame                       ( const FramePtr &frame );
            
    signals:
            void setCurrentFPS                       ( const QString &sFPS );
            void setFrameCounter                     ( const unsigned int &nFrame );

    private:
        typedef QSharedPointer<unsigned char>   FrameDataPtr;

        FPSCounter                              m_FPSCamera;
        FPSCounter                              m_FPSReceived;
        CameraPtr                               m_pCam;                     // camera to wait for frames on

        unsigned int                            m_nFrames;
        unsigned int                            m_nFramesCounter;       // nr frames received

        QSharedPointer<ImageProcessingThread>   m_pImageProcessingThread;   // Image processing thread
        bool                                m_IsStopping;
        QMutex                              m_StoppingLock;
            
};

#pragma once

#include <qobject.h>
#include <string>
#include <Andor/cameraThreadInput.h>

class AndorCameraThreadWorker : public QObject {
    Q_OBJECT

    public:
        AndorCameraThreadWorker(cameraThreadWorkerInput* input_);
        ~AndorCameraThreadWorker ();
    private:
        cameraThreadWorkerInput * input;

    public Q_SLOTS:
        void process ();
		
	Q_SIGNALS:
		void error (QString, unsigned);
        void notify (QString, unsigned);
        void pictureTaken (int);
        void acquisitionFinished ();
};
 

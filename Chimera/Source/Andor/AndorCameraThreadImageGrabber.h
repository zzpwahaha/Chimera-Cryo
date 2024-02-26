#pragma once
#include <qobject.h>
#include <Andor/cameraThreadInput.h>

class AndorCameraThreadImageGrabber : public QObject
{
	Q_OBJECT
public:
	AndorCameraThreadImageGrabber(cameraThreadImageGrabberInput* input_);
	~AndorCameraThreadImageGrabber();

private:
	cameraThreadImageGrabberInput* input;

public slots:
	void process();

signals:
	void pictureGrabbed(NormalImage);
	void pauseExperiment();
	void error(QString, unsigned);
	void notify(QString, unsigned);

};
#include "stdafx.h"
#include "QtMakoWindow.h"
#include <qdesktopwidget.h>
#include <PrimaryWindows/QtScriptWindow.h>
#include <PrimaryWindows/QtAndorWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>
#include <PrimaryWindows/QtMainWindow.h>
#include "ExperimentMonitoringAndStatus/colorbox.h"
#include <ExcessDialogs/saveWithExplorer.h>
#include <ExcessDialogs/openWithExplorer.h>
#include <qlayout.h>

QtMakoWindow::QtMakoWindow(QWidget* parent) 
	: IChimeraQtWindow(parent)
	, cam(MAKO_IPADDRS[0],this)
{
	setWindowTitle("Mako Camera Window");
}

QtMakoWindow::~QtMakoWindow()
{
}

void QtMakoWindow::initializeWidgets()
{
	statBox = new ColorBox(this, mainWin->getDevices());
	QWidget* centralWidget = new QWidget(this);
	setCentralWidget(centralWidget);

	QHBoxLayout* layout = new QHBoxLayout(centralWidget);
	cam.initialize();
	layout->addWidget(&cam, 0);
}

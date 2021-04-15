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
	, cam{ MakoCamera(MAKO_IPADDRS[0], MAKO_SAFEMODE[0], this),
	MakoCamera(MAKO_IPADDRS[1], MAKO_SAFEMODE[1], this) }
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
	cam[0].initialize();
	cam[1].initialize();
	layout->addWidget(&cam[0], 1);
	layout->addWidget(&cam[1], 1);
}

#include "stdafx.h"
#include "InExpCalControl.h"
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <qcheckbox.h>
#include <qboxlayout.h>
#include <qlineedit.h>
#include <qtimer.h>
#include <boost/lexical_cast.hpp>

InExpCalControl::InExpCalControl(IChimeraQtWindow* parent)
	: timer(new QTimer(parent))
{
	parent->connect(timer, &QTimer::timeout, parent, [this]() { interrupt = true; });
}

void InExpCalControl::initialize(IChimeraQtWindow* parent)
{
	auto configUpdate = [parent]() {parent->configUpdated(); };
	auto changeBkgColor = [](QCheckBox* chkBox) { chkBox->isChecked() ?
		chkBox->setStyleSheet("QCheckBox { background-color: chartreuse }")
		: chkBox->setStyleSheet("QCheckBox { background-color: (240,240,240) }"); };

	layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	inExpCalibrationButton = new QCheckBox("In-Exp Calibration?", parent);
	parent->connect(inExpCalibrationButton, &QCheckBox::stateChanged, parent, configUpdate);
	parent->connect(inExpCalibrationButton, &QCheckBox::stateChanged, parent, [this, changeBkgColor]() {
		changeBkgColor(inExpCalibrationButton);
		if (inExpCalibrationButton->isChecked()) {
			double interval;
			try {
				interval = boost::lexical_cast<double>(str(inExpCalibrationDurationEdit->text()));
			}
			catch (boost::bad_lexical_cast&) { // just go with default value if the input is invalid.
			}
			interrupt = false;
			timer->start(int(interval * 60 * 1000));
		}
		else {
			timer->stop();
			interrupt = false;
		}
		});

	inExpCalibrationDurationEdit = new QLineEdit("10.0", parent);
	parent->connect(inExpCalibrationDurationEdit, &QLineEdit::returnPressed, parent, [this]() {
		double interval;
		try {
			interval = boost::lexical_cast<double>(str(inExpCalibrationDurationEdit->text()));
			if (inExpCalibrationButton->isChecked()) {
				timer->start(int(interval * 60 * 1000));
			}
		}
		catch (boost::bad_lexical_cast&) {
			errBox("Bad value for calibration interval value");
		}
		});
	layout->addWidget(inExpCalibrationButton, 0);
	layout->addWidget(new QLabel("Every "), 0);
	layout->addWidget(inExpCalibrationDurationEdit, 0);
	layout->addWidget(new QLabel("min."), 0);
	layout->addStretch(1);
}

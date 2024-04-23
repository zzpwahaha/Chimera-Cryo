#include "stdafx.h"
#include "PicoScrewSystem.h"
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <PrimaryWindows/QtMainWindow.h>
#include <qpushbutton.h>
#include <qlayout.h>

PicoScrewSystem::PicoScrewSystem(IChimeraQtWindow* parent) :
	IChimeraSystem(parent),
	expActive(false),
	core(PICOSCREW_SAFEMODE, PICOSCREW_KEY)
{
}

void PicoScrewSystem::initialize()
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	this->setMaximumWidth(1300);

	QLabel* title = new QLabel("PICOSCREW", this);
	layout->addWidget(title, 0);

	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);

	auto programNowButton = new QPushButton("Program PS Now", this);
	connect(programNowButton, &QPushButton::released, [this]() {
		try {
			handleProgramNowPress();
			updateCurrentValues();
		}
		catch (ChimeraError& err) {
			parentWin->reportErr(err.qtrace());
		}
		});
	ctrlButton = new QCheckBox("Ctrl?", this);
	ctrlButton->setChecked(true);
	if (!expActive) {
		// never enabled. You should be able to always modify the piezo values in the middle of the experiment safely. 
		ctrlButton->setEnabled(false);
		ctrlButton->setText("(Not Used in Exp)");
	}
	connect(ctrlButton, &QCheckBox::stateChanged, [this]() {
		try {
			updateCtrlEnable();
			parentWin->configUpdated();
		}
		catch (ChimeraError& err) {
			parentWin->reportErr(err.qtrace());
		}
		});

	layout1->addWidget(programNowButton, 0);
	layout1->addWidget(ctrlButton, 0);
	layout1->addStretch(1);
	layout->addLayout(layout1, 0);


	QGridLayout* layout2 = new QGridLayout();
	layout2->setContentsMargins(0, 0, 0, 0);

	for (auto ch : range(PICOSCREW_NUM)) {
		//auto strChan = qstr(ch + 1);
		//labels[ch] = new QLabel(strChan, this);
		setHomeButtons[ch] = new QPushButton("Set Home", this);
		edits[ch] = new QLineEdit(this);
		currentVals[ch] = new QLabel("---", this);

		connect(edits[ch], &QLineEdit::textChanged, [this]() { parentWin->configUpdated(); });

	}
	updateCurrentValues();

	for (auto ch : range(PICOSCREW_NUM)) {
		QHBoxLayout* lay = new QHBoxLayout();
		lay->setContentsMargins(0, 0, 0, 0);
		auto strChan = qstr(ch + 1);
		auto label = new QLabel(strChan, this);
		//lay->addWidget(labels[ch], 0);
		lay->addWidget(label, 0);
		lay->addWidget(setHomeButtons[ch], 0);
		layout2->addLayout(lay, 0, ch);
		layout2->addWidget(currentVals[ch], 1, ch);
		layout2->addWidget(edits[ch], 2, ch);
	}
	layout->addLayout(layout2);

	QTimer* timer = new QTimer(this);
	connect(timer, &QTimer::timeout, [this]() {
		try {
			if (!parentWin->mainWin->expIsRunning()) {
				updateCurrentValues();
			}
		}
		catch (ChimeraError&) {}
		});
	// could probably make this time a front panel option.
	timer->start(10000);
}

void PicoScrewSystem::updateCurrentValues()
{
	try{
		for (auto ch : range(PICOSCREW_NUM)) {
			currentVals[ch]->setText(qstr(core.motorPosition(ch)));
		}
	}
	catch (ChimeraError& e) {
		emit error(e.qtrace());
	}

}

void PicoScrewSystem::updateCtrlEnable()
{
	auto ctrl = ctrlButton->isChecked();
	for (auto& e : edits) {
		e->setEnabled(ctrl);
	}
}

void PicoScrewSystem::handleProgramNowPress()
{

}

std::string PicoScrewSystem::getConfigDelim()
{
	return core.configDelim;
}

std::string PicoScrewSystem::getDeviceInfo()
{
	return core.getDeviceInfo();
}

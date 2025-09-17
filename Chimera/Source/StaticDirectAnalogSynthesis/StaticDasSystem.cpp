#include "stdafx.h"
#include "StaticDasSystem.h"
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <PrimaryWindows/QtMainWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>
#include <qpushbutton.h>
#include <qlayout.h>

StaticDasSystem::StaticDasSystem(IChimeraQtWindow* parent) :
	IChimeraSystem(parent),
	expActive(false),
	core(STATICDAS_SAFEMODE, STATICDAS_PORT, STATICDAS_BAUDRATE)
{
}

void StaticDasSystem::initialize()
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	this->setMaximumWidth(600);

	QLabel* title = new QLabel("STATIC DAS", this);
	layout->addWidget(title, 0);

	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);

	auto programNowButton = new QPushButton("Program DAS Now", this);
	connect(programNowButton, &QPushButton::released, [this]() {
		try {
			handleProgramNowPress(parentWin->auxWin->getUsableConstants());
		}
		catch (ChimeraError& err) {
			parentWin->reportErr("Failed to program Static DAS system! \n" + err.qtrace());
		}
		});

	auto reconnectPush = new QPushButton("Reconnect", this);
	connect(reconnectPush, &QPushButton::released, [this]() {
		emit notification("----------------------\r\nReconnect Static DAS system... \n");
		try {
			core.resetConnection();
			emit notification("Finished Reconnecting Static analog system VALON.\r\n");
		}
		catch (ChimeraError& exception) {
			errBox(exception.trace());
			emit notification(": " + exception.qtrace() + "\r\n");
			emit error(": " + exception.qtrace() + "\r\n");
		}
		});

	ctrlButton = new QCheckBox("Ctrl?", this);
	ctrlButton->setChecked(false);
	connect(ctrlButton, &QCheckBox::clicked, [this]() {
		try {
			updateCtrlEnable();
			parentWin->configUpdated();
		}
		catch (ChimeraError& err) {
			parentWin->reportErr(err.qtrace());
		}
		});

	layout1->addWidget(programNowButton, 0);
	layout1->addWidget(reconnectPush, 0);
	layout1->addWidget(ctrlButton, 0);
	layout->addLayout(layout1, 0);


	QHBoxLayout* layoutWR = new QHBoxLayout(this);
	layoutWR->setContentsMargins(0, 0, 0, 0);
	auto deviceSelect = new QComboBox(this);
	for (auto id : range(STATICDAS_NUM)) {
		deviceSelect->addItem("Dev-" + qstr(id));
	}
	auto writeNow = new QPushButton("Write Now", this);
	auto writeTxt = new QLineEdit(this);
	connect(writeNow, &QPushButton::pressed, this, [this, writeTxt, deviceSelect]() {
		try {
			auto txt = writeTxt->text();
			int selection = deviceSelect->currentIndex();
			core.directWrite(str(txt), selection);
		}
		catch (ChimeraError& e) {
			emit error("Error seen in trying to write to Microwave System:\n" + e.qtrace());
		}
		});
	writeTxt->setMaximumWidth(100);

	auto readNow = new QPushButton("Read Now", this);
	auto readTxt = new QLabel("", this);
	connect(readNow, &QPushButton::pressed, this, [this, readTxt, deviceSelect]() {
		try {
			int selection = deviceSelect->currentIndex();
			auto res = core.directRead(selection);
			readTxt->setText(qstr(res));
			readTxt->setToolTip(qstr(res));
		}
		catch (ChimeraError& e) {
			emit error("Error seen in trying to write to Microwave System:\n" + e.qtrace());
		}
		});
	readTxt->setMaximumWidth(150);
	readTxt->setMinimumWidth(100);
	readTxt->setFrameShape(QFrame::Box);

	layoutWR->addWidget(deviceSelect);
	layoutWR->addWidget(writeNow);
	layoutWR->addWidget(writeTxt);
	layoutWR->addWidget(readNow);
	layoutWR->addWidget(readTxt);
	layoutWR->addStretch(1);
	layout->addLayout(layoutWR, 0);

	QGridLayout* layout2 = new QGridLayout();
	layout2->setContentsMargins(0, 0, 0, 0);

	for (auto ch : range(size_t(StaticDASGrid::total))) {
		auto strChan = qstr(ch);
		labels[ch] = new QLabel(strChan + ":", this);
		edits[ch] = new QLineEdit(this);
		edits[ch]->setText("0.0");
		connect(edits[ch], &QLineEdit::textChanged, [this]() { parentWin->configUpdated(); });
	}

	for (auto ch : range(size_t(StaticDASGrid::total))) {
		QHBoxLayout* lay = new QHBoxLayout();
		lay->setContentsMargins(0, 0, 0, 0);
		auto strChan = qstr(ch);
		lay->addWidget(labels[ch], 0);
		lay->addWidget(edits[ch], 0);
		lay->addStretch(1);
		layout2->addLayout(lay, ch / 4, ch % 4);
	}
	layout->addLayout(layout2);
}

void StaticDasSystem::handleOpenConfig(ConfigStream& configFile)
{
	auto configVals = core.getSettingsFromConfig(configFile);
	for (auto ch : range(size_t(StaticDASGrid::total))) {
		edits[ch]->setText(qstr(configVals.staticDASs[ch].expressionStr));
	}
	ctrlButton->setChecked(configVals.ctrlDAS);
	updateCtrlEnable();
}

void StaticDasSystem::handleSaveConfig(ConfigStream& configFile)
{
	configFile << core.configDelim;
	for (auto ch : range(size_t(StaticDASGrid::total))) {
		auto strChan = str(ch);
		configFile << "\n/* DAS-" + strChan + " Value:*/\t\t" << Expression(str(edits[ch]->text()));
	}
	configFile << "\n/*Control?*/\t\t\t" << ctrlButton->isChecked()
		<< "\nEND_" + core.configDelim << "\n";
}

void StaticDasSystem::updateCtrlEnable()
{
	auto ctrl = ctrlButton->isChecked();
	for (auto& e : edits) {
		e->setEnabled(!ctrl);
	}
}

void StaticDasSystem::handleProgramNowPress(std::vector<parameterType> constants)
{
	StaticDASSettings tmpSetting;
	for (auto ch : range(size_t(StaticDASGrid::total))) {
		tmpSetting.staticDASs[ch].expressionStr = str(edits[ch]->text());
	}
	tmpSetting.ctrlDAS = true;

	core.setStaticDASExpSetting(tmpSetting);
	core.calculateVariations(constants, nullptr);
	core.programVariation(0, constants, nullptr);

	emit notification("Finished programming Static DAS system!\n", 0);
}

std::string StaticDasSystem::getDeviceInfo()
{
	return core.getDeviceInfo();
}

void StaticDasSystem::setDasEditValue(std::string dasfreq, unsigned channel)
{
	if (channel >= size_t(StaticDASGrid::total)) {
		thrower("Channel " + str(channel) + " outside range of static DAS " + str(size_t(StaticDASGrid::total)));
	}
	edits[channel]->setText(qstr(dasfreq));
}
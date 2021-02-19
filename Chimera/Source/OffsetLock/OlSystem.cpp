#include "stdafx.h"
#include "OlSystem.h"
#include "PrimaryWindows/IChimeraQtWindow.h"
#include "PrimaryWindows/QtAuxiliaryWindow.h"

OlSystem::OlSystem(IChimeraQtWindow* parent)
	: IChimeraSystem(parent)
	, core(OFFSETLOCK_SAFEMODE)
	, roundToOlPrecision(false)
{
}

void OlSystem::initialize(IChimeraQtWindow* parent)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	this->setMaximumWidth(1000);
	// title

	olTitle = new QLabel("OffsetLock", parent);
	layout->addWidget(olTitle, 0);

	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);

	olSetButton = new CQPushButton("Set New ol Values", parent);
	olSetButton->setToolTip("Press this button to attempt force all ol values to the values currently recorded in the"
		" edits below.");
	parent->connect(olSetButton, &QPushButton::released, [parent]() {parent->auxWin->SetOls(); });
	zeroOlsButton = new CQPushButton("Zero OffsetLocks", parent);
	zeroOlsButton->setToolTip("Press this button to set all offset values to default minimum.");
	parent->connect(zeroOlsButton, &QPushButton::released, [parent]() { parent->auxWin->zeroOls(); });
	// 
	quickChange = new CQCheckBox("Quick-Change", parent);
	quickChange->setChecked(false);
	quickChange->setToolTip("With this checked, you can quickly change a Offsetlock's value by using the arrow keys while "
		"having the cursor before the desired digit selected in the Offsetlock's edit.");

	layout1->addWidget(olSetButton, 0);
	layout1->addWidget(zeroOlsButton, 0);
	layout1->addWidget(quickChange, 0);
	layout->addLayout(layout1);


	QGridLayout* OLGridLayout = new QGridLayout();
	unsigned runningCount = 0;
	std::array<std::string, size_t(OLGrid::total)> olNames;
	for (auto& out : outputs)
	{
		out.initialize(parent, runningCount);
		OLGridLayout->addLayout(out.getLayout(),
			runningCount / size_t(OLGrid::numPERunit),
			runningCount % size_t(OLGrid::numPERunit));
		olNames[runningCount] = out.info.name;
		runningCount++;
		out.info.currFreq = out.info.defaultFreq;
	}
	updateEdits();
	layout->addLayout(OLGridLayout);
	core.setNames(olNames);

}

bool OlSystem::eventFilter(QObject* obj, QEvent* event) {
	for (auto& out : outputs) {
		if (out.eventFilter(obj, event) && quickChange->isChecked())
		{
			parentWin->auxWin->SetOls();
			return true;
		}
	}
	return false;
}

void OlSystem::handleOpenConfig(ConfigStream& openFile)
{
	/*ProfileSystem::checkDelimiterLine(openFile, "DACS");
	prepareForce();
	std::vector<double> values(getNumberOfDacs());
	unsigned olInc = 0;
	for (auto& ol : values)
	{
		std::string olString;
		openFile >> olString;
		try
		{
			double olValue = std::stod(olString);
			prepareDacForceChange(olInc, olValue);
		}
		catch (std::invalid_argument&)
		{
			thrower("ERROR: failed to convert ol value to voltage. string was " + olString);
		}
		olInc++;
	}
	ProfileSystem::checkDelimiterLine(openFile, "END_DACS");*/
	emit notification("OL system finished opening config, which is not yet added.\n"
		+ QString(__FILE__) + "line: " + QString::number(__LINE__), 2);

}

void OlSystem::handleSaveConfig(ConfigStream& saveFile)
{
	saveFile << "OFFSETLOCKS\nEND_OFFSETLOCKS\n";
}





void OlSystem::handleRoundToOl()
{
	if (roundToOlPrecision)
	{
		roundToOlPrecision = false;
	}
	else
	{
		roundToOlPrecision = true;
	}
}

void OlSystem::updateEdits()
{
	for (auto& ol : outputs)
	{
		ol.updateEdit(roundToOlPrecision);
	}
}

void OlSystem::setDefaultValue(unsigned olNum, double val) {
	outputs[olNum].info.defaultFreq = val;
}

double OlSystem::getDefaultValue(unsigned olNum) {
	return outputs[olNum].info.defaultFreq;
}

void OlSystem::setMinMax(int olNumber, double minv, double maxv) {
	if (!(minv <= maxv)) {
		thrower("Min ol value must be less than max ol value.");
	}
	if (minv < 1000 || minv > 6000 || maxv < 1000 || maxv > 6000) {
		thrower("Min and max ol values must be withing [1000,6000].");
	}
	outputs[olNumber].info.minFreq = minv;
	outputs[olNumber].info.maxFreq = maxv;
}

std::pair<double, double> OlSystem::getOlRange(int olNumber) {
	return { outputs[olNumber].info.minFreq , outputs[olNumber].info.maxFreq };
}

void OlSystem::setName(int olNumber, std::string name) {
	outputs[olNumber].setName(name);
}

void OlSystem::setNote(int olNum, std::string note) {
	outputs[olNum].setNote(note);
}

std::string OlSystem::getNote(int olNumber) {
	return outputs[olNumber].info.note;
}

std::string OlSystem::getName(int olNumber) {
	return outputs[olNumber].info.name;
}


unsigned OlSystem::getNumberOfOls() {
	return outputs.size();
}


double OlSystem::getOlValue(int olNumber) {
	return outputs[olNumber].info.currFreq;
}

std::array<double, size_t(OLGrid::total)> OlSystem::getOlValues()
{
	std::array<double, size_t(OLGrid::total)> olValueTmp;
	for (size_t i = 0; i < size_t(OLGrid::total); i++)
	{
		olValueTmp[i] = getOlValue(i);
	}
	return olValueTmp;
}

std::array<OlInfo, size_t(OLGrid::total)> OlSystem::getOlInfo() {
	std::array<OlInfo, size_t(OLGrid::total)> info;
	for (auto olNum : range(outputs.size())) {
		info[olNum] = outputs[olNum].info;
	}
	return info;
}

void OlSystem::updateCoreNames()
{
	std::array<std::string, size_t(OLGrid::total)> names_;
	for (size_t i = 0; i < size_t(OLGrid::total); i++)
	{
		names_[i] = outputs[i].info.name;
	}
	core.setNames(names_);
}



void OlSystem::handleEditChange(unsigned olNumber) {
	if (olNumber >= outputs.size()) {
		thrower("attempted to handle offsetlock edit change, but the offsetlock number reported doesn't exist!");
	}
	outputs[olNumber].handleEdit(roundToOlPrecision);
}

void OlSystem::handleSetOlsButtonPress(DoCore& doCore, DOStatus doStatus, bool useDefault)
{
	for (auto olInc : range(outputs.size())) {
		handleEditChange(olInc);
	}
	setOLs(doCore, doStatus);
}

void OlSystem::zeroOffsetLock(DoCore& doCore, DOStatus doStatus)
{
	for (auto olInc : range(outputs.size())) {
		outputs[olInc].info.currFreq = outputs[olInc].info.defaultFreq;
	}
	updateEdits();
	setOLs(doCore, doStatus);
	emit notification("Zero'd Analog Outputs.\n", 2);
}

void OlSystem::setOLs(DoCore& doCore, DOStatus doStatus)
{
	std::array<double, size_t(OLGrid::total)> status = getOlValues();
	core.OLForceOutput(status, doCore, doStatus);
}

void OlSystem::setOlStatusNoForceOut(std::array<double, size_t(OLGrid::total)> status)
{
	for (auto olInc : range(outputs.size())) {
		outputs[olInc].info.currFreq = status[olInc];
	}
	updateEdits();
}

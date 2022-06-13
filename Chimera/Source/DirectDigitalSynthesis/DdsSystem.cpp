#include "stdafx.h"
#include "ftd2xx.h"
#include "DirectDigitalSynthesis/DdsSystem.h"
#include "PrimaryWindows/QtAuxiliaryWindow.h"
#include "GeneralObjects/multiDimensionalKey.h"
#include <ParameterSystem/Expression.h>
#include <boost/lexical_cast.hpp>
#include <qheaderview.h>
#include <qmenu.h>
#include <PrimaryWindows/QtMainWindow.h>

#include <qlayout.h>

DdsSystem::DdsSystem (IChimeraQtWindow* parent, bool ftSafemode) 
	: core( ftSafemode )
	, IChimeraSystem (parent)
	, roundToDdsPrecision(false)
{ }

void DdsSystem::handleContextMenu (const QPoint& pos)
{
	QTableWidgetItem* item = rampListview->itemAt (pos);
	QMenu menu;
	menu.setStyleSheet (chimeraStyleSheets::stdStyleSheet ());
	auto* deleteAction = new QAction ("Delete This Item", rampListview);
	rampListview->connect (deleteAction, &QAction::triggered, [this, item]() {
		refreshCurrentRamps ();
		currentRamps.erase(currentRamps.begin()+item->row());
		redrawListview ();
		});
	auto* newPerson = new QAction ("New Item", rampListview);
	rampListview->connect (newPerson, &QAction::triggered,
		[this]() {
			refreshCurrentRamps ();
			currentRamps.push_back (ddsIndvRampListInfo ());
			redrawListview ();
		});
	if (item) { menu.addAction (deleteAction); }
	menu.addAction (newPerson);
	menu.exec (rampListview->mapToGlobal (pos));
}

void DdsSystem::initialize(IChimeraQtWindow* parent)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	this->setMaximumWidth(600);

	ddsTitle = new QLabel("DDS OUTPUT", this);
	layout->addWidget(ddsTitle, 0);

	QHBoxLayout* layoutBtn = new QHBoxLayout();
	layoutBtn->setContentsMargins(0, 0, 0, 0);
	ddsSetButton = new CQPushButton("Set New DDS Values", parent);
	ddsSetButton->setToolTip("Press this button to attempt force all DDS values to the values currently recorded in the"
		" edits below.");
	parent->connect(ddsSetButton, &QPushButton::released, [parent]() {parent->auxWin->SetDds(); });
	zeroDdsButton = new CQPushButton("Zero DDSs", parent);
	zeroDdsButton->setToolTip("Press this button to turn off all DDS");
	parent->connect(zeroDdsButton, &QPushButton::released, [parent]() { parent->auxWin->zeroDds(); });
	QPushButton* relockPLLButton = new QPushButton("Relock PLL", parent);
	relockPLLButton->setToolTip("Press this button to relock the PLL in the DDS system");
	parent->connect(relockPLLButton, &QPushButton::released, [parent]() { parent->auxWin->relockPLL(); });
	// 
	quickChange = new CQCheckBox("Quick-Change", parent);
	quickChange->setChecked(false);
	quickChange->setToolTip("With this checked, you can quickly change a DAC's value by using the arrow keys while "
		"having the cursor before the desired digit selected in the DAC's edit.");

	layoutBtn->addWidget(ddsSetButton, 0);
	layoutBtn->addWidget(zeroDdsButton, 0);
	layoutBtn->addWidget(relockPLLButton , 0);
	layoutBtn->addWidget(quickChange, 0);

	layout->addLayout(layoutBtn);

	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);
	for (size_t i = 0; i < size_t(DDSGrid::numOFunit); i++)
	{
		QGridLayout* layoutGrid = new QGridLayout();
		//layoutGrid->addWidget(new QLabel(QString("DDS %1").arg(i)), 0, 0, 1, 2);
		layoutGrid->addWidget(new QLabel(QString("DDS %1: Freq(MHz)").arg(i)), 1, 0, Qt::AlignLeft);
		layoutGrid->addWidget(new QLabel("Ampl(\%)"), 1, 1,Qt::AlignLeft);
		for (size_t j = 0; j < size_t(DDSGrid::numPERunit); j++)
		{
			auto& out = outputs[i * size_t(DDSGrid::numPERunit) + j];
			out.initialize(parent, i * size_t(DDSGrid::numPERunit) + j);
			layoutGrid->addLayout(out.getLayout(), 2 + j, 0, 1, 2);
		}
		updateCoreNames();
		layout1->addLayout(layoutGrid, 0);
	}
	layout->addLayout(layout1, 0);

	//resetDds();
	connect(this, &DdsSystem::setExperimentActiveColor, this, [this](std::vector<DdsCommand> ddsCommand, bool expFinished) {
		if (ddsCommand.empty()) {
			for (auto& out : outputs) {
				out.setExpActiveColor(false);
			}
		}
		for (const auto& dcmd : ddsCommand) {
			outputs[dcmd.line].setExpActiveColor(true, expFinished);
		} });
}


bool DdsSystem::eventFilter(QObject* obj, QEvent* event) {
	for (auto& out : outputs) 
	{/*by calling out.eventFilter(obj, event), the edit is evaluated no matter whether the event is up/down*/
		if (out.eventFilter(obj, event) && quickChange->isChecked())
		{
			parentWin->auxWin->SetDds();
			return true;
		}
		else if (event->type() == QEvent::KeyPress) {
			if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Enter) {
				parentWin->auxWin->SetDds();
				return true;
			}
		}
	}
	return false;
}


void DdsSystem::initialize ( IChimeraQtWindow* parent, std::string title ){
	QVBoxLayout* layout = new QVBoxLayout(this);
	this->setMaximumWidth(900);

	ddsHeader = new QLabel (cstr (title), parent);
	layout->addWidget(ddsHeader, 0);
	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);
	programNowButton = new QPushButton ("Program Now", parent);
	layout1->addWidget(programNowButton, 0);
	parent->connect (programNowButton, &QPushButton::released, [this, parent]() {
		try	{
			programNow (parent->auxWin->getUsableConstants ());
		}
		catch (ChimeraError& err) {
			parent->reportErr (err.qtrace ());
		}
	});
	controlCheck = new CQCheckBox ("Control?", parent);
	layout1->addWidget(controlCheck, 0);
	layout->addLayout(layout1);
	rampListview = new QTableWidget (parent);
	layout->addWidget(rampListview);

	rampListview->horizontalHeader()->setDefaultSectionSize(80);
	rampListview->setColumnWidth (0, 60);
	rampListview->setColumnWidth (1, 60);
	rampListview->setColumnWidth (2, 60);
	rampListview->setColumnWidth (3, 60);
	rampListview->setColumnWidth (4, 60);
	rampListview->setColumnWidth (5, 60);
	rampListview->setColumnWidth (6, 120);
	//rampListview->verticalHeader()->setDefaultSectionSize(80);

	rampListview->setContextMenuPolicy (Qt::CustomContextMenu);
	parent->connect (rampListview, &QTableWidget::customContextMenuRequested,
		[this](const QPoint& pos) {handleContextMenu (pos); });
	rampListview->setShowGrid (true);

	QStringList labels;
	labels << "Index" << "Channel" << "Freq 1" << "Amp 1" << "Freq 2" << "Amp 2" << "Time";
	rampListview->setColumnCount (labels.size());
	rampListview->setHorizontalHeaderLabels (labels);
}

void DdsSystem::redrawListview ( ){
	rampListview->setRowCount (0);
	for (auto rampInc : range (currentRamps.size ()))	{
		rampListview->insertRow (rampListview->rowCount ());
		auto rowN = rampListview->rowCount ()-1;
		auto& ramp = currentRamps[rampInc];
		rampListview->setItem (rowN, 0, new QTableWidgetItem (cstr (ramp.index)));
		rampListview->setItem (rowN, 1, new QTableWidgetItem (cstr (ramp.channel)));
		rampListview->setItem (rowN, 2, new QTableWidgetItem (cstr (ramp.freq1.expressionStr)));
		rampListview->setItem (rowN, 3, new QTableWidgetItem (cstr (ramp.amp1.expressionStr)));
		rampListview->setItem (rowN, 4, new QTableWidgetItem (cstr (ramp.freq2.expressionStr)));
		rampListview->setItem (rowN, 5, new QTableWidgetItem (cstr (ramp.amp2.expressionStr)));
		rampListview->setItem (rowN, 6, new QTableWidgetItem (cstr (ramp.rampTime.expressionStr)));
	}
}

void DdsSystem::programNow ( std::vector<parameterType> constants ){
	try{
		refreshCurrentRamps ();
		core.manualLoadExpRampList (currentRamps);
		core.evaluateDdsInfo ( constants );
		core.generateFullExpInfo ( 1 );
		core.programVariation ( 0, constants, nullptr);
		emit notification ("Finished Programming DDS System!\n", 0);
	}
	catch ( ChimeraError& ){
		throwNested ( "Error seen while programming DDS system via Program Now Button." );
	}
}


void DdsSystem::handleSaveConfig (ConfigStream& file ){
	//refreshCurrentRamps ();
	file << core.getDelim() << "\n";
	file << "/*DDS Name:*/ ";
	for (auto& out : outputs) {
		file << out.info.name << " ";
	}
	file << "\n";

	file << "/*DDS Amplitude:*/ ";
	for (auto& out : outputs) {
		file << out.info.currAmp << " ";
	}
	file << "\n";

	file << "/*DDS Amplitude Max:*/ ";
	for (auto& out : outputs) {
		file << out.info.maxAmp << " ";
	}
	file << "\n";

	file << "/*DDS Amplitude Min:*/ ";
	for (auto& out : outputs) {
		file << out.info.minAmp << " ";
	}
	file << "\n";

	file << "/*DDS Frequency:*/ ";
	for (auto& out : outputs) {
		file << out.info.currFreq << " ";
	}
	file << "\n";

	file << "/*DDS Frequency Max:*/ ";
	for (auto& out : outputs) {
		file << out.info.maxFreq << " ";
	}
	file << "\n";

	file << "/*DDS Frequency Min:*/ ";
	for (auto& out : outputs) {
		file << out.info.minFreq << " ";
	}
	file << "\n";
	
	file << "/*DDS Description:*/ ";
	for (auto& out : outputs) {
		file << out.info.note << " ";
	}
	file << "\nEND_" + getDelim() << "\n";
}


void DdsSystem::handleOpenConfig ( ConfigStream& file )
{
	std::string test;
	for (auto& out : outputs) {
		file >> out.info.name;
	}

	try {
		for (auto& out : outputs) {
			file >> test;
			out.info.currAmp = boost::lexical_cast<double>(test);
		}
		for (auto& out : outputs) {
			file >> test;
			out.info.maxAmp = boost::lexical_cast<double>(test);
		}
		for (auto& out : outputs) {
			file >> test;
			out.info.minAmp = boost::lexical_cast<double>(test);
		}
		for (auto& out : outputs) {
			file >> test;
			out.info.currFreq= boost::lexical_cast<double>(test);
		}
		for (auto& out : outputs) {
			file >> test;
			out.info.maxFreq = boost::lexical_cast<double>(test);
		}
		for (auto& out : outputs) {
			file >> test;
			out.info.minFreq = boost::lexical_cast<double>(test);
		}
	}
	catch (boost::bad_lexical_cast&) {
		throwNested("DDS control failed to convert values recorded in the config file "
			"to doubles");
	}
	for (auto& out : outputs) {
		file >> out.info.note;
	}
	updateEdits();
	updateCoreNames();
	setDDSs();
	// update tooltip through setName and setNote
	for (auto idx : range(outputs.size())) {
		setName(idx, outputs[idx].info.name);
		setNote(idx, outputs[idx].info.note);
	}
}


std::string DdsSystem::getSystemInfo ( ){
	return core.getSystemInfo();
}

std::array<DdsInfo, size_t(DDSGrid::total)> DdsSystem::getDdsInfo()
{
	std::array<DdsInfo, size_t(DDSGrid::total)> info;
	for (auto ddsNum : range(outputs.size())) {
		info[ddsNum] = outputs[ddsNum].info;
	}
	return info;
}


std::string DdsSystem::getDelim ( ){
	return core.getDelim();
}

DdsCore& DdsSystem::getCore ( ){
	return core;
}

/******************************************************************************************************************/

//void DdsSystem::prepareForce()
//{
//	initializeDataObjects(1);
//}

//void DdsSystem::initializeDataObjects(unsigned cmdNum) 
//{
	//ddsCommandFormList = std::vector<DdsCommandForm>(cmdNum);

	//ddsCommandList.clear();
	//ddsSnapshots.clear();
	//ddsChannelSnapshots.clear();

	//loadSkipDdsSnapshots.clear();
	//finalFormatDdsData.clear();
	//loadSkipDdsFinalFormat.clear();

	//ddsCommandList.resize(cmdNum);
	//ddsSnapshots.resize(cmdNum);
	//ddsChannelSnapshots.resize(cmdNum);

	//loadSkipDdsSnapshots.resize(cmdNum);
	//finalFormatDdsData.resize(cmdNum);
	//loadSkipDdsFinalFormat.resize(cmdNum);

//}

void DdsSystem::handleSetDdsButtonPress(bool useDefault)
{
	//ddsCommandFormList.clear();
	//prepareForce();
	//updateDdsValues();
	setDDSs();
}

//void DdsSystem::forceDds(DdsSnapshot initSnap)
//{
//	//resetDdsEvents();
//	//prepareForce();
//	//updateEdits();
//	//setDDSs();
//}

void DdsSystem::zeroDds()
{
	for (auto& out : outputs)
	{
		//out.info.currFreq = 0.0;
		out.info.currAmp = 0.0;
	}
	updateEdits();
	//updateDdsValues();
	emit notification("Off'd DDS Outputs.\n", 2);
	setDDSs();
}

void DdsSystem::resetDds()
{
	for (auto& out : outputs)
	{
		out.info.currFreq = out.info.defaultFreq;
		out.info.currAmp = out.info.defaultAmp;
	}
	updateEdits();
	//updateDdsValues();
	emit notification("Default'd DDS Outputs.\n", 2);
	setDDSs();
}

//void DdsSystem::resetDdsEvents()
//{
//	initializeDataObjects(0);
//}

void DdsSystem::setDDSs()
{
	try {
		std::vector<std::vector<DdsChannelSnapshot>> channelSnapShot;
		channelSnapShot.resize(1);
		for (unsigned short line = 0; line < size_t(DDSGrid::total); ++line) {
			channelSnapShot[0].push_back({ 'f',line,1.0 + line/*time in ms*/,
				outputs[line].info.currFreq ,outputs[line].info.currFreq ,0.0 });
		}
		for (unsigned short line = 0; line < size_t(DDSGrid::total); ++line) {
			channelSnapShot[0].push_back({ 'a',line,1.0 + line/*time in ms*/,
				outputs[line].info.currAmp ,outputs[line].info.currAmp ,0.0 });
		}
		core.setGUIDdsChange(channelSnapShot);
	}
	catch (ChimeraError& e) {
		thrower(e.what());
	}
}

void DdsSystem::relockPLL()
{
	int tcp_connect;
	try {
		tcp_connect = zynq_tcp.connectTCP(ZYNQ_ADDRESS);
	}
	catch (ChimeraError& err) {
		tcp_connect = 1;
		errBox(err.what());
	}

	if (tcp_connect == 0) {
		zynq_tcp.writeCommand("lockPLL");
		zynq_tcp.disconnect();
	}
	else {
		throw("connection to zynq failed. can't lock DDS PLLs\n");
	}
}



void DdsSystem::updateEdits()
{
	for (auto& dds : outputs)
	{
		dds.updateEdit();
	}
}

std::string DdsSystem::getName(int ddsNumber)
{
	return outputs[ddsNumber].info.name;
}
std::string DdsSystem::getNote(int ddsNumber)
{
	return outputs[ddsNumber].info.note;
}
std::pair<double, double> DdsSystem::getAmplRange(int ddsNumber)
{
	return std::make_pair(outputs[ddsNumber].info.minAmp, outputs[ddsNumber].info.maxAmp);
}
void DdsSystem::setName(int ddsNumber, std::string name)
{
	outputs[ddsNumber].setName(name);
}
void DdsSystem::setNote(int ddsNumber, std::string note)
{
	outputs[ddsNumber].setNote(note);
}
void DdsSystem::setAmpMinMax(int ddsNumber, double min, double max)
{
	if (!(min <= max)) {
		thrower("Min dac value must be less than max dac value.");
	}
	if (min < 0 || min > 10 || max < 0 || max > 10) {
		thrower("Min and max dds amplitude values must be withing [0,10].");
	}
	outputs[ddsNumber].info.minAmp = min;
	outputs[ddsNumber].info.maxAmp = max;
}
void DdsSystem::updateCoreNames()
{
	std::array<std::string, size_t(DDSGrid::total)> names_;
	for (size_t i = 0; i < size_t(DDSGrid::total); i++)
	{
		names_[i] = outputs[i].info.name;
	}
	core.setNames(names_);
}

void DdsSystem::standardExperimentPrep(unsigned variation)
{
	emit setExperimentActiveColor(core.getDdsCommand(variation), false);
	core.standardExperimentPrep(variation);
}


//void DdsSystem::updateDdsValues()
//{
//	long dacInc = 0;
//	for (auto& dds : outputs)
//	{
//		dds.handleEdit();
//		ddsValues[dacInc][0] = dds.info.currFreq;
//		ddsValues[dacInc][1] = dds.info.currAmp;
//		dacInc++;
//	}
//}


void DdsSystem::refreshCurrentRamps () {
	currentRamps.resize (rampListview->rowCount ());
	for (auto rowI : range(rampListview->rowCount ())) {
		try {
			currentRamps[rowI].index = boost::lexical_cast<int>(cstr(rampListview->item (rowI, 0)->text ()));
			currentRamps[rowI].channel = boost::lexical_cast<int>(cstr (rampListview->item (rowI, 1)->text ()));
			currentRamps[rowI].freq1 = str (rampListview->item (rowI, 2)->text ());
			currentRamps[rowI].amp1 = str(rampListview->item (rowI, 3)->text ());
			currentRamps[rowI].freq2 = str(rampListview->item (rowI, 4)->text ());
			currentRamps[rowI].amp2 = str(rampListview->item (rowI, 5)->text ());
			currentRamps[rowI].rampTime = str(rampListview->item (rowI, 6)->text ());
		}
		catch (ChimeraError &) {
			throwNested ("Failed to convert dds table data to ramp structure!");
		}
	}
}
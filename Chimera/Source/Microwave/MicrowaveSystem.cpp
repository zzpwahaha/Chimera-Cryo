// created by Mark O. Brown
#include "stdafx.h"
#include "Microwave/MicrowaveSystem.h"
#include "LowLevel/constants.h"
#include "PrimaryWindows/QtAuxiliaryWindow.h"
#include <qheaderview.h>
#include <qmenu.h>
#include "PrimaryWindows/QtMainWindow.h"
#include <QTableWidget.h>

MicrowaveSystem::MicrowaveSystem(IChimeraQtWindow* parent) : IChimeraSystem(parent) {}

std::string MicrowaveSystem::getIdentity(){ 
	return core.queryIdentity();
}

void MicrowaveSystem::handleContextMenu (const QPoint& pos){
	QTableWidgetItem* item = uwListListview->itemAt (pos);
	QMenu menu;
	menu.setStyleSheet (chimeraStyleSheets::stdStyleSheet ());
	auto* deleteAction = new QAction ("Delete This Item", uwListListview);
	uwListListview->connect (deleteAction, &QAction::triggered, [this, item]() {uwListListview->removeRow (item->row ()); });
	auto* newPerson = new QAction ("New Item", uwListListview);
	uwListListview->connect (newPerson, &QAction::triggered,
		[this]() {currentList.push_back (microwaveListEntry ()); refreshListview (); });
	if (item) { menu.addAction (deleteAction); }
	menu.addAction (newPerson);
	menu.exec (uwListListview->mapToGlobal (pos));
}

void MicrowaveSystem::initialize( IChimeraQtWindow* parent ){
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	header = new QLabel ("MICROWAVE SYSTEM", parent); 
	layout->addWidget(header);

	QHBoxLayout* layout1 = new QHBoxLayout(this);
	layout1->setContentsMargins(0, 0, 0, 0);

	controlOptionCheck = new QCheckBox ("Control?", parent);
	reconnectPush = new QPushButton("Reconnect", parent);
	programNowPush = new QPushButton ("Program Now", parent);

	parent->connect(reconnectPush, &QPushButton::released, [this, parent]() {
		emit notification("----------------------\r\nReconnect Offsetlocks... \n");
		try {
			core.uwFlume.resetConnection();
			emit notification("Finished Reconnecting UWSYSTEM WINDFREAK.\r\n");
		}
		catch (ChimeraError& exception) {
			errBox(exception.trace());
			emit notification(": " + exception.qtrace() + "\r\n");
			emit error(": " + exception.qtrace() + "\r\n");
		}
		});

	parent->connect (programNowPush, &QPushButton::released, [this, parent]() {
		try	{
			programNow (parent->auxWin->getUsableConstants ());
		}
		catch (ChimeraError& err) {
			parent->reportErr ("Failed to program microwave system! " + err.qtrace ());
		}
	});
	layout1->addWidget(reconnectPush);
	layout1->addWidget(programNowPush);
	layout1->addWidget(controlOptionCheck);

	QHBoxLayout* layout2 = new QHBoxLayout(this);
	layout2->setContentsMargins(0, 0, 0, 0);
	writeNow = new QPushButton ("Write Now", parent);
	writeNow->connect (writeNow, &QPushButton::pressed, this, &MicrowaveSystem::handleWritePress);
	writeTxt = new QLineEdit (parent);
	writeTxt->setMaximumWidth(50);
	layout2->addWidget(writeNow);
	layout2->addWidget(writeTxt);

	//QHBoxLayout* layout3 = new QHBoxLayout(this);
	//layout3->setContentsMargins(0, 0, 0, 0);
	readNow = new QPushButton("Read Now", parent);
	readNow->connect (readNow, &QPushButton::pressed, this, &MicrowaveSystem::handleReadPress);
	readTxt = new QLabel("", parent);
	readTxt->setMaximumWidth(100);
	readTxt->setMinimumWidth(50);
	readTxt->setFrameShape(QFrame::Box);
	layout2->addWidget(readNow);
	layout2->addWidget(readTxt);

	QFrame* line = new QFrame(this);
	line->setFrameShape(QFrame::VLine);
	layout2->addWidget(line);


	//QHBoxLayout* layout4 = new QHBoxLayout(this);
	//layout4->setContentsMargins(0, 0, 0, 0);
	triggerStepTimeLabel = new QLabel ("Trig. Time (ms)", parent);
	triggerStepTimeEdit = new QLineEdit (parent);
	triggerStepTimeEdit->setMaximumWidth(50);
	triggerStepTimeEdit->setText("0.5");
	triggerStepTimeEdit->connect (triggerStepTimeEdit, &QLineEdit::textChanged, [this, parent]() {
		try {
			auto time = boost::lexical_cast<double>(str(triggerStepTimeEdit->text ()));
			core.setTrigTime (time);
		}
		catch (boost::bad_lexical_cast &) {} // probably just happens while user is trying to type
		});
	layout2->addWidget(triggerStepTimeLabel);
	layout2->addWidget(triggerStepTimeEdit);
	layout2->addStretch(1);
	
	uwListListview = new QTableWidget (parent);
	uwListListview->setColumnCount (3);
	QStringList labels;
	labels << "#" << "Frequency (GHz)" << "Power (dBm)";
	uwListListview->setHorizontalHeaderLabels (labels);
	uwListListview->horizontalHeader ()->setFixedHeight (20);
	uwListListview->verticalHeader ()->setFixedWidth (25);
	uwListListview->verticalHeader ()->setDefaultSectionSize (20);
	uwListListview->setContextMenuPolicy (Qt::CustomContextMenu);
	parent->connect (uwListListview, &QTableWidget::customContextMenuRequested,
		[this](const QPoint& pos) {handleContextMenu (pos); });
	uwListListview->setColumnWidth (0, 40);
	uwListListview->setColumnWidth (1, 240);
	uwListListview->setColumnWidth (2, 140);

	uwListListview->setShowGrid (true);
	uwListListview->setMaximumHeight(100);
	refreshListview ();
	
	layout->addLayout(layout1);
	layout->addLayout(layout2);
	//layout->addLayout(layout3);
	//layout->addLayout(layout4);
	layout->addWidget(uwListListview);

	emit notification(qstr(core.queryIdentity()), 0);
}

void MicrowaveSystem::refreshCurrentUwList () {
	currentList.resize (uwListListview->rowCount ());
	for (auto rowI : range (uwListListview->rowCount ())) {
		try {
			currentList[rowI].frequency = str (uwListListview->item (rowI, 1)->text ());
			currentList[rowI].power = str (uwListListview->item (rowI, 2)->text ());
		}
		catch (ChimeraError&) {
			throwNested ("Failed to convert microwave table data to uw list structure!");
		}
	}
}

void MicrowaveSystem::handleReadPress (){
	try {
		auto res = core.uwFlume.read();
		readTxt->setText(qstr(res));
		readTxt->setToolTip(qstr(res));
	}
	catch (ChimeraError& e) {
		emit error("Error seen in trying to read from Microwave System:\n" + e.qtrace());
	}
	
}

void MicrowaveSystem::handleWritePress (){
	try {
		auto txt = writeTxt->text();
		core.uwFlume.write(str(txt));
	}
	catch (ChimeraError& e) {
		emit error("Error seen in trying to write to Microwave System:\n" + e.qtrace());
	}
	
}


void MicrowaveSystem::programNow(std::vector<parameterType> constants){
	// ignore the check if the user literally presses program now.
	core.experimentSettings.control = true;
	core.experimentSettings.list = currentList;
	core.experimentActive = true;
	std::string warnings;

	core.calculateVariations (constants, nullptr);
	core.programVariation (0, constants, nullptr);
	emit notification("Finished programming microwave system!\n", 0);
}


void MicrowaveSystem::handleSaveConfig (ConfigStream& saveFile){
	refreshCurrentUwList ();
	saveFile << core.configDelim
		<< "\n/*Control?*/ " << controlOptionCheck->isChecked ()
		<< "\n/*List Size:*/ " << currentList.size ();
	for (auto listElem : currentList){
		saveFile << "\n/*Freq:*/ " << listElem.frequency 
				 << "\n/*Power:*/ " << listElem.power;
	}
	saveFile << "\nEND_" << core.configDelim << "\n";
}

void MicrowaveSystem::setMicrowaveSettings (microwaveSettings settings){
	controlOptionCheck->setChecked (settings.control);
	currentList = settings.list;
	refreshListview ();
}


void MicrowaveSystem::refreshListview (){
	unsigned count = 0;
	uwListListview->setRowCount (0);
	for (auto listElem : currentList){
		auto ind = uwListListview->rowCount ();
		uwListListview->insertRow (ind);
		uwListListview->setItem (ind, 0, new QTableWidgetItem (cstr(count)));
		uwListListview->item (ind, 0)->setFlags (uwListListview->item (ind, 0)->flags () ^ Qt::ItemIsEnabled);
		uwListListview->setItem (ind, 1, new QTableWidgetItem (cstr (listElem.frequency.expressionStr)));
		uwListListview->setItem (ind, 2, new QTableWidgetItem (cstr (listElem.power.expressionStr)));
		count++;
	}
}

MicrowaveCore& MicrowaveSystem::getCore (){
	return core;
}


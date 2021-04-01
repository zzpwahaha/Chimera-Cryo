// created by Mark O. Brown
#include "stdafx.h"
#include "AiSystem.h"
#include <qtimer.h>
#include <qlayout.h>
#include <QtConcurrent/qtconcurrentrun.h>
#include <Windows.h>

AiSystem::AiSystem(IChimeraQtWindow* parent)
	: IChimeraSystem(parent)
	, core()
{
}


std::string AiSystem::getSystemStatus( ){
	std::string answerStr = "AI System: Connected through: " + AI_SOCKET_ADDRESS + "at port: " + str(AI_SOCKET_PORT);
	return answerStr;
}

void AiSystem::refreshDisplays( ){
	for ( auto dispInc : range(voltDisplays.size())){
		voltDisplays[dispInc]->setText(qstr(core.getAiVals()[dispInc].mean, numDigits)
			+ "(" + qstr(core.getAiVals()[dispInc].std * pow(10.0, numDigits), 0) + ")");
		aiCombox[dispInc]->setCurrentIndex(int(core.getAiVals()[dispInc].status.range));
	}

}

void AiSystem::initialize (IChimeraQtWindow* parent) 
{
	//initDaqmx ();
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	this->setMaximumWidth(1300);
	QLabel* title = new QLabel ("ANALOG-INPUT", parent);
	layout->addWidget(title, 0);

	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);

	getValuesButton = new CQPushButton ("Get Values", parent);
	parent->connect (getValuesButton, &QPushButton::released, [this]() { 
		refreshCurrentValues (); 
		refreshDisplays (); });
	continuousQueryCheck = new CQCheckBox ("Qry Cont.", parent);
	queryBetweenVariations = new CQCheckBox ("Qry Btwn Vars", parent);
	layout1->addWidget(getValuesButton, 0);
	layout1->addWidget(continuousQueryCheck, 0);
	layout1->addWidget(queryBetweenVariations, 0);
	layout1->addStretch(1);
	layout->addLayout(layout1, 0);

	QHBoxLayout* layout2 = new QHBoxLayout();
	layout2->setContentsMargins(0, 0, 0, 0);

	continuousIntervalLabel = new QLabel ("Cont. Interval:", parent);
	continuousInterval = new CQLineEdit (qstr (AiSettings ().continuousModeInterval), parent);
	
	QTimer::singleShot (1000, this, &AiSystem::handleTimer);
	avgNumberLabel = new QLabel ("# To Avg:", parent);
	avgNumberEdit = new CQLineEdit (qstr (AiSettings ().numberMeasurementsToAverage), parent);
	layout2->addWidget(continuousIntervalLabel, 0);
	layout2->addWidget(continuousInterval, 1);
	layout2->addWidget(avgNumberLabel, 0);
	layout2->addWidget(avgNumberEdit, 1);
	layout2->addStretch(1);
	layout->addLayout(layout2, 0);

	QGridLayout* AIGridLayout = new QGridLayout();
	std::array<std::string, 2> chnlStr = { "A","B" };
	unsigned x = 4, y =4;
	for (size_t i = 0; i < size_t(AIGrid::total); i++)
	{
		auto& cbox = aiCombox[i];
		QHBoxLayout* lay = new QHBoxLayout();
		lay->setContentsMargins(0, 0, 0, 0);
		cbox = new CQComboBox(parent);
		cbox->setMaximumWidth(50);
		parent->connect(cbox, qOverload<int>(&QComboBox::activated), [parent, this, i, cbox](int) {
			int selection = cbox->currentIndex();
			core.setAiRange(i, AiChannelRange::which(selection));
			try { core.updateChannelRange(); }
			catch (ChimeraError& e) { emit error(e.what()); }
			 });

		for (auto& whichR : AiChannelRange::allRanges) {
			cbox->addItem(AiChannelRange::toStr(whichR).c_str());
		}
		lay->addWidget(cbox, 0);

		QLabel* label = new QLabel(qstr(chnlStr[i / size_t(AIGrid::numPERunit)] + str(i % size_t(AIGrid::numPERunit))));
		QFont font = label->font();
		font.setUnderline(true);
		label->setFont(font);
		lay->addWidget(label, 0);

		voltDisplays[i] = new QLabel(qstr(0.0, numDigits), parent);
		lay->addWidget(voltDisplays[i], 0);
		lay->addStretch(1);
		AIGridLayout->addLayout(lay, 2 * (i % size_t(AIGrid::numPERunit)) / x, (2 * i + i / size_t(AIGrid::numPERunit)) % x);
	}
	//this->setStyleSheet("border: 2px solid  black;");
	layout->addLayout(AIGridLayout);
	refreshDisplays();

}

void AiSystem::handleTimer () {
	int interval = 1000;
	try {
		if (continuousQueryCheck->isChecked ()) {
			refreshCurrentValues ();
			refreshDisplays ();
		}
		try {
			interval = boost::lexical_cast<int>(str (continuousInterval->text ()));
		}
		catch (boost::bad_lexical_cast&) { // just go with 1s if the input is invalid.
		}
	}
	catch (ChimeraError & err) {
		errBox (err.trace ());
	}
	QTimer::singleShot (interval, this, &AiSystem::handleTimer);
}

AiSettings AiSystem::getAiSettings (){
	AiSettings settings;
	settings.queryBtwnVariations = queryBetweenVariations->isChecked ();
	settings.queryContinuously = continuousQueryCheck->isChecked ();
	try{
		settings.continuousModeInterval = boost::lexical_cast<int>(str(continuousInterval->text()));
	}
	catch (ChimeraError &) { errBox ("Failed to convert ai-system number of measurements to average string to int!"); };
	try{
		settings.numberMeasurementsToAverage = boost::lexical_cast<unsigned>(str(avgNumberEdit->text()));
		if (settings.numberMeasurementsToAverage < 2){
			settings.numberMeasurementsToAverage = 2;
			setAiSettings (settings);
		}
	}
	catch (ChimeraError &) { errBox ("Failed to convert ai-system number of measurements to average string to unsigned int!"); };
	return settings;
}

AiSettings AiSystem::getSettingsFromConfig (ConfigStream& file){
	AiSettings settings;
	file >> settings.queryBtwnVariations;
	file >> settings.queryContinuously;
	file >> settings.numberMeasurementsToAverage;
	file >> settings.continuousModeInterval;
	core.getSettingsFromConfig(file);
	
	return settings;
}

void AiSystem::handleSaveConfig (ConfigStream& file){
	auto settings = getAiSettings ();
	file << core.configDelim
		<< "\n/*Query Between Variations?*/ " << settings.queryBtwnVariations
		<< "\n/*Query Continuously?*/ " << settings.queryContinuously
		<< "\n/*Average Number:*/ " << settings.numberMeasurementsToAverage
		<< "\n/*Contiuous Mode Interval:*/ " << settings.continuousModeInterval;
	core.saveSettingsToConfig(file);
	file << "\nEND_" + core.configDelim + "\n";
}

void AiSystem::handleOpenConfig(ConfigStream& file)
{
	setAiSettings(getSettingsFromConfig(file));
	refreshDisplays();
}

void AiSystem::setAiSettings (AiSettings settings){
	queryBetweenVariations->setChecked (settings.queryBtwnVariations);
	continuousQueryCheck->setChecked (settings.queryContinuously);
	avgNumberEdit->setText (qstr(settings.numberMeasurementsToAverage));
	continuousInterval->setText (qstr (settings.continuousModeInterval));
}


bool AiSystem::wantsContinuousQuery( ){
	return continuousQueryCheck->isChecked( );
}


void AiSystem::refreshCurrentValues( ){
	try {
		//QFuture<void> future = QtConcurrent::run(&core, &AiCore::getSingleSnap, getAiSettings().numberMeasurementsToAverage);
		core.getSingleSnap(getAiSettings().numberMeasurementsToAverage);
	}
	catch (ChimeraError& e) {
		emit error(e.what());
	}
}


//void AiSystem::getAquisitionData( ){
//	int32 sampsRead;
//	daqmx.readAnalogF64( analogInTask0, aquisitionData, sampsRead );
//}

bool AiSystem::wantsQueryBetweenVariations( ){
	return queryBetweenVariations->isChecked( );
}



double AiSystem::getSingleChannelValue( unsigned chan, unsigned n_to_avg ){
	if (core.getAiVals()[chan].status.range == AiChannelRange::which::off) {
		core.setAiRange(chan, AiChannelRange::which::full);
		core.updateChannelRange();
	}
	Sleep(1);
	core.getSingleSnap( n_to_avg );
	return core.getCurrentValues()[chan];
}


std::array<double, size_t(AIGrid::total)> AiSystem::getSingleSnapArray( unsigned n_to_avg ){
	core.getSingleSnap( n_to_avg );
	return core.getCurrentValues();
}



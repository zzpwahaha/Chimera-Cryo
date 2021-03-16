// created by Mark O. Brown
#include "stdafx.h"
#include "AiSystem.h"
#include <qtimer.h>
#include <qlayout.h>

AiSystem::AiSystem(IChimeraQtWindow* parent)
	: IChimeraSystem(parent)
	, socket(AI_SAFEMODE, AI_SOCKET_ADDRESS, AI_SOCKET_PORT)
	, daqmx( ANALOG_IN_SAFEMODE ) {
	socket.write("mac", terminator);
	socket.read();
}

/*
 *	We use a PCI card for analog input currently.
 */
void AiSystem::initDaqmx( ){
	daqmx.createTask( "Analog-Input", analogInTask0 );
	daqmx.createAiVoltageChan( analogInTask0, cstr(boardName + "/ai0:7"), "", DAQmx_Val_Diff, -10.0, 10.0, DAQmx_Val_Volts, nullptr );
}


std::string AiSystem::getSystemStatus( ){
	long answer = daqmx.getProductCategory( cstr(boardName) );
	std::string answerStr = "AI System: Connected... device category = " + str( answer );
	return answerStr;
}


void AiSystem::refreshDisplays( ){
	for ( auto dispInc : range(voltDisplays.size())){
		voltDisplays[dispInc]->setText( str(currentValues[dispInc], numDigits).c_str() );
	}
}


void AiSystem::initialize (IChimeraQtWindow* parent) 
{
	//initDaqmx ();
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	this->setMaximumWidth(1000);
	QLabel* title = new QLabel ("ANALOG-INPUT", parent);
	layout->addWidget(title, 0);

	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);

	getValuesButton = new CQPushButton ("Get Values", parent);
	parent->connect (getValuesButton, &QPushButton::released, [this]() { refreshCurrentValues (); refreshDisplays (); });
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
	for (size_t i = 0; i < NUMBER_AI_CHANNELS; i++)
	{
		QHBoxLayout* lay = new QHBoxLayout();
		lay->setContentsMargins(0, 0, 0, 0);
		lay->addWidget(new QLabel(qstr(chnlStr[i % AI_NumOfUnit] + str(i / AI_NumOfUnit))),0);
		voltDisplays[AI_NumPerUnit * (i % AI_NumOfUnit) + (i / AI_NumOfUnit)] = new QLabel("0", parent);
		lay->addWidget(voltDisplays[AI_NumPerUnit * (i % AI_NumOfUnit) + (i / AI_NumOfUnit)], 0);
		lay->addStretch(1);
		AIGridLayout->addLayout(lay, i / AI_NumPerUnit, i % AI_NumPerUnit);
	}
	layout->addLayout(AIGridLayout);

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
	return settings;
}

void AiSystem::handleSaveConfig (ConfigStream& file){
	auto settings = getAiSettings ();
	file << configDelim 
		<< "\n/*Query Between Variations?*/ " << settings.queryBtwnVariations 
		<< "\n/*Query Continuously?*/ " << settings.queryContinuously 
		<< "\n/*Average Number:*/ " << settings.numberMeasurementsToAverage 
		<< "\n/*Contiuous Mode Interval:*/ " << settings.continuousModeInterval 
		<< "\nEND_" + configDelim + "\n";
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
	currentValues = getSingleSnapArray( getAiSettings().numberMeasurementsToAverage );
}


void AiSystem::armAquisition( unsigned numSnapshots ){
	// may need to use numSnapshots X NUM_AI_CHANNELS?
	daqmx.configSampleClkTiming( analogInTask0, "", 10000.0, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, numSnapshots );
	daqmx.startTask( analogInTask0 );
	aquisitionData = std::vector<float64>( NUMBER_AI_CHANNELS * numSnapshots );
}

std::vector<float64> AiSystem::getCurrentValues( ){
	return { currentValues.begin( ), currentValues.end( ) };
}

void AiSystem::getAquisitionData( ){
	int32 sampsRead;
	daqmx.readAnalogF64( analogInTask0, aquisitionData, sampsRead );
}

bool AiSystem::wantsQueryBetweenVariations( ){
	return queryBetweenVariations->isChecked( );
}

std::vector<float64> AiSystem::getSingleSnap( unsigned n_to_avg ){
	try{
		daqmx.configSampleClkTiming( analogInTask0, "", 10000.0, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, n_to_avg );
		daqmx.startTask( analogInTask0 );
		// don't understand why but need 2 samples min???
		std::vector<float64> tmpdata( NUMBER_AI_CHANNELS*n_to_avg );
		int32 sampsRead;
		daqmx.readAnalogF64( analogInTask0, tmpdata, sampsRead );
		daqmx.stopTask( analogInTask0 );
		std::vector<float64> data( NUMBER_AI_CHANNELS );
		unsigned count = 0;
		for ( auto& d : data ){
			d = 0;
			for ( auto i : range( n_to_avg ) ){
				d += tmpdata[count++];
			}
			d /= n_to_avg;
		}
		return data;
	}
	catch (ChimeraError &){
		throwNested ("Error exception thrown while getting Ai system single snap!");
	}
}


double AiSystem::getSingleChannelValue( unsigned chan, unsigned n_to_avg ){
	auto all = getSingleSnap( n_to_avg );
	return all[chan];
}


std::array<float64, AiSystem::NUMBER_AI_CHANNELS> AiSystem::getSingleSnapArray( unsigned n_to_avg ){
	std::vector<float64> data = getSingleSnap( n_to_avg );
	std::array<float64, NUMBER_AI_CHANNELS> retData;
	for ( auto dataInc : range( NUMBER_AI_CHANNELS ) ){
		retData[dataInc] = data[dataInc];
	}
	return retData;
}

void AiSystem::logSettings (DataLogger& log, ExpThreadWorker* threadworker){
}

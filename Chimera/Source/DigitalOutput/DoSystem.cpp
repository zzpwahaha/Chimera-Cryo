// created by Mark O. Brown
#include "stdafx.h"

#include "DoSystem.h"

#include "ConfigurationSystems/Version.h"
#include "LowLevel/constants.h"
#include "PrimaryWindows/QtAuxiliaryWindow.h"
#include "GeneralUtilityFunctions/range.h"

#include <sstream>
#include <unordered_map>
#include <bitset>
#include "nidaqmx2.h"
#include <boost/lexical_cast.hpp>

#include "ExperimentMonitoringAndStatus/ColorBox.h""
#include <qlayout.h> 

// I don't use this because I manually import dll functions.
// #include "Dio64.h"
DoSystem::DoSystem( IChimeraQtWindow* parent, bool ftSafemode, bool serialSafemode ) 
	: core()
	, IChimeraSystem(parent)
	, holdStatus(false)
{
	for (auto& out : outputs) { out.set(0); }
}

DoSystem::~DoSystem() { }

void DoSystem::handleSaveConfig(ConfigStream& saveFile){
	/// ttl settings
	saveFile << "TTLS\n";
	// nothing at the moment.
	saveFile << "END_TTLS\n";
}

void DoSystem::handleOpenConfig(ConfigStream& openFile)
{
}


void DoSystem::setTtlStatusNoForceOut(std::array< std::array<bool, size_t(DOGrid::numPERunit)>, size_t(DOGrid::numOFunit) > status)
{
	for ( auto rowInc : range(status.size()) ){
		for ( auto numInc : range(status[rowInc].size()) ){
			outputs(rowInc,numInc).set(status[rowInc][numInc]);
		}
	}
}

Matrix<std::string> DoSystem::getAllNames(){
	return core.getAllNames ();
}

void DoSystem::updatePush(unsigned row, unsigned number ){
	outputs(row, number).updateStatus();
}


void DoSystem::updateDefaultTtl(unsigned row, unsigned column, bool state){
	outputs(row, column).defaultStatus = state;
}

bool DoSystem::getDefaultTtl(unsigned row, unsigned column){
	return outputs(row, column).defaultStatus;
}

std::pair<unsigned, unsigned> DoSystem::getTtlBoardSize(){
	return { outputs.numRows, outputs.numColumns };
}

void DoSystem::initialize(IChimeraQtWindow* parent) {
	QVBoxLayout* layout = new QVBoxLayout(this);
	this->setMaximumWidth(1000);
	// title
	ttlTitle = new QLabel("DIGITAL OUTPUT", parent);
	layout->addWidget(ttlTitle, 0);
	// all number numberLabels

	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);

	ttlHold = new CQPushButton("Hold Current Values", parent);
	ttlHold->setToolTip("Press this button to change multiple TTLs simultaneously. Press the button, then change the "
		"ttls, then press the button again to release it. Upon releasing the button, the TTLs will change.");
	parent->connect(ttlHold, &QPushButton::released, [parent, this]() {
		try {
			parent->configUpdated();
			handleHoldPress();
			emit notification("Handling Hold Press.\n", 2);
		}
		catch (ChimeraError& exception) {
			emit error("TTL Hold Handler Failed: " + exception.qtrace() + "\n");
		}
		});
	ttlHold->setCheckable(true);

	zeroTtls = new CQPushButton("Zero DOs", parent);
	zeroTtls->setToolTip("Press this button to set all ttls to their zero (false) state.");
	parent->connect(zeroTtls, &QPushButton::released, [parent, this]() {
		try {
			zeroBoard();
			parent->configUpdated();
			emit notification("Zero'd DOs.\n", 2);
		}
		catch (ChimeraError& exception) {
			emit notification("Failed to Zero DOs!!!\n", 1);
			emit error(exception.qtrace());
		}
		});
	layout1->addWidget(ttlHold);
	layout1->addWidget(zeroTtls);
	layout->addLayout(layout1);


	QGridLayout* DOGridLayout = new QGridLayout();
	unsigned runningCount = 0;
	auto names = core.getAllNames();
	
	for (auto row : range(size_t(DOGrid::numOFunit))) 
	{
		runningCount++;
		QHBoxLayout* DOsubGridLayout = new QHBoxLayout();
		DOsubGridLayout->addWidget(new QLabel(QString::number(row + 1)), 0, Qt::AlignRight);
		for (size_t number = 0; number < outputs.numColumns; number++) 
		{
			auto& out = outputs(row, number);
			out.initialize(parent);
			names(row, number) = "do" +
				str((runningCount - 1) / size_t(DDSGrid::numPERunit) + 1) + "_" +
				str(((runningCount - 1)) % size_t(DDSGrid::numPERunit)); /*default name, always accepted by script*/
			out.setName(names(row, number));
			
			parent->connect(out.check, &QCheckBox::stateChanged, [this, &out, parent]() {
				try {
					handleTTLPress(out);
					emit notification("Handled DO " + qstr(out.getPosition().first) + ","
						+ qstr(out.getPosition().second) + " State Change.\n", 2);
					parent->configUpdated();
				}
				catch (ChimeraError& exception) {
					emit error("DO Press Handler Failed.\n" + exception.qtrace() + "\n");
				}
				});
			DOsubGridLayout->addWidget(out.check);
		}
		DOsubGridLayout->setSpacing(8);
		DOGridLayout->addLayout(DOsubGridLayout, runningCount % 3, 2 - runningCount / 3);
	}
	core.setNames(names);
	DOGridLayout->setHorizontalSpacing(20);
	DOGridLayout->setVerticalSpacing(12);
	layout->addLayout(DOGridLayout);



}

int DoSystem::getNumberOfTTLRows(){
	return outputs.numRows;
}

int DoSystem::getNumberOfTTLsPerRow(){
	return outputs.numColumns;
}

void DoSystem::handleTTLPress(DigitalOutput& out){
	if ( holdStatus == false ){
		out.set (out.check->isChecked ());
		core.FPGAForceOutput(getCurrentStatus ());
	}
	else{
		out.setHoldStatus ( !out.holdStatus );
	}
}

// this function handles when the hold button is pressed.
void DoSystem::handleHoldPress(){
	if (holdStatus == true){
		// set all the holds.
		holdStatus = false;
		// make changes
		for ( auto& out : outputs )	{
			out.set ( out.holdStatus );
		}
		core.FPGAForceOutput(getCurrentStatus());
	}
	else{
		holdStatus = true;
		for ( auto& out : outputs )	{
			out.setHoldStatus ( out.getStatus ( ) );
		}
	}
}

std::array< std::array<bool, size_t(DOGrid::numPERunit)>, size_t(DOGrid::numOFunit) > DoSystem::getCurrentStatus()
{
	std::array< std::array<bool, size_t(DOGrid::numPERunit)>, size_t(DOGrid::numOFunit) > currentStatus;
	for ( auto& out : outputs )
	{
		currentStatus[out.getPosition().first][out.getPosition().second] = out.getStatus();
	}
	return currentStatus;
}

void DoSystem::setName(unsigned row, unsigned number, std::string name){
	if (name == ""){
		// no empty names allowed.
		return;
	}
	outputs(row, number).setName(name);
	auto names = core.getAllNames ();
	names(unsigned(row), number) = name;
	core.setNames(names);
}


std::string DoSystem::getName(unsigned row, unsigned number) {
	return core.getAllNames()(row, number);
}

bool DoSystem::getTtlStatus(unsigned row, int number){
	return outputs(row, number).getStatus ( );
}

allDigitalOutputs& DoSystem::getDigitalOutputs ( ){
	return outputs;
}



std::pair<unsigned short, unsigned short> DoSystem::calcDoubleShortTime( double time ){
	unsigned short lowordTime, hiwordTime;
	// convert to system clock ticks. Assume that the crate is running on a 10 MHz signal, so multiply by
	// 10,000,000, but then my time is in milliseconds, so divide that by 1,000, ending with multiply by 10,000
	lowordTime = unsigned __int64( time * 10000 ) % 65535;
	hiwordTime = unsigned __int64( time * 10000 ) / 65535;
	if ( unsigned __int64( time * 10000 ) / 65535 > 65535 ){
		thrower ( "DIO system was asked to calculate a time that was too long! this is limited by the card." );
	}
	return { lowordTime, hiwordTime };
}



void DoSystem::zeroBoard( ){
	for ( auto& out : outputs ){
		out.set (0); 
	}
	core.FPGAForceOutput(getCurrentStatus());
}

DoCore& DoSystem::getCore (){
	return core;
}

void DoSystem::standardExperimentPrep(unsigned variationInc, double currLoadSkipTime, std::vector<parameterType>& expParams) {
	core.organizeTtlCommands(variationInc, { ZYNQ_DEADTIME,getCurrentStatus() });
	core.findLoadSkipSnapshots(currLoadSkipTime, expParams, variationInc);
	//convertToFtdiSnaps (variationInc);
	//convertToFinalFtdiFormat (variationInc);
	//core.convertToFinalFormat(variationInc);/*seems useless*/
	core.formatForFPGA(variationInc);
}
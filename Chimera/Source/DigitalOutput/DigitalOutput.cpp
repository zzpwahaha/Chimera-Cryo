#include "stdafx.h"
#include "DigitalOutput.h"


void DigitalOutput::updateStatus ( ){ // function name should probably be changed, updating check display only here
	if ( check == nullptr )	{
		return;
	}
	if (check->isChecked () != status){
		check->setChecked (status);
	}
}


void DigitalOutput::initLoc ( unsigned numIn, unsigned rowIn ){
	row = rowIn;
	num = numIn;
}


void DigitalOutput::set ( bool stat, bool updateDisplay ){
	status = stat;
	if (updateDisplay) {
		updateStatus();
	}
}


bool DigitalOutput::getStatus ( ){
	return status;
}


std::pair<unsigned, unsigned> DigitalOutput::getPosition ( ){
	return { row, num };
}


void DigitalOutput::setName ( std::string nameStr )
{
	check->setToolTip ( nameStr.c_str());
}

void DigitalOutput::setExpActiveColor(bool usedInExp, bool expFinished)
{
	check->setStyleSheet(usedInExp ? 
		(expFinished ? "QCheckBox { border: 2px solid rgb(255, 255, 0)}" : "QCheckBox { border: 2px solid rgb(0, 255, 0)}") :
		"QCheckBox { border: 2px solid rgb(240, 240, 240); }");
}

void DigitalOutput::setHoldStatus ( bool stat ){
	holdStatus = stat;
}

void DigitalOutput::initialize ( IChimeraQtWindow* parent )
{
	check = new CQCheckBox ("", parent);

}


void DigitalOutput::enable ( bool enabledStatus ){
	check->setEnabled( enabledStatus );
}


DigitalOutput& allDigitalOutputs::operator()(  unsigned row, unsigned num ){
	return core[  row  * size_t(DOGrid::numPERunit) + num ];
}

allDigitalOutputs::allDigitalOutputs ( ){
	for ( auto row : range(numRows))
	{
		for ( auto num : range ( numColumns ) )
		{
			(*this)(row, num).initLoc(num, row);
		}
	}
}

#include "stdafx.h"
#include "DigitalOutput.h"


void DigitalOutput::updateStatus ( ){
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


void DigitalOutput::set ( bool stat ){
	status = stat;
	updateStatus ( );
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


DigitalOutput& allDigitalOutputs::operator()( unsigned num, unsigned row ){
	return core[ unsigned ( row ) * size_t(DOGrid::numPERunit) + num ];
}

allDigitalOutputs::allDigitalOutputs ( ){
	for ( auto row : range(numRows))
	{
		for ( auto num : range ( numColumns ) )
		{
			(*this)( num, row ).initLoc ( num, row );
		}
	}
}

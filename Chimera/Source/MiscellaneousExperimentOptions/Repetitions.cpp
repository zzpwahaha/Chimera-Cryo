// created by Mark O. Brown
#include "stdafx.h"
#include "Repetitions.h"
#include <unordered_map>
#include "LowLevel/constants.h"
#include "PrimaryWindows/QtAuxiliaryWindow.h"
#include <boost/lexical_cast.hpp>
#include "PrimaryWindows/QtMainWindow.h"
#include <qlayout.h>

unsigned Repetitions::getSettingsFromConfig (ConfigStream& openFile ){
	unsigned repNum;
	openFile >> repNum;
	return repNum;
}

void Repetitions::handleSaveConfig(ConfigStream& saveFile){
	saveFile << "REPETITIONS\n";
	saveFile << "/*Reps:*/" << getRepetitionNumber ();
	saveFile << "\nEND_REPETITIONS\n";
}

void Repetitions::updateNumber(long repNumber){
	repetitionDisp->setText (cstr (repNumber));
}


void Repetitions::initialize(IChimeraQtWindow* parent )
{
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	repetitionNumber = 100;
	// title
	repetitionText = new QLabel ("Repetition #", parent);
	repetitionEdit = new CQLineEdit (cstr (repetitionNumber), parent);
	parent->connect (repetitionEdit, &QLineEdit::textEdited, [parent]() {parent->configUpdated (); });
	repetitionDisp = new QLabel ("-", parent);

	layout->addWidget(repetitionText, 0);
	layout->addWidget(repetitionEdit, 1);
	layout->addWidget(repetitionDisp, 0);

}

void Repetitions::setRepetitions(unsigned number){
	repetitionNumber = number;
	repetitionEdit->setText (cstr (number));
	repetitionDisp->setText("---");
}

unsigned Repetitions::getRepetitionNumber(){
	auto text = repetitionEdit->text ();
	try	{
		repetitionNumber = boost::lexical_cast<int>(str(text));
	}
	catch ( boost::bad_lexical_cast&){
		throwNested ("Failed to convert repetition number text to an integer!");
	}
	return repetitionNumber;
}


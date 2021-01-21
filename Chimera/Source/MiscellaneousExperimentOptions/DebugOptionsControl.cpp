// created by Mark O. Brown
#include "stdafx.h"
#include "DebugOptionsControl.h"
#include "ConfigurationSystems/ConfigSystem.h"
#include <boost/lexical_cast.hpp>
#include <qlayout.h>

void DebugOptionsControl::initialize( IChimeraQtWindow* parent )
{
	QVBoxLayout* layout = new  QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	header = new QLabel ("DEBUGGING OPTIONS", parent);
	auto configUpdate = [parent]() {parent->configUpdated (); };
	layout->addWidget(header, 0);

	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);
	pauseText = new QLabel ("Pause Btwn Variations (ms):", parent);
	pauseEdit = new QLineEdit ("0", parent);
	parent->connect(pauseEdit, &QLineEdit::textChanged, configUpdate);
	layout1->addWidget(pauseText, 0);
	layout1->addWidget(pauseEdit, 1);

	layout->addLayout(layout1);

}


void DebugOptionsControl::handleSaveConfig(ConfigStream& saveFile){
	saveFile << "DEBUGGING_OPTIONS"
			 << "\n/*Sleep Time:*/\t\t\t\t\t\t" << currentOptions.sleepTime
			 << "\nEND_DEBUGGING_OPTIONS\n";
}

void DebugOptionsControl::handleOpenConfig(ConfigStream& openFile ){
	std::string sleep;
	openFile >> sleep;
	try{
		currentOptions.sleepTime = boost::lexical_cast<long>(sleep);
	}
	catch ( boost::bad_lexical_cast&){
		currentOptions.sleepTime = 0;
	}
	setOptions( currentOptions );
}

debugInfo DebugOptionsControl::getOptions(){
	try{
		currentOptions.sleepTime = boost::lexical_cast<long>( str ( pauseEdit->text()) );
	}
	catch ( boost::bad_lexical_cast& ){
		throwNested ( "could not convert sleep time to long!" );
	}
	return currentOptions;
}

void DebugOptionsControl::setOptions(debugInfo options){
	currentOptions = options;
	pauseEdit->setText( cstr( currentOptions.sleepTime ) );
}

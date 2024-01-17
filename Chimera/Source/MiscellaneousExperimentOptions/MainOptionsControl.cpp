// created by Mark O. Brown

#include "stdafx.h"
#include "MainOptionsControl.h"
#include <PrimaryWindows/IChimeraQtWindow.h>
#include "ConfigurationSystems/ConfigSystem.h"
#include "PrimaryWindows/QtMainWindow.h"
#include <boost/lexical_cast.hpp>
#include <qdebug.h>
#include <qlayout.h>

void MainOptionsControl::initialize( IChimeraQtWindow* parent )
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	header = new QLabel ("MAIN OPTIONS", parent);
	auto configUpdate = [parent]() {parent->configUpdated (); };
	auto changeBkgColor = [](QCheckBox* chkBox) { chkBox->isChecked() ?
		chkBox->setStyleSheet("QCheckBox { background-color: chartreuse }")
		: chkBox->setStyleSheet("QCheckBox { background-color: (240,240,240) }"); };

	randomizeVariationsButton = new QCheckBox ("Randomize Variations?", parent);
	parent->connect (randomizeVariationsButton, &QCheckBox::clicked, configUpdate);
	connect(randomizeVariationsButton, &QCheckBox::stateChanged, [this, changeBkgColor]() {
		changeBkgColor(randomizeVariationsButton); });

	repetitionFirstButton = new QCheckBox("Repetition First?", parent);
	parent->connect(repetitionFirstButton, &QCheckBox::clicked, configUpdate);
	connect(repetitionFirstButton, &QCheckBox::stateChanged, [this, changeBkgColor]() {
		changeBkgColor(repetitionFirstButton); });

	inExpCalControl = new InExpCalControl(parent);
	inExpCalControl->initialize(parent);

	delayAutoCal = new QCheckBox ("Delay Auto-Calibration", parent);
	connect(delayAutoCal, &QCheckBox::stateChanged, [this, changeBkgColor]() {
		changeBkgColor(delayAutoCal); });
	delayAutoCal->setChecked(true);

	layout->addWidget(header, 0);
	layout->addWidget(randomizeVariationsButton, 0);
	layout->addWidget(repetitionFirstButton, 0);
	layout->addLayout(inExpCalControl->layout);
	layout->addWidget(delayAutoCal, 0);

	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);
	atomThresholdForSkipText = new QLabel ("Atom Threshold for Load Skip:", parent);
	atomThresholdForSkipEdit = new QLineEdit ("-1", parent);
	parent->connect (atomThresholdForSkipEdit, &QLineEdit::textEdited, configUpdate);

	layout1->addWidget(atomThresholdForSkipText, 0);
	layout1->addWidget(atomThresholdForSkipEdit, 1);

	layout->addLayout(layout1);

}

void MainOptionsControl::handleSaveConfig(ConfigStream& saveFile){
	try {
		// in case people forget to press enter on inExpCalibrationDurationEdit before they start exp or save
		currentOptions.inExpCalInterval = boost::lexical_cast<double>(str(inExpCalControl->inExpCalibrationDurationEdit->text()));
	}
	catch (boost::bad_lexical_cast&) {
		throwNested("Bad value for calibration interval value!");
	}
	saveFile << "MAIN_OPTIONS"
			 << "\n/*Randomize Variations?*/ " << randomizeVariationsButton->isChecked()
			 << "\n/*Repetition First Over Variation?*/ " << repetitionFirstButton->isChecked()
			 << "\n/*In-Experiment Calibration?*/ " << inExpCalControl->inExpCalibrationButton->isChecked()
			 << "\n/*In-Experiment Calibration Interval*/ " << str(inExpCalControl->inExpCalibrationDurationEdit->text())
			 << "\n/*Atom Threshold for Load Skip*/ " << str(atomThresholdForSkipEdit->text());
	saveFile << "\nEND_MAIN_OPTIONS\n";
}

mainOptions MainOptionsControl::getSettingsFromConfig (ConfigStream& openFile ){
	mainOptions options;
	openFile >> options.randomizeVariations;
	openFile >> options.repetitionFirst;
	openFile >> options.inExpCalibration;
	std::string txt;
	openFile >> txt;
	try {
		options.inExpCalInterval = boost::lexical_cast<double>(txt);
	}
	catch (boost::bad_lexical_cast&) {
		errBox("Bad value for calibration interval value! Falls back to the default value of 10 minute");
		options.inExpCalInterval = 10.0;
	}

	openFile >> txt;
	try{
		options.atomSkipThreshold = boost::lexical_cast<unsigned long>( txt );
	}
	catch ( boost::bad_lexical_cast& ){
		errBox ( "atom threshold for load skip failed to convert to an unsigned long! The code will force "
					"the threshold to the maximum threshold." );
		options.atomSkipThreshold = -1;
	}
	return options;
}

void MainOptionsControl::setOptions ( mainOptions opts ){
	currentOptions = opts;
	randomizeVariationsButton->setChecked( currentOptions.randomizeVariations );
	repetitionFirstButton->setChecked(currentOptions.repetitionFirst);
	inExpCalControl->inExpCalibrationButton->setChecked(currentOptions.inExpCalibration);
	inExpCalControl->inExpCalibrationDurationEdit->setText(qstr(currentOptions.inExpCalInterval, 2));
	if (inExpCalControl->inExpCalibrationButton->isChecked()) {
		inExpCalControl->timer->start(int(currentOptions.inExpCalInterval * 60 * 1000));
	}
	atomThresholdForSkipEdit->setText( qstr ( currentOptions.atomSkipThreshold ) );
}

mainOptions MainOptionsControl::getOptions(){
	currentOptions.randomizeVariations = randomizeVariationsButton->isChecked();
	currentOptions.repetitionFirst= repetitionFirstButton->isChecked();
	currentOptions.inExpCalibration = inExpCalControl->inExpCalibrationButton->isChecked();
	try {
		currentOptions.inExpCalInterval = boost::lexical_cast<double>(str(inExpCalControl->inExpCalibrationDurationEdit->text()));
	}
	catch (boost::bad_lexical_cast&) {
		throwNested("Bad value for calibration interval value!");
	}
	currentOptions.delayAutoCal = delayAutoCal->isChecked ();
	try{
		currentOptions.atomSkipThreshold = boost::lexical_cast<unsigned long>( str( atomThresholdForSkipEdit->text() ) );
	}
	catch ( boost::bad_lexical_cast& ){
		throwNested ( "failed to convert atom threshold for load-skip to an unsigned long!" );
	}
	return currentOptions;
}

void MainOptionsControl::startInExpCalibrationTimer()
{
	if (inExpCalControl->inExpCalibrationButton->isChecked()) {
		double interval;
		try {
			interval = boost::lexical_cast<double>(str(inExpCalControl->inExpCalibrationDurationEdit->text()));
		}
		catch (boost::bad_lexical_cast&) { 
			thrower("Invalid InExpCalibration Duration ??? This should not happen.");
		}
		if (inExpCalControl->inExpCalibrationButton->isChecked() != currentOptions.inExpCalibration ||
			fabs(interval - currentOptions.inExpCalInterval) > 1e6 * DBL_EPSILON) {
			thrower("Inconsistent InExpCalibration Option or Duration ??? This is a low level bug.");;
		}
		inExpCalControl->interrupt = false;
		inExpCalControl->timer->start(int(interval * 60 * 1000));
	}
	else {
		inExpCalControl->timer->stop();
		inExpCalControl->interrupt = false;
	}
}

std::atomic<bool>* MainOptionsControl::interruptPointer()
{
	return &inExpCalControl->interrupt;
}

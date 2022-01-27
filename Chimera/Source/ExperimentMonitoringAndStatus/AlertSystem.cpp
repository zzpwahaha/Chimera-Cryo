// created by Mark O. Brown
#include "stdafx.h"
#include "AlertSystem.h"
//#include <Mmsystem.h>
//#include <mciapi.h>
//#pragma comment(lib, "Winmm.lib")
#include "PrimaryWindows/QtAndorWindow.h"
#include "GeneralUtilityFunctions/miscCommonFunctions.h"
#include <boost/lexical_cast.hpp>
#include <qlayout.h>


void AlertSystem::initialize(IChimeraQtWindow* parent ){
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	title = new QLabel ("ALERT SYSTEM", parent);
	layout->addWidget(title, 0);
	
	QHBoxLayout* layout1 = new QHBoxLayout(this);
	layout1->setContentsMargins(0, 0, 0, 0);
	atomsAlertActiveCheckBox = new QCheckBox ("If No Atoms?", parent);
	atomsAlertActiveCheckBox->setChecked( false );
	motAlertActiveCheckBox = new QCheckBox ("If No MOT?", parent);
	motAlertActiveCheckBox->setChecked ( true );
	alertThresholdText = new QLabel("Alert Threshold:", parent);
	alertThresholdEdit = new QLineEdit("10", parent);

	layout1->addWidget(atomsAlertActiveCheckBox, 0);
	layout1->addWidget(motAlertActiveCheckBox, 0);
	layout1->addWidget(alertThresholdText, 0);
	layout1->addWidget(alertThresholdEdit, 1);

	QHBoxLayout* layout2 = new QHBoxLayout(this);
	layout2->setContentsMargins(0, 0, 0, 0);
	autoPauseAtAlert = new QCheckBox("Automatically Pause on Alert?", parent);
	autoPauseAtAlert->setChecked(true);
	/// Sound checkbox
	soundAtFinishCheck = new QCheckBox("Play Sound at Finish?", parent);
	layout2->addWidget(autoPauseAtAlert, 0);
	layout2->addWidget(soundAtFinishCheck, 0);


	layout->addLayout(layout1);
	layout->addLayout(layout2);

}


bool AlertSystem::wantsAutoPause( )
{
	return autoPauseAtAlert->isChecked( );
}


unsigned AlertSystem::getAlertThreshold()
{
	try
	{
		alertThreshold = boost::lexical_cast<unsigned long>( str(alertThresholdEdit->text()) );
	}
	catch ( boost::bad_lexical_cast& )
	{
		throwNested ( "ERROR: alert threshold failed to reduce to unsigned long!" );
	}
	return alertThreshold;
}


void AlertSystem::setAlertThreshold()
{
	try
	{
		alertThreshold = boost::lexical_cast<int>( str(alertThresholdEdit->text()) );
	}
	catch ( boost::bad_lexical_cast& )
	{
		throwNested ( "ERROR: Alert threshold must be an integer!" );
	}
}


void AlertSystem::alertMainThread(int runsWithoutAtoms)
{
	int* alertLevel = new int;
	if (runsWithoutAtoms == alertThreshold)
	{
		*alertLevel = 2;
		//PostMessage(eCameraWindowHandle, alertMessageID, 0, (LPARAM)alertLevel);
	}
	else if (runsWithoutAtoms % alertThreshold == 0)
	{
		*alertLevel = 1;
		//PostMessage(eCameraWindowHandle, alertMessageID, 0, (LPARAM)alertLevel);
	}
	// don't sound the alert EVERY time... hence the % above.
}


void AlertSystem::soundAlert()
{
	//Beep(523, 100);
	//Beep(523, 100);
	//Beep(523, 100);
}




unsigned AlertSystem::getAlertMessageID()
{
	return alertMessageID;
}


bool AlertSystem::wantsAtomAlerts()
{
	return atomsAlertActiveCheckBox->isChecked();
}


bool AlertSystem::wantsMotAlerts ( )
{
	return motAlertActiveCheckBox->isChecked();
}


void AlertSystem::playSound()
{
	//mciSendString("play mp3 from 0", NULL, 0, NULL);
}


void AlertSystem::stopSound()
{
	//mciSendString("stop mp3", NULL, 0, NULL);
}


bool AlertSystem::soundIsToBePlayed()
{
	return soundAtFinishCheck->isChecked();
}
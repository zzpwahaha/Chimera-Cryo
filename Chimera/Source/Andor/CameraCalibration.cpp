// created by Mark O. Brown
#include "stdafx.h"
#include "CameraCalibration.h"
#include <GeneralUtilityFunctions/commonFunctions.h>

void CameraCalibration::initialize( IChimeraQtWindow* parent ){
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	header = new QLabel ("Camera-Bkgd:", parent);
	calButton = new QPushButton ("Calibrate", parent);
	parent->connect (calButton, &QPushButton::released, [parent]() {
		commonFunctions::calibrateCameraBackground (parent); });

	autoCalButton = new QCheckBox ("Auto-Cal", parent);
	useButton = new QCheckBox ("Use-Cal", parent);
	layout->addWidget(header);
	layout->addWidget(calButton);
	layout->addWidget(autoCalButton);
	layout->addWidget(useButton);
	layout->addStretch();
}


void CameraCalibration::setAutoCal(bool option)
{
	autoCalButton->setChecked(option);
}


void CameraCalibration::setUse(bool option)
{
	useButton->setChecked(option);
}

bool CameraCalibration::use( )
{
	return useButton->isChecked( );
}


bool CameraCalibration::autoCal( )
{
	return autoCalButton->isChecked ( );
}


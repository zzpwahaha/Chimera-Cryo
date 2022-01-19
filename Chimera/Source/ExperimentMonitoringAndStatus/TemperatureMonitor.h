#pragma once
#include "GeneralObjects/commonTypes.h"
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <qlabel.h>


class TemperatureMonitor
{
public:
	void initialize(IChimeraQtWindow* parent);
	

private:
	QLabel* status;
	
};
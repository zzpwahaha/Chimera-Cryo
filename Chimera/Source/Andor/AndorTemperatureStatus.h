#pragma once
#include <string>
#include <qcolor.h>

struct AndorTemperatureStatus
{
	int temperature;
	int temperatureSetting;
	std::string andorRawMsg;
	std::string msg;
	QColor colorCode;
};
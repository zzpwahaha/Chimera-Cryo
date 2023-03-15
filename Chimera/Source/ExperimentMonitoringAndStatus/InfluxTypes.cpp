#include "stdafx.h"
#include "InfluxTypes.h"


const std::array<InfluxDataType::mode, 2> InfluxDataType::allModes = {
	InfluxDataType::mode::Temperature,
	InfluxDataType::mode::Pressure };

const std::array<InfluxDataUnitType::mode, 3> InfluxDataUnitType::allModes = {
	InfluxDataUnitType::mode::K,
	InfluxDataUnitType::mode::mBar,
	InfluxDataUnitType::mode::Torr };
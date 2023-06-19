#pragma once

struct InfluxDataType
{
	enum class mode {
		Temperature,
		Pressure
	};
	static const std::array<InfluxDataType::mode, 2> allModes;
};

struct InfluxDataUnitType
{
	enum class mode {
		K,
		mBar,
		Torr
	};
	static const std::array<InfluxDataUnitType::mode, 3> allModes;
};


#pragma once
#include <string>

enum class AIGrid : size_t
{
	numPERunit = 8,
	numOFunit = 2,
	total = numPERunit * numOFunit
};

struct AiSettings
{
	bool queryContinuously=false;
	int continuousModeInterval=1000;
	bool queryBtwnVariations=false;
	unsigned int numberMeasurementsToAverage=100;
};

struct AiChannelRange
{
	enum class which
	{
		off, quarter, half, full
	};
	which range = which::off;
	static const std::array<which, 4> allRanges;
	static std::string toStr(which m);
	static which fromStr(std::string txt);
	static int toInt(which m);
	static which fromInt(int idx);
	static double scales(which m);
	double scales();
	static unsigned short codes(which m);
	unsigned short codes();
};

struct AiValue {
	double mean = 0.0;
	double std = 0.0;
	AiChannelRange status;
	std::string name = "";
	std::string note = "None";
};
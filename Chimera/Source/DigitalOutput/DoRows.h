// created by Mark O. Brown
#pragma once
#include <string>
#include <array>
// as of now not yet used extensively in the actual dio system

enum class DOGrid : size_t 
{
	numPERunit = 8,
	numOFunit = 8
};

struct DoRows{
	/*this 'which' enum should  be in accordance with the numPERunit*/
	enum class which{
		A, B, C, D, E, F, G, H
	};
	static const std::array<which, size_t(DOGrid::numOFunit)> allRows;
	static std::string toStr ( which m );
	static which fromStr ( std::string txt );
};



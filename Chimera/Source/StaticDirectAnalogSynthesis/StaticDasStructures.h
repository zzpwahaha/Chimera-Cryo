#pragma once
#include <ParameterSystem/Expression.h>

enum class StaticDASGrid : size_t
{
	numPERunit = 2,
	numOFunit = STATICDAS_NUM,
	total = numPERunit * numOFunit
};

struct StaticDASSettings
{
	std::array<Expression, size_t(StaticDASGrid::total)> staticDASs;
	bool ctrlDAS;
};
#include <stdafx.h>
#include <RealTimeMOTAnalysis/MOTAnalysisType.h>

const std::vector<MOTAnalysisType::type> MOTAnalysisType::allTypes = std::vector<MOTAnalysisType::type>(
	{ type::min ,type::max ,type::meanx ,type::meany ,type::sigmax ,type::sigmay ,type::atomNum, type::density2d });
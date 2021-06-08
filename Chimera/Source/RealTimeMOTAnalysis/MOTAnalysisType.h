#pragma once
#include <GeneralUtilityFunctions/Thrower.h>
#include <GeneralUtilityFunctions/my_str.h>
#include <vector>
#include <string>

struct MOTAnalysisType
{
	enum class type {
		min,
		max,
		meanx,
		meany,
		sigmax,
		sigmay,
		atomNum,
		density2d
	};
	static const std::vector<type> allTypes;
	static std::string toStr(type m_) {
		switch (m_) {
		case type::min:
			return "Min";
		case type::max:
			return "Max";
		case type::meanx:
			return "Center X";
		case type::meany:
			return "Center Y";
		case type::sigmax:
			return "Width X";
		case type::sigmay:
			return "Width Y";
		case type::atomNum:
			return "Atom Number";
		case type::density2d:
			return "Atom Density (2D)";
		}
		return "Undefined name, lower level error";
	}
	static type fromStr(std::string txt) {
		for (auto opt : allTypes) {
			if (toStr(opt) == txt) {
				return opt;
			}
		}
		thrower("Failed to convert string to MOT analysis option!");
		return type::min;
	}
};


#pragma once
#include "ArbGenSettings.h"

enum class ArbGenType {
	Agilent, Siglent
};


/*this name is only for assisting the coding, not related to script at all*/
struct ArbGenEnum {
	enum class name {
		Siglent0, Agilent0/*, Flashing, Microwave*/
	};
	static const std::array<name, numArbGen> allAgs;
	static std::string toStr (name m_) {
		switch (m_) {
		case name::Siglent0:
			return UWAVE_SIGLENT_SETTINGS.deviceName;
		case name::Agilent0:
			return UWAVE_AGILENT_SETTINGS.deviceName;
		//case name::Flashing:
		//	return "Flashing";
		//case name::Microwave:
		//	return "Microwave";
		}
		return "";
	}
	static name fromStr (std::string txt) {
		for (auto opt : allAgs) {
			if (toStr (opt) == txt) {
				return opt;
			}
		}
		thrower ("Failed to convert string to Which ArbGen option!");
		return name::Siglent0;
	}
};

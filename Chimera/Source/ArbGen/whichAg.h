#pragma once
#include "ArbGenSettings.h"

enum class ArbGenType {
	Agilent, Siglent
};


/*this name is only for assisting the coding, not related to script at all*/
struct ArbGenEnum {
	enum class name {
		Siglent0, Agilent0, Siglent0FlashingSlave, Siglent1, Agilent1
	};
	static const std::array<name, numArbGen> allAgs;
	static std::string toStr (name m_) {
		switch (m_) {
		case name::Siglent0:
			return UWAVE_SIGLENT_SETTINGS.deviceName;
		case name::Siglent0FlashingSlave:
			return UWAVE_SIGLENT2_SETTINGS.deviceName;
		case name::Agilent0:
			return UWAVE_AGILENT_SETTINGS.deviceName;
		case name::Siglent1:
			return UWAVE_SIGLENT3_SETTINGS.deviceName;
		case name::Agilent1:
			return UWAVE_AGILENT2_SETTINGS.deviceName;
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

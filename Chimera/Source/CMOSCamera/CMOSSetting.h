#pragma once
#pragma once
#include <string>
#include "GeneralImaging/imageParameters.h"

struct CMOSAutoExposure {
	enum class mode {
		Continuous,
		Off,
		Once
	};
	static std::string toStr(mode m);
	static mode fromStr(std::string txt);
};


struct CMOSTrigger {
	enum class mode {
		External,
		AutomaticSoftware,
		ManualSoftware,
		ContinuousSoftware
	};
	virtual std::string toStr(mode m);
	virtual mode fromStr(std::string txt);
};

struct CMOSAcquisition {
	enum class mode {
		Finite,
		Continuous
	};
	static std::string toStr(mode m);
	static mode fromStr(std::string m);
};


struct CMOSSettings {
	bool on;
	bool expActive;
	unsigned int rawGain;
	CMOSAutoExposure::mode exposureMode = CMOSAutoExposure::mode::Off;
	double exposureTime;
	CMOSAcquisition::mode acquisitionMode = CMOSAcquisition::mode::Continuous;
	unsigned picsPerRep;
	unsigned repsPerVar;
	unsigned variations;
	CMOSTrigger::mode triggerMode = CMOSTrigger::mode::External;
	double frameRate;
	imageParameters dims;
	unsigned totalPictures() { return picsPerRep * repsPerVar * variations; }
};

struct MakoTrigger : public CMOSTrigger
{
	static std::string toStr(mode m);
	static mode fromStr(std::string txt);
};

struct MakoSettings {
	bool on;
	bool expActive;
	unsigned int rawGain;
	bool trigOn;
	MakoTrigger::mode triggerMode = MakoTrigger::mode::External;
	double exposureTime;
	double frameRate;
	imageParameters dims; // mako do not do binning
	unsigned picsPerRep;
	unsigned repsPerVar;
	unsigned variations;
	unsigned totalPictures() { return picsPerRep * repsPerVar * variations; }
};
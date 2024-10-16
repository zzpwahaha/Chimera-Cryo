#pragma once
#include <string>
#include "GeneralImaging/imageParameters.h"

struct CameraInfo
{
	static const unsigned WINDOW_MAKO_NUMBER = 2;
	enum class name {
		Mako1, Mako2, Mako3, Mako4
	};
	static const std::array<name, MAKO_NUMBER> allCams;
	CameraInfo::name camName;
	std::string ip;
	std::string delim;
	bool safemode = true;
	std::pair<unsigned, unsigned> triggerLine;
	static std::string toStr(name m_);
	static CameraInfo::name fromStr(std::string txt);

};

namespace MakoInfo {
	const CameraInfo camInfo1{ CameraInfo::name::Mako1,MAKO_IPADDRS[0],MAKO_DELIMS[0],MAKO_SAFEMODE[0], MAKO_TRIGGER_LINE[0] };
	const CameraInfo camInfo2{ CameraInfo::name::Mako2,MAKO_IPADDRS[1],MAKO_DELIMS[1],MAKO_SAFEMODE[1], MAKO_TRIGGER_LINE[1] };
	const CameraInfo camInfo3{ CameraInfo::name::Mako3,MAKO_IPADDRS[2],MAKO_DELIMS[2],MAKO_SAFEMODE[2], MAKO_TRIGGER_LINE[2] };
	const CameraInfo camInfo4{ CameraInfo::name::Mako4,MAKO_IPADDRS[3],MAKO_DELIMS[3],MAKO_SAFEMODE[3], MAKO_TRIGGER_LINE[3] };

	const std::array<CameraInfo, CameraInfo::WINDOW_MAKO_NUMBER> camWindow1({ camInfo1 ,camInfo2 });
	const std::array<CameraInfo, CameraInfo::WINDOW_MAKO_NUMBER> camWindow2({ camInfo3 ,camInfo4 });
}


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
	bool expActive = false;
	unsigned int rawGain;
	bool trigOn;
	MakoTrigger::mode triggerMode = MakoTrigger::mode::External;
	double exposureTime;
	double frameRate;
	imageParameters dims; // mako do not do binning
	unsigned picsPerRep;
	unsigned repsPerVar;
	bool repFirst;
	unsigned variations;
	std::vector<size_t> variationShuffleIndex; // used for randomize variation case, to look up the true variation
	unsigned totalPictures() { return picsPerRep * repsPerVar * variations; }
};


Q_DECLARE_METATYPE(MakoSettings*)
Q_DECLARE_METATYPE(CameraInfo)
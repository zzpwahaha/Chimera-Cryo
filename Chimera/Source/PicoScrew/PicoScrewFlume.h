#pragma once
#include <PicoScrew/NewportUSB.h>
//#include <NewportUSB.h>

class PicoScrewFlume
{
public:
	// THIS CLASS IS NOT COPYABLE.
	PicoScrewFlume& operator=(const PicoScrewFlume&) = delete;
	PicoScrewFlume(const PicoScrewFlume&) = delete;

	PicoScrewFlume(bool safemode, std::string deviceKey);
	~PicoScrewFlume();

	std::string read();
	void write(std::string cmd);
	std::string query(std::string cmd);

	const bool safemode;
	const std::string deviceKey;

private:
	NewportUSB npUSB;
};


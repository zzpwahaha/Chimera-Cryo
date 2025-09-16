#pragma once
#include <GeneralObjects/IChimeraSystem.h>
#include <ParameterSystem/ParameterSystem.h>
#include <StaticDirectAnalogSynthesis/StaticDasCore.h>

class IChimeraQtWindow;
class StaticDasSystem : public IChimeraSystem
{
	Q_OBJECT
public:
	// THIS CLASS IS NOT COPYABLE.
	StaticDasSystem& operator=(const StaticDasSystem&) = delete;
	StaticDasSystem(const StaticDasSystem&) = delete;
	StaticDasSystem(IChimeraQtWindow* parent);

	void initialize();
	void handleOpenConfig(ConfigStream& configFile);
	void handleSaveConfig(ConfigStream& configFile);
	void updateCtrlEnable();
	void handleProgramNowPress(std::vector<parameterType> constants);
	std::string getConfigDelim() { return core.getDelim(); };
	StaticDasCore& getCore() { return core; };
	std::string getDeviceInfo();

	void setDasEditValue(std::string dasfreq, unsigned channel); // should only be used in CommandModulator
private:
	bool expActive;
	StaticDasCore core;
	QCheckBox* ctrlButton;
	std::array<QLabel*, size_t(StaticDASGrid::total)> labels;
	std::array<QLineEdit*, size_t(StaticDASGrid::total)> edits;


};



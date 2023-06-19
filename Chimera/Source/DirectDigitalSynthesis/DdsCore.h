#pragma once 


#include "DdsSystemStructures.h"
#include "DdsOutput.h"
#include <GeneralObjects/Matrix.h>

#include "GeneralObjects/ExpWrap.h"
#include "GeneralObjects/IDeviceCore.h"
#include "ConfigurationSystems/Version.h"
#include "GeneralFlumes/ftdiFlume.h"
#include "Scripts/ScriptStream.h"
#include "ConfigurationSystems/ConfigStream.h"
#include <vector>
#include <array>

#include "ZynqTCP/ZynqTCP.h"


/*
This class handles the programming of the DDS and not any of the gui elements associated with the gui system.
This is a part of the DdsSystem, and is meant to be subclassed as such. It is used by the gui system itself when the
user programms the dds immediately without running an experiment. It is also the only part of the DdsSystem which
needs to be passed to the main experiment thread. This is part of a new attempt to better divide the gui functionality
from the core functionality and have a more minimal object passed into the main experiment thread.
*/
struct ddsExpSettings {
	bool control;
	std::vector< ddsIndvRampListInfo> ramplist;
};




class DdsCore : public IDeviceCore{
	public:
		// THIS CLASS IS NOT COPYABLE.
		DdsCore& operator=(const DdsCore&) = delete;
		DdsCore (const DdsCore&) = delete;

		DdsCore ( bool safemode );
		~DdsCore ( );

		ddsExpSettings getSettingsFromConfig (ConfigStream& file );
		void writeRampListToConfig ( std::vector<ddsIndvRampListInfo> list, ConfigStream& file );
		void programVariation ( unsigned variationNum, std::vector<parameterType>& params, ExpThreadWorker* threadworker) override;
		void connectasync ( );
		void disconnect ( );
		void writeOneRamp ( ddsRampFinFullInfo boxRamp, UINT8 rampIndex );
		std::vector<ddsRampFinFullInfo> analyzeRampList ( std::vector<ddsIndvRampListInfo> rampList, unsigned variation );
		void generateFullExpInfo ( unsigned numVariations );
		void assertDdsValuesValid ( std::vector<parameterType>& params );
		void evaluateDdsInfo ( std::vector<parameterType> params= std::vector<parameterType>());
		void forceRampsConsistent ( );
		void calculateVariations (std::vector<parameterType>& params, ExpThreadWorker* threadworker) override;
		std::string getSystemInfo ( );
		void clearDdsRampMemory ( );
		void manualLoadExpRampList (std::vector< ddsIndvRampListInfo> ramplist);
		const std::string configDelim = "DDS_SYSTEM";
		std::string getDelim () override { return configDelim; }
		void logSettings (DataLogger& log, ExpThreadWorker* threadworker) override;
		void loadExpSettings (ConfigStream& stream) override;
		void normalFinish () override {};
		void errorFinish () override {};
		
	private:
		std::vector<ddsIndvRampListInfo> expRampList;
		ExpWrap<std::vector<ddsRampFinFullInfo>> fullExpInfo;
		ddsConnectionType::type connType;
		const unsigned MSGLENGTH = 7;
		const unsigned char WBWRITE = (unsigned char) 161;
		const unsigned char WBWRITE_ARRAY = (unsigned char) 2; //Add 2 to WBWRITE
		const double INTERNAL_CLOCK = ( double ) 500.0; //Internal clock in MHz

		const UINT16 RESET_FREQ_ADDR_OFFSET = 0x0;
		const UINT16 RESET_AMP_ADDR_OFFSET = 0x100;
		const UINT16 REPS_ADDRESS_OFFSET = 0x200;
		const UINT16 RAMP_CHANNEL_ADDR_SPACING = 0x200;
		const UINT16 RAMP_FREQ_ADDR_OFFSET = 0x400;
		const UINT16 RAMP_AMP_ADDR_OFFSET = 0x1400;

		ftdiFlume ftFlume;
		const double defaultFreq = 10;
		const double defaultAmp = 100;

		void longUpdate ( );
		void lockPLLs ( );
		// get (frequency/amplitude) ? word
		std::array<UINT8, 4> intTo4Bytes ( int i_ );
		void writeDDS ( UINT8 DEVICE, UINT16 ADDRESS, std::array<UINT8, 4> data );
		INT getFTW ( double freq );
		unsigned getATW ( double amp );
		UINT16 getRepsFromTime ( double time );
		INT get32bitATW ( double amp );
		void channelSelect ( UINT8 device, UINT8 channel );

		void writeAmpMultiplier ( UINT8 device );
		void writeResetFreq ( UINT8 device, UINT8 channel, double freq );
		void writeResetAmp ( UINT8 device, UINT8 channel, double amp );
		void writeRampReps ( UINT8 index, UINT16 reps );
		void writeRampDeltaFreq ( UINT8 device, UINT8 channel, UINT8 index, double deltafreq );
		void writeRampDeltaAmp ( UINT8 device, UINT8 channel, UINT8 index, double deltaamp );
		void writeDDS ( UINT8 device, UINT16 address, UINT8 dat1, UINT8 dat2, UINT8 dat3, UINT8 dat4 );




public:
	bool isValidDDSName(std::string name);
	int getDDSIdentifier(std::string name);
	std::string getName(int ddsNumber);
	std::array<std::string, size_t(DDSGrid::total)> getName();
	void setNames(std::array<std::string, size_t(DDSGrid::total)> namesIn);


	void resetDDSEvents();
	void prepareForce();
	void sizeDataStructures(unsigned variations);
	void initializeDataObjects(unsigned variationNum);
	std::vector<DdsCommand> getDdsCommand(unsigned variation);
	void setDDSCommandForm(DdsCommandForm command);
	void handleDDSScriptCommand(DdsCommandForm command, std::string name, std::vector<parameterType>& vars);
	void calculateVariations(std::vector<parameterType>& variables, ExpThreadWorker* threadworker, 
		std::vector<calSettings>& calibrationSettings);
	void constructRepeats(repeatManager& repeatMgr);
	void organizeDDSCommands(UINT variation);
	void makeFinalDataFormat(UINT variation);
	void standardExperimentPrep(UINT variation);
	void writeDDSs(UINT variation, bool loadSkip);
	void setGUIDdsChange(std::vector<std::vector<DdsChannelSnapshot>> channelSnapShot);

private:
	std::array<std::string, size_t(DDSGrid::total)> names;


	//std::array <const double, 2> ddsResolution;
	std::vector<DdsCommandForm> ddsCommandFormList;
	// the first vector is for each variation.
	std::vector<std::vector<DdsCommand>> ddsCommandList;
	std::vector<std::vector<DdsSnapshot>> ddsSnapshots;
	std::vector<std::vector<DdsChannelSnapshot>> ddsChannelSnapshots;
	std::vector<std::pair<double, std::vector<DdsCommand>>> timeOrganizer;

	ZynqTCP zynq_tcp;

public:
	constexpr static double ddsFreqResolution = DdsOutput::ddsFreqResolution; // 500.0 / 0xffffffff; /*32bit, 500MHz clock freq*/
	constexpr static double ddsAmplResolution = DdsOutput::ddsAmplResolution; // 10.0 / 0x3ff; /*10bit dac 0b1111111111, 10mA max dac current*/
	const int numFreqDigits = static_cast<int>(abs(round(log10(ddsFreqResolution) - 0.49)));
	const int numAmplDigits = static_cast<int>(abs(round(log10(ddsAmplResolution) - 0.49)));

	//constexpr static double DDS_TIME_RESOLUTION = 1.6;/*temporary, should be fixed after felix update the dac trigger*/

};

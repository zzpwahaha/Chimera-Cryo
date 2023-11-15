#pragma once
//#include "GeneralFlumes/ftdiFlume.h"
//#include "GeneralFlumes/ftdiStructures.h"
#include <ParameterSystem/ParameterSystemStructures.h>
#include "GeneralObjects/ExpWrap.h"
#include <GeneralObjects/Matrix.h>
#include <Plotting/PlotInfo.h>

#include "DoStructures.h"
#include "ZynqTCP/ZynqTCP.h"

class ExpThreadWorker;

/*
	(Stands for DigitalOutput Core)
*/
class DoCore
{
	public:
		// THIS CLASS IS NOT COPYABLE.
		DoCore& operator=(const DoCore&) = delete;
		DoCore (const DoCore&) = delete;
		DoCore();
		~DoCore ();

		const std::string configDelim = "TTL_SYSTEM";
		std::string getDelim() { return configDelim; }

		//void ftdi_connectasync (const char devSerial[]);
		//void ftdi_disconnect ();
		//DWORD ftdi_write (unsigned variation, bool loadSkipf);
		//DWORD ftdi_trigger ();
		
		void standardNonExperimentStartDoSequence (DoSnapshot initSnap);
		void restructureCommands ();
		void initializeDataObjects (unsigned variationNum);

		void ttlOn (unsigned row, unsigned column, timeType time, repeatInfoId repeatId);
		void ttlOff (unsigned row, unsigned column, timeType time, repeatInfoId repeatId);
		void ttlOnDirect (unsigned row, unsigned column, double timev, unsigned variation);
		void ttlOffDirect (unsigned row, unsigned column, double timev, unsigned variation);
		void ttlPulseDirect(unsigned row, unsigned column, double timev, double dur, unsigned variation);
		
		void sizeDataStructures (unsigned variations);
		void calculateVariations (std::vector<parameterType>& params, ExpThreadWorker* threadworker);
		void constructRepeats(repeatManager& repeatMgr);
		bool repeatsExistInCommandForm(repeatInfoId repeatId);
		void addPlaceholderRepeatCommand(repeatInfoId repeatId);
		std::vector<std::vector<plotDataVec>> getPlotData (unsigned variation );
		std::string getTtlSequenceMessage (unsigned variation);
		std::vector<double> getFinalTimes ();
		unsigned countTriggers (std::pair<unsigned, unsigned> which, unsigned variation);
		
		void checkLongTimeRun(unsigned variation);
		// returns -1 if not a name.
		bool isValidTTLName (std::string name);
		int getNameIdentifier (std::string name, unsigned& row, unsigned& number);
		void organizeTtlCommands (unsigned variation, DoSnapshot initSnap = { 0,0 });

		unsigned long getNumberEvents (unsigned variation);
		std::vector<DoCommand> getTtlCommand(unsigned variation);

		void resetTtlEvents ();

		void wait2 (double time);
		void prepareForce ();
		void findLoadSkipSnapshots (double time, std::vector<parameterType>& variables, unsigned variation);

		std::vector<std::vector<DoSnapshot>> getTtlSnapshots ();

		void handleTtlScriptCommand (std::string command, timeType time, std::string name, Expression pulseLength,
			std::vector<parameterType>& vars, std::string scope, repeatInfoId repeatId);
		void handleTtlScriptCommand (std::string command, timeType time, std::string name,
			std::vector<parameterType>& vars, std::string scope, repeatInfoId repeatID);
		void setNames (std::array<std::string, size_t(DOGrid::total)> namesIn);
		std::array<std::string, size_t(DOGrid::total)> getAllNames ();
		//void standardExperimentPrep (unsigned variationInc, double currLoadSkipTime, std::vector<parameterType>& expParams);

		void DoCore::formatForFPGA(UINT variation);
		void DoCore::writeTtlDataToFPGA(UINT variation, bool loadSkip);
		void FPGAForceOutput(DOStatus status);
		void FPGAForcePulse(DOStatus status, std::vector<std::pair<unsigned, unsigned>> rowcol, double dur);

		DOStatus getFinalSnapshot();

	private:
		std::array<std::string, size_t(DOGrid::total)> names;
		//Matrix<std::string> names;

		//Zynq tcp connection
		ZynqTCP zynq_tcp;

		std::vector<std::vector<std::array<char[DIO_LEN_BYTE_BUF], 1>>> doFPGA;
		std::vector<DoCommandForm> ttlCommandFormList;
		// Each element of first vector is for each variation.
		std::vector<std::vector<DoCommand>> ttlCommandList;
		// Each element of first vector is for each variation.
		std::vector<std::vector<DoSnapshot>> ttlSnapshots, loadSkipTtlSnapshots;
		// Each element of first vector is for each variation.
		std::vector<std::vector<std::array<WORD, 6>>> formattedTtlSnapshots, loadSkipFormattedTtlSnapshots;
		// this is just a flattened version of the above snapshots. This is what gets directly sent to the dio64 card.
		std::vector<std::vector<WORD>> finalFormatTtlData, loadSkipFinalFormatTtlData;
		DOStatus defaultTtlState;



};
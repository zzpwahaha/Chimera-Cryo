#include "stdafx.h"
#include "DoCore.h"
#include "DoStructures.h"
#include "qdebug.h"
#include <ExperimentMonitoringAndStatus/ExperimentSeqPlotter.h>

#include <bitset>

DoCore::DoCore()
	//: names(size_t(DOGrid::numPERunit), size_t(DOGrid::numOFunit), "")
{
	//try	{
	//	connectType = ftdiConnectionOption::Async;
	//	ftdi_connectasync ("FT2E722BB");
	//}
	//catch (ChimeraError &)	{
	//	throwNested ("Failed to initialize DO Core!?!");
	//}
}

DoCore::~DoCore () { /*ftdi_disconnect ();*/ }

void DoCore::setNames (std::array<std::string, size_t(DOGrid::total)> namesIn)
{
	std::for_each(namesIn.begin(), namesIn.end(), [&](auto& name) {
		std::transform(name.begin(), name.end(), name.begin(), ::tolower); });
	//std::transform(names[doInc].begin(), names[doInc].end(), names[doInc].begin(), ::tolower);
	names = std::move(namesIn);
}

//void DoCore::ftdi_connectasync (const char devSerial[]){
//	if (ftFlume.getNumDevices () <= 0){
//		thrower ("No devices found.");
//	}
//	ftFlume.open (devSerial);
//	ftFlume.setUsbParams ();
//	connectType = ftdiConnectionOption::Async;
//}
//
//void DoCore::ftdi_disconnect (){
//	ftFlume.close ();
//	connectType = ftdiConnectionOption::None;
//}
//
//DWORD DoCore::ftdi_trigger (){
//	return ftFlume.trigger ();
//}
//
///*
//* Takes data from "mem" structure and writes to the dio board.
//*/
//DWORD DoCore::ftdi_write (unsigned variation, bool loadSkip){
//	if (connectType == ftdiConnectionOption::Serial || connectType == ftdiConnectionOption::Async){
//		auto& buf = loadSkip ? finFtdiBuffers_loadSkip (variation) : finFtdiBuffers (variation);
//		// please note that Serial mode has not been thoroughly tested (by me, MOB at least)!
//		bool proceed = true;
//		int count = 0;
//		int idx = 0;
//		unsigned int totalBytes = 0;
//		unsigned int number = 0;
//		unsigned long dwNumberOfBytesSent = 0;
//		totalBytes += ftFlume.write (buf.pts, buf.bytesToWrite);
//		return totalBytes;
//	}
//	else{
//		thrower ("No ftdi connection exists! Can't write without a connection.");
//	}
//	return 0;
//}
//
//void DoCore::fillFtdiDataBuffer (std::vector<unsigned char>& dataBuffer, unsigned offset, unsigned count, ftdiPt pt){
//	if (offset + 20 >= dataBuffer.size ()){
//		thrower ("tried to write data buffer out of bounds!");
//	}
//
//	dataBuffer[offset] = WBWRITE;
//	dataBuffer[offset + 1] = (((TIMEOFFS + count) >> 8) & 0xFF);
//	dataBuffer[offset + 2] = ((TIMEOFFS + count) & 0xFF);
//	dataBuffer[offset + 3] = (((pt.time) >> 24) & 0xFF);
//	dataBuffer[offset + 4] = (((pt.time) >> 16) & 0xFF);
//	dataBuffer[offset + 5] = (((pt.time) >> 8) & 0xFF);
//	dataBuffer[offset + 6] = ((pt.time) & 0xFF);
//
//	dataBuffer[offset + 7] = WBWRITE;
//	dataBuffer[offset + 8] = (((BANKAOFFS + count) >> 8) & 0xFF);
//	dataBuffer[offset + 9] = ((BANKAOFFS + count) & 0xFF);
//	dataBuffer[offset + 10] = pt.pts[0];
//	dataBuffer[offset + 11] = pt.pts[1];
//	dataBuffer[offset + 12] = pt.pts[2];
//	dataBuffer[offset + 13] = pt.pts[3];
//
//	dataBuffer[offset + 14] = WBWRITE;
//	dataBuffer[offset + 15] = (((BANKBOFFS + count) >> 8) & 0xFF);
//	dataBuffer[offset + 16] = ((BANKBOFFS + count) & 0xFF);
//	dataBuffer[offset + 17] = pt.pts[4];
//	dataBuffer[offset + 18] = pt.pts[5];
//	dataBuffer[offset + 19] = pt.pts[6];
//	dataBuffer[offset + 20] = pt.pts[7];
//}

//void DoCore::convertToFinalFormat(UINT variation)
//{
//	// excessive but just in case.
//	formattedTtlSnapshots[variation].clear();
//	loadSkipFormattedTtlSnapshots[variation].clear();
//	finalFormatTtlData[variation].clear();
//	loadSkipFinalFormatTtlData[variation].clear();
//	// do bit arithmetic.
//	for (auto& snapshot : ttlSnapshots[variation])
//	{
//		// each major index is a row, each minor index is a ttl state (0, 1) in that row.
//		std::array<std::bitset<8>, 8> ttlBits;
//		for (UINT rowInc : range(8))
//		{
//			for (UINT numberInc : range(8))
//			{
//				ttlBits[rowInc].set(numberInc, snapshot.ttlStatus[rowInc][numberInc]);
//			}
//		}
//		// I need to put it as an int (just because I'm not actually sure how the bitset gets stored... it'd probably 
//		// work just passing the address of the bitsets, but I'm sure this will work so whatever.)
//		std::array<USHORT, 6> tempCommand;
//		tempCommand[0] = calcDoubleShortTime(snapshot.time).first;
//		tempCommand[1] = calcDoubleShortTime(snapshot.time).second;
//		tempCommand[2] = static_cast <unsigned short>(ttlBits[0].to_ulong());
//		tempCommand[3] = static_cast <unsigned short>(ttlBits[1].to_ulong());
//		tempCommand[4] = static_cast <unsigned short>(ttlBits[2].to_ulong());
//		tempCommand[5] = static_cast <unsigned short>(ttlBits[3].to_ulong());
//		formattedTtlSnapshots[variation].push_back(tempCommand);
//	}
//	// same loop with the loadSkipSnapshots.
//	for (auto& snapshot : loadSkipTtlSnapshots[variation])
//	{
//		// each major index is a row, each minor index is a ttl state (0, 1) in that row.
//		std::array<std::bitset<8>, 8> ttlBits;
//		for (UINT rowInc : range(8))
//		{
//			for (UINT numberInc : range(8))
//			{
//				ttlBits[rowInc].set(numberInc, snapshot.ttlStatus[rowInc][numberInc]);
//			}
//		}
//		// I need to put it as an int (just because I'm not actually sure how the bitset gets stored... it'd probably 
//		// work just passing the address of the bitsets, but I'm sure this will work so whatever.)
//		std::array<USHORT, 6> tempCommand;
//		tempCommand[0] = calcDoubleShortTime(snapshot.time).first;
//		tempCommand[1] = calcDoubleShortTime(snapshot.time).second;
//		tempCommand[2] = static_cast <unsigned short>(ttlBits[0].to_ulong());
//		tempCommand[3] = static_cast <unsigned short>(ttlBits[1].to_ulong());
//		tempCommand[4] = static_cast <unsigned short>(ttlBits[2].to_ulong());
//		tempCommand[5] = static_cast <unsigned short>(ttlBits[3].to_ulong());
//		loadSkipFormattedTtlSnapshots[variation].push_back(tempCommand);
//	}
//
//	/// flatten the data.
//	finalFormatTtlData[variation].resize(formattedTtlSnapshots[variation].size() * 6);
//	int count = 0;
//	for (auto& element : finalFormatTtlData[variation])
//	{
//		// concatenate
//		element = formattedTtlSnapshots[variation][count / 6][count % 6];
//		count++;
//	}
//	// the arrays are usually not the same length and need to be dealt with separately.
//	loadSkipFinalFormatTtlData[variation].resize(loadSkipFormattedTtlSnapshots[variation].size() * 6);
//	count = 0;
//	for (auto& element : loadSkipFinalFormatTtlData[variation])
//	{
//		// concatenate
//		element = loadSkipFormattedTtlSnapshots[variation][count / 6][count % 6];
//		count++;
//	}
//}




//void DoCore::convertToFinalFtdiFormat (unsigned variation){
//	for (auto loadSkip : { false, true }){
//		// first convert from diosnapshot to ftdi snapshot
//		auto& snaps = loadSkip ? ftdiSnaps_loadSkip (variation) : ftdiSnaps (variation);
//		auto& buf = loadSkip ? finFtdiBuffers_loadSkip (variation) : finFtdiBuffers (variation);
//		// please note that Serial mode has not been thoroughly tested (by me, MOB at least)!
//		auto bufSize = (connectType == ftdiConnectionOption::Serial ? DIO_BUFFERSIZESER : DIO_BUFFERSIZEASYNC);
//		buf.pts = std::vector<unsigned char> (bufSize * DIO_MSGLENGTH * DIO_WRITESPERDATAPT, 0);
//		bool proceed = true;
//		int count = 0;
//		unsigned int totalBytes = 0;
//		buf.bytesToWrite = 0;
//		unsigned int number = 0;
//		while ((number < bufSize) && proceed){
//			unsigned offset = DIO_WRITESPERDATAPT * number * DIO_MSGLENGTH;
//			fillFtdiDataBuffer (buf.pts, offset, count, snaps[count]);
//			if (snaps[count] == ftdiPt ({ 0,0,0,0,0,0,0,0,0 }) && number != 0){
//				proceed = false;//this is never false since we reach 43008 size?
//			}
//			if (count == NUMPOINTS){
//				thrower ("Non-Terminated table, data was filled all the way to end of data array... "
//					"Unit will not work right..., last element of data should be all zeros.");
//			}
//			number++;
//			count++;
//			buf.bytesToWrite += DIO_WRITESPERDATAPT * DIO_MSGLENGTH;
//		}
//	}
//}


DOStatus DoCore::getFinalSnapshot ()
{
	auto numVar = ttlSnapshots.size();
	if (numVar > 0){
		if (ttlSnapshots [numVar - 1].size () > 0){
			return ttlSnapshots [ numVar - 1 ].back ().ttlStatus;
		}
	}
	thrower ("Attempted to get final snapshot from dio system but no snapshots!");
}


//std::string DoCore::getDoSystemInfo (){
//	unsigned numDev;
//	std::string msg = "";
//	try{
//		numDev = ftFlume.getNumDevices ();
//		msg += "Number ft devices: " + str (numDev) + "\n";
//	}
//	catch (ChimeraError & err){
//		msg += "Failed to Get number ft Devices! Error was: " + err.trace ();
//	}
//	msg += ftFlume.getDeviceInfoList ();
//	return msg;
//}

/* mostly if not entirely used for setting dacs */
void DoCore::standardNonExperimentStartDoSequence (DoSnapshot initSnap){
	organizeTtlCommands (0, initSnap);
	std::vector<parameterType> variables = std::vector<parameterType>();
	findLoadSkipSnapshots (0, variables, 0);
	//convertToFtdiSnaps (0);
	//convertToFinalFtdiFormat (0);

}

void DoCore::initializeDataObjects (unsigned variationNum){
	ttlCommandFormList = std::vector<DoCommandForm>();

	doFPGA.clear();
	doFPGA.resize(variationNum);

	ttlCommandList.clear();
	ttlCommandList.resize(variationNum);

	ttlSnapshots.clear();
	ttlSnapshots.resize(variationNum);

	loadSkipTtlSnapshots.clear();
	loadSkipTtlSnapshots.resize(variationNum);

	formattedTtlSnapshots.clear();
	formattedTtlSnapshots.resize(variationNum);

	loadSkipFormattedTtlSnapshots.clear();
	loadSkipFormattedTtlSnapshots.resize(variationNum);
	
	finalFormatTtlData.clear();
	finalFormatTtlData.resize(variationNum);
	
	loadSkipFinalFormatTtlData.clear();
	loadSkipFinalFormatTtlData.resize(variationNum);


}


void DoCore::ttlOn (unsigned row, unsigned column, timeType time){
	ttlCommandFormList.push_back ({ {row, column}, time, {}, true });
}


void DoCore::ttlOff (unsigned row, unsigned column, timeType time){
	ttlCommandFormList.push_back ({ {row, column}, time, {}, false });
}


void DoCore::ttlOnDirect (unsigned row, unsigned column, double timev, unsigned variation){
	DoCommand command;
	command.line = { row, column };
	command.time = timev;
	command.value = true;
	ttlCommandList [variation].push_back (command);
}


void DoCore::ttlOffDirect (unsigned row, unsigned column, double timev, unsigned variation){
	DoCommand command;
	command.line = { row, column };
	command.time = timev;
	command.value = false;
	ttlCommandList [variation].push_back (command);
}

void DoCore::ttlPulseDirect(unsigned row, unsigned column, double timev, double dur, unsigned variation)
{
	if (dur < DIO_TIME_RESOLUTION) {
		thrower("The duration for ttl direct pulse: " + str(dur) + " is smaller than Zynq resolution 10ns! \r\n");
		return;
	}
	ttlOnDirect(row, column, timev, variation);
	ttlOffDirect(row, column, timev + dur, variation);
}


void DoCore::restructureCommands (){
	/* this is to be done after key interpretation. */
	if (ttlCommandFormList.size () == 0){
		thrower ("No TTL Commands???");
	}
	ttlCommandList.clear();
	ttlCommandList.resize (ttlCommandFormList[0].timeVals.size ());
	for (auto varInc : range (ttlCommandList.size ())){
		for (auto& cmd : ttlCommandFormList){
			DoCommand nCmd;
			nCmd.line = cmd.line;
			nCmd.time = cmd.timeVals[varInc];
			nCmd.value = cmd.value;
			ttlCommandList [varInc].push_back (nCmd);
		}
	}
}



void DoCore::sizeDataStructures (unsigned variations){
	/// imporantly, this sizes the relevant structures.
	doFPGA.clear();
	doFPGA.resize(variations);

	ttlCommandList.clear();
	ttlCommandList.resize(variations);

	ttlSnapshots.clear();
	ttlSnapshots.resize(variations);

	loadSkipTtlSnapshots.clear();
	loadSkipTtlSnapshots.resize(variations);

	formattedTtlSnapshots.clear();
	formattedTtlSnapshots.resize(variations);

	loadSkipFormattedTtlSnapshots.clear();
	loadSkipFormattedTtlSnapshots.resize(variations);

	finalFormatTtlData.clear();
	finalFormatTtlData.resize(variations);

	loadSkipFinalFormatTtlData.clear();
	loadSkipFinalFormatTtlData.resize(variations);

}


/*
 * Read key values from variables and convert command form to the final commands.
 */
void DoCore::calculateVariations (std::vector<parameterType>& params, ExpThreadWorker* threadworker){
	unsigned variations = params.size () == 0 ? 1 : params.front ().keyValues.size ();
	if (variations == 0){
		variations = 1;
	}
	sizeDataStructures (variations);
	// and interpret the command list for each variation.
	for (auto& dioCommandForm : ttlCommandFormList){
		dioCommandForm.timeVals.resize (variations);
	}
	for (auto variationNum : range (variations)){
		for (auto& dioCommandForm : ttlCommandFormList){
			double variableTime = 0;
			// add together current values for all variable times.
			if (dioCommandForm.time.first.size () != 0){
				for (auto varTime : dioCommandForm.time.first){
					variableTime += varTime.evaluate (params, variationNum);
				}
			}
			dioCommandForm.timeVals[variationNum] = variableTime + dioCommandForm.time.second;
		}
	}
	restructureCommands ();
}

std::vector<double> DoCore::getFinalTimes ()
{
	std::vector<double> finTimes;
	finTimes.resize (ttlSnapshots.size ());
	for (auto varNum : range (ttlSnapshots.size ())){
		finTimes[varNum] = ttlSnapshots [varNum].back ().time;
	}
	return finTimes;
}


std::vector<std::vector<plotDataVec>> DoCore::getPlotData (unsigned variation){
	unsigned linesPerPlot = size_t(DOGrid::total) / ExperimentSeqPlotter::NUM_TTL_PLTS;
	std::vector<std::vector<plotDataVec>> ttlData(ExperimentSeqPlotter::NUM_TTL_PLTS, 
		std::vector<plotDataVec>(linesPerPlot));
	if (ttlSnapshots.size () <= variation) {
		thrower ("Attempted to retrieve ttl data from variation " + str (variation) + ", which does not "
			"exist in the dio code object!");
	}
	// each element of ttlData should be one ttl line.
	for (auto line : range (size_t(DOGrid::total))) {
		auto& data = ttlData[line / linesPerPlot][line % linesPerPlot];
		data.clear ();
		unsigned frst = line / size_t(DOGrid::numPERunit);
		unsigned secd = line % size_t(DOGrid::numPERunit);
		for (auto snapn : range(ttlSnapshots [variation].size())){
			if (snapn != 0){
				data.push_back({ ttlSnapshots[variation][snapn].time,
								  double(ttlSnapshots[variation][snapn - 1].ttlStatus[frst][secd]), 0 });
			}
			data.push_back({ ttlSnapshots[variation][snapn].time,
							  double(ttlSnapshots[variation][snapn].ttlStatus[frst][secd]), 0 });
		}
	}
	return ttlData;
}

/// DIO64 Wrapper functions that I actually use
std::string DoCore::getTtlSequenceMessage (unsigned variation)
{
	std::string message;

	if (ttlSnapshots.size () <= variation)
	{
		thrower ("Attempted to retrieve ttl sequence message from snapshot " + str (variation) + ", which does not "
			"exist!");
	}
	for (auto snap : ttlSnapshots [variation])
	{
		message += str (snap.time) + ":\n";
		int rowInc = 0;
		for (auto row : snap.ttlStatus)
		{
			switch (rowInc)
			{
			case 0:
				message += "A: ";
				break;
			case 1:
				message += "B: ";
				break;
			case 2:
				message += "C: ";
				break;
			case 3:
				message += "D: ";
				break;
			}
			rowInc++;
			for (auto num : row)
			{
				message += str (num) + ", ";
			}
			message += "\r\n";
		}
		message += "\r\n---\r\n";
	}
	return message;
}

// counts the number of triggers on a given line.
// which.first = row, which.second = number.
unsigned DoCore::countTriggers (std::pair<unsigned, unsigned> which, unsigned variation)
{
	auto& snaps = ttlSnapshots [variation];
	unsigned count = 0;
	if (snaps.size () == 0)
	{
		return 0;
		//thrower ( "No ttl events to examine in countTriggers?" );
	}
	for (auto snapshotInc : range (ttlSnapshots [variation].size () - 1))
	{
		// count each rising edge. Also count if the first snapshot is high. 
		if ((snaps[snapshotInc].ttlStatus[which.first][which.second] == false
			&& snaps[snapshotInc + 1].ttlStatus[which.first][which.second] == true)
			|| (snaps[snapshotInc].ttlStatus[which.first][which.second] == true
				&& snapshotInc == 0))
		{
			count++;
		}
	}
	return count;
}

// check the time to see if the overall time is longer than one wrap of FPGA max time: 2**32*10ns ~ 42.9s
// if it does, will insert operation at time 0xFFFFFFFF to force the FPGA timer rewind
// this should be called before organizeTtlCommands and after calculateVariations/restructureCommands, where ttlCommandList is generated for all variation
void DoCore::checkLongTimeRun(unsigned variation)
{
	// make a copy of the commandList and then sort and will modify the original one if needed
	std::vector<DoCommand> orderedCommandList(ttlCommandList[variation]);
	std::sort(orderedCommandList.begin(), orderedCommandList.end(),
		[variation](DoCommand a, DoCommand b) {return a.time < b.time; });
	typedef unsigned long long l64;
	const unsigned int timeConv = 100000; // DIO time given in multiples of 10 ns
	const l64 rewindTime = l64(1) << 32; // 0xFFFFFFFF + 1, correspond to 32 bit time 
	int durCounter = l64(orderedCommandList[0].time * timeConv) / rewindTime;
	if (durCounter > 0) {// the first time stamp is larger than a rewind
		for (auto idx : range(durCounter)) {// if counter=1, range gives {0}
			ttlPulseDirect(DIO_REWIND.first, DIO_REWIND.second, double((durCounter + 1) * rewindTime - 1) / timeConv, 2.0 / timeConv, variation);
		}
	}
	for (auto& cmd : orderedCommandList) {
		if (l64(cmd.time * timeConv) / rewindTime > durCounter) {
			durCounter++;
			ttlPulseDirect(DIO_REWIND.first, DIO_REWIND.second, double(durCounter * rewindTime - 1) / timeConv, 2.0 / timeConv, variation);
		}
	}

}



void DoCore::FPGAForceOutput(DOStatus status)
{

	resetTtlEvents();
	sizeDataStructures(1);
	ttlSnapshots[0].push_back({ 1, status });
	formatForFPGA(0);
	writeTtlDataToFPGA(0, false);

	int tcp_connect;
	try
	{
		tcp_connect = zynq_tcp.connectTCP(ZYNQ_ADDRESS);
	}
	catch (ChimeraError& err)
	{
		tcp_connect = 1;
		thrower(err.what());
	}

	if (tcp_connect == 0)
	{
		zynq_tcp.writeCommand("trigger");
		Sleep(100);
		zynq_tcp.writeCommand("disableMod");
		zynq_tcp.disconnect();
	}
	else
	{
		thrower("connection to zynq failed. can't write TTL data\n");
	}

}

void DoCore::FPGAForcePulse(DOStatus status, std::vector<std::pair<unsigned, unsigned>> rowcol, double dur)
{
	resetTtlEvents();
	sizeDataStructures(2);
	for (auto& rc : rowcol)
	{
		status[rc.first][rc.second] = !status[rc.first][rc.second];
		ttlSnapshots[0].push_back({ 0.1, status });
		status[rc.first][rc.second] = !status[rc.first][rc.second];
	}
	for (size_t i = 0; i < rowcol.size(); i++)
	{

	}
	ttlSnapshots[0].push_back({ 0.1 + dur, status });
	formatForFPGA(0);
	writeTtlDataToFPGA(0, false);

	int tcp_connect;
	try
	{
		tcp_connect = zynq_tcp.connectTCP(ZYNQ_ADDRESS);
	}
	catch (ChimeraError& err)
	{
		tcp_connect = 1;
		thrower(err.what());
	}

	if (tcp_connect == 0)
	{
		zynq_tcp.writeCommand("trigger");
		zynq_tcp.writeCommand("disableMod");
		zynq_tcp.disconnect();
	}
	else
	{
		thrower("connection to zynq failed. can't write DAC data\n");
	}

}

void DoCore::formatForFPGA(UINT variation)
{
	typedef unsigned long long l64;
	int snapIndex = 0;
	const l64 timeConv = 100000; // DIO time given in multiples of 10 ns
	std::array<char[DIO_LEN_BYTE_BUF], 1> byte_buf;
	std::array<bool, size_t(DOGrid::numPERunit)> bankA;
	std::array<bool, size_t(DOGrid::numPERunit)> bankB;
	//char byte_buf[DIO_LEN_BYTE_BUF];
	unsigned int time;
	unsigned int outputA;
	unsigned int outputB;
	//int outputAtest;
	//int outputBtest;

	const l64 rewindTime = l64(1) << 32; // 0xFFFFFFFF + 1, correspond to 32 bit time 
	int durCounter = l64(std::llround(ttlSnapshots[variation][0].time * timeConv)) / rewindTime;
	if (durCounter > 0) {// the first time stamp is larger than a rewind, shouldn't happen after organizeTTL, otherwise it is impossible to know the ttl state before exp
		thrower("The TTL didn't start at ZYNQ_DEADTIME, which is " + str(ZYNQ_DEADTIME) + ". Something low level wrong.");
	}
	for (auto snapshot : ttlSnapshots[variation])
	{
		if (l64(std::llround(snapshot.time * timeConv)) / rewindTime > durCounter) {
			durCounter++;
			unsigned int windTime = (l64(durCounter) * rewindTime - 1) & l64(0xffffffff);
			sprintf_s(byte_buf[0], DIO_LEN_BYTE_BUF, "t%08X_b%08X%08X", windTime, outputB, outputA); // use the output from previous loop
			doFPGA[variation].push_back(byte_buf);
			snapIndex++;
		}
		time = l64(std::llround(snapshot.time * timeConv)) & l64(0xffffffff);
		//for each DIO bank convert the boolean array to a byte
		outputA = 0;
		outputB = 0;
		//outputAtest = 0;
		//outputBtest = 0;
		for (unsigned i = 0; i < 4; i++)
		{
			bankA = snapshot.ttlStatus[i]; //bank here is set of 8 booleans
			bankB = snapshot.ttlStatus[i + 4]; //bank here is set of 8 booleans
			for (int j = 0; j < 8; j++)
			{
				outputA |= (unsigned int)(bankA[j]) << 8 * i << j;
				outputB |= (unsigned int)(bankB[j]) << 8 * i << j;

				//outputAtest += pow(256, i) * pow(2, j) * bankA[j];
				//outputBtest += pow(256, i) * pow(2, j) * bankB[j];
				//if (outputA != outputAtest || outputB != outputBtest)
				//	qDebug() << "not equal!!!!";
			}
		}


		
		sprintf_s(byte_buf[0], DIO_LEN_BYTE_BUF, "t%08X_b%08X%08X", time, outputB, outputA);
		doFPGA[variation].push_back(byte_buf);
		snapIndex++;
	}

}

void DoCore::writeTtlDataToFPGA(UINT variation, bool loadSkip) //arguments unused, just paralleling original DIO structure
{

	//dioFPGA[variation].write();
	int tcp_connect;
	try
	{
		tcp_connect = zynq_tcp.connectTCP(ZYNQ_ADDRESS);
	}
	catch (ChimeraError& err)
	{
		tcp_connect = 1;
		thrower(err.what());
	}

	if (tcp_connect == 0)
	{
		zynq_tcp.writeDIO(doFPGA[variation]);
		zynq_tcp.disconnect();
	}
	else
	{
		thrower("connection to zynq failed. can't write Ttl data\n");
	}


}


void DoCore::findLoadSkipSnapshots (double time, std::vector<parameterType>& variables, unsigned variation)
{
	// find the splitting time and set the loadSkip snapshots to have everything after that time.
	auto& snaps = ttlSnapshots [variation];
	auto& loadSkipSnaps = loadSkipTtlSnapshots [variation];
	for (auto snapshotInc : range (ttlSnapshots [variation].size () - 1))
	{
		if (snaps[snapshotInc].time < time && snaps[snapshotInc + 1].time >= time)
		{
			loadSkipSnaps = std::vector<DoSnapshot> (snaps.begin () + snapshotInc + 1, snaps.end ());
			break;
		}
	}
	// need to zero the times.
	for (auto& snapshot : loadSkipSnaps)
	{
		snapshot.time -= time;
	}
}


std::vector<std::vector<DoSnapshot>> DoCore::getTtlSnapshots ()
{
	/* used in the unit testing suite */
	return ttlSnapshots;
}



void DoCore::organizeTtlCommands (unsigned variation, DoSnapshot initSnap)
{
	// each element of this is a different time (the double), and associated with each time is a vector which locates 
	// which commands were on at this time, for ease of retrieving all of the values in a moment.
	std::vector<std::pair<double, std::vector<unsigned short>>> timeOrganizer;
	std::vector<DoCommand> orderedCommandList (ttlCommandList [variation]);
	// sort using a lambda. std::sort is effectively a quicksort algorithm.
	std::sort (orderedCommandList.begin (), orderedCommandList.end (),
		[variation](DoCommand a, DoCommand b) {return a.time < b.time; });
	/// organize all of the commands.
	for (auto commandInc : range (ttlCommandList [variation].size ()))
	{
		// because the events are sorted by time, the time organizer will already be sorted by time, and therefore I 
		// just need to check the back value's time. DIO64 uses a 10MHz clock, can do 100ns spacing, check diff 
		// threshold to extra room. If dt<1ns, probably just some floating point issue. 
		// If 1ns<dt<100ns(Zynq is 10ns) I want to actually complain to the user since it seems likely that  this was intentional and 
		// not a floating error.
		if (commandInc == 0 || fabs (orderedCommandList[commandInc].time - timeOrganizer.back ().first) > DIO_TIME_RESOLUTION)
		{
			// new time
			std::vector<unsigned short> testVec = { unsigned short (commandInc) };
			timeOrganizer.push_back ({ orderedCommandList[commandInc].time, testVec });
		}
		else
		{
			// old time
			timeOrganizer.back ().second.push_back (commandInc);
			if (commandInc != 0) {
				//then it must be that the time interval is <10ns, complain it
				//thrower("The time spacing between two ttl change is" + str(fabs(orderedCommandList[commandInc].time - timeOrganizer.back().first)) +
				//	"ms, which is smaller than 10ns. Aborted \r\n");
			}
		}
	}
	if (timeOrganizer.size () == 0)
	{
		thrower ("No ttl commands! The Ttl system is the master behind everything in a repetition, and so it "
			"must contain something.\r\n");
	}
	/// now figure out the state of the system at each time.
	auto& snaps = ttlSnapshots [variation];
	snaps.clear ();
	// start with the initial status.
	snaps.push_back (initSnap);
	///
	if (timeOrganizer[0].first != 0)
	{
		// then there were no commands at time 0, so just set the initial state to be exactly the original state before
		// the experiment started. I don't need to modify the first snapshot in this case, it's already set. Add a snapshot
		// here so that the thing modified is the second snapshot not the first. 
		snaps.push_back (initSnap);
	}

	// handle the zero case specially. This may or may not be the literal first snapshot.
	snaps.back ().time = timeOrganizer[0].first;
	for (auto zeroInc : range (timeOrganizer[0].second.size ()))
	{
		// make sure to address the correct ttl. the ttl location is located in individuaTTL_CommandList but you need 
		// to make sure you access the correct command.
		unsigned cmdNum = timeOrganizer[0].second[zeroInc];
		unsigned row = orderedCommandList[cmdNum].line.first;
		unsigned column = orderedCommandList[cmdNum].line.second;
		snaps.back ().ttlStatus[row][column] = orderedCommandList[cmdNum].value;
	}
	///
	// already handled the first case.
	for (unsigned commandInc = 1; commandInc < timeOrganizer.size (); commandInc++)
	{
		// first copy the last set so that things that weren't changed remain unchanged.
		snaps.push_back (snaps.back ());
		snaps.back ().time = timeOrganizer[commandInc].first;
		for (auto cmdIndex : timeOrganizer[commandInc].second)
		{
			// see description of this command above... update everything that changed at this time.
			unsigned row = orderedCommandList[cmdIndex].line.first;
			unsigned column = orderedCommandList[cmdIndex].line.second;
			snaps.back ().ttlStatus[row][column] = orderedCommandList[cmdIndex].value;
		}
	}
	// phew. Check for good input by user:
	for (auto& snapshot : snaps)
	{
		if (snapshot.time < 0)
		{
			thrower ("The code tried to set a ttl event at a negative time value! This is clearly not allowed."
				" Aborting.");
		}
	}
	/* adding 10ns to the starting time would solve the problem of the initial state of ttl*/
	//for (int i = 0; i < snaps.size (); i++) 
	//{
	//	snaps[i].time = snaps[i].time + 1e-5;
	//}
}



unsigned long DoCore::getNumberEvents (unsigned variation)
{
	return ttlSnapshots [variation].size ();
}

std::array<std::string, size_t(DOGrid::total)> DoCore::getAllNames () { return names; }
void DoCore::resetTtlEvents () { initializeDataObjects (0); }
void DoCore::wait2 (double time) { Sleep (time + 10); }
void DoCore::prepareForce () { initializeDataObjects (1); }


bool DoCore::isValidTTLName (std::string name){
	unsigned row;
	unsigned number;
	return getNameIdentifier (name, row, number) != -1;
}

/*
Returns a single number which corresponds to the dio control with the name
*/
int DoCore::getNameIdentifier (std::string name, unsigned& row, unsigned& number)
{
	for (UINT doInc = 0; doInc < size_t(DOGrid::total); doInc++)
	{
		// check names set by user.

		//std::string DioName = str(names, 13, false, true); /*4th arg is toLower*/ /*this is not supported by str, do NOT use macro!!!Hard to debug!!!*/
		if (name == names[doInc] || name == "do" +
			str(doInc / size_t(DOGrid::numPERunit)) + "_" +
			str(doInc % size_t(DOGrid::numPERunit)) ) // check standard names which are always acceptable.
		{
			row = doInc / size_t(DOGrid::numPERunit);
			number = doInc % size_t(DOGrid::numPERunit);
			return doInc;
		}
	}
	// not an identifier.
	return -1;

}

void DoCore::handleTtlScriptCommand (std::string command, timeType time, std::string name, Expression pulseLength,
									 std::vector<parameterType>& vars, std::string scope){
	if (!isValidTTLName (name)){
		thrower ("the name " + name + " is not the name of a ttl!");
	}
	timeType pulseEndTime = time;
	unsigned collumn;
	unsigned row;
	getNameIdentifier (name, row, collumn);
	if (command == "on:"){
		ttlOn (int (row), collumn, time);
	}
	else if (command == "off:"){
		ttlOff (int (row), collumn, time);
	}
	else if (command == "pulseon:" || command == "pulseoff:"){
		try	{
			pulseEndTime.second += pulseLength.evaluate ();
		}
		catch (ChimeraError&){
			pulseLength.assertValid (vars, scope);
			pulseEndTime.first.push_back (pulseLength);
		}
		if (command == "pulseon:"){
			ttlOn (int (row), collumn, time);
			ttlOff (int (row), collumn, pulseEndTime);
		}
		if (command == "pulseoff:"){
			ttlOff (int (row), collumn, time);
			ttlOn (int (row), collumn, pulseEndTime);
		}
	}
}

void DoCore::handleTtlScriptCommand (std::string command, timeType time, std::string name, 
	std::vector<parameterType>& vars, std::string scope){
	// use an empty expression.
	handleTtlScriptCommand (command, time, name, Expression (), vars, scope);
}

//void DoCore::standardExperimentPrep (unsigned variationInc, double currLoadSkipTime, std::vector<parameterType>& expParams){
//	organizeTtlCommands (variationInc);
//	findLoadSkipSnapshots (currLoadSkipTime, expParams, variationInc);
//	//convertToFtdiSnaps (variationInc);
//	//convertToFinalFtdiFormat (variationInc);
//	convertToFinalFormat(variationInc);/*seems useless*/
//	formatForFPGA(variationInc);
//}
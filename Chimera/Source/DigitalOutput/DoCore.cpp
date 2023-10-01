#include "stdafx.h"
#include "DoCore.h"
#include "DoStructures.h"
#include "qdebug.h"
#include <ExperimentMonitoringAndStatus/ExperimentSeqPlotter.h>
#include <ExperimentThread/ExpThreadWorker.h>

#include <bitset>
#include <iterator>
#include <algorithm>

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

void DoCore::ttlOn (unsigned row, unsigned column, timeType time, repeatInfoId repeatId){
	ttlCommandFormList.push_back ({ {row, column}, time, {}, true, repeatId });
}


void DoCore::ttlOff (unsigned row, unsigned column, timeType time, repeatInfoId repeatId){
	ttlCommandFormList.push_back ({ {row, column}, time, {}, false, repeatId });
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
	ttlCommandList.resize (ttlCommandFormList[0].timeVals.size ()); // resize to variation number
	for (auto varInc : range (ttlCommandList.size ())){
		for (auto& cmd : ttlCommandFormList){
			DoCommand nCmd;
			nCmd.line = cmd.line;
			nCmd.time = cmd.timeVals[varInc];
			nCmd.value = cmd.value;
			nCmd.repeatId = cmd.repeatId;
			ttlCommandList [varInc].push_back (nCmd);
		}
	}
}

void DoCore::prepareForce()
{
	// purposefully preserve ttlCommandFormList, for inExpCal
	sizeDataStructures(1);
}

void DoCore::sizeDataStructures (unsigned variations){
	/// imporantly, this sizes the relevant structures.
	doFPGA.clear();
	doFPGA.resize(variations);

	ttlCommandList.clear();
	ttlCommandList.resize(variations);

	ttlSnapshots.clear();
	ttlSnapshots.resize(variations);

}

void DoCore::initializeDataObjects(unsigned variationNum) {
	ttlCommandFormList = std::vector<DoCommandForm>(variationNum);
	sizeDataStructures(variationNum);
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

void DoCore::constructRepeats(repeatManager& repeatMgr)
{
	typedef DoCommand Command;
	/* this is to be done after ttlCommandList is filled with all variations. */
	if (ttlCommandFormList.size() == 0 || ttlCommandList.size() == 0) {
		thrower("No TTL Commands???");
	}

	unsigned variations = ttlCommandList.size();
	repeatMgr.saveCalculationResults(); // repeatAddedTime is changed during construction, need to save and reset it before and after the construction body
	auto* repearRoot = repeatMgr.getRepeatRoot();
	auto allDescendant = repearRoot->getAllDescendant();
	if (allDescendant.empty()) {
		return; // no repeats need to handle.
	}


	// iterate through all variations
	for (auto varInc : range(variations)) {
		auto& cmds = ttlCommandList[varInc];

		// Recursively add these repeat for always starting with maxDepth repeat. And also update the already constructed one to its parent layer
		// The loop will end when all commands is not associated with repeat, i.e. the maxDepth command's repeatId.repeatTreeMap is root
		while (true) {
			/*find the max depth repeated command*/
			auto maxDepthIter = std::max_element(cmds.begin(), cmds.end(), [&](const Command& a, const Command& b) {
				return (a.repeatId.repeatTreeMap.first < b.repeatId.repeatTreeMap.first); });
			Command maxDepth = *maxDepthIter;

			/*check if all command is with zero repeat. If so, exit the loop*/
			if (maxDepth.repeatId.repeatTreeMap == repeatInfoId::root) {
				break;
			}

			/*find the repeat num and the repeat added time with the unique identifier*/
			auto repeatIIter = std::find_if(allDescendant.begin(), allDescendant.end(), [&](TreeItem<repeatInfo>* a) {
				return (maxDepth.repeatId.repeatIdentifier == a->data().identifier); });
			if (repeatIIter == allDescendant.end()) {
				thrower("Can not find the ID for the repeat in the DoCommand with max depth of the tree. This is a low level bug.");
			}
			TreeItem<repeatInfo>* repeatI = *repeatIIter;
			unsigned repeatNum = repeatI->data().repeatNums[varInc];
			double repeatAddedTime = repeatI->data().repeatAddedTimes[varInc];
			/*find the parent of this repeat and record its repeatInfoId for updating the repeated ones*/
			TreeItem<repeatInfo>* repeatIParent = repeatI->parentItem();
			repeatInfoId repeatIdParent{ repeatIParent->data().identifier, repeatIParent->itemID() };
			/*collect command that need to be repeated*/
			std::vector<Command> cmdToRepeat;
			std::copy_if(cmds.begin(), cmds.end(), std::back_inserter(cmdToRepeat), [&](Command doc) {
				return (doc.repeatId.repeatIdentifier == maxDepth.repeatId.repeatIdentifier); });
			/*check if the repeated command is continuous in the cmds vector, it should be as the cmds is representing the script's order at this stage*/
			auto cmdToRepeatStart = std::search(cmds.begin(), cmds.end(), cmdToRepeat.begin(), cmdToRepeat.end(),
				[&](const Command& a, const Command& b) {
					return (a.repeatId.repeatIdentifier == b.repeatId.repeatIdentifier);
				});
			if (cmdToRepeatStart == cmds.end()) {
				thrower("The repeated command is not contiguous inside the CommandList, which is not suppose to happen.");
			}
			int cmdToRepeatStartPos = std::distance(cmds.begin(), cmdToRepeatStart);
			auto cmdToRepeatEnd = cmdToRepeatStart + cmdToRepeat.size(); // this will point to first cmd that is after those repeated one in CommandList
			/*if repeatNum is zero, delete the repeated command and reduce the time for those comand comes after the repeated commands and that is all*/
			if (repeatNum == 0) {
				cmds.erase(std::remove_if(cmds.begin(), cmds.end(), [&](Command doc) {
					return (doc.repeatId.repeatIdentifier == maxDepth.repeatId.repeatIdentifier); }), cmds.end());
				/*de-advance the time of thoses command that is later in CommandList than the repeat block*/
				cmdToRepeatEnd = cmds.begin() + cmdToRepeatStartPos; // this will point to first cmd that is after those repeated one in CommandList, since the repeated is removed, should equal to cmdToRepeatStart
				std::for_each(cmdToRepeatEnd, cmds.end(), [&](Command& doc) {
					doc.time -= repeatAddedTime; });
				continue;
			}
			/*transform the repeating commandlist to its parent repeatInfoId so that it can be repeated in its parents level*/
			// could also use this: std::transform(cmds.cbegin(), cmds.cend(), cmds.begin(), [&](Command doc) { with a return
			std::for_each(cmds.begin(), cmds.end(), [&](Command& doc) {
				if (doc.repeatId.repeatIdentifier == maxDepth.repeatId.repeatIdentifier) {
					doc.repeatId = repeatIdParent;
				} });
			/*start to insert the repeated 'cmdToRpeat' to end of the repeat block, after insertion, 'cmdToRepeatEnd' can not be used*/
			std::vector<Command> cmdToInsert;
			cmdToInsert.clear();
			for (unsigned repeatInc : range(repeatNum - 1)) {
				// if only repeat for once, below will be ignored, since the first repeat is already in the list
				/*transform the repeating commandlist to its parent repeatInfoId and also increment its time so that it can be repeated in its parents level*/
				std::for_each(cmdToRepeat.begin(), cmdToRepeat.end(), [&](Command& doc) {
					doc.repeatId = repeatIdParent;
					doc.time += repeatAddedTime; });
				cmdToInsert.insert(cmdToInsert.end(), cmdToRepeat.begin(), cmdToRepeat.end());
			}
			cmds.insert(cmdToRepeatEnd, cmdToInsert.begin(), cmdToInsert.end());
			/*advance the time of thoses command that is later in CommandList than the repeat block*/
			cmdToRepeatEnd = cmds.begin() + cmdToRepeatStartPos + cmdToRepeat.size() + cmdToInsert.size();
			std::for_each(cmdToRepeatEnd, cmds.end(), [&](Command& doc) {
				doc.time += repeatAddedTime * (repeatNum - 1); });
			/*advance the time of the parent repeat, if the parent is not root*/
			if (repeatIParent != repearRoot) {
				repeatIParent->data().repeatAddedTimes[varInc] += repeatAddedTime * repeatNum;
			}
		}
	}
	repeatMgr.loadCalculationResults();
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
	prepareForce();
	ttlSnapshots[0].push_back({ 0.1, status });
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
		Sleep(1);
		zynq_tcp.writeCommand("trigger");
		zynq_tcp.disconnect();
	}
	else
	{
		thrower("connection to zynq failed. can't write TTL data\n");
	}

}

void DoCore::FPGAForcePulse(DOStatus status, std::vector<std::pair<unsigned, unsigned>> rowcol, double dur)
{
	prepareForce();
	ttlSnapshots[0].push_back({ 0.1, status });
	for (auto& rc : rowcol)
	{
		status[rc.first][rc.second] = !status[rc.first][rc.second];		
	}
	ttlSnapshots[0].push_back({ 0.1 + dur, status });
	for (auto& rc : rowcol)
	{
		status[rc.first][rc.second] = !status[rc.first][rc.second];
	}
	ttlSnapshots[0].push_back({ 0.1 + dur + dur, status });
	formatForFPGA(0);
	writeTtlDataToFPGA(0, false);

	int tcp_connect;
	try {
		tcp_connect = zynq_tcp.connectTCP(ZYNQ_ADDRESS);
	}
	catch (ChimeraError& err) {
		tcp_connect = 1;
		thrower(err.what());
	}
	Sleep(15); // somehow has to wait 15ms, have to sleep for this amount of time to make TCP connect smoothly??????,  same for ExpThreadWorker::startRep zzp 2022/06/10 very annoying
	if (tcp_connect == 0) {
		zynq_tcp.writeCommand("trigger");
		zynq_tcp.disconnect();
	}
	else {
		thrower("connection to zynq failed. can't write TTL data\n");
	}


	// set up a sequence that will just flush out the current static output so that when dac gui get updated, 
	// the sequencer does not run the triggering sequence but this static sequence to avoid unexpected triggering 
	// for example, the gigamood will froze if receive a trigger but not a data beforehand
	Sleep(0.1 + dur + dur);
	FPGAForceOutput(status);

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
	//auto& snaps = ttlSnapshots [variation];
	//auto& loadSkipSnaps = loadSkipTtlSnapshots [variation];
	//for (auto snapshotInc : range (ttlSnapshots [variation].size () - 1))
	//{
	//	if (snaps[snapshotInc].time < time && snaps[snapshotInc + 1].time >= time)
	//	{
	//		loadSkipSnaps = std::vector<DoSnapshot> (snaps.begin () + snapshotInc + 1, snaps.end ());
	//		break;
	//	}
	//}
	//// need to zero the times.
	//for (auto& snapshot : loadSkipSnaps)
	//{
	//	snapshot.time -= time;
	//}
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

std::vector<DoCommand> DoCore::getTtlCommand(unsigned variation)
{
	if (ttlCommandList.size() - 1 < variation) {
		thrower("Aquiring TTL command out of range");
	}
	return ttlCommandList[variation];
}

std::array<std::string, size_t(DOGrid::total)> DoCore::getAllNames () 
{ 
	return names; 
}

void DoCore::resetTtlEvents () 
{ 
	initializeDataObjects (0); 
}

void DoCore::wait2 (double time) { Sleep (time + 10); }

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

void DoCore::handleTtlScriptCommand(std::string command, timeType time, std::string name, Expression pulseLength,
	std::vector<parameterType>& vars, std::string scope, repeatInfoId repeatId) {
	if (!isValidTTLName (name)){
		thrower ("the name " + name + " is not the name of a ttl!");
	}
	timeType pulseEndTime = time;
	unsigned collumn;
	unsigned row;
	getNameIdentifier (name, row, collumn);
	if (command == "on:"){
		ttlOn (int (row), collumn, time, repeatId);
	}
	else if (command == "off:"){
		ttlOff (int (row), collumn, time, repeatId);
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
			ttlOn (int (row), collumn, time, repeatId);
			ttlOff (int (row), collumn, pulseEndTime, repeatId);
		}
		if (command == "pulseoff:"){
			ttlOff (int (row), collumn, time, repeatId);
			ttlOn (int (row), collumn, pulseEndTime, repeatId);
		}
	}
};

void DoCore::handleTtlScriptCommand (std::string command, timeType time, std::string name, 
	std::vector<parameterType>& vars, std::string scope, repeatInfoId repeatId){
	// use an empty expression.
	handleTtlScriptCommand (command, time, name, Expression (), vars, scope, repeatId);
}

//void DoCore::standardExperimentPrep (unsigned variationInc, double currLoadSkipTime, std::vector<parameterType>& expParams){
//	organizeTtlCommands (variationInc);
//	findLoadSkipSnapshots (currLoadSkipTime, expParams, variationInc);
//	//convertToFtdiSnaps (variationInc);
//	//convertToFinalFtdiFormat (variationInc);
//	convertToFinalFormat(variationInc);/*seems useless*/
//	formatForFPGA(variationInc);
//}
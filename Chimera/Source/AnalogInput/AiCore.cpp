#include "stdafx.h"
#include "AiCore.h"
#include <Windows.h>
#include <sstream>
#include <qelapsedtimer.h>
#include <qdebug.h>

AiCore::AiCore() 
	: socket(AI_SAFEMODE, AI_SOCKET_ADDRESS, AI_SOCKET_PORT)
{
	//socket.write("mac", terminator);
	//QByteArray tmp = socket.read();
}

void AiCore::getSettingsFromConfig(ConfigStream& file)
{
	std::string tmp;
	for (auto& av : aiValues) {
		file >> tmp;
		av.status.range = av.status.fromStr(tmp);
	}
}

void AiCore::saveSettingsToConfig(ConfigStream& file)
{
	file << "\n/*Channel Ranges*/ ";
	for (auto& av : aiValues) {
		file << AiChannelRange::toStr(av.status.range) << " ";
	}
}

void AiCore::updateChannelRange()
{
	/*A3-A0,A7-A4,B3-B0,B7-B4*/
	unsigned char buff[5] = { 0,0,0,0,0 }; // the last 0 is important to terminate the char*, otherwise it is undefined when append to string, ie might contain junking trailing behind
	for (size_t i = 0; i < 4/*range register*/; i++)
	{
		for (size_t j = 0; j < 4/*2 bit code*/; j++)
		{
			buff[i] += aiValues[i * 4 + j].status.codes() << (2 * j);
		}
	}
	try {
		QElapsedTimer timerE;
		timerE.start();
		socket.open();
		qDebug() << "After open the port, time at" << timerE.elapsed() << "ms";
		socket.write(QByteArray::fromStdString(std::string("(rng,") + (char*)buff + ")"), terminator);
		qDebug() << "After write the port, time at" << timerE.elapsed() << "ms";
		std::string rc = socket.read();
		qDebug() << "After read the port, time at" << timerE.elapsed() << "ms";
		socket.close();
		if (str(rc, 13, false, true).find("Error") != std::string::npos) {
			thrower("Error in updating range in Analoge in. \r\n" + rc);
		}
	}
	catch (ChimeraError& e) {
		throwNested("Error seen in updateChannelRange function in AnalogIn:");
	}
}

void AiCore::getSingleSnap(unsigned n_to_avg)
{

	std::vector<unsigned> onChannel;
	char buff[3] = { 0,0,0 };
	for (unsigned i = 0; i < size_t(AIGrid::total); i++)
	{
		if (aiValues[i].status.range != AiChannelRange::which::off) {
			onChannel.push_back(i);
			buff[i / size_t(AIGrid::numPERunit)] |= 1 << (i % size_t(AIGrid::numPERunit));
		}
	}
	if (buff[0] == 0 && buff[1] == 0) {
		thrower("Error: Analog in - No channel is on for acquisition");
		return; // no channel to be sampled
	}

	// make sure when all channels in one bank (A or B) are off, the cmd is not terminated by that 0
	std::array<bool, 2> epty = { false,false };
	for (short i = 0; i < 2; i++) { if (buff[i] == 0) { buff[i] = 1; epty[i] = true; } }
	unsigned n_chnl = onChannel.size() + (epty[0] || epty[1]);
	if (epty[0] || epty[1]) { epty[0] ? onChannel.insert(onChannel.begin(), 0) : onChannel.push_back(size_t(AIGrid::numPERunit) - 1); }

	//socket.resetConnection();
	QElapsedTimer timerE;
	timerE.start();
	socket.open();
	qDebug() << "After open the port, time at" << timerE.elapsed() << "ms";
	socket.write(QByteArray("(") + placeholder + QByteArray(buff, 2) + QByteArray(",")
		+ QByteArray::fromStdString(str(n_to_avg)) + QByteArray(")"), terminator);
	qDebug() << "After write the port, time at" << timerE.elapsed() << "ms";
	Sleep(5);
	socket.resetConnection();
	qDebug() << "After Sleep for 5ms and reset connection, time at" << timerE.elapsed() << "ms";
	socket.write("(trg, )", terminator);
	qDebug() << "After write the port, time at" << timerE.elapsed() << "ms";
	QByteArray rc;
	Sleep(20);
	try {
		//rc = socket.readAll();
		//rc = socket.readAll();
		//rc = std::move(socket.readRaw());
		rc = std::move(socket.readTillFull(n_chnl * n_to_avg * 2));
		qDebug() << "After Sleep for 20ms and read all data, time at" << timerE.elapsed() << "ms";
		socket.close();
	}
	catch (ChimeraError& e) {
		throwNested("Error seen in getting single snap data in AnalogIn:");
	}
	// see if the return start with error
	if (std::string(rc.left(10)).find("Error") != std::string::npos) {
		thrower(str("Error exception thrown while getting Ai system single snap! \r\n") + rc.data());
		return;
	}



	//std::vector<double> datatmp;
	//std::stringstream ss(rc.data());
	//while (ss.good())
	//{
	//	std::string substr;
	//	std::getline(ss, substr, ',');
	//	if (substr.size() == 0) {
	//		thrower("Analogin Error: Empty string encountered in converting to int");
	//		return;
	//	}
	//	datatmp.push_back(std::stoi(substr));

	//}
	//if (datatmp.size() != n_chnl * n_to_avg) {
	//	thrower("Error in Analog in: the data size is not as expected, data might correupted.\r\n");
	//	return;
	//}
	//std::vector<std::vector<double>> data(n_chnl);
	//for (unsigned chanCnts = 0; chanCnts < n_chnl; chanCnts++)
	//{
	//	data[chanCnts].resize(n_to_avg);
	//	for (unsigned i = 0; i < datatmp.size() / n_chnl; i++)
	//	{
	//		data[chanCnts][i] = datatmp[(i * n_chnl + chanCnts)];
	//	}
	//}



	if (rc.size() != n_chnl * n_to_avg * 2/*byte*/) {
		thrower("Error in Analog in: the data size is " + str(rc.size()) + " and is not as expected to be"
			+ str(n_chnl * n_to_avg * 2) + ", data might correupted.\r\n");
		return;
	}
	std::vector<std::vector<double>> data(n_chnl);
	for (unsigned chanCnts = 0; chanCnts < n_chnl; chanCnts++)
	{
		data[chanCnts].resize(n_to_avg);
		for (unsigned i = 0; i < rc.size() / n_chnl / 2/*byte*/; i++)
		{
			unsigned short high = (unsigned short(rc[(i * n_chnl + chanCnts) * 2]) << 8) & 0xff00;
			unsigned short low = unsigned short(rc[(i * n_chnl + chanCnts) * 2 + 1]) & 0x00ff;
			data[chanCnts][i] = signed short(high | low);
		}
	}

	for (unsigned ch = 0; ch < n_chnl; ch++)
	{
		double sum = std::accumulate(data[ch].begin(), data[ch].end(), 0.0);
		double m = sum / n_to_avg;

		double accum = 0.0;
		std::for_each(data[ch].begin(), data[ch].end(), [&](const double d) {
			accum += (d - m) * (d - m); });

		double stdev = sqrt(accum / (n_to_avg - 1));
		if (aiValues[onChannel[ch]].status.range != AiChannelRange::which::off) {
			aiValues[onChannel[ch]].mean = m * aiValues[onChannel[ch]].status.scales() / 0x7fff;
			aiValues[onChannel[ch]].std = stdev * aiValues[onChannel[ch]].status.scales() / 0x7fff;
		}
	}


	//auto tmpD = reinterpret_cast<short*>(rc.data());
	//std::vector<double> tmpdata  = std::vector<double>(tmpD, tmpD + rc.size() / 2);

}

std::array<double, size_t(AIGrid::total)> AiCore::getCurrentValues()
{
	std::array<double, size_t(AIGrid::total)> vals;
	for (size_t i = 0; i < size_t(AIGrid::total); i++)
	{
		vals[i] = aiValues[i].mean;
	}
	return vals;
}

void AiCore::setAiRange(unsigned channel, AiChannelRange::which range)
{
	aiValues[channel].status.range = range;
}

void AiCore::setName(int aiNumber, std::string name)
{
	if (name == "") {
		// no empty names allowed.
		return;
	}
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	aiValues[aiNumber].name = name;
}

void AiCore::setNote(int aiNumber, std::string note)
{
	aiValues[aiNumber].note = note;
}

int AiCore::getAiIdentifier(std::string name)
{
	std::array<std::string, 2> chnlStr = { "a","b" };
	for (auto adcInc : range(size_t(AIGrid::total)))
	{
		auto dacName = str(aiValues[adcInc].name, 13, false, true);
		// check names set by user and check standard names which are always acceptable
		if (name == dacName || name == "adc_" + str(chnlStr[adcInc / size_t(AIGrid::numPERunit)]) 
			+ str(adcInc % size_t(AIGrid::numPERunit)) ) {
			return adcInc;
		}
	}
	// not an identifier.
	return -1;
}
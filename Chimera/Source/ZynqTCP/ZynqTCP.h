#pragma once

//#include <stdio.h> 
//#include <stdlib.h> 
#include <string.h> 
#include <winsock2.h>
#include <Ws2tcpip.h>
#include "AnalogOutput/AoStructures.h"
#include "DigitalOutput/DoStructures.h"
#include "DirectDigitalSynthesis/DdsSystemStructures.h"


class ZynqTCP 
{

public:
	ZynqTCP();
	~ZynqTCP();
	void disconnect();
	int connectTCP(const char ip_address[]);
	int writeDIO(std::vector<std::array<char[DIO_LEN_BYTE_BUF], 1>> TtlSnapshots);
	int writeDACs(std::vector<AoChannelSnapshot> dacSnapshots);
	int writeDDSs(std::vector<DdsChannelSnapshot> ddsSnapshots);
	int sendCommand(std::string command);/*do connection inside*/
	int writeCommand(std::string command);/*this is only for writing, requires conection already estabilished*/
	
private:
	int sendWithCatch(const SOCKET& socket, const char* byte_buf, int buffLen, int flags = 0);


private:
	//const bool ZYNQ_SAFEMODE = false;

	SOCKET ConnectSocket;

	const unsigned int timeConv = 100000; // SEQ time given in multiples of 10 ns, 1/DIO_TIME_RESOLUTION
	const unsigned int timeConvDAC = 100000; 
	const unsigned int dacRes = 0xffff; //16 bit dac resolution

};
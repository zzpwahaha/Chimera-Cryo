#pragma once

//#include <stdio.h> 
//#include <stdlib.h> 
#include <string.h> 
#include <winsock2.h>
#include <Ws2tcpip.h>
#include "AnalogOutput/AoStructures.h"
#include "DigitalOutput/DoStructures.h"
#include "DirectDigitalSynthesis/DdsSystemStructures.h"

//#include "DacStructures.h"
//#include "DDSStructures.h"
//#include "DioStructures.h"

#define ZNYQ_SAFEMODE false
#define ZYNQ_ADDRESS "10.10.0.2"
#define ZYNQ_PORT "8080"
#define ZYNQ_MAX_BUFF 64
#define DIO_LEN_BYTE_BUF 28
#define DAC_LEN_BYTE_BUF 42
#define DDS_LEN_BYTE_BUF 46
#define DAC_TIME_RESOLUTION 1.6 // in ms
#define DAC_RAMP_MAX_PTS 0xffff // 65535 ???
#define DDS_TIME_RESOLUTION 1.6 // in ms
#define DDS_MAX_AMP 1.25 // in mW
#define DDS_FPGA_ADDRESS "FT1I6IBSB" //Device Serial: FT1I6IBS, Use FT1I6IBSB in C++ to select Channel B


class ZynqTCP 
{

private:
	SOCKET ConnectSocket;
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


};
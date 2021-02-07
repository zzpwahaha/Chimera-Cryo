#include "stdafx.h"
#include "ZynqTCP.h"

ZynqTCP::ZynqTCP()
{
	if (!ZNYQ_SAFEMODE) {
		ConnectSocket = INVALID_SOCKET;
	}

}

ZynqTCP::~ZynqTCP() {
	//disconnect();
}

void ZynqTCP::disconnect() {
	if (!ZNYQ_SAFEMODE) {
		char buff[ZYNQ_MAX_BUFF];
		memset(buff, 0, sizeof(buff));
		std::string  end_command = "end_0";

		//int n = sprintf_s(buff, ZYNQ_MAX_BUFF, "%s", end_command);
		for (size_t i = 0; i < end_command.length(); i++)
		{
			buff[i] = end_command[i];
		}

		int BytesSent;
		BytesSent = send(ConnectSocket, buff, sizeof(buff), 0);
		if (BytesSent == SOCKET_ERROR)
		{
			thrower("Unable to send end message to server!");
		}
		else
		{
			memset(buff, 0, sizeof(buff));
		}
		closesocket(ConnectSocket);
	}
}

int ZynqTCP::connectTCP(const char ip_address[])
{
	WSADATA wsaData;

	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		thrower("WSAStartup failed");
		return 1;
	}

	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(ip_address, ZYNQ_PORT, &hints, &result);
	if (iResult != 0) {
		thrower("getaddrinfo failed");
		WSACleanup();
		return 1;
	}

	ConnectSocket = INVALID_SOCKET;

	// Attempt to connect to the first address returned by
// the call to getaddrinfo
	ptr = result;

	// Create a SOCKET for connecting to server
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
		ptr->ai_protocol);

	if (ConnectSocket == INVALID_SOCKET) {
		thrower("Error at socket()");
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}


	// Connect to server.
	iResult = ::connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		thrower("Unable to connect to server!");
		WSACleanup();
		return 1;
	}
	else
	{
		return 0;
	}

}

int ZynqTCP::writeCommand(std::string command)
{
	char buff[ZYNQ_MAX_BUFF];
	memset(buff, 0, sizeof(buff));
	if (command.length() > ZYNQ_MAX_BUFF)
	{
		thrower("Command exceeding ZYNQ_MAX_BUFF: " + std::to_string(ZYNQ_MAX_BUFF));
		return 1;
	}
	for (size_t i = 0; i < command.length(); i++)
	{
		buff[i] = command[i];
	}


	int BytesSent = 0;
	//char commandc[ZYNQ_MAX_BUFF];
	//strcpy_s(commandc, ZYNQ_MAX_BUFF, command.c_str());
	//sprintf_s(buff, ZYNQ_MAX_BUFF, commandc);

	BytesSent = send(ConnectSocket, buff, sizeof(buff), 0);
	if (BytesSent == SOCKET_ERROR)
	{
		thrower("Unable to send message to server!");
		return 1;
	}
	else
	{
		return 0;
	}
}

int ZynqTCP::writeDIO(std::vector<std::array<char[DIO_LEN_BYTE_BUF], 1>> TtlSnapshots)
{
	char buff[ZYNQ_MAX_BUFF];
	memset(buff, 0, sizeof(buff));

	int BytesSent = 0;

	char command[ZYNQ_MAX_BUFF];
	sprintf_s(command, ZYNQ_MAX_BUFF, "DIOseq_%u", TtlSnapshots.size());
	//sprintf_s(buff, ZYNQ_MAX_BUFF, "DIOseq_%u", TtlSnapshots.size());
	for (size_t i = 0; i < strlen(command); i++)
	{
		buff[i] = command[i];
	}


	BytesSent = send(ConnectSocket, buff, sizeof(buff), 0);
	if (BytesSent == SOCKET_ERROR)
	{
		thrower("Unable to send message to server!");
		return 1;
	}
	else
	{
		memset(buff, 0, sizeof(buff));

		for (auto TtlSnapshot : TtlSnapshots)
		{
			BytesSent = send(ConnectSocket, TtlSnapshot[0], DIO_LEN_BYTE_BUF, 0);
			if (BytesSent == SOCKET_ERROR)
			{
				thrower("Unable to send message to server!");
				return 1;
			}
		}

		return 0;

	}

	

}

int ZynqTCP::writeDACs(std::vector<AoChannelSnapshot> dacChannelSnapshots)
{
	char buff[ZYNQ_MAX_BUFF];
	char buffCommand[ZYNQ_MAX_BUFF];
	memset(buff, 0, sizeof(buff));

	int snapIndex = 0;
	unsigned int timeConv = 100000; // SEQ time given in multiples of 10 ns
	unsigned int timeConvDAC = 1000; // DAC time given multiples of 1 us
	//unsigned int dacRes = 0xffff;
	char byte_buf[DAC_LEN_BYTE_BUF];
	//char command[DAC_LEN_BYTE_BUF];
	unsigned int time, duration;
	unsigned short channel;
	double start, end;

	int BytesSent = 0;

	sprintf_s(buffCommand, ZYNQ_MAX_BUFF, "DACseq_%u", dacChannelSnapshots.size());
	for (size_t i = 0; i < strlen(buffCommand); i++)
	{
		buff[i] = buffCommand[i];
	}


	BytesSent = send(ConnectSocket, buff, sizeof(buff), 0);
	if (BytesSent == SOCKET_ERROR)
	{
		thrower("Unable to send message to server!");
		return 1;
	}
	else
	{
		memset(buff, 0, sizeof(buff));

		for (int i = 0; i < dacChannelSnapshots.size(); ++i)
		{
			AoChannelSnapshot snapshot = dacChannelSnapshots[i];

			time = (unsigned int)(snapshot.time * timeConv);
			channel = snapshot.channel;
			start = snapshot.dacValue;
			end = snapshot.dacEndValue;
			duration = (unsigned int)(snapshot.dacRampTime * timeConvDAC);

			sprintf_s(byte_buf, DAC_LEN_BYTE_BUF, "t%08X_c%04X_s%06.3f_e%06.3f_d%08x", time, channel, start, end, duration);
			//for (size_t i = 0; i < strlen(command); i++)
			//{
			//	byte_buf[i] = command[i];
			//}

			BytesSent = send(ConnectSocket, byte_buf, DAC_LEN_BYTE_BUF, 0);
			if (BytesSent == SOCKET_ERROR)
			{
				thrower("Unable to send message to server!");
				return 1;
			}

		}

		return 0;
	}

}

int ZynqTCP::writeDDSs(std::vector<DdsChannelSnapshot> ddsChannelSnapshots)
{
	char buff[ZYNQ_MAX_BUFF];
	char buffCommand[ZYNQ_MAX_BUFF];
	memset(buff, 0, sizeof(buff));

	int snapIndex = 0;
	unsigned int timeConv = 100000; // SEQ time given in multiples of 10 ns
	unsigned int timeConvDAC = 1000; // DDS time given multiples of 1 us
	unsigned int dacRes = 0xffff;

	char byte_buf[DDS_LEN_BYTE_BUF];
	char byte_bufCommand[DDS_LEN_BYTE_BUF];
	memset(byte_buf, 0, sizeof(byte_buf));
	unsigned int time, duration;
	unsigned short channel;
	char type;
	double start, end;

	int BytesSent = 0;

	sprintf_s(buffCommand, ZYNQ_MAX_BUFF, "DDSseq_%u", ddsChannelSnapshots.size());
	for (size_t i = 0; i < strlen(buffCommand); i++)
	{
		buff[i] = buffCommand[i];
	}


	BytesSent = send(ConnectSocket, buff, sizeof(buff), 0);
	if (BytesSent == SOCKET_ERROR)
	{
		thrower("Unable to send message to server!");
		return 1;
	}
	else
	{
		memset(buff, 0, sizeof(buff));

		for (int i = 0; i < ddsChannelSnapshots.size(); ++i)
		{
			DdsChannelSnapshot snapshot = ddsChannelSnapshots[i];

			time = (unsigned int)(snapshot.time * timeConv);
			type = snapshot.ampOrFreq;
			channel = snapshot.channel;
			start = snapshot.val;
			end = snapshot.endVal;
			duration = (unsigned int)(snapshot.rampTime * timeConvDAC);

			sprintf_s(byte_bufCommand, DDS_LEN_BYTE_BUF, "t%08X_c%04X_%c_s%07.3f_e%07.3f_d%08x", 
				time, channel, type, start, end, duration);
			for (size_t i = 0; i < strlen(byte_bufCommand); i++)
			{
				byte_buf[i] = byte_bufCommand[i];
			}
			BytesSent = send(ConnectSocket, byte_buf, DDS_LEN_BYTE_BUF, 0);
			if (BytesSent == SOCKET_ERROR)
			{
				thrower("Unable to send message to server!");
				return 1;
			}

		}

		return 0;
	}

}

int ZynqTCP::sendCommand(std::string command)
{
	int tcp_connect;
	try
	{
		tcp_connect = connectTCP(ZYNQ_ADDRESS);
	}
	catch (ChimeraError& err)
	{
		tcp_connect = 1;
		throwNested("Unable to connect to server during programming the experiment, Check if Zynq tcp server is started");
		//errBox(err.what()); should not have gui stuff here since this function is used for sending 'init' in exp thread which can not handle gui (qt forbid thread other than main thread handle gui)
	}

	if (tcp_connect == 0)
	{
		int zynq_write = writeCommand(command);
		if (zynq_write == 1) {
			thrower("failed to write command {" + command + "} to Zynq.");
		}
		disconnect();
	}
	else
	{
		throwNested("Unable to connect to server during programming the experiment, Check if Zynq tcp server is started");
	}
}

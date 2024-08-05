#pragma once
#include <GeneralObjects/IChimeraSystem.h>
#include <GeneralFlumes/BoostAsyncTCPServer.h>
#include <ExternalController/MessageConsumer.h>
#include <ExternalController/MessageDisplay.h>
#include <ExternalController/CommandModulator.h>
#include <qlabel.h>

class TCPServerSystem : public IChimeraSystem
{
	Q_OBJECT
public:
	// THIS CLASS IS NOT COPYABLE.
	TCPServerSystem& operator=(const TCPServerSystem&) = delete;
	TCPServerSystem(const TCPServerSystem&) = delete;
	TCPServerSystem(IChimeraQtWindow* parent_in);
	void initialize();

private:
	BoostAsyncTCPServer tcpServer;
	MessageConsumer consumer;
	MessageDisplay* logger;
	CommandModulator modulator;

};


#include "stdafx.h"
#include "TCPServerSystem.h"
#include <qboxlayout.h>

TCPServerSystem::TCPServerSystem(IChimeraQtWindow* parent_in) :
	IChimeraSystem(parent_in),
	tcpServer("127.0.0.1", 8888),
	modulator(this),
	consumer(tcpServer.dataQueue(), modulator)
{
	consumer.start();
}

void TCPServerSystem::initialize()
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	QLabel* header = new QLabel(qstr("Chimera TCP Server @ ")+"127.0.0.1:8888", this);
	logger = new MessageDisplay(100, parentWin);
	logger->setReadOnly(true);
	layout->addWidget(header, 0);
	layout->addWidget(logger, 1);

	connect(&consumer, &MessageConsumer::logMessage, logger, &MessageDisplay::appendText);

	modulator.initialize(parentWin);
}

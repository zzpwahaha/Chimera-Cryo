#include "stdafx.h"
#include "MessageSender.h"
#include <iostream>
#include "Messagetypes.h"

MessageSender::MessageSender()
{
}


MessageSender::~MessageSender()
{
}

void MessageSender::enqueue(Message &m)
{
	queue.push_back(m);
}

void MessageSender::getQueueElementCount()
{
	flog << "MessageSender::getQueueElementCount: Queued messages: " << queue.size() << fendl;
}

std::vector<int> MessageSender::getMessageBytes()
{
	std::vector<int> bytevector;
	for (auto& message : queue) {
		for (auto& byte : KA007_Factory.getBytes(message.getParameters())){
			bytevector.push_back(byte);
		}
	}
	//queue.clear(); ZZP: commented out 20250607. This shouldn't clear the queue otherwise can not call this function again. Not sure what is their initial idea
	return bytevector;
}

MessageBuilder Message::make()
{
	return MessageBuilder();
}
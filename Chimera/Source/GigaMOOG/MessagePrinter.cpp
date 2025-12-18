#include "stdafx.h"
#include "MessagePrinter.h"

void MessagePrinter::callback(int byte)
{
	//std::cout << byte;
	//return;
	if (!isReceiving) {
		header[headeridx] = byte;
		headeridx++;

		//received complete header
		if (headeridx == 4) {
			messagetype = (header[0] & 0xF0) >> 4;
			messagesource = (header[0] & 0x0F);
			messagelength = header[1];
			messagedata_h = header[2];
			messagedata_l = header[3];
			
			headeridx = 0;

			if (messagelength > 0) {
				isReceiving = true;
				expectedBytes = messagelength * 4;
				byteCount = 0;
			}
			else {
				flog << "Header only message: " << "\n";
				flog << "  Type: " << messagetype << "\n";
				flog << "  Source: " << messagesource << "\n";
				flog << "  Length: " << messagelength << "\n";
				flog << "  Data: " << messagedata_h << " " << messagedata_l << "\n\n";
			}
		}
	}
	else {
		if (byteCount < expectedBytes - 1) {
			received.push_back(byte);
			byteCount++;
		}
		else {
			//received all data
			received.push_back(byte);
			isReceiving = false;
			flog << "Header: " << "\n";
			flog << "  Type: " << messagetype << "\n";
			flog << "  Source: " << messagesource << "\n";
			flog << "  Length: " << messagelength << "\n";
			flog << "  Data: " << messagedata_h << " " << messagedata_l << "\n";
			flog << "Message payload: " << "\n" << " ";
			for (auto& item : received) {
				flog << " " << item;
			}
			flog << "\n\n";
			flog << fendl;
			received.clear();
		}
	}
}

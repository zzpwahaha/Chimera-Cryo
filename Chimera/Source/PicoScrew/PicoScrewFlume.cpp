#include "stdafx.h"
#include "PicoScrewFlume.h"

PicoScrewFlume::PicoScrewFlume(bool safemode, std::string deviceKey)
	:safemode(safemode), 
	deviceKey(deviceKey)
{
	if (safemode) {
		return;
	}
	//newp_usb_SetLogging(true);
	newp_usb_init_system();
	// If the devices were not opened successfully
	if (!npUSB.OpenDevices(0, true)) {
		thrower("\n***** Error:  Could not open the devices. *****\n\n"
			"Please make sure that the devices are powered on, \n"
			"connected to the PC, and that the drivers are properly installed.\n\n");
	}

	// Get the device table
	std::map<std::string, int> deviceTable = npUSB.GetDeviceTable();

	// If there are no open instruments
	if (deviceTable.size() == 0) {
		newp_usb_uninit_system();
		thrower("No devices discovered.\n\n");
		return;
	}
	if (deviceTable.find(deviceKey) == deviceTable.end()) {
		// somehow if called newp_usb_init_system() on the top, the deviceTable will be an map with empty key
		// and if not calling newp_usb_init_system(), will see weird break of the program during starting. 
		//thrower("Error in finding the desired PicoScrew device: " + str(deviceKey) +
		//	"Please make sure that the devices are powered on, \n"
		//	"connected to the PC, and that the drivers are properly installed.\n\n");
	}


}

PicoScrewFlume::~PicoScrewFlume()
{
	if (safemode) {
		return;
	}
	// Make sure that the system is properly shut down
	newp_usb_uninit_system();
}

std::string PicoScrewFlume::read()
{
	if (safemode) {
		return std::string("SAFEMODE");
	}
	char szBuffer[NewportUSB::m_knMaxBufferLength];
	unsigned long lBytesRead = 0;

	int nStatus = npUSB.Read(deviceKey, szBuffer, NewportUSB::m_knMaxBufferLength, &lBytesRead);

	if (nStatus != 0) {
		thrower("Error:  Device Read Error Code = " + str(nStatus) + ". *****\n\n");
	}
	return std::string(szBuffer);
}

void PicoScrewFlume::write(std::string cmd)
{
	if (safemode) {
		return;
	}
	int nStatus = npUSB.Write(deviceKey, cmd);
	if (nStatus != 0) {
		thrower("Error:  Device Write Error Code = " + str(nStatus) + ". *****\n\n");
	}
}

std::string PicoScrewFlume::query(std::string cmd)
{
	write(cmd);
	return read();
}

# include "stdafx.h"
////////////////////////////////////////////////////////////////////////////////
// This native C++ wrapper class provides support for interacting with the 
// USB driver (usbdll.dll).
////////////////////////////////////////////////////////////////////////////////
#include "NewportUSB.h"

////////////////////////////////////////////////////////////////////////////////
// Constructor.
////////////////////////////////////////////////////////////////////////////////
NewportUSB::NewportUSB(void)
{
	m_bOpen = false;
}

////////////////////////////////////////////////////////////////////////////////
// Destructor.
////////////////////////////////////////////////////////////////////////////////
NewportUSB::~NewportUSB(void)
{
}

////////////////////////////////////////////////////////////////////////////////
// This method opens all USB devices on the bus with the specified product ID 
// and queries the address of each.
// Input:
//    nProductId:	The product ID of the devices to open (zero opens all 
//					devices on the bus).
// Returns:
//    True for success, false for failure.
////////////////////////////////////////////////////////////////////////////////
bool NewportUSB::OpenDevices (int nProductID)
{
	// If the USB devices are already open
	if (m_bOpen)
	{
		return true;
	}

	try
	{
		// Open the specified USB devices on the bus
		m_bOpen = newp_usb_init_product (nProductID) == 0;

		if (m_bOpen)
		{
			char szDevInfo[1024];

            // If all device information cannot be retrieved
            if (newp_usb_get_device_info (szDevInfo) != 0)
            {
                CloseDevices ();
            }
            else
            {
                // Init the device information list
                FillDevInfoList (szDevInfo);
            }
		}
	}
	catch (...)
    {
        CloseDevices ();
    }

    return m_bOpen;
}


////////////////////////////////////////////////////////////////////////////////
// This method opens all USB devices on the bus with the specified product ID 
// and queries the address of each.
// Input:
//    nProductId:		The product ID of the devices to open (zero opens all 
//						devices on the bus).
//    bUsingDeviceKey:	True if referencing devices by key, false if using 
//						the USB address.
// Returns:
//    True for success, false for failure.
////////////////////////////////////////////////////////////////////////////////
bool NewportUSB::OpenDevices (int nProductID, bool bUsingDeviceKey)
{
    // If referencing devices by USB address
    if (!bUsingDeviceKey)
    {
        return OpenDevices (nProductID);
    }

	// If the USB devices are already open
	if (m_bOpen)
	{
		return true;
	}

	try
	{
        int nNumDevices = 0;

		// Open the specified USB devices on the bus
        m_bOpen = newp_usb_open_devices (nProductID, !bUsingDeviceKey, &nNumDevices) == 0;

		if (m_bOpen)
		{
			char** ppArray = new char* [nNumDevices];

			for (int i = 0; i < nNumDevices; i++)
			{
				ppArray[i] = new char (0);
			}

            // If all device information cannot be retrieved
            if (newp_usb_get_model_serial_keys (ppArray) != 0)
            {
                CloseDevices ();
            }
            else
            {
				// Create the map of key / value pairs where 
				// the key is the device key and the value is the device ID
				for (int i = 0; i < nNumDevices; i++)
				{
					m_mapDeviceTable[ppArray[i]] = i;
				}
            }

			delete [] ppArray;
		}
	}
	catch (...)
    {
        CloseDevices ();
    }

    return m_bOpen;
}

////////////////////////////////////////////////////////////////////////////////
// This method closes all USB devices on the bus.
////////////////////////////////////////////////////////////////////////////////
void NewportUSB::CloseDevices ()
{
	// If the USB devices are open
	if (m_bOpen)
	{
		m_bOpen = false;

		try
		{
			// Clear the device information list
			m_DevInfoList.clear ();
			m_mapDeviceTable.clear ();

			// Close all USB devices on the bus
			newp_usb_uninit_system ();
		}
		catch (...)
		{
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// This method reads the response data from the specified device.
// Input:
//    strDeviceKey:	The device key.
//    lpBuffer:		The character buffer used to hold the response data.
//    nLength:		The length of the character buffer.
// Output:
//    lBytesRead:	The number of bytes read.
// Returns:
//    Zero for success, non-zero for failure.
////////////////////////////////////////////////////////////////////////////////
int NewportUSB::Read (std::string strDeviceKey, char* lpBuffer, int nLength, unsigned long* lBytesRead)
{
	try
	{
		char szKey[m_knMaxBufferLength];
		StringToChar (szKey, strDeviceKey);
		int nStatus = newp_usb_read_by_key (szKey, lpBuffer, nLength, lBytesRead);
		int i = *lBytesRead;

		// Search from the end of the response string
		while (--i >= 0)
		{
			// If a linefeed is found
			if (lpBuffer[i] == '\n')
			{
				// If a return is found
				if (lpBuffer[i - 1] == '\r')
				{
					i--;
				}

				// Update the number of bytes read
				*lBytesRead = i;

				// Add a string terminator
				lpBuffer[i] = NULL;
				break;
			}
		}

	    return nStatus;
	}
	catch (...)
    {
	    return -1;
    }
}

////////////////////////////////////////////////////////////////////////////////
// This method reads the response data from the specified device.
// Input:
//    nDeviceID:	The USB address of the device.  The valid range is from 0 - 31.
//    lpBuffer:		The character buffer used to hold the response data.
//    nLength:		The length of the character buffer.
// Output:
//    lBytesRead:	The number of bytes read.
// Returns:
//    Zero for success, non-zero for failure.
////////////////////////////////////////////////////////////////////////////////
int NewportUSB::Read (int nDeviceID, char* lpBuffer, int nLength, unsigned long* lBytesRead)
{
	try
	{
		int nStatus = newp_usb_get_ascii (nDeviceID, lpBuffer, nLength, lBytesRead);
		int i = *lBytesRead;

		// Search from the end of the response string
		while (--i >= 0)
		{
			// If a linefeed is found
			if (lpBuffer[i] == '\n')
			{
				// If a return is found
				if (lpBuffer[i - 1] == '\r')
				{
					i--;
				}

				// Update the number of bytes read
				*lBytesRead = i;

				// Add a string terminator
				lpBuffer[i] = NULL;
				break;
			}
		}

	    return nStatus;
	}
	catch (...)
    {
	    return -1;
    }
}

////////////////////////////////////////////////////////////////////////////////
// This method reads the response data from the specified device.
// Input:
//    strDeviceKey:	The device key.
//    lpBuffer:		The character buffer used to hold the response data.
//    nLength:		The length of the character buffer.
// Output:
//    lBytesRead:	The number of bytes read.
// Returns:
//    Zero for success, non-zero for failure.
////////////////////////////////////////////////////////////////////////////////
int NewportUSB::ReadBinary (std::string strDeviceKey, char* lpBuffer, int nLength, unsigned long* lBytesRead)
{	try
	{
		char szKey[m_knMaxBufferLength];
		StringToChar (szKey, strDeviceKey);
		int nStatus = newp_usb_read_by_key (szKey, lpBuffer, nLength, lBytesRead);
	    return nStatus;
	}
	catch (...)
    {
	    return -1;
    }
}

////////////////////////////////////////////////////////////////////////////////
// This method reads the response data from the specified device.
// Input:
//    nDeviceID:	The USB address of the device.  The valid range is from 0 - 31.
//    lpBuffer:		The character buffer used to hold the response data.
//    nLength:		The length of the character buffer.
// Output:
//    lBytesRead:	The number of bytes read.
// Returns:
//    Zero for success, non-zero for failure.
////////////////////////////////////////////////////////////////////////////////
int NewportUSB::ReadBinary (int nDeviceID, char* lpBuffer, int nLength, unsigned long* lBytesRead)
{
	try
	{
		int nStatus = newp_usb_get_ascii (nDeviceID, lpBuffer, nLength, lBytesRead);
	    return nStatus;
	}
	catch (...)
    {
	    return -1;
    }
}

////////////////////////////////////////////////////////////////////////////////
// This method sends the passed in command string to the specified device.
// Input:
//    strDeviceKey:	The device key.
//    lpBuffer:		The character buffer used to hold the command string.
// Returns:
//    Zero for success, non-zero for failure.
////////////////////////////////////////////////////////////////////////////////
int NewportUSB::Write (std::string strDeviceKey, char* lpBuffer)
{
	try
	{
		char szKey[m_knMaxBufferLength];
		StringToChar (szKey, strDeviceKey);
		return newp_usb_write_by_key (szKey, lpBuffer, (unsigned long) strlen (lpBuffer));
	}
	catch (...)
    {
	    return -1;
    }
}

////////////////////////////////////////////////////////////////////////////////
// This method sends the passed in command string to the specified device.
// Input:
//    nDeviceID:	The USB address of the device.  The valid range is from 0 - 31.
//    lpBuffer:		The character buffer used to hold the command string.
// Returns:
//    Zero for success, non-zero for failure.
////////////////////////////////////////////////////////////////////////////////
int NewportUSB::Write (int nDeviceID, char* lpBuffer)
{
	try
	{
		return newp_usb_send_ascii (nDeviceID, lpBuffer, (unsigned long) strlen (lpBuffer));
	}
	catch (...)
    {
	    return -1;
    }
}

////////////////////////////////////////////////////////////////////////////////
// This method sends the passed in command string to the specified device.
// Input:
//    strDeviceKey:	The device key.
//    strBuffer:	The character buffer used to hold the command string.
// Returns:
//    Zero for success, non-zero for failure.
////////////////////////////////////////////////////////////////////////////////
int NewportUSB::Write (std::string strDeviceKey, std::string strBuffer)
{
	try
	{
		char szBuffer[m_knMaxBufferLength];
		StringToChar (szBuffer, strBuffer);
		return Write (strDeviceKey, szBuffer);
	}
	catch (...)
    {
	    return -1;
    }
}

////////////////////////////////////////////////////////////////////////////////
// This method sends the passed in command string to the specified device.
// Input:
//    nDeviceID:	The USB address of the device.  The valid range is from 0 - 31.
//    strBuffer:	The character buffer used to hold the command string.
// Returns:
//    Zero for success, non-zero for failure.
////////////////////////////////////////////////////////////////////////////////
int NewportUSB::Write (int nDeviceID, std::string strBuffer)
{
	try
	{
		char szBuffer[m_knMaxBufferLength];
		StringToChar (szBuffer, strBuffer);
		return Write (nDeviceID, szBuffer);
	}
	catch (...)
    {
	    return -1;
    }
}

////////////////////////////////////////////////////////////////////////////////
// This method copies the passed in std::string into a null terminated 
// character array.
// Input:
//    lpDest:		The destination character array.
//    strSrc:		The source string data.
////////////////////////////////////////////////////////////////////////////////
void NewportUSB::StringToChar (char* lpDest, std::string strSrc)
{
#pragma warning (disable : 4996)	// Disable unsafe parameter warning
	strSrc.copy (lpDest, strSrc.size ());
	lpDest[strSrc.size ()] = NULL;
#pragma warning (default : 4996)	// Enable unsafe parameter warning
}

////////////////////////////////////////////////////////////////////////////////
// This method returns a copy of the device table.
// Returns:
//    A copy of the device table.
////////////////////////////////////////////////////////////////////////////////
std::map <std::string, int> NewportUSB::GetDeviceTable ()
{
	return m_mapDeviceTable;
}

////////////////////////////////////////////////////////////////////////////////
// This method returns a copy of the device information list.
// Returns:
//    A copy of the device information list.
////////////////////////////////////////////////////////////////////////////////
std::vector<NewportUSB::DevInfo> NewportUSB::GetDevInfoList ()
{
	return m_DevInfoList;
}

////////////////////////////////////////////////////////////////////////////////
// This method fills / initializes the device information list.
// Input:
//    lpDevInfo:	The character buffer used to hold the device information.
////////////////////////////////////////////////////////////////////////////////
void NewportUSB::FillDevInfoList (char* lpDevInfo)
{
	try
	{
		DevInfo devInfo;
		int nCount = 0;
		char szDelimiters[] = ",;";
		char* pNextToken;

		// Get the next token
		char* pToken = strtok_s (lpDevInfo, szDelimiters, &pNextToken);

		while (pToken != NULL)
		{
			nCount++;

			// If the count is odd
			if (nCount % 2 == 1)
			{
				// Get the device ID
				devInfo.nID = atoi (pToken);
			}
			else
			{
				// Get the device description
				std::string strDesc (pToken);
				int nIdx = strDesc.size ();

				if (strDesc[nIdx - 1] == '\r')
				{
					nIdx--;
				}

				strDesc[nIdx] = NULL;
				devInfo.strDescription = strDesc;

				// Add the entry to the device information list
				m_DevInfoList.push_back (devInfo);
			}

			// Get the next token
			pToken = strtok_s (NULL, szDelimiters, &pNextToken);
		}
	}
	catch (...)
    {
    }
}

////////////////////////////////////////////////////////////////////////////////
// This method creates the device key that will be used by the event handling 
// methods to access a particular device.
// Input:
//    handle:		The unique device handle that was created by the USB driver.
//    strDeviceKey:	The device key used to uniquely identify each instrument 
//					for I/O operations.
// Returns:
//    True for success, false for failure.
////////////////////////////////////////////////////////////////////////////////
bool NewportUSB::CreateDeviceKey (int handle, std::string& strDeviceKey)
{
	int nStatus = Write (handle, (std::string) "*IDN?");

	// If there was not a Write error
	if (nStatus == 0)
	{
		char szBuffer[m_knMaxBufferLength];
		unsigned long lBytesRead = 0;

		// Read the command response from the device
		nStatus = Read (handle, szBuffer, m_knMaxBufferLength, &lBytesRead);

		// If there was not a Read error
		if (nStatus == 0)
		{
			ParseDeviceKey (szBuffer, strDeviceKey);
		}
	}

	// Return the I/O status
	return nStatus == 0;
}

////////////////////////////////////////////////////////////////////////////////
// This method parses the passed in identification string for the model and 
// serial number.
// Input:
//    lpID:			The identification string.
//    strDeviceKey:	The device key used to uniquely identify each instrument 
//					for I/O operations.
////////////////////////////////////////////////////////////////////////////////
void NewportUSB::ParseDeviceKey (char* lpID, std::string& strDeviceKey)
{
	int nCount = 0;
	char szDelimiters[] = " ";
	char* pNextToken;

	// Get the next token
	char* pToken = strtok_s (lpID, szDelimiters, &pNextToken);

	while (pToken != NULL)
	{
		if (nCount == 1)
		{
			// Place the model number at the beginning of the device key
			strDeviceKey = pToken;
		}
		else if (nCount >= 4)
		{
			// Append any other tokens from the serial number on
			strDeviceKey.append (" ");
			strDeviceKey.append (pToken);
		}

		// Get the next token
		pToken = strtok_s (NULL, szDelimiters, &pNextToken);
		nCount++;
	}
}


//Command Description Command executed when motion is in progress
//* IDN ? Identification string query x
//* RCL Recall parameters
//* RST Reset instrument x
//AB Abort motion x
//AC Set acceleration x
//AC ? Get acceleration x
//DH Define home position
//DH ? Get home position x
//MC Motor check
//MD ? Get motion done status x
//MV Move indefinitely
//MV ? Get motion direction x
//PA Move to a target position
//PA ? Get destination position x
//PR Move relative
//PR ? Get destination position x
//QM Set motor type x
//QM ? Get motor type x
//RS Reset the controller x
//SA Set controller address x
//SA ? Get controller address x
//SC Scan RS - 485 network x
//SC ? Get RS - 485 network controller addresses x
//SD ? Get scan status x
//SM Save to non - volatile memory x
//ST Stop motion x
//TB ? Get error message x
//TE ? Get error number x
//TP ? Get position x
//VA Set velocity x
//VA ? Get velocity x
//VE ? Firmware version string query x
//XX Purge memory
//ZZ Set configuration register x
//ZZ ? Get configuration register x
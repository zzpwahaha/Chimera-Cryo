// created by Mark O. Brown
#pragma once

#include "GeneralUtilityFunctions/my_str.h"
#include <string>
#include <vector>
#include <array>
#include <utility>

// running in safemode means that the program doesn't actually try to connect to physical devices. Generally, it will 
// follow the exact coding logic, but at the exact point where it would normally communicate with a device, it will 
// instead simply skip this step. It might generate example data where useful / necessary (e.g. after querying a
// camera system for a picture). It can be used to build and debug other aspects of the program, such as the gui, 
// coding logic, etc.
 
#define MASTER_COMPUTER

#ifdef MASTER_COMPUTER
	//constexpr bool DOFTDI_SAFEMODE = true;
	constexpr bool DDS_SAFEMODE = true;
	constexpr bool ANDOR_SAFEMODE = true;
	constexpr bool OFFSETLOCK_SAFEMODE = false/*false*/;
	//constexpr bool ANALOG_IN_SAFEMODE = true;
	#ifdef _DEBUG
		constexpr bool PYTHON_SAFEMODE = true;
	#else
		constexpr bool  PYTHON_SAFEMODE = true;
	#endif
	//constexpr bool DAQMX_SAFEMODE = true;
	//constexpr bool ANALOG_OUT_SAFEMODE = true;

	constexpr auto CODE_ROOT = "C:\\Chimera\\Chimera-Cryo";

	const std::string PLOT_FILES_SAVE_LOCATION = str (CODE_ROOT) + "\\Plotting";
	const std::string DATA_ANALYSIS_CODE_LOCATION = "C:\\Users\\Regal-Lab\\Code\\Data_Analysis_Control\\";
	const std::string DEFAULT_SCRIPT_FOLDER_PATH = str (CODE_ROOT) + "\\Default-Scripts\\";
	const std::string ACTUAL_CODE_FOLDER_PATH = str (CODE_ROOT) + "\\Chimera\\";
	const std::string PROFILES_PATH = str (CODE_ROOT) + "\\Profiles\\";

	//const std::string DATA_SAVE_LOCATION = "J:\\Data Repository\\New Data Repository\\";
	const std::string DATA_SAVE_LOCATION = "C:\\Chimera\\Chimera-Cryo\\tmpDataSave\\";

	const std::string MUSIC_LOCATION = str (CODE_ROOT) + "\\Final Fantasy VII - Victory Fanfare [HQ].mp3";
	const std::string FUNCTIONS_FOLDER_LOCATION = str (CODE_ROOT) + "\\Functions\\";
	const std::string MASTER_CONFIGURATION_FILE_ADDRESS (str (CODE_ROOT) + "\\Master-Configuration.txt");
	const std::string CAMERA_CAL_ROUTINE_ADDRESS = PROFILES_PATH + "Hotkey Experiments\\Camera";
	const std::string MOT_ROUTINES_ADDRESS = PROFILES_PATH + "Hotkey Experiments\\MOT";
	// location where wave data can be outputted for analyzing with another computer.


	// Zynq realated
	const double ZYNQ_DEADTIME = 0.1; // give zynq 0.1ms to avoid sending zero at t=0 for ttl
	
	const bool ZYNQ_SAFEMODE = false;
	const auto ZYNQ_ADDRESS = "10.10.0.2";
	const auto ZYNQ_PORT = "8080";
	const int ZYNQ_MAX_BUFF = 64;
	const int DIO_LEN_BYTE_BUF = 28;
	const int DAC_LEN_BYTE_BUF = 42;
	const int DDS_LEN_BYTE_BUF = 46;

	const double DIO_TIME_RESOLUTION = 1e-5;
	const double DAC_TIME_RESOLUTION = 1.6; // in ms
	const int DAC_RAMP_MAX_PTS = 0xffff; // 65535 ???
	const double DDS_TIME_RESOLUTION = 1.6; // in ms
	const double DDS_MAX_AMP = 1.25; // in mW

	const double OL_TIME_RESOLUTION = 0.02; //in ms
	const std::vector<std::pair<unsigned, unsigned>> OL_TRIGGER_LINE
		= { std::make_pair(3 - 1,6),std::make_pair(3 - 1,7) }; /*the first is the label on the box minus 1, has minus'd 1 explicitly */
	const double OL_TRIGGER_TIME = 0.05; //in ms i.e. 50us

	//#define DDS_FPGA_ADDRESS "FT1I6IBSB"; //Device Serial: FT1I6IBS, Use FT1I6IBSB in C++ to select Channel B


	//ArbGens
	const bool UWAVE_SAFEMODE = true;
	const bool UWAVE_SAFEMODE_SIG = true;
	const bool UWAVE_SAFEMODE_AGI = false;
	const int numArbGen = 2;
	//const std::string UWAVE_AGILENT_ADDRESS = "TCPIP0::10.10.0.5::inst0::INSTR";
	const std::string UWAVE_AGILENT_ADDRESS = "USB0::0x0957::0x2807::MY57400998::INSTR";
	const std::string UWAVE_SIGLENT_ADDRESS = "TCPIP0::10.10.0.4::inst0::INSTR";
	const std::string RAMP_LOCATION = str(CODE_ROOT) + "\\Ramp_Files\\";

	//Analog in 
	const bool AI_SAFEMODE = true;
	const std::string AI_SOCKET_ADDRESS = "10.10.0.10";
	const std::string AI_SOCKET_PORT = "80";

	//Mako camera
	const unsigned MAKO_NUMBER = 2;
	const std::array<bool, MAKO_NUMBER> MAKO_SAFEMODE = { false,false };
	const std::array<std::string, MAKO_NUMBER> MAKO_DELIMS = { "MAKO1_CAMERA_SETTING", "MAKO2_CAMERA_SETTING" };
	const std::array<std::string, MAKO_NUMBER> MAKO_IPADDRS= { "10.10.0.6", "10.10.0.7" };
	const std::vector<std::pair<unsigned, unsigned>> MAKO_TRIGGER_LINE
		= { std::make_pair(3 - 1,4),std::make_pair(3 - 1,5) }; 
	/*the first is the label on the box minus 1, has minus'd 1 explicitly and this is not used in the code, just a reminder*/

#endif
/// Random other Constants
constexpr double PI = 3.14159265358979323846264338327950288;





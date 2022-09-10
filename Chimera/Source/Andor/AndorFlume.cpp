#include "stdafx.h"
#include "AndorFlume.h"

AndorFlume::AndorFlume ( bool safemode_option ) : safemode( safemode_option ){}

AndorFlume::~AndorFlume()
{
	if (!safemode) {
		andorErrorChecker(AT_Close(camHndl), true);
		andorErrorChecker(AT_FinaliseLibrary(), true);
	}
}


void AndorFlume::initialize ( ){
	char aBuffer[ 256 ];
	// Look in current working directory for driver files
	//... later... not sure what driver files this was referring to.
	GetCurrentDirectory ( 256, aBuffer );
	if ( !safemode ){
		andorErrorChecker ( Initialize ( aBuffer ) ); // for EMCCD
		// for SDK3
		andorErrorChecker(AT_InitialiseLibrary(), true);
		AT_64 iNumberDevices = 0;
		andorErrorChecker(AT_GetInt(AT_HANDLE_SYSTEM, L"Device Count", &iNumberDevices), true);
		if (iNumberDevices <= 0) {
			thrower("No cameras detected");
		}
		andorErrorChecker(AT_Open(0, &camHndl), true); //open first camera, almost always only have one andor connected
		setBool(L"SpuriousNoiseFilter", AT_FALSE);
	}

	
}


void AndorFlume::setBaselineClamp ( int clamp ){
	if ( !safemode ){
		andorErrorChecker ( SetBaselineClamp ( clamp ) );
	}
}

void AndorFlume::setBaselineOffset ( int offset ){
	if ( !safemode ){
		andorErrorChecker ( SetBaselineOffset ( offset ) );
	}
}

void AndorFlume::setDMAParameters ( int maxImagesPerDMA, float secondsPerDMA ){
	if ( !safemode ){
		andorErrorChecker ( SetDMAParameters ( maxImagesPerDMA, secondsPerDMA ) );
	}
}


void AndorFlume::waitForAcquisition ( ){
	if ( !safemode ){
		andorErrorChecker ( WaitForAcquisition ( ) );
	}
}

/* Special non-standard handling for this function because I always expect it to return something. I changed this on 
June 29th 2020 in order to remove a lot of the excess error notifications in the visual studio output window. */
int AndorFlume::getTemperature ( int& temp ){
	if ( !safemode ){
		return GetTemperature (&temp);
		//andorErrorChecker ( GetTemperature ( &temp ) );
	}
}

//
void AndorFlume::getAdjustedRingExposureTimes ( int size, float* timesArray ){
	if ( !safemode ){
		andorErrorChecker ( GetAdjustedRingExposureTimes ( size, timesArray ) );
	}
}


void AndorFlume::setNumberKinetics ( int number ){
	if ( !safemode ){
		andorErrorChecker ( SetNumberKinetics ( number ) );
	}
}


void AndorFlume::getTemperatureRange ( int& min, int& max ){
	if ( !safemode ){
		andorErrorChecker ( GetTemperatureRange ( &min, &max ) );
	}
}


void AndorFlume::temperatureControlOn ( ){
	if ( !safemode ){
		andorErrorChecker ( CoolerON ( ) );
	}
}

void AndorFlume::temperatureControlOff ( ){
	if ( !safemode ){
		andorErrorChecker ( CoolerOFF ( ) );
	}
}

void AndorFlume::setTemperature ( int temp ){
	if ( !safemode ){
		andorErrorChecker ( SetTemperature ( temp ) );
	}
}

void AndorFlume::setADChannel ( int channel ){
	if ( !safemode ){
		andorErrorChecker ( SetADChannel ( channel ) );
	}
}

void AndorFlume::setHSSpeed ( int type, int index ){
	if ( !safemode ){
		andorErrorChecker ( SetHSSpeed ( type, index ) );
	}
}

unsigned AndorFlume::getNumberVSSpeeds () {
	int num = 0;
	if (!safemode) {
		andorErrorChecker (GetNumberVSSpeeds (&num));
	}
	return num;
}


unsigned AndorFlume::getNumberHSSpeeds () {
	int num = 0;
	if (!safemode) {
		andorErrorChecker (GetNumberHSSpeeds (1,0,&num));
	}
	return num;
}

void AndorFlume::setVSSpeed (int index) {
	if (!safemode) {
		andorErrorChecker (SetVSSpeed (index));
	}
}

float AndorFlume::getVSSpeed (int index) {
	float speedVal=0;
	if (!safemode) {
		andorErrorChecker (GetVSSpeed (index, &speedVal));
	}
	return speedVal;
}

float AndorFlume::getHSSpeed (int channel, int type, int index) {
	float speedVal = 0;
	if (!safemode) {
		andorErrorChecker (GetHSSpeed (channel, type, index, &speedVal));
	}
	return speedVal;
}

// note that the function used here could be used to get actual information about the number of images, I just only use
// it to check whether there are any new images or not. Not sure if this is the smartest way to do this.
unsigned AndorFlume::checkForNewImages ( ){
	long first, last;
	unsigned res;
	if ( !safemode ){
		res = GetNumberNewImages (&first, &last);
		andorErrorChecker ( res );
	}
	return res;
	// don't do anything with the info.
}

void AndorFlume::getOldestImage ( Matrix<long>& dataMatrix ){
	if ( !safemode ){
		andorErrorChecker ( GetOldestImage ( dataMatrix.data.data ( ), dataMatrix.data.size ( ) ) );
	}
}

void AndorFlume::setTriggerMode ( int mode ){
	if ( !safemode ){
		andorErrorChecker ( SetTriggerMode ( mode ) );
	}
}

void AndorFlume::setAcquisitionMode ( int mode ){
	if ( !safemode ){
		andorErrorChecker ( SetAcquisitionMode ( mode ) );
	}
}

void AndorFlume::setReadMode ( int mode ){
	if ( !safemode ){
		andorErrorChecker ( SetReadMode ( mode ) );
	}
}

void AndorFlume::setRingExposureTimes ( int sizeOfTimesArray, float* arrayOfTimes ){
	if ( !safemode ){
		andorErrorChecker ( SetRingExposureTimes ( sizeOfTimesArray, arrayOfTimes ) );
	}
}

void AndorFlume::setImage ( int hBin, int vBin, int lBorder, int rBorder, int tBorder, int bBorder ){
	if ( !safemode ){
		//andorErrorChecker ( SetImage ( hBin, vBin, lBorder, rBorder, tBorder, bBorder ) );
		setInt(L"AOIHBin", hBin);
		setInt(L"AOIWidth", rBorder - lBorder + 1);
		setInt(L"AOILeft", lBorder);
		setInt(L"AOIVBin", vBin);
		setInt(L"AOIHeight", bBorder - tBorder + 1);
		setInt(L"AOITop", tBorder);
	}
}

void AndorFlume::setKineticCycleTime ( float cycleTime ){
	if ( !safemode ){
		andorErrorChecker ( SetKineticCycleTime ( cycleTime ) );
	}
}

void AndorFlume::setFrameTransferMode ( int mode ){
	if ( !safemode ){
		andorErrorChecker ( SetFrameTransferMode ( mode ) );
	}
}

void AndorFlume::getAcquisitionTimes ( float& exposure, float& accumulation, float& kinetic ){
	if ( !safemode ){
		andorErrorChecker ( GetAcquisitionTimings ( &exposure, &accumulation, &kinetic ) );
	}
}

int AndorFlume::queryStatus ( ){
	int status = DRV_IDLE;
	if ( !safemode ){
		andorErrorChecker ( GetStatus ( &status ) );
	}
	return status;
}


void AndorFlume::startAcquisition ( ){
	if ( !safemode ){
		//andorErrorChecker ( StartAcquisition ( ) );
		command(L"Acquisition Start");
	}
}

void AndorFlume::abortAcquisition ( ){
	if ( !safemode ){
		//andorErrorChecker ( AbortAcquisition ( ) );
		command(L"AcquisitionStop");
		flush();
	}
}

void AndorFlume::setAccumulationCycleTime ( float time ){
	if ( !safemode ){
		andorErrorChecker ( SetAccumulationCycleTime ( time ) );
	}
}

void AndorFlume::setAccumulationNumber ( int number ){
	if ( !safemode ){
		andorErrorChecker ( SetNumberAccumulations ( number ) );
	}
}

void AndorFlume::getNumberOfPreAmpGains ( int& number ){
	if ( !safemode ){
		andorErrorChecker ( GetNumberPreAmpGains ( &number ) );
	}
}

void AndorFlume::setPreAmpGain ( int index ){
	if ( !safemode ){
		andorErrorChecker ( SetPreAmpGain ( index ) );
	}
}

void AndorFlume::getPreAmpGain ( int index, float& gain ){
	if ( !safemode ){
		andorErrorChecker ( GetPreAmpGain ( index, &gain ) );
	}
}

void AndorFlume::setOutputAmplifier ( int type ){
	if ( !safemode ){
		andorErrorChecker ( SetOutputAmplifier ( type ) );
	}
}

void AndorFlume::setEmGainSettingsAdvanced ( int state ){
	if ( !safemode ){
		andorErrorChecker ( SetEMAdvanced ( state ) );
	}
}

void AndorFlume::setEmCcdGain ( int gain ){
	if ( !safemode ){
		andorErrorChecker ( SetEMCCDGain ( gain ) );
	}
}

/*
* the input here will store how many whole pictures (not accumulations) have been taken.
*/
void AndorFlume::getAcquisitionProgress ( long& seriesNumber ){
	if ( !safemode ){
		long dummyAccumulationNumber;
		andorErrorChecker ( GetAcquisitionProgress ( &dummyAccumulationNumber, &seriesNumber ) );
	}
}

/*
* overload to get both the acccumulation progress and the whole picture progress.
*/
void AndorFlume::getAcquisitionProgress ( long& accumulationNumber, long& seriesNumber ){
	if ( !safemode ){
		andorErrorChecker ( GetAcquisitionProgress ( &accumulationNumber, &seriesNumber ) );
	}
}

void AndorFlume::getCapabilities ( AndorCapabilities& caps ){
	if ( !safemode ){
		andorErrorChecker ( GetCapabilities ( &caps ) );
	}
}

void AndorFlume::getSerialNumber ( int& num ){
	if ( !safemode ){
		andorErrorChecker ( GetCameraSerialNumber ( &num ) );
	}
}

std::string AndorFlume::getHeadModel ( ){
	char nameChars[ 1024 ];
	if ( !safemode ){
		andorErrorChecker ( GetHeadModel ( nameChars ) );
	}
	else{
		return "safemode";
	}
	return str ( nameChars );
}

std::string AndorFlume::getErrorMsg (int errCode, bool SDK3) {
	std::string errorMessage = "uninitialized";
	if (!SDK3) {
		switch (errCode) {
		case 20001: errorMessage = "DRV_ERROR_CODES";					break;
		case 20002:	errorMessage = "DRV_SUCCESS";						break;
		case 20003:	errorMessage = "DRV_VXDNOTINSTALLED";				break;
		case 20004:	errorMessage = "DRV_ERROR_SCAN";					break;
		case 20005:	errorMessage = "DRV_ERROR_CHECK_SUM";				break;
		case 20006:	errorMessage = "DRV_ERROR_FILELOAD";				break;
		case 20007:	errorMessage = "DRV_UNKNOWN_FUNCTION";				break;
		case 20008:	errorMessage = "DRV_ERROR_VXD_INIT";				break;
		case 20009:	errorMessage = "DRV_ERROR_ADDRESS";					break;
		case 20010:	errorMessage = "DRV_ERROR_PAGELOCK";				break;
		case 20011:	errorMessage = "DRV_ERROR_PAGE_UNLOCK";				break;
		case 20012:	errorMessage = "DRV_ERROR_BOARDTEST";				break;
		case 20013:	errorMessage = "DRV_ERROR_ACK";						break;
		case 20014: errorMessage = "DRV_ERROR_UP_FIFO";					break;
		case 20015:	errorMessage = "DRV_ERROR_PATTERN";					break;
		case 20017:	errorMessage = "DRV_ACQUISITION_ERRORS";			break;
		case 20018:	errorMessage = "DRV_ACQ_BUFFER";					break;
		case 20019:	errorMessage = "DRV_ACQ_DOWNFIFO_FULL";				break;
		case 20020:	errorMessage = "DRV_PROC_UNKNOWN_INSTRUCTION";		break;
		case 20021:	errorMessage = "DRV_ILLEGAL_OP_CODE";				break;
		case 20022:	errorMessage = "DRV_KINETIC_TIME_NOT_MET";			break;
		case 20023:	errorMessage = "DRV_KINETIC_TIME_NOT_MET";			break;
		case 20024:	errorMessage = "DRV_NO_NEW_DATA";					break;
		case 20026:	errorMessage = "DRV_SPOOLERROR";					break;
		case 20033:	errorMessage = "DRV_TEMPERATURE_CODES";				break;
		case 20034:	errorMessage = "DRV_TEMPERATURE_OFF";				break;
		case 20035:	errorMessage = "DRV_TEMPERATURE_NOT_STABILIZED";	break;
		case 20036:	errorMessage = "DRV_TEMPERATURE_STABILIZED";		break;
		case 20037:	errorMessage = "DRV_TEMPERATURE_NOT_REACHED";		break;
		case 20038:	errorMessage = "DRV_TEMPERATURE_OUT_RANGE";			break;
		case 20039:	errorMessage = "DRV_TEMPERATURE_NOT_SUPPORTED";		break;
		case 20040:	errorMessage = "DRV_TEMPERATURE_DRIFT";				break;
		case 20049:	errorMessage = "DRV_GENERAL_ERRORS";				break;
		case 20050:	errorMessage = "DRV_INVALID_AUX";					break;
		case 20051:	errorMessage = "DRV_COF_NOTLOADED";					break;
		case 20052:	errorMessage = "DRV_FPGAPROG";						break;
		case 20053:	errorMessage = "DRV_FLEXERROR";						break;
		case 20054:	errorMessage = "DRV_GPIBERROR";						break;
		case 20064:	errorMessage = "DRV_DATATYPE";						break;
		case 20065:	errorMessage = "DRV_DRIVER_ERRORS";					break;
		case 20066:	errorMessage = "DRV_P1INVALID";						break;
		case 20067:	errorMessage = "DRV_P2INVALID";						break;
		case 20068:	errorMessage = "DRV_P3INVALID";						break;
		case 20069:	errorMessage = "DRV_P4INVALID";						break;
		case 20070:	errorMessage = "DRV_INIERROR";						break;
		case 20071:	errorMessage = "DRV_COFERROR";						break;
		case 20072:	errorMessage = "DRV_ACQUIRING";						break;
		case 20073:	errorMessage = "DRV_IDLE";							break;
		case 20074:	errorMessage = "DRV_TEMPCYCLE";						break;
		case 20075:	errorMessage = "DRV_NOT_INITIALIZED";				break;
		case 20076:	errorMessage = "DRV_P5INVALID";						break;
		case 20077:	errorMessage = "DRV_P6INVALID";						break;
		case 20078:	errorMessage = "DRV_INVALID_MODE";					break;
		case 20079:	errorMessage = "DRV_INVALID_FILTER";				break;
		case 20080:	errorMessage = "DRV_I2CERRORS";						break;
		case 20081:	errorMessage = "DRV_DRV_ICDEVNOTFOUND";				break;
		case 20082:	errorMessage = "DRV_I2CTIMEOUT";					break;
		case 20083:	errorMessage = "DRV_P7INVALID";						break;
		case 20089:	errorMessage = "DRV_USBERROR";						break;
		case 20090:	errorMessage = "DRV_IOCERROR";						break;
		case 20091:	errorMessage = "DRV_NOT_SUPPORTED";					break;
		case 20093:	errorMessage = "DRV_USB_INTERRUPT_ENDPOINT_ERROR";	break;
		case 20094:	errorMessage = "DRV_RANDOM_TRACK_ERROR";			break;
		case 20095:	errorMessage = "DRV_INVALID_tRIGGER_MODE";			break;
		case 20096:	errorMessage = "DRV_LOAD_FIRMWARE_ERROR";			break;
		case 20097:	errorMessage = "DRV_DIVIDE_BY_ZERO_ERROR";			break;
		case 20098:	errorMessage = "DRV_INVALID_RINGEXPOSURES";			break;
		case 20099:	errorMessage = "DRV_BINNING_ERROR";					break;
		case 20100:	errorMessage = "DRV_INVALID_AMPLIFIER";				break;
		case 20115:	errorMessage = "DRV_ERROR_MAP";						break;
		case 20116:	errorMessage = "DRV_ERROR_UNMAP";					break;
		case 20117:	errorMessage = "DRV_ERROR_MDL";						break;
		case 20118:	errorMessage = "DRV_ERROR_UNMDL";					break;
		case 20119:	errorMessage = "DRV_ERROR_BUFSIZE";					break;
		case 20121:	errorMessage = "DRV_ERROR_NOHANDLE";				break;
		case 20130:	errorMessage = "DRV_GATING_NOT_AVAILABLE";			break;

		case 20131:	errorMessage = "DRV_FPGA_VOLTAGE_ERROR";			break;
		case 20990:	errorMessage = "DRV_ERROR_NOCAMERA";				break;
		case 20991:	errorMessage = "DRV_NOT_SUPPORTED";					break;
		case 20992:	errorMessage = "DRV_NOT_AVAILABLE";					break;
		default: {
			errorMessage = "UNKNOWN ERROR MESSAGE RETURNED FROM CAMERA FUNCTION!";
			break;
		}
		}
	}
	else {
		switch (errCode) {
		case   0: errorMessage = "AT_SUCCESS";                        break;
		case   1: errorMessage = "AT_ERR_NOTINITIALISED";             break;
		case   2: errorMessage = "AT_ERR_NOTIMPLEMENTED";             break;
		case   3: errorMessage = "AT_ERR_READONLY";                   break;
		case   4: errorMessage = "AT_ERR_NOTREADABLE";                break;
		case   5: errorMessage = "AT_ERR_NOTWRITABLE";                break;
		case   6: errorMessage = "AT_ERR_OUTOFRANGE";                 break;
		case   7: errorMessage = "AT_ERR_INDEXNOTAVAILABLE";          break;
		case   8: errorMessage = "AT_ERR_INDEXNOTIMPLEMENTED";        break;
		case   9: errorMessage = "AT_ERR_EXCEEDEDMAXSTRINGLENGTH";    break;
		case  10: errorMessage = "AT_ERR_CONNECTION";                 break;
		case  11: errorMessage = "AT_ERR_NODATA";                     break;
		case  12: errorMessage = "AT_ERR_INVALIDHANDLE";              break;
		case  13: errorMessage = "AT_ERR_TIMEDOUT";                   break;
		case  14: errorMessage = "AT_ERR_BUFFERFULL";                 break;
		case  15: errorMessage = "AT_ERR_INVALIDSIZE";                break;
		case  16: errorMessage = "AT_ERR_INVALIDALIGNMENT";           break;
		case  17: errorMessage = "AT_ERR_COMM";                       break;
		case  18: errorMessage = "AT_ERR_STRINGNOTAVAILABLE";         break;
		case  19: errorMessage = "AT_ERR_STRINGNOTIMPLEMENTED";       break;
		case  20: errorMessage = "AT_ERR_NULL_FEATURE";               break;
		case  21: errorMessage = "AT_ERR_NULL_HANDLE";                break;
		case  22: errorMessage = "AT_ERR_NULL_IMPLEMENTED_VAR";       break;
		case  23: errorMessage = "AT_ERR_NULL_READABLE_VAR";          break;
		case  24: errorMessage = "AT_ERR_NULL_READONLY_VAR";          break;
		case  25: errorMessage = "AT_ERR_NULL_WRITABLE_VAR";          break;
		case  26: errorMessage = "AT_ERR_NULL_MINVALUE";              break;
		case  27: errorMessage = "AT_ERR_NULL_MAXVALUE";              break;
		case  28: errorMessage = "AT_ERR_NULL_VALUE";                 break;
		case  29: errorMessage = "AT_ERR_NULL_STRING";                break;
		case  30: errorMessage = "AT_ERR_NULL_COUNT_VAR";             break;
		case  31: errorMessage = "AT_ERR_NULL_ISAVAILABLE_VAR";       break;
		case  32: errorMessage = "AT_ERR_NULL_MAXSTRINGLENGTH";       break;
		case  33: errorMessage = "AT_ERR_NULL_EVCALLBACK";            break;
		case  34: errorMessage = "AT_ERR_NULL_QUEUE_PTR";             break;
		case  35: errorMessage = "AT_ERR_NULL_WAIT_PTR";              break;
		case  36: errorMessage = "AT_ERR_NULL_PTRSIZE";               break;
		case  37: errorMessage = "AT_ERR_NOMEMORY";                   break;
		case  38: errorMessage = "AT_ERR_DEVICEINUSE";                break;
		case  39: errorMessage = "AT_ERR_DEVICENOTFOUND";             break;
		case 100: errorMessage = "AT_ERR_HARDWARE_OVERFLOW";          break;
		default: {
			errorMessage = "UNKNOWN ERROR MESSAGE RETURNED FROM CAMERA FUNCTION!";
			break;
		}
		}
	}

	return errorMessage;
}

void AndorFlume::andorErrorChecker ( int errorCode, bool SDK3){
	auto errorMessage = getErrorMsg (errorCode, SDK3);
	/// So no throw is considered success.
	if ( (errorMessage != "DRV_SUCCESS") && (errorMessage != "AT_SUCCESS") ){
		thrower ( errorMessage );
	}
}

void AndorFlume::isImplemented(const AT_WC* Feature, AT_BOOL* Implemented) {
	if (!safemode) {
		andorErrorChecker(AT_IsImplemented(camHndl, Feature, Implemented), true);
	}
}

void AndorFlume::isReadable(const AT_WC* Feature, AT_BOOL* Readable) {
	if (!safemode) {
		andorErrorChecker(AT_IsReadable(camHndl, Feature, Readable), true);
	}
}

void AndorFlume::isWritable(const AT_WC* Feature, AT_BOOL* Writable) {
	if (!safemode) {
		andorErrorChecker(AT_IsWritable(camHndl, Feature, Writable), true);
	}
}

void AndorFlume::isReadOnly(const AT_WC* Feature, AT_BOOL* ReadOnly) {
	if (!safemode) {
		andorErrorChecker(AT_IsReadOnly(camHndl, Feature, ReadOnly), true);
	}
}

void AndorFlume::setInt(const AT_WC* Feature, AT_64 Value) {
	if (!safemode) {
		andorErrorChecker(AT_SetInt(camHndl, Feature, Value), true);
	}
}

void AndorFlume::getInt(const AT_WC* Feature, AT_64* Value) {
	if (!safemode) {
		andorErrorChecker(AT_GetInt(camHndl, Feature, Value), true);
	}
}

void AndorFlume::getIntMax(const AT_WC* Feature, AT_64* MaxValue) {
	if (!safemode) {
		andorErrorChecker(AT_GetIntMax(camHndl, Feature, MaxValue), true);
	}
}

void AndorFlume::getIntMin(const AT_WC* Feature, AT_64* MinValue) {
	if (!safemode) {
		andorErrorChecker(AT_GetIntMin(camHndl, Feature, MinValue), true);
	}
}

void AndorFlume::setFloat(const AT_WC* Feature, double Value) {
	if (!safemode) {
		andorErrorChecker(AT_SetFloat(camHndl, Feature, Value), true);
	}
}

void AndorFlume::getFloat(const AT_WC* Feature, double* Value) {
	if (!safemode) {
		andorErrorChecker(AT_GetFloat(camHndl, Feature, Value), true);
	}
}

void AndorFlume::getFloatMax(const AT_WC* Feature, double* MaxValue) {
	if (!safemode) {
		andorErrorChecker(AT_GetFloatMax(camHndl, Feature, MaxValue), true);
	}
}

void AndorFlume::getFloatMin(const AT_WC* Feature, double* MinValue) {
	if (!safemode) {
		andorErrorChecker(AT_GetFloatMin(camHndl, Feature, MinValue), true);
	}
}

void AndorFlume::setBool(const AT_WC* Feature, AT_BOOL Value) {
	if (!safemode) {
		andorErrorChecker(AT_SetBool(camHndl, Feature, Value), true);
	}
}

void AndorFlume::getBool(const AT_WC* Feature, AT_BOOL* Value) {
	if (!safemode) {
		andorErrorChecker(AT_GetBool(camHndl, Feature, Value), true);
	}
}

void AndorFlume::setEnumIndex(const AT_WC* Feature, int Value) {
	if (!safemode) {
		andorErrorChecker(AT_SetEnumIndex(camHndl, Feature, Value), true);
	}
}

void AndorFlume::setEnumString(const AT_WC* Feature, const AT_WC* String) {
	if (!safemode) {
		andorErrorChecker(AT_SetEnumString(camHndl, Feature, String), true);
	}
}

void AndorFlume::getEnumIndex(const AT_WC* Feature, int* Value) {
	if (!safemode) {
		andorErrorChecker(AT_GetEnumIndex(camHndl, Feature, Value), true);
	}
}

void AndorFlume::getEnumCount(const  AT_WC* Feature, int* Count) {
	if (!safemode) {
		andorErrorChecker(AT_GetEnumCount(camHndl, Feature, Count), true);
	}
}

void AndorFlume::isEnumIndexAvailable(const AT_WC* Feature, int Index, AT_BOOL* Available) {
	if (!safemode) {
		andorErrorChecker(AT_IsEnumIndexAvailable(camHndl, Feature, Index, Available), true);
	}
}

void AndorFlume::isEnumIndexImplemented(const AT_WC* Feature, int Index, AT_BOOL* Implemented) {
	if (!safemode) {
		andorErrorChecker(AT_IsEnumIndexImplemented(camHndl, Feature, Index, Implemented), true);
	}
}

void AndorFlume::getEnumStringByIndex(const AT_WC* Feature, int Index, AT_WC* String, int StringLength) {
	if (!safemode) {
		andorErrorChecker(AT_GetEnumStringByIndex(camHndl, Feature, Index, String, StringLength), true);
	}
}

void AndorFlume::command(const AT_WC* Feature) {
	if (!safemode) {
		andorErrorChecker(AT_Command(camHndl, Feature), true);
	}
}

void AndorFlume::setString(const AT_WC* Feature, const AT_WC* String) {
	if (!safemode) {
		andorErrorChecker(AT_SetString(camHndl, Feature, String), true);
	}
}

void AndorFlume::getString(const AT_WC* Feature, AT_WC* String, int StringLength) {
	if (!safemode) {
		andorErrorChecker(AT_GetString(camHndl, Feature, String, StringLength), true);
	}
}

void AndorFlume::getStringMaxLength(const AT_WC* Feature, int* MaxStringLength) {
	if (!safemode) {
		andorErrorChecker(AT_GetStringMaxLength(camHndl, Feature, MaxStringLength), true);
	}
}

void AndorFlume::queueBuffer(AT_U8* Ptr, int PtrSize) {
	if (!safemode) {
		andorErrorChecker(AT_QueueBuffer(camHndl, Ptr, PtrSize), true);
	}
}

void AndorFlume::waitBuffer(AT_U8** Ptr, int* PtrSize, unsigned int Timeout) {
	if (!safemode) {
		andorErrorChecker(AT_WaitBuffer(camHndl, Ptr, PtrSize, Timeout), true);
	}
}

void AndorFlume::flush() {
	if (!safemode) {
		andorErrorChecker(AT_Flush(camHndl), true);
	}
}

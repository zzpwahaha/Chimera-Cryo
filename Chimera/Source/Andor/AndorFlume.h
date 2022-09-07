#pragma once
#include "ATMCD32D.h"
#include "atcore.h"
#include <GeneralObjects/Matrix.h>

class AndorFlume{
	public:
		// THIS CLASS IS NOT COPYABLE.
		AndorFlume& operator=(const AndorFlume&) = delete;
		AndorFlume (const AndorFlume&) = delete;

		AndorFlume ( bool safemode_option );
		~AndorFlume();
		void initialize ( );
		void setVSSpeed (int index);
		unsigned getNumberVSSpeeds ();
		float getVSSpeed (int index);

		void setHSSpeed (int type, int index);
		unsigned getNumberHSSpeeds ();
		float getHSSpeed (int channel, int type, int index);


		void setBaselineClamp ( int clamp );
		void setBaselineOffset ( int offset );
		void setDMAParameters ( int maxImagesPerDMA, float secondsPerDMA );
		void waitForAcquisition ( );
		int getTemperature ( int& temp );
		std::string getErrorMsg (int errCode, bool SDK3 = false);
		void getAdjustedRingExposureTimes ( int size, float* timesArray );
		void setNumberKinetics ( int number );
		void getTemperatureRange ( int& min, int& max );
		void temperatureControlOn ( );
		void temperatureControlOff ( );
		void setTemperature ( int temp );
		void setADChannel ( int channel );
		unsigned checkForNewImages ( );
		void getOldestImage ( Matrix<long>& dataMatrix );
		void setTriggerMode ( int mode );
		void setAcquisitionMode ( int mode );
		void setReadMode ( int mode );
		void setRingExposureTimes ( int sizeOfTimesArray, float* arrayOfTimes );
		void setImage ( int hBin, int vBin, int lBorder, int rBorder, int tBorder, int bBorder );
		void setKineticCycleTime ( float cycleTime );
		void setFrameTransferMode ( int mode );
		void getAcquisitionTimes ( float& exposure, float& accumulation, float& kinetic );
		int queryStatus ( );
		void startAcquisition ( );
		void abortAcquisition ( );
		void setAccumulationCycleTime ( float time );
		void setAccumulationNumber ( int number );
		void getNumberOfPreAmpGains ( int& number );
		void setPreAmpGain ( int index );
		void getPreAmpGain ( int index, float& gain );
		void setOutputAmplifier ( int type );
		void setEmGainSettingsAdvanced ( int state );
		void setEmCcdGain ( int gain );
		void getAcquisitionProgress ( long& seriesNumber );
		void getAcquisitionProgress ( long& accumulationNumber, long& seriesNumber );
		void getCapabilities ( AndorCapabilities& caps );
		void getSerialNumber ( int& num );
		std::string getHeadModel ( );
		void andorErrorChecker ( int errorCode, bool SDK3 = false );

		void isImplemented(AT_WC* Feature, AT_BOOL* Implemented);
		void isReadOnly(AT_WC* Feature, AT_BOOL* ReadOnly);
		void isReadable(AT_WC* Feature, AT_BOOL* Readable);
		void isWritable(AT_WC* Feature, AT_BOOL* Writable);
		void setInt(AT_WC* Feature, AT_64 Value);
		void getInt(AT_WC* Feature, AT_64* Value);
		void getIntMax(AT_WC* Feature, AT_64* MaxValue);
		void getIntMin(AT_WC* Feature, AT_64* MinValue);
		void setFloat(AT_WC* Feature, double Value);
		void getFloat(AT_WC* Feature, double* Value);
		void getFloatMax(AT_WC* Feature, double* MaxValue);
		void getFloatMin(AT_WC* Feature, double* MinValue);
		void setBool(AT_WC* Feature, AT_BOOL Value);
		void getBool(AT_WC* Feature, AT_BOOL* Value);
		void setEnumIndex(AT_WC* Feature, int Value);
		void setEnumString(AT_WC* Feature, AT_WC* String);
		void getEnumIndex(AT_WC* Feature, int* Value);
		void getEnumCount(AT_WC* Feature, int* Count);
		void isEnumIndexAvailable(AT_WC* Feature, int Index, AT_BOOL* Available);
		void isEnumIndexImplemented(AT_WC* Feature, int Index, AT_BOOL* Implemented);
		void getEnumStringByIndex(AT_WC* Feature, int Index, AT_WC* String, int StringLength);
		void command(AT_WC* Feature);
		void setString(AT_WC* Feature, AT_WC* Value);
		void getString(AT_WC* Feature, AT_WC* Value, int StringLength);
		void getStringMaxLength(AT_WC* Feature, int* MaxStringLength);
		void queueBuffer(AT_U8* Ptr, int PtrSize);
		void waitBuffer(AT_U8** Ptr, int* PtrSize, unsigned int Timeout);
		void flush();
	private:
		const bool safemode;
		AT_H camHndl;
};

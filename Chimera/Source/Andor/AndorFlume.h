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

		//baisc wrapper of andor sdk3 funtionality with errorCheck and safemodeCheck
		void isImplemented            ( const AT_WC* Feature,  AT_BOOL* Implemented);
		void isReadable               ( const AT_WC* Feature,  AT_BOOL* Readable);
		void isWritable               ( const AT_WC* Feature,  AT_BOOL* Writable);
		void isReadOnly               ( const AT_WC* Feature,  AT_BOOL* ReadOnly);
		void setInt                   ( const AT_WC* Feature,  AT_64 Value);
		void getInt                   ( const AT_WC* Feature,  AT_64* Value);
		void getIntMax                ( const AT_WC* Feature,  AT_64* MaxValue);
		void getIntMin                ( const AT_WC* Feature,  AT_64* MinValue);
		void setFloat                 ( const AT_WC* Feature,  double Value);
		void getFloat                 ( const AT_WC* Feature,  double* Value);
		void getFloatMax              ( const AT_WC* Feature,  double* MaxValue);
		void getFloatMin              ( const AT_WC* Feature,  double* MinValue);
		void setBool                  ( const AT_WC* Feature,  AT_BOOL Value);
		void getBool                  ( const AT_WC* Feature,  AT_BOOL* Value);
		void setEnumIndex             ( const AT_WC* Feature,  int Value);
		void setEnumString            ( const AT_WC* Feature,  const AT_WC* String);
		void getEnumIndex             ( const AT_WC* Feature,  int* Value);
		void getEnumCount             (const  AT_WC* Feature,  int* Count);
		void isEnumIndexAvailable     ( const AT_WC* Feature,  int Index,  AT_BOOL* Available);
		void isEnumIndexImplemented   ( const AT_WC* Feature,  int Index,  AT_BOOL* Implemented);
		void getEnumStringByIndex     ( const AT_WC* Feature,  int Index,  AT_WC* String,  int StringLength);
		void command                  ( const AT_WC* Feature);
		void setString                ( const AT_WC* Feature,  const AT_WC* String);
		void getString                ( const AT_WC* Feature,  AT_WC* String,  int StringLength);
		void getStringMaxLength       ( const AT_WC* Feature,  int* MaxStringLength);
		void queueBuffer              ( AT_U8* Ptr,  int PtrSize);
		void waitBuffer               ( AT_U8** Ptr,  int* PtrSize,  unsigned int Timeout);
		void flush                    ();
	private:
		const bool safemode;
		AT_H camHndl;
};

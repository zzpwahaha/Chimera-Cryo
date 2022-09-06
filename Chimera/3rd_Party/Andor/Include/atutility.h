//---------------------------------------------------------------------------

#ifndef atutilityH
#define atutilityH
//---------------------------------------------------------------------------
#include "atcore.h"

#define AT_ERR_INVALIDOUTPUTPIXELENCODING 1002
#define AT_ERR_INVALIDINPUTPIXELENCODING 1003
#define AT_ERR_INVALIDMETADATAINFO 1004
#define AT_ERR_CORRUPTEDMETADATA 1005
#define AT_ERR_METADATANOTFOUND 1006

#define AT_ERR_INVALIDFORMAT 1008
#define AT_ERR_INVALIDPATH 1009
#define AT_ERR_NO_NEW_DATA 1010
#define AT_ERR_SPOOLING_NOT_CONFIGURED 1011


#ifdef __cplusplus
extern "C" {
#endif

int AT_EXP_CONV AT_ConvertBuffer(AT_U8* inputBuffer,
                                 AT_U8* outputBuffer,
                                 AT_64 width,
                                 AT_64 height,
                                 AT_64 stride,
                                 const AT_WC * inputPixelEncoding,
                                 const AT_WC * outputPixelEncoding);
int AT_EXP_CONV AT_ConvertBufferUsingMetadata(AT_U8* inputBuffer,
                                              AT_U8* outputBuffer,
                                              AT_64 imagesizebytes,
                                              const AT_WC * outputPixelEncoding);
int AT_EXP_CONV AT_GetWidthFromMetadata(AT_U8* inputBuffer, AT_64 imagesizebytes, AT_64& width);
int AT_EXP_CONV AT_GetHeightFromMetadata(AT_U8* inputBuffer, AT_64 imagesizebytes, AT_64& height);
int AT_EXP_CONV AT_GetStrideFromMetadata(AT_U8* inputBuffer, AT_64 imagesizebytes, AT_64& stride);
int AT_EXP_CONV AT_GetPixelEncodingFromMetadata(AT_U8* inputBuffer, AT_64 imagesizebytes, AT_WC* pixelEncoding, AT_U8 pixelEncodingSize);
int AT_EXP_CONV AT_GetTimeStampFromMetadata(AT_U8* inputBuffer, AT_64 imagesizebytes, AT_64& timeStamp);
int AT_EXP_CONV AT_GetIRIGFromMetadata(AT_U8* inputBuffer, AT_64 imagesizebytes, AT_64* seconds, AT_64* minutes, AT_64* hours, AT_64* days, AT_64* years);
int AT_EXP_CONV AT_GetExtendedIRIGFromMetadata(AT_U8* inputBuffer, AT_64 imagesizebytes, AT_64 clockfrequency, double* nanoseconds, AT_64* seconds, AT_64* minutes, AT_64* hours, AT_64* days, AT_64* years);

int AT_EXP_CONV AT_ConfigureSpooling(AT_H camera, const AT_WC* format, const AT_WC* path);
int AT_EXP_CONV AT_GetSpoolProgress(AT_H camera, int * imageNumber);
int AT_EXP_CONV AT_GetMostRecentImage(AT_H camera, AT_U8* buffer, int bufferSize);

int AT_EXP_CONV AT_InitialiseUtilityLibrary();
int AT_EXP_CONV AT_FinaliseUtilityLibrary();

#ifdef __cplusplus
}
#endif

#endif

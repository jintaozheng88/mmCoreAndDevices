// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the Mightex_SoftWLS_SDK_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// Mightex_SoftWLS_SDK_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.

#define MODULE_OASIS

typedef int (WINAPI *  MTUSB_SOFTWLSInitDevicesPtr) (void);
typedef int (WINAPI *  MTUSB_SOFTWLSOpenDevicePtr)(int DeviceIndexr);
typedef int (WINAPI *  MTUSB_SOFTWLSCloseDevicePtr) (int DevHandle);
typedef int (WINAPI *  MTUSB_SOFTWLSGetDeviceNamePtr) (int DevHandle, char* DevName, int Size);
typedef int (WINAPI *  MTUSB_SOFTWLSGetSerialNoPtr) (int DevHandle, char* SerNumber, int Size);
typedef int (WINAPI *  MTUSB_SOFTWLSGetExistingLEDsPtr) (int DevHandle);
typedef int (WINAPI *  MTUSB_SOFTWLSGetChannelTitlePtr) (int DevHandle, int Chl, char* ChnlTitle);
typedef int (WINAPI *  MTUSB_SOFTWLSGetChannelConfigPtr) (int DevHandle, int Chl, int ParaIndex);
typedef int (WINAPI *  MTUSB_SOFTWLSSetChannelPtr) (int DevHandle, int Channel);
typedef int (WINAPI *  MTUSB_SOFTWLSSetModePtr) (int DevHandle, int Mode);
typedef int (WINAPI *  MTUSB_SOFTWLSSetNormalCurrentPtr) (int DevHandle, int Current);
typedef int (WINAPI *  MTUSB_SOFTWLSSetStrobeRepeatPtr) (int DevHandle, int RepeatCnt);
typedef int (WINAPI *  MTUSB_SOFTWLSSetStrobeParaPtr) (int DevHandle, int Step, int Current, int Time);
typedef int (WINAPI *  MTUSB_SOFTWLSSetPulseParaPtr) (int DevHandle, int Step, int i1, int t1, int i2, int  t2, int i3, int t3);
typedef int (WINAPI *  MTUSB_SOFTWLSSoftStartPtr) (int DevHandle);
typedef int (WINAPI *  MTUSB_SOFTWLSSetHomePtr) (int DevHandle);
typedef int (WINAPI *  MTUSB_SOFTWLSEnableMotorPtr) (int DevHandle, int EnableMotor);
typedef int (WINAPI *  MTUSB_SOFTWLSGetFirmwareVersionPtr) (int DevHandle);
typedef int (WINAPI *  MTUSB_SOFTWLSStoreParaPtr) (int DevHandle);
typedef int (WINAPI *  MTUSB_SOFTWLSSendCommandPtr) (int DevHandle, char* CommandString);
typedef int (WINAPI *  MTUSB_SOFTWLSSendCommandWithoutResponsePtr) (int DevHandle, char* CommandString);
typedef int (WINAPI *  MTUSB_SOFTWLSSendCommandGetResponsePtr) (int DevHandle, char* CommandString, char* ResponseString, int Size);

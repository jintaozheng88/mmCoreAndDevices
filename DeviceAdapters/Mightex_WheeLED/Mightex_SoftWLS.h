///////////////////////////////////////////////////////////////////////////////
// FILE:          Mightex_SoftWLS.h
//-----------------------------------------------------------------------------
// DESCRIPTION:   Controls the Mightex wheel LEDs through a
//						serial or USB port. These devices are implemented as shutter devices,
//						although they are illumination devices. This makes the
//						synchronisation easier. So "Open" and "Close" means "On" or
//						"Off". "Fire" does nothing at all. All other commands are
//						realized as properties and differ from device to device.
//						Supported devices are:
//							+ Mightex WheeLED control module
//
// COPYRIGHT:     Mightex
// LICENSE:       LGPL
// VERSION:			1.0.1
// DATE:		2024-07-12
// AUTHOR:        Yihui wu
//

#ifndef _Mightex_SoftWLS_PLUGIN_H_
#define _Mightex_SoftWLS_PLUGIN_H_


#include "DeviceBase.h"

//////////////////////////////////////////////////////////////////////////////
// Error codes
//
#define ERR_PORT_CHANGE_FORBIDDEN	101
#define ERR_INVALID_DEVICE				102
#define ERR_Mightex_SoftWLS_OFFSET				120

/****************************************************************************
 class: 			DynError
 description:	This class represents one dynamic error which can be read from
 					the device and can be stored in an error vector.
****************************************************************************/
class DynError
{
public:
	DynError() {err = 0;};					// nothing to construct
	~DynError(){};								// nothing to destroy
	DynError(const DynError& oDynError) // copy constructor
	{
		err = oDynError.err;
		descr = oDynError.descr;
   }

public:
	int 			err;				// error number
	std::string descr;			// error description
};


/****************************************************************************
 class: 			Mightex_SoftWLS
 description:	The class Mightex_SoftWLS is derived from a shutter base interface and
					can be used for Mightex wheel LEDs devices.
****************************************************************************/

typedef enum
{
	CHL_DISABLE = 0,
	CHL_NORMAL,
	CHL_STROBE,
	CHL_EXTERNAL,
} eCHANNEL_MODE;

#define MODULE_WLS22	0
#define MODULE_WLS23M	1

struct pulse
{
	int timing[3];
	int current[3];
};

class Mightex_SoftWLS: public CShutterBase<Mightex_SoftWLS>
{
public:

	// constructor and destructor
	// ------------
	Mightex_SoftWLS();									// constructs a Mightex_SoftWLS device
	~Mightex_SoftWLS();									// destroys a Mightex_SoftWLS device

	// MMDevice API
	// ------------
	int Initialize();							// initialize a Mightex_SoftWLS device and creates the actions
	int Shutdown();							// sets the LED output to off in case the Mightex_SoftWLS was initialized

	// public functions
	// ------------
	void GetName(char* pszName) const;	// returns the device name
	bool Busy();								// returns true in case device is busy

	// Shutter API
	// ------------
	int SetOpen (bool open = true);
	int GetOpen(bool& open);
	int Fire(double deltaT);


	// action interface (work like callback/event functions)
	// -----------------------------------------------------
	int OnDevices(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnMode(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnChannel(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnSetNormalCurrent(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnSetRepeatCnt(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnSetI1(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnSetI2(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnSetI3(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnSetT1(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnSetT2(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnSetT3(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnSetPulse_Follow_Mode(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnSetION(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnSetIOFF(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnSetSoftStart(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnStatus(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
	int LEDOnOff(int);								// sets the LED output on(1) or off(0)
	int CreateStaticReadOnlyProperties(void);

private:
	bool dynErrlist_free		(void);
	bool dynErrlist_lookup	(int err, std::string* descr);
	bool dynErrlist_add		(int err, std::string descr);
	bool getLastError			(int* err);


private:
	std::string 	m_name;
	std::string 	m_LEDOn;
	std::string 	m_mode;
	std::string 	m_status;
	std::string 	m_serialNumber;
	bool 				m_busy;
	bool 				m_initialized;

	long m_channel;
	long m_repeatCnt;
	long m_normal_CurrentSet;
	long m_pulse_follow_mode;
	long m_iON;
	long m_iOFF;
	long m_isSoftStart;
	struct pulse m_pulse;

	int dev_num;
	int cur_dev;
	int devHandle;
	int devModuleType;
	long channels;
	long mode;

	std::vector<std::string> devNameList;

	// dynamic error list
	std::vector<DynError*>	m_dynErrlist;
};


#endif	// _Mightex_SoftWLS_PLUGIN_H_
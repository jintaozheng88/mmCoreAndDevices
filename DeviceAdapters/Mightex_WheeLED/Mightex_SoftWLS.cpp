///////////////////////////////////////////////////////////////////////////////
// FILE:          Mightex_SoftWLS.cpp
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


#include <windows.h>
#include "Mightex_SoftWLS.h"
#include "Mightex_SoftWLS_SDK_Stdcall.h"


MTUSB_SOFTWLSInitDevicesPtr MTUSB_SOFTWLSInitDevices;
MTUSB_SOFTWLSOpenDevicePtr MTUSB_SOFTWLSOpenDevice;
MTUSB_SOFTWLSCloseDevicePtr MTUSB_SOFTWLSCloseDevice;
MTUSB_SOFTWLSGetDeviceNamePtr MTUSB_SOFTWLSGetDeviceName;
MTUSB_SOFTWLSGetSerialNoPtr MTUSB_SOFTWLSGetSerialNo;
MTUSB_SOFTWLSGetExistingLEDsPtr MTUSB_SOFTWLSGetExistingLEDs;
MTUSB_SOFTWLSGetChannelTitlePtr MTUSB_SOFTWLSGetChannelTitle;
MTUSB_SOFTWLSGetChannelConfigPtr MTUSB_SOFTWLSGetChannelConfig;
MTUSB_SOFTWLSSetChannelPtr MTUSB_SOFTWLSSetChannel;
MTUSB_SOFTWLSSetModePtr MTUSB_SOFTWLSSetMode;
MTUSB_SOFTWLSSetNormalCurrentPtr MTUSB_SOFTWLSSetNormalCurrent;
MTUSB_SOFTWLSSetStrobeRepeatPtr MTUSB_SOFTWLSSetStrobeRepeat;
MTUSB_SOFTWLSSetStrobeParaPtr MTUSB_SOFTWLSSetStrobePara;
MTUSB_SOFTWLSSetPulseParaPtr MTUSB_SOFTWLSSetPulsePara;
MTUSB_SOFTWLSSoftStartPtr MTUSB_SOFTWLSSoftStart;
MTUSB_SOFTWLSSetHomePtr MTUSB_SOFTWLSSetHome;
MTUSB_SOFTWLSEnableMotorPtr MTUSB_SOFTWLSEnableMotor;
MTUSB_SOFTWLSGetFirmwareVersionPtr MTUSB_SOFTWLSGetFirmwareVersion;
MTUSB_SOFTWLSStoreParaPtr MTUSB_SOFTWLSStorePara;
MTUSB_SOFTWLSSendCommandPtr MTUSB_SOFTWLSSendCommand;
MTUSB_SOFTWLSSendCommandWithoutResponsePtr MTUSB_SOFTWLSSendCommandWithoutResponse;
MTUSB_SOFTWLSSendCommandGetResponsePtr MTUSB_SOFTWLSSendCommandGetResponse;
//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// GLOBALS
///////////////////////////////////////////////////////////////////////////////
//
const char* g_DeviceMightex_SoftWLSName = "Mightex_SoftWLS";
const char* g_DeviceMightex_SoftWLSDescription = "Mightex WheeLED control module";


///////////////////////////////////////////////////////////////////////////////
//
// Exported MMDevice API
//
///////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------
 Initialize module data. It publishes the Mightex wheel LEDs names to Manager.
---------------------------------------------------------------------------*/
MODULE_API void InitializeModuleData()
{
	RegisterDevice(g_DeviceMightex_SoftWLSName, MM::ShutterDevice, g_DeviceMightex_SoftWLSDescription);
}


/*---------------------------------------------------------------------------
 Creates and returns a device specified by the device name.
---------------------------------------------------------------------------*/
MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
	// no name, no device
	if(deviceName == 0)	return 0;

	if(strcmp(deviceName, g_DeviceMightex_SoftWLSName) == 0)
	{
		return new Mightex_SoftWLS;
	}

	return 0;
}


/*---------------------------------------------------------------------------
 Deletes a device pointed by pDevice.
---------------------------------------------------------------------------*/
MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}






/****************************************************************************

 class: 			Mightex_SoftWLS
 description:	The class Mightex_SoftWLS is derived from a shutter base interface and
					can be used for Mightex wheel LEDs devices.

****************************************************************************/
/*---------------------------------------------------------------------------
 Default constructor.
---------------------------------------------------------------------------*/
Mightex_SoftWLS::Mightex_SoftWLS() :
	dev_num(0),
	cur_dev(0),
	devHandle(-1),
	channels(-1),
	devModuleType(MODULE_WLS22),
	mode(CHL_DISABLE),
	m_pulse_follow_mode(0), //default pulse mode
	m_iON(50),
	m_iOFF(0),
	m_isSoftStart(0),
	m_repeatCnt(1),
	m_normal_CurrentSet(50),
	m_channel(1),
	m_name(""),
    m_LEDOn("Off"),
	m_mode("DISABLE"),
   m_status("No Fault"),
	m_serialNumber("n/a"),
   m_busy(false),
   m_initialized(false)
{
	InitializeDefaultErrorMessages();
	SetErrorText(ERR_PORT_CHANGE_FORBIDDEN, "You can't change the port after device has been initialized.");
	SetErrorText(ERR_INVALID_DEVICE, "The selected plugin does not fit for the device.");

	m_pulse.current[0] = 0;
	m_pulse.current[1] = 50;
	m_pulse.current[2] = 0;
	m_pulse.timing[0] = 500000;
	m_pulse.timing[1] = 500000;
	m_pulse.timing[2] = 500000;

	// Name
	CreateProperty(MM::g_Keyword_Name, g_DeviceMightex_SoftWLSName, MM::String, true);

	// Description
	CreateProperty(MM::g_Keyword_Description, g_DeviceMightex_SoftWLSDescription, MM::String, true);

	CPropertyAction* pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnDevices);
	CreateProperty("Devices", "", MM::String, false, pAct, true);

	AddAllowedValue( "Devices", ""); // no device yet

	HINSTANCE HDll = LoadLibraryA("Mightex_SoftWLS_SDK.dll");
	if (HDll)
	{
		MTUSB_SOFTWLSInitDevices = (MTUSB_SOFTWLSInitDevicesPtr)GetProcAddress(HDll, "MTUSB_SOFTWLSInitDevices");
		MTUSB_SOFTWLSOpenDevice = (MTUSB_SOFTWLSOpenDevicePtr)GetProcAddress(HDll, "MTUSB_SOFTWLSOpenDevice");
		MTUSB_SOFTWLSCloseDevice = (MTUSB_SOFTWLSCloseDevicePtr)GetProcAddress(HDll, "MTUSB_SOFTWLSCloseDevice");
		MTUSB_SOFTWLSGetDeviceName = (MTUSB_SOFTWLSGetDeviceNamePtr)GetProcAddress(HDll, "MTUSB_SOFTWLSGetDeviceName");
		MTUSB_SOFTWLSGetSerialNo = (MTUSB_SOFTWLSGetSerialNoPtr)GetProcAddress(HDll, "MTUSB_SOFTWLSGetSerialNo");
		MTUSB_SOFTWLSGetExistingLEDs = (MTUSB_SOFTWLSGetExistingLEDsPtr)GetProcAddress(HDll, "MTUSB_SOFTWLSGetExistingLEDs");
		MTUSB_SOFTWLSGetChannelTitle = (MTUSB_SOFTWLSGetChannelTitlePtr)GetProcAddress(HDll, "MTUSB_SOFTWLSGetChannelTitle");
		MTUSB_SOFTWLSGetChannelConfig = (MTUSB_SOFTWLSGetChannelConfigPtr)GetProcAddress(HDll, "MTUSB_SOFTWLSGetChannelConfig");
		MTUSB_SOFTWLSSetChannel = (MTUSB_SOFTWLSSetChannelPtr)GetProcAddress(HDll, "MTUSB_SOFTWLSSetChannel");
		MTUSB_SOFTWLSSetMode = (MTUSB_SOFTWLSSetModePtr)GetProcAddress(HDll, "MTUSB_SOFTWLSSetMode");
		MTUSB_SOFTWLSSetNormalCurrent = (MTUSB_SOFTWLSSetNormalCurrentPtr)GetProcAddress(HDll, "MTUSB_SOFTWLSSetNormalCurrent");
		MTUSB_SOFTWLSSetStrobeRepeat = (MTUSB_SOFTWLSSetStrobeRepeatPtr)GetProcAddress(HDll, "MTUSB_SOFTWLSSetStrobeRepeat");
		MTUSB_SOFTWLSSetStrobePara = (MTUSB_SOFTWLSSetStrobeParaPtr)GetProcAddress(HDll, "MTUSB_SOFTWLSSetStrobePara");
		MTUSB_SOFTWLSSetPulsePara = (MTUSB_SOFTWLSSetPulseParaPtr)GetProcAddress(HDll, "MTUSB_SOFTWLSSetPulsePara");
		MTUSB_SOFTWLSSoftStart = (MTUSB_SOFTWLSSoftStartPtr)GetProcAddress(HDll, "MTUSB_SOFTWLSSoftStart");
		MTUSB_SOFTWLSSetHome = (MTUSB_SOFTWLSSetHomePtr)GetProcAddress(HDll, "MTUSB_SOFTWLSSetHome");
		MTUSB_SOFTWLSEnableMotor = (MTUSB_SOFTWLSEnableMotorPtr)GetProcAddress(HDll, "MTUSB_SOFTWLSEnableMotor");
		MTUSB_SOFTWLSGetFirmwareVersion = (MTUSB_SOFTWLSGetFirmwareVersionPtr)GetProcAddress(HDll, "MTUSB_SOFTWLSGetFirmwareVersion");
		MTUSB_SOFTWLSStorePara = (MTUSB_SOFTWLSStoreParaPtr)GetProcAddress(HDll, "MTUSB_SOFTWLSStorePara");
		MTUSB_SOFTWLSSendCommand = (MTUSB_SOFTWLSSendCommandPtr)GetProcAddress(HDll, "MTUSB_SOFTWLSSendCommand");
		MTUSB_SOFTWLSSendCommandWithoutResponse = (MTUSB_SOFTWLSSendCommandWithoutResponsePtr)GetProcAddress(HDll, "MTUSB_SOFTWLSSendCommandWithoutResponse");
		MTUSB_SOFTWLSSendCommandGetResponse = (MTUSB_SOFTWLSSendCommandGetResponsePtr)GetProcAddress(HDll, "MTUSB_SOFTWLSSendCommandGetResponse");
	}
	else
		return;

	char ledName[64];
	char devName[32];
	char serialNum[32];
	int dev_Handle;
	std::string s_devName;
	dev_num = MTUSB_SOFTWLSInitDevices();
	for(int i = 0; i < dev_num; i++)
	{
		dev_Handle = MTUSB_SOFTWLSOpenDevice(i);
		if(dev_Handle >= 0)
			if(MTUSB_SOFTWLSGetDeviceName(dev_Handle, devName, sizeof(devName)) > 0)
				if(MTUSB_SOFTWLSGetSerialNo(dev_Handle, serialNum, sizeof(serialNum)) > 0)
				{
					sprintf(ledName, "%s:%s", devName, serialNum);
					AddAllowedValue( "Devices", ledName);
					s_devName = ledName;
					devNameList.push_back(s_devName);
				}
		MTUSB_SOFTWLSCloseDevice(dev_Handle);
	}
}


/*---------------------------------------------------------------------------
 Destructor.
---------------------------------------------------------------------------*/
Mightex_SoftWLS::~Mightex_SoftWLS()
{
	Shutdown();

	//HidUnInit();
}


/*---------------------------------------------------------------------------
 This function returns the device name ("Mightex_SoftWLS").
---------------------------------------------------------------------------*/
void Mightex_SoftWLS::GetName(char* Name) const
{
	CDeviceUtils::CopyLimitedString(Name, g_DeviceMightex_SoftWLSName);
}


/*---------------------------------------------------------------------------
 This function initialize a Mightex_SoftWLS device and creates the actions.
---------------------------------------------------------------------------*/
int Mightex_SoftWLS::Initialize()
{

	int nRet;

	if(dev_num > 0)
	{
		devHandle = MTUSB_SOFTWLSOpenDevice(cur_dev);
		if(devHandle >= 0)
		{
			channels = MTUSB_SOFTWLSGetExistingLEDs(devHandle);
			//devModuleType = MODULE_WLS22;
		}
	}

	// Initialize the Mode action
	CPropertyAction* pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnMode);
	nRet = CreateProperty("mode", "DISABLE", MM::String, false, pAct);
	if (DEVICE_OK != nRet)	return nRet;
	AddAllowedValue( "mode", "DISABLE");
	AddAllowedValue( "mode", "NORMAL");
	AddAllowedValue( "mode", "STROBE");
	AddAllowedValue( "mode", "EXTERNAL");

	pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnChannel);
	nRet = CreateProperty("channel", "1", MM::Integer, false, pAct);
	if (DEVICE_OK != nRet)	return nRet;
	if(channels > -1)
		SetPropertyLimits("channel", 1, channels);

	pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnSetNormalCurrent);
	nRet = CreateProperty("normal_CurrentSet", "50", MM::Integer, false, pAct);
	if (DEVICE_OK != nRet)	return nRet;
	SetPropertyLimits("normal_CurrentSet", 0, 1000);

	pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnSetRepeatCnt);
	nRet = CreateProperty("pulse_repeatCnt", "1", MM::Integer, false, pAct);
	if (DEVICE_OK != nRet)	return nRet;

	pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnSetI1);
	nRet = CreateProperty("pulse_i1", "0", MM::Integer, false, pAct);
	if (DEVICE_OK != nRet)	return nRet;
	SetPropertyLimits("pulse_i1", 0, 1000);

	pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnSetI2);
	nRet = CreateProperty("pulse_i2", "50", MM::Integer, false, pAct);
	if (DEVICE_OK != nRet)	return nRet;
	SetPropertyLimits("pulse_i2", 0, 1000);

	pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnSetI3);
	nRet = CreateProperty("pulse_i3", "0", MM::Integer, false, pAct);
	if (DEVICE_OK != nRet)	return nRet;
	SetPropertyLimits("pulse_i3", 0, 1000);

	pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnSetT1);
	nRet = CreateProperty("pulse_t1", "500000", MM::Integer, false, pAct);
	if (DEVICE_OK != nRet)	return nRet;

	pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnSetT2);
	nRet = CreateProperty("pulse_t2", "500000", MM::Integer, false, pAct);
	if (DEVICE_OK != nRet)	return nRet;

	pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnSetT3);
	nRet = CreateProperty("pulse_t3", "500000", MM::Integer, false, pAct);
	if (DEVICE_OK != nRet)	return nRet;

	//pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnSetPulse_Follow_Mode);
	//nRet = CreateProperty("pulse_follow_mode", "PULSE", MM::String, false, pAct);
	//if (DEVICE_OK != nRet)	return nRet;
	//AddAllowedValue( "pulse_follow_mode", "PULSE");
	//AddAllowedValue( "pulse_follow_mode", "FOLLOW");

	//pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnSetION);
	//nRet = CreateProperty("iON", "50", MM::Integer, false, pAct);
	//if (DEVICE_OK != nRet)	return nRet;
	//SetPropertyLimits("iON", 0, 1000);

	//pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnSetIOFF);
	//nRet = CreateProperty("iOFF", "0", MM::Integer, false, pAct);
	//if (DEVICE_OK != nRet)	return nRet;
	//SetPropertyLimits("iOFF", 0, 1000);

	pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnSetSoftStart);
	nRet = CreateProperty("softStart", "NO", MM::String, false, pAct);
	if (DEVICE_OK != nRet)	return nRet;
	AddAllowedValue( "softStart", "NO");
	AddAllowedValue( "softStart", "YES");

	// Initialize the status query action
	pAct = new CPropertyAction (this, &Mightex_SoftWLS::OnStatus);
	nRet = CreateProperty("Status", "No Fault", MM::String, true, pAct);
	if (DEVICE_OK != nRet)	return nRet;

	CreateStaticReadOnlyProperties();

	m_initialized = true;

	// init message
	std::ostringstream log;
	log << "Mightex_SoftWLS - initializied " << "S/N: " << m_serialNumber;
	LogMessage(log.str().c_str());

	return DEVICE_OK;
}


/*---------------------------------------------------------------------------
 This function sets the LED output to off in case the Mightex_SoftWLS was initialized.
---------------------------------------------------------------------------*/
int Mightex_SoftWLS::Shutdown()
{
   if (m_initialized)
   {
      LEDOnOff(false);
		if(devHandle >= 0)
			MTUSB_SOFTWLSCloseDevice(devHandle);
		channels = -1;
		devHandle = -1;
	 	m_initialized = false;
   }

   return DEVICE_OK;
}


/*---------------------------------------------------------------------------
 This function returns true in case device is busy.
---------------------------------------------------------------------------*/
bool Mightex_SoftWLS::Busy()
{
	return false;
}


/*---------------------------------------------------------------------------
 This function sets the LED output.
---------------------------------------------------------------------------*/
int Mightex_SoftWLS::SetOpen(bool open)
{
	int val = 0;

	(open) ? val = 1 : val = 0;
   return LEDOnOff(val);
}


/*---------------------------------------------------------------------------
 This function returns the LED output.
---------------------------------------------------------------------------*/
int Mightex_SoftWLS::GetOpen(bool &open)
{
	m_LEDOn = "Undefined";
	int ret = DEVICE_OK;

	if (m_LEDOn == "On")
	{
		open = false;
	}
	else
	{
		open = true;
	}

	return ret;
}


/*---------------------------------------------------------------------------
 This function does nothing but is recommended by the shutter API.
---------------------------------------------------------------------------*/
int Mightex_SoftWLS::Fire(double)
{
   return DEVICE_UNSUPPORTED_COMMAND;
}


/*---------------------------------------------------------------------------
 This function sets the LED output on or off.
---------------------------------------------------------------------------*/
int Mightex_SoftWLS::LEDOnOff(int onoff)
{
	int ret = DEVICE_OK;

	if (onoff == 0)
	{
		m_LEDOn = "Off";
		if(devHandle >= 0)
			MTUSB_SOFTWLSSetMode(devHandle, 0); //mode is "DISABLE"
	}
	else
	{
		m_LEDOn = "On";
		if(devHandle >= 0)
		{
			MTUSB_SOFTWLSSetMode(devHandle, mode);
			//if(mode == CHL_STROBE)
			//{
			//	if(m_pulse_follow_mode == 0)
			//	{
			//		MTUSB_BLSDriverSetPulseProfile(devHandle, m_channel, 1, 1, m_repeatCnt);
			//		MTUSB_BLSDriverSetPulseDetail(devHandle, m_channel, 
			//			0, m_pulse.timing[0], m_pulse.timing[1], m_pulse.timing[2], m_pulse.current[0], m_pulse.current[1], m_pulse.current[2]);
			//	}
			//	else
			//		MTUSB_BLSDriverSetFollowModeDetail(devHandle, m_channel, m_iON, m_iOFF);

			//	if(m_isSoftStart == 1)
			//		MTUSB_BLSDriverSoftStart(devHandle, m_channel);
			//}
		}
	}

	if (ret != DEVICE_OK) return ret;
	// error handling
	getLastError(&ret);

	return ret;
}


///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------
 This function reacts on device changes.
---------------------------------------------------------------------------*/

int Mightex_SoftWLS::OnDevices(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	std::string temp;
	if (eAct == MM::BeforeGet)
	{
		pProp->Set(m_name.c_str());
	}
	else if (eAct == MM::AfterSet)
	{
		if (m_initialized)
		{
			// revert
			pProp->Set(m_name.c_str());
			return ERR_PORT_CHANGE_FORBIDDEN;
		}

		pProp->Get(temp);
		if(temp == m_name || temp == "")
			return DEVICE_OK;
		else
		{
			int devIndex = 0;
			std::vector<std::string>::iterator ii;
			for( ii = devNameList.begin(); devNameList.end()!=ii; ++ii)
			{
				if(temp == *ii)
				{
					m_name = temp;
					cur_dev = devIndex;
				}
				devIndex++;
			}
		}

	}

	return DEVICE_OK;
}

int Mightex_SoftWLS::OnMode(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set(m_mode.c_str());
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(m_mode);
		if(m_mode == "DISABLE")
			mode = CHL_DISABLE;
		else if(m_mode == "NORMAL")
			mode = CHL_NORMAL;
		else if(m_mode == "STROBE")
			mode = CHL_STROBE;
		else if (m_mode == "EXTERNAL")
			mode = CHL_EXTERNAL;
		else
			mode = 0;
		if(devHandle >= 0)
			//if(mode < CHL_EXTERNAL)
				MTUSB_SOFTWLSSetMode(devHandle, mode);
	}

	return DEVICE_OK;
}

int Mightex_SoftWLS::OnChannel(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set(m_channel);
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(m_channel);
		if (devHandle >= 0)
			MTUSB_SOFTWLSSetChannel(devHandle, m_channel);
	}

	return DEVICE_OK;
}

int Mightex_SoftWLS::OnSetNormalCurrent(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	long temp;
	if (eAct == MM::BeforeGet)
	{
		pProp->Set(m_normal_CurrentSet);
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(temp);
		if(devHandle >= 0)
			if(MTUSB_SOFTWLSSetNormalCurrent(devHandle, temp) == 0)
				m_normal_CurrentSet = temp;
	}

	return DEVICE_OK;
}

int Mightex_SoftWLS::OnSetRepeatCnt(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set(m_repeatCnt);
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(m_repeatCnt);
		if (devHandle >= 0)
		{
			MTUSB_SOFTWLSSetStrobeRepeat(devHandle, m_repeatCnt);
			MTUSB_SOFTWLSSetPulsePara(devHandle, 1, m_pulse.current[0], m_pulse.timing[0], m_pulse.current[1], m_pulse.timing[1], m_pulse.current[2], m_pulse.timing[2]);
		}
	}

	return DEVICE_OK;
}

int Mightex_SoftWLS::OnSetI1(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	long temp;
	if (eAct == MM::BeforeGet)
	{
		temp = m_pulse.current[0];
		pProp->Set(temp);
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(temp);
		m_pulse.current[0] = temp;
	}

	return DEVICE_OK;
}

int Mightex_SoftWLS::OnSetI2(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	long temp;
	if (eAct == MM::BeforeGet)
	{
		temp = m_pulse.current[1];
		pProp->Set(temp);
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(temp);
		m_pulse.current[1] = temp;
	}

	return DEVICE_OK;
}

int Mightex_SoftWLS::OnSetI3(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	long temp;
	if (eAct == MM::BeforeGet)
	{
		temp = m_pulse.current[2];
		pProp->Set(temp);
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(temp);
		m_pulse.current[2] = temp;
	}

	return DEVICE_OK;
}

int Mightex_SoftWLS::OnSetT1(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	long temp;
	if (eAct == MM::BeforeGet)
	{
		temp = m_pulse.timing[0];
		pProp->Set(temp);
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(temp);
		m_pulse.timing[0] = temp;
	}

	return DEVICE_OK;
}

int Mightex_SoftWLS::OnSetT2(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	long temp;
	if (eAct == MM::BeforeGet)
	{
		temp = m_pulse.timing[1];
		pProp->Set(temp);
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(temp);
		m_pulse.timing[1] = temp;
	}

	return DEVICE_OK;
}

int Mightex_SoftWLS::OnSetT3(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	long temp;
	if (eAct == MM::BeforeGet)
	{
		temp = m_pulse.timing[2];
		pProp->Set(temp);
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(temp);
		m_pulse.timing[2] = temp;
	}

	return DEVICE_OK;
}

int Mightex_SoftWLS::OnSetPulse_Follow_Mode(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	std::string temp;
	if (eAct == MM::BeforeGet)
	{
		temp = (m_pulse_follow_mode==0)?"PULSE":"FOLLOW";
		pProp->Set(temp.c_str());
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(temp);
		if(temp == "PULSE")
			m_pulse_follow_mode = 0;
		else
			m_pulse_follow_mode = 1;
	}

	return DEVICE_OK;
}

int Mightex_SoftWLS::OnSetION(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set(m_iON);
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(m_iON);
	}

	return DEVICE_OK;
}

int Mightex_SoftWLS::OnSetIOFF(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set(m_iOFF);
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(m_iOFF);
	}

	return DEVICE_OK;
}

int Mightex_SoftWLS::OnSetSoftStart(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	std::string temp;
	if (eAct == MM::BeforeGet)
	{
		temp = (m_isSoftStart==0)?"NO":"YES";
		pProp->Set(temp.c_str());
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(temp);
		if(temp == "NO")
			m_isSoftStart = 0;
		else
		{
			m_isSoftStart = 1;
			if (devHandle >= 0)
				MTUSB_SOFTWLSSoftStart(devHandle);
		}
	}

	return DEVICE_OK;
}

/*---------------------------------------------------------------------------
 This function is the callback function for the "status" query.
---------------------------------------------------------------------------*/
int Mightex_SoftWLS::OnStatus(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	int ret = DEVICE_OK;

	if (eAct == MM::BeforeGet)
	{
		// clear string
		m_status.clear();
		if (m_LEDOn == "On")
			m_status = "LED open ";
		else
			m_status = "LED close ";

		pProp->Set(m_status.c_str());
	}

	return ret;
}

/*---------------------------------------------------------------------------
 This function clears the dynamic error list.
---------------------------------------------------------------------------*/
bool Mightex_SoftWLS::dynErrlist_free(void)
{
	for(unsigned int i = 0; i < m_dynErrlist.size(); i++)
	{
		delete m_dynErrlist[i];
	}

	m_dynErrlist.clear();

	return true;
}


/*---------------------------------------------------------------------------
 This function searches for specified error code within the dynamic error
 list. It returns TRUE(1) and the error string in case the error code was
 found. FALSE(0) otherwise.
---------------------------------------------------------------------------*/
bool Mightex_SoftWLS::dynErrlist_lookup(int err, std::string* descr)
{
	for(unsigned int i = 0; i < m_dynErrlist.size(); i++)
	{
		if(m_dynErrlist[i]->err == err)
		{
			if(descr)	*descr = m_dynErrlist[i]->descr;
			return true;
		}
	}

	return false;
}


/*---------------------------------------------------------------------------
 This function adds an errror and its description to list. In case the error
 is already known, it returns true.
---------------------------------------------------------------------------*/
bool Mightex_SoftWLS::dynErrlist_add(int err, std::string descr)
{
	// when the error is already in list return true
	if((dynErrlist_lookup(err, NULL)) == true)	return true;

	// add error to list
	DynError* dynError = new DynError();
	dynError->err = err;
	dynError->descr = descr;
	m_dynErrlist.push_back(dynError);

	// publish the new error to Manager
	SetErrorText((int)(dynError->err), descr.c_str());

	return true;
}


/*---------------------------------------------------------------------------
 This function requests the last error from the Mightex Sirius BLS USB device.
---------------------------------------------------------------------------*/
bool Mightex_SoftWLS::getLastError(int* err)
{
	std::string errDescr = "";
	int errCode = 0;

	// preset
	if(err) *err = 0;
	// check if there is no error
	if(errCode == 0)
	{
		if(err) *err = 0;
		return true;
	}
	// add error to list
	dynErrlist_add(errCode + ERR_Mightex_SoftWLS_OFFSET, errDescr);
	// publish the error code
	if(err) *err = errCode + ERR_Mightex_SoftWLS_OFFSET;
	// return
	return true;
}


/*---------------------------------------------------------------------------
 This function creates the static read only properties.
---------------------------------------------------------------------------*/
int Mightex_SoftWLS::CreateStaticReadOnlyProperties(void)
{
	int nRet = DEVICE_OK;
	char serialNum[32] = "";
	char devName[32] = "";

	if (devHandle >= 0)
		MTUSB_SOFTWLSGetDeviceName(devHandle, devName, sizeof(devName));
	m_name = devName;
	nRet = CreateProperty("Device Name", m_name.c_str(), MM::String, true);
	if (DEVICE_OK != nRet)	return nRet;

	if(devHandle >= 0)
		MTUSB_SOFTWLSGetSerialNo(devHandle, serialNum, sizeof(serialNum));
	m_serialNumber = serialNum;
	nRet = CreateProperty("Serial Number", m_serialNumber.c_str(), MM::String, true);
	if (DEVICE_OK != nRet)	return nRet;

	return nRet;
}

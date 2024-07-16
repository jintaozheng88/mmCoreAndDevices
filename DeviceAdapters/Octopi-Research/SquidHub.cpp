#include "squid.h"
#include "crc8.h"

#ifdef WIN32
   #define WIN32_LEAN_AND_MEAN
   #include <windows.h>
#endif


static const uint8_t CRC_TABLE[256] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
    0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
    0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
    0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
    0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
    0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
    0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
    0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
    0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
    0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
    0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
    0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
    0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
    0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
    0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
    0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
    0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};



const char* g_HubDeviceName = "SquidHub";


MODULE_API void InitializeModuleData() 
{
   RegisterDevice(g_HubDeviceName, MM::HubDevice, g_HubDeviceName);
   RegisterDevice(g_LEDShutterName, MM::ShutterDevice, "LEDs");
}


MODULE_API MM::Device* CreateDevice(const char* deviceName)
{

   if (strcmp(deviceName, g_HubDeviceName) == 0)
   {
      return new SquidHub();
   }
   else if (strcmp(deviceName, g_LEDShutterName) == 0)
   {
      return new SquidLEDShutter();
   }

   // ...supplied name not recognized
   return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}


SquidHub::SquidHub() :
   initialized_(false),
   monitoringThread_(0),
   port_("Undefined")
{
   InitializeDefaultErrorMessages();

   CPropertyAction* pAct = new CPropertyAction(this, &SquidHub::OnPort);
   CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);

}

SquidHub::~SquidHub()   
{
   LogMessage("Destructor called");
}

void SquidHub::GetName(char* name) const
{
   CDeviceUtils::CopyLimitedString(name, g_HubDeviceName);
}

int SquidHub::Initialize() {
   Sleep(200);

   monitoringThread_ = new SquidMonitoringThread(*this->GetCoreCallback(), *this, true);
   monitoringThread_->Start();
   
   const unsigned cmdSize = 8;
   unsigned char cmd[cmdSize];
   cmd[0] = 0x00;
   cmd[1] = 255; // CMD_SET.RESET
   for (unsigned i = 2; i < cmdSize; i++) {
      cmd[i] = 0;
   }
   int ret = SendCommand(cmd, cmdSize);
   if (ret != DEVICE_OK) {
      return ret;
   }

   cmd[0] = 1; 
   cmd[1] = 254; // CMD_INITIALIZE_DRIVERS
   ret = SendCommand(cmd, cmdSize);
   if (ret != DEVICE_OK) {
      return ret;
   }


   return DEVICE_OK;

   //SET_ILLUMINATION_LED_MATRIX = 13
   /*
   cmd[0] = 2; 
   cmd[1] = 13; 
   cmd[2] = 1;
   cmd[3] = 128;
   cmd[4] = 128;
   cmd[5] = 0;
   ret = SendCommand(cmd, cmdSize);
   if (ret != DEVICE_OK) {
      return ret;
   }

   cmd[0] = 0x03;
   cmd[1] = 10; // CMD_SET. TURN_ON_ILLUMINATION 
   for (unsigned i = 2; i < cmdSize; i++) {
      cmd[i] = 0;
   }
   ret = SendCommand(cmd, cmdSize);
   if (ret != DEVICE_OK) {
      return ret;
   }

   
   const unsigned msgLength = 24;
   unsigned char msg[msgLength];
   unsigned long read = 0;
   unsigned tries = 0;
   while (read == 0 && tries < 20) {
      Sleep(20);
      ReadFromComPort(port_.c_str(), msg, msgLength, read);
      tries++;
      if (read > 0) {
         LogMessage("Read something from serial port", false);
         std::ostringstream os;
         os << "Tries: " << tries << ", Read # of bytes: " << read;
         LogMessage(os.str().c_str(), false);
      }
   }
   if (tries >= 20) {
      LogMessage("Read nothing from serial port", false);
   }
   

   const unsigned TURN_ON_ILLUMINATION = 10;
   for (unsigned i = 0; i < cmdSize; i++) {
      cmd[i] = 0;
   }
   cmd[1] = TURN_ON_ILLUMINATION;
   ret = SendCommand(cmd, cmdSize);
   if (ret != DEVICE_OK) {
      return ret;
   }
   */
}

int SquidHub::Shutdown() {
   if (initialized_)
   {
      monitoringThread_->Stop();
      monitoringThread_->wait();
      delete(monitoringThread_);
      initialized_ = false;
   }
   return DEVICE_OK;
}

bool SquidHub::Busy()
{
    return false;
}

bool SquidHub::SupportsDeviceDetection(void)
{
   return false;  // can implement this later

}

MM::DeviceDetectionStatus SquidHub::DetectDevice(void)
{
   // TODO
   return MM::CanCommunicate;  
}


int SquidHub::DetectInstalledDevices()
{
   if (MM::CanCommunicate == DetectDevice())
   {
      std::vector<std::string> peripherals;
      peripherals.clear();
      peripherals.push_back(g_LEDShutterName);
      for (size_t i = 0; i < peripherals.size(); i++)
      {
         MM::Device* pDev = ::CreateDevice(peripherals[i].c_str());
         if (pDev)
         {
            AddInstalledDevice(pDev);
         }
      }
   }

   return DEVICE_OK;
}



int SquidHub::OnPort(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet) {
      pProp->Set(port_.c_str());
   }
   else if (eAct == MM::AfterSet) {
      if (initialized_) {
         // revert}
         pProp->Set(port_.c_str());
         return ERR_PORT_CHANGE_FORBIDDEN;
      }
      // take this port.  TODO: should we check if this is a valid port?
      pProp->Get(port_);
   }

   return DEVICE_OK;
}


uint8_t SquidHub::crc8ccitt(const void* data, size_t size) {
   uint8_t val = 0;

   uint8_t* pos = (uint8_t*)data;
   uint8_t* end = pos + size;

   while (pos < end) {
      val = CRC_TABLE[val ^ *pos];
      pos++;
   }

   return val;
}


int SquidHub::SendCommand(unsigned char* cmd, unsigned cmdSize)
{
   cmd[cmdSize - 1] = crc8ccitt(cmd, cmdSize - 1);
   if (true) {
      std::ostringstream os;
      os << "Sending message: ";
      for (unsigned int i = 0; i < cmdSize; i++) {
         os << std::hex << (unsigned int)cmd[i] << " ";
      }
      LogMessage(os.str().c_str(), false);
   }
   return WriteToComPort(port_.c_str(), cmd, cmdSize);
}
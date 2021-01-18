#include "Arduino.h"
#include "iplat_err_code.h"
#include "iplat_esp01.h"
#include "iplat_device.h"
#include "iplat_packet.h"

#define IPLAT_LIB_VER "v1.3"

class Ciplat {
  private:
    char* ip;
    int* port;

    CiplatESP01 * wifi;

    CiplatDevice * registeredDevice;
    int sensorNum = 0;
    
  public:
    Ciplat(uint32_t baud, int rx, int tx);
    Ciplat(uint32_t baud, int serialType);
    
    void welcome();
    
    int connect(
      char* SSID, 
      char* PASSWORD, 
      char* HOST_NAME, 
      int HOST_PORT, 
      CiplatDevice device);
                 
    void wait(int wtime);
    
    // Communication
    bool requestConnection(CiplatDevice device);

    // Fast mode(Only single sensor)
    //  payload : | Data |
    uint8_t sendDataF(int data); 

    // Byte mode(Supported multi sensor)
    //  payload : | SID | Data | ... |
    uint8_t sendDataB(char* sid1, int data1, char* sid2 = "", int data2 = 0, char* sid3 = "", int data3 = 0); 
    
    // Half-word mode(Supported multi sensor)
    //  payload : | SID | Data1 | Data2 | ... |
    uint8_t sendDataH(char* sid1, char* data1, char* sid2 = "", char* data2 = "", char* sid3 = "", char* data3 = ""); 
    
    // Word mode (Supported multi sensor)
    //  payload : | SID | Data1 | Data2 | Data3 | Data4 | ... |
    uint8_t sendDataW(char* sid1, char* data1, char* sid2 = "", char* data2 = "", char* sid3 = "", char* data3 = ""); 
      
    // Block mode (Only single sensor)
    //  payload : | SID | Data1 | Data2 | ... |
    uint8_t sendDataK(char* sid, char* data); 

    uint8_t isRegisteredSid(char* sid1, char* sid2, char* sid3);
    
    bool isAlive();

    void trace_err(uint8_t errCode);
};

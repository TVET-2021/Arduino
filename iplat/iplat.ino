#include "iplat.h"

#define DID "a881"   // Device1 ID
#define SID1 "cac1"  // Sensor1 ID
#define SPS1 1       // Sensor1 Sample Per Second
#define BPS1 1       // Sensor1 Resolution
#define SID2 "dbd5"  // Sensor1 ID
#define SPS2 1       // Sensor1 Sample Per Second
#define BPS2 1       // Sensor1 Resolution
#define SID3 "8ba0"  // Sensor1 ID
#define SPS3 1       // Sensor1 Sample Per Second
#define BPS3 1       // Sensor1 Resolution
#define SENSOR_NUM 1

#define WAIT_TIME 1000

Ciplat iplat(9600, IPLAT_ESP01_HS1); // Class Instantiation

void setup() {

    Serial.begin(9600); // Serial Monitor
    
    // CiplatDevice : iPLAT device manager
    //  - Parameter : DID, SID1, SPS1, BPS1, SID2, SPS2, BPS2, SID3, SPS3, BPS3
    CiplatDevice device(DID, SID1, SPS1, BPS1);
    
    int result = iplat.connect(
        "janglab", //"mylab",      // WiFi SSID
        "emsys2020", //"mypasswd",    // Passwd
        "192.168.0.37",  // iPLAT Server IP
        8895,           // Port Number
        device          // Device and sensor information
    );
    
    if (!result)  {
        while (true){
            Serial.print(">>> Failed to connect iPLAT, Reset Please");
            delay(5000);
        }
    }
}

void loop() {
    // ------------------------------------------------------------------------------- //
    // Data Packet - | Header(3B) | Payload(nB) |
    //  
    //  1. Header : | SOF(1B) | Packet Type(1B) | Data Length(1B) |
    //
    //  2. Payload
    //      - Fast mode      : uint8_t sendDataF(data)
    //                       : | Data |
    //
    //      - Byte mode      : uint8_t sendDataB(sid1, data1, sid2, data2, sid3, data3)
    //                       : | SID  | Data  | ...   |
    //
    //      - Half-word mode : uint8_t sendDataH(sid1, data1, sid2, data2, sid3, data3)
    //                       : | SID  | Data1 | Data2 | ...   | 
    //
    //      - Word mode      : uint8_t sendDataW(sid1, data1, sid2, data2, sid3, data3)
    //                       : | SID  | Data1 | Data2 | Data3 | Data4 | ... |
    //
    //      - Block mode     : uint8_t sendDataK(sid, data)
    //                       : | SID  | Data1 | Data2 | ... |
    // ------------------------------------------------------------------------------- //
    int value = random(300);
    uint8_t errCode = iplat.sendDataB(SID1, 64); // - OK
//      uint8_t errCode = iplat.sendDataB(SID1, 65, SID2, 33); // - OK
//    uint8_t errCode = iplat.sendDataB(SID1, 65, "1111", 54); // - OK
//    uint8_t errCode = iplat.sendDataH(SID1, "A1", SID2, "B2", SID3, "C3"); // - OK
//    uint8_t errCode = iplat.sendDataW(SID1, "A11AZZ", SID2, "B22B", SID3, "C33C");  // - OK
//    uint8_t errCode = iplat.sendDataK(SID1, "29.00"); // - OK
    iplat.trace_err(errCode);

    iplat.wait(WAIT_TIME);
}

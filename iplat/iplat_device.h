#include "Arduino.h"
#include "stdio.h"
#include "string.h"

#define MAX_SENSOR_NUM  3

typedef struct {
  char sid[5];  // Sensor ID + '\0'
  char sps;     // sample per seconds
  char bps;     // resolution
} iPlatSensor;

class CiplatDevice {
private:
    char did[5];
    iPlatSensor sensors[3];
    int sensorNum = 0;

public:
    CiplatDevice(
      char* _did, 
      char *sid1, char sps1, char bps1, 
      char *sid2="", char sps2=-1, char bps2=-1, 
      char *sid3="", char sps3=-1, char bps3=-1);
      
    int getSensorNum();
    char* getDid();
    char* getSensorList();
    bool isRegisteredSid(char *sid);
};

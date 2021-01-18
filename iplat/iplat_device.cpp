#include "iplat_device.h"

CiplatDevice::CiplatDevice(
      char* _did, 
      char *sid1, char sps1, char bps1, 
      char *sid2="", char sps2=-1, char bps2=-1, 
      char *sid3="", char sps3=-1, char bps3=-1){
    strncpy(did, _did, 4);
    
    iPlatSensor sensor;
    
    strcpy(sensor.sid, sid1);
    sensor.sps = sps1;
    sensor.bps = bps1;

    sensors[sensorNum++] = sensor;      
    
    if(strcmp(sid2, "")){
        strcpy(sensor.sid, sid2);
        sensor.sps = sps2;
        sensor.bps = bps2;

        sensors[sensorNum++] = sensor;
    }

    if(strcmp(sid3, "")){
        strcpy(sensor.sid, sid3);
        sensor.sps = sps3;
        sensor.bps = bps3;

        sensors[sensorNum++] = sensor;
    }
}
    
int CiplatDevice::getSensorNum(){  return sensorNum; }

char* CiplatDevice::getDid(){ return did; }
    
char* CiplatDevice::getSensorList(){
    // SID(4B) + SPS(1B) + BPS(1B)
    char* buffer = new char[sensorNum * 6 + 1];
    strcpy(buffer, ""); // 초기화

    for(int i = 0 ; i < sensorNum ; i++)
    {
        char ps[2];
        
        strncat(buffer, sensors[i].sid, 4);
        sprintf(ps, "%d%d", sensors[i].sps, sensors[i].bps);
        strncat(buffer, ps, 2);
    }
    return buffer;
}

bool CiplatDevice::isRegisteredSid(char *sid){
    int i = 0;
    bool isRegistered = false;
    
    for(i = 0 ; i < 3 ; i++){
        if(strcmp(sensors[i].sid, sid) == 0){
            isRegistered = true;
            break;
        }
    }
    
    if(isRegistered){
      
      Serial.print(strcmp(sensors[1].sid, sid));
      Serial.print(" ");
      Serial.println(sid);
      
    }
    
    return isRegistered;
}

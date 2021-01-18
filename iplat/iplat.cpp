#include "iplat.h"

Ciplat::Ciplat(uint32_t baud, int rx, int tx){
    wifi = new CiplatESP01(baud, rx, tx);
}

Ciplat::Ciplat(uint32_t baud, int serialType){
    wifi = new CiplatESP01(baud, serialType);
}

void Ciplat::welcome(){
    Serial.println();
    Serial.println();
    Serial.println("*****************************************************");
    Serial.print("\t\tiPLAT Sensor Node       ");
    Serial.print(IPLAT_LIB_VER);
    Serial.println("@506");
    Serial.println("*****************************************************");
}

int Ciplat::connect(char* SSID, char* PASSWORD, char* HOST_NAME, int HOST_PORT, CiplatDevice device){
    // v1.3
    // > : command
    // - : result
    // ! : error
    
    welcome();
    
    Serial.println(">> Start wifi module connection");
    Serial.println("> Restart wifi module");
    while(!wifi->restart());
    Serial.println("- Successfully restarted");
    
    Serial.println("> Setup module operation mode");
    if(!wifi->setOprToStationSoftAP()) {
        Serial.print("! Failed to setup");
        return IPLAT_ERR_SET_OPERATION_MODE;
    }

    Serial.print(">> Wifi AP Connection : ");
    Serial.println(SSID);
    if (!wifi->joinAP(SSID, PASSWORD)) {
        Serial.print("! Failed to connect to AP : ");
        Serial.println(SSID);
        return IPLAT_ERR_CONNECT_AP;
    } else {
        Serial.println("- Successfully connected (AP_IP, Device_IP)");
        String ip_list = wifi->getLocalIP().c_str();
        Serial.println(ip_list);  
    }
         
    Serial.println("> Disable multi-connection");
    if(!wifi->disableMUX()) {
        Serial.println("! Failed to disable");
        return IPLAT_ERR_DISABLE_MUX;
    }
  
    Serial.println(">> Start iPLAT connection");
    Serial.print("- Server IP : ");
    Serial.print(HOST_NAME);
    Serial.print("\tPort : ");
    Serial.println(HOST_PORT);
    if(!wifi->createTCP(HOST_NAME, (HOST_PORT))) {
        Serial.print("! Failed to connection");
        return IPLAT_ERR_CONNECT_SERVER;
    }
    
    bool result = requestConnection(device);
    if(!result) {
        Serial.println("! REQC packet sending failed");
        return IPLAT_ERR_REQC;
    } else {
        Serial.println("- Successful server connection");
    }
    Serial.println("*****************************************************");

    sensorNum = device.getSensorNum();
    
    registeredDevice = &device;
    
    return IPLAT_ERR_SUCCESS;
}

void Ciplat::wait(int wtime){
    delay(wtime);
}

// Communication
bool Ciplat::requestConnection(CiplatDevice device){
    // Start of frame | Packet Type | Data Length | DID | SID1 | SPS1 | BPS1 | ...
    
    if(device.getSensorNum() == 0){    
        return false;
    }
    char * did = device.getDid();
    char * sensorListBuffer = "";
    sensorListBuffer = device.getSensorList();
    
    int lenSensorsBuffer = strlen(sensorListBuffer);
  
    // HEADER(4B) + DID(4B) + SENSORS(nB)
    char packet[8 + lenSensorsBuffer] = {0, };
  
    packet[0] = IPLAT_PT_SOF;
    packet[1] = IPLAT_PT_REQC;
    packet[2] = 4 + lenSensorsBuffer;
    
    for(int i = 0 ; i < 4 ; i++)
        packet[i + 3] = did[i];
    
    for(int i = 0 ; i < lenSensorsBuffer ; i++)
        packet[i + 7] = sensorListBuffer[i];
    
    bool result = wifi->send(packet, strlen(packet));
  
    if(result == false) return false;
  
    uint8_t recv_data[6] = {0};
    
    uint32_t len = wifi->recv(recv_data, sizeof(recv_data), 3000);
    
    // Uncorrect packet
    if(recv_data[0] != IPLAT_PT_SOF)
        return false;
    
    // Not control packet
    if(recv_data[1] != IPLAT_PT_ACK && recv_data[1] != IPLAT_PT_NACK)
        return false;
    
    // TODO : Error Code
    if(recv_data[1] == IPLAT_PT_NACK)
        return false;
    
    return true;
}

uint8_t Ciplat::sendDataF(int data){
    if(sensorNum != 1)  return IPLAT_ERR_NOT_MATCHED_SENSOR_NUM;
        
    char packet[4];
  
    packet[0] = IPLAT_PT_SOF;
    packet[1] = IPLAT_PT_DATA_F;
    packet[2] = 1;
    packet[3] = data;
      
    bool res = wifi->send(packet, sizeof(packet) / sizeof(packet[0]));
  
    if(res)   return IPLAT_SUCCESS_SEND_DATA;
    else      return IPLAT_ERR_SEND_DATA;
}

uint8_t Ciplat::sendDataB(char* sid1, int data1, char* sid2, int data2, char* sid3, int data3){
    int currentSensorNum = 1;
    int lenPayload = 5; // SID1(4B) + DATA1(1B)
    if(strcmp(sid2, "")){ lenPayload += 5; currentSensorNum++;  }
    if(strcmp(sid3, "")){ lenPayload += 5; currentSensorNum++;  }
    
    if(currentSensorNum != sensorNum) return IPLAT_ERR_NOT_MATCHED_SENSOR_NUM;

    uint8_t errCode = isRegisteredSid(sid1, sid2, sid3);
    if(errCode != IPLAT_ERR_ALL_REGISTERED_SENSORS)
        return errCode;
    
    char packet[3 + lenPayload] = {0};  // Header(3B) + Payload
    packet[0] = IPLAT_PT_SOF;
    packet[1] = IPLAT_PT_DATA_B;
    packet[2] = lenPayload;

    strncat(packet, sid1, 4);
    packet[7] = data1;

    if(strcmp(sid2, "")){ strncat(packet, sid2, 4);  packet[12] = data2; }  // Add sid2, data2
    if(strcmp(sid3, "")){ strncat(packet, sid3, 4);  packet[17] = data3; }  // Add sid3, data3

    bool res = wifi->send(packet, sizeof(packet) / sizeof(packet[0]));

    if(res)   return IPLAT_SUCCESS_SEND_DATA;
    else      return IPLAT_ERR_SEND_DATA;
}

uint8_t Ciplat::sendDataH(char* sid1, char* data1, char* sid2, char* data2, char* sid3, char* data3){
    int currentSensorNum = 1;
    int lenPayload = 6; // SID1(4B) + DATA1(2B)
    if(strcmp(sid2, "")){ lenPayload += 6; currentSensorNum++;  }
    if(strcmp(sid3, "")){ lenPayload += 6; currentSensorNum++;  }

    if(currentSensorNum != sensorNum) 
        return IPLAT_ERR_NOT_MATCHED_SENSOR_NUM;
    
    char packet[3 + lenPayload] = {0};  // Header(3B) + Payload
    packet[0] = IPLAT_PT_SOF;
    packet[1] = IPLAT_PT_DATA_H;
    packet[2] = lenPayload;

    strncat(packet, sid1, 4);
    strncat(packet, data1, 2);

    if(strcmp(sid2, "")){ strncat(packet, sid2, 4);  strncat(packet, data2, 2); }  // Add sid2, data2
    if(strcmp(sid3, "")){ strncat(packet, sid3, 4);  strncat(packet, data3, 2); }  // Add sid3, data3
    
    bool res = wifi->send(packet, sizeof(packet) / sizeof(packet[0]));

    if(res)   return IPLAT_SUCCESS_SEND_DATA;
    else      return IPLAT_ERR_SEND_DATA;
}

uint8_t Ciplat::sendDataW(char* sid1, char* data1, char* sid2, char* data2, char* sid3, char* data3){
    int currentSensorNum = 1;
    int lenPayload = 8; // SID1(4B) + DATA1(4B)
    if(strcmp(sid2, "")){ lenPayload += 8; currentSensorNum++;  }
    if(strcmp(sid3, "")){ lenPayload += 8; currentSensorNum++;  }

    if(currentSensorNum != sensorNum) 
        return IPLAT_ERR_NOT_MATCHED_SENSOR_NUM;
    
    char packet[3 + lenPayload] = {0};  // Header(3B) + Payload
    packet[0] = IPLAT_PT_SOF;
    packet[1] = IPLAT_PT_DATA_W;
    packet[2] = lenPayload;

    strncat(packet, sid1, 4);
    strncat(packet, data1, 4);

    if(strcmp(sid2, "")){ strncat(packet, sid2, 4);  strncat(packet, data2, 4); }  // Add sid2, data2
    if(strcmp(sid3, "")){ strncat(packet, sid3, 4);  strncat(packet, data3, 4); }  // Add sid3, data3
    
    bool res = wifi->send(packet, sizeof(packet) / sizeof(packet[0]));

    if(res)   return IPLAT_SUCCESS_SEND_DATA;
    else      return IPLAT_ERR_SEND_DATA;
}

uint8_t Ciplat::sendDataK(char* sid, char* data){
    int currentSensorNum = 1;
    if(currentSensorNum != sensorNum) 
        return IPLAT_ERR_NOT_MATCHED_SENSOR_NUM;
    
    int lenData = strlen(data);
    int lenPayload = 4 + lenData; // SID(4B) + DATA(nB)

    char packet[3 + lenPayload] = {0};  // Header(3B) + Payload
    packet[0] = IPLAT_PT_SOF;
    packet[1] = IPLAT_PT_DATA_K;
    packet[2] = lenPayload;

    strncat(packet, sid, 4);
    strncat(packet, data, lenData);

    bool res = wifi->send(packet, sizeof(packet) / sizeof(packet[0]));

    if(res)   return IPLAT_SUCCESS_SEND_DATA;
    else      return IPLAT_ERR_SEND_DATA;
}

void Ciplat::trace_err(uint8_t errCode){
    switch(errCode){
        case IPLAT_ERR_SUCCESS:
        break;
        case IPLAT_ERR_SET_OPERATION_MODE:
        break;
        case IPLAT_ERR_CONNECT_AP:
        break;
        case IPLAT_ERR_DISABLE_MUX:
        break;
        case IPLAT_ERR_CONNECT_SERVER:
        break;
        case IPLAT_ERR_REQC:
        break;
        
        // ---------------------------------------------------------------- //
        // - Data
        case IPLAT_ERR_NOT_REGISTERED_SENSOR_1:
        case IPLAT_ERR_NOT_REGISTERED_SENSOR_2:
        case IPLAT_ERR_NOT_REGISTERED_SENSOR_3:
        Serial.println("! Do not send to unregistered sensor id");
        break;
        case IPLAT_ERR_NOT_MATCHED_SENSOR_NUM:
        Serial.println("! Sensor node and data are not the same length");
        break;
        case IPLAT_ERR_SEND_DATA:
        Serial.println("! Failed to send data");
        break;
        case IPLAT_SUCCESS_SEND_DATA:
        Serial.println("! Success to send data");
        break;
        // ---------------------------------------------------------------- //
        default:
        
        break;
    }
}

uint8_t Ciplat::isRegisteredSid(char* sid1, char* sid2, char* sid3){
    bool isRegistered1 = registeredDevice->isRegisteredSid(sid1);
    bool isRegistered2 = registeredDevice->isRegisteredSid(sid2);
    bool isRegistered3 = registeredDevice->isRegisteredSid(sid3);
    
    if(!isRegistered1 && strcmp(sid1, ""))
        return IPLAT_ERR_NOT_REGISTERED_SENSOR_1;
    else if(!isRegistered2 && strcmp(sid2, ""))
        return IPLAT_ERR_NOT_REGISTERED_SENSOR_2;
    else if(!isRegistered3 && strcmp(sid3, ""))
        return IPLAT_ERR_NOT_REGISTERED_SENSOR_3;
    else    
        return IPLAT_ERR_ALL_REGISTERED_SENSORS;
}

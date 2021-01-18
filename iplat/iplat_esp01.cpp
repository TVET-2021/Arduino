#include "iplat_esp01.h"

#define LOG_OUTPUT_DEBUG            (1)
#define LOG_OUTPUT_DEBUG_PREFIX     (1)

#define logDebug(arg)\
    do {\
        if (LOG_OUTPUT_DEBUG)\
        {\
            if (LOG_OUTPUT_DEBUG_PREFIX)\
            {\
                Serial.print("[LOG Debug: ");\
                Serial.print((const char*)__FILE__);\
                Serial.print(",");\
                Serial.print((unsigned int)__LINE__);\
                Serial.print(",");\
                Serial.print((const char*)__FUNCTION__);\
                Serial.print("] ");\
            }\
            Serial.print(arg);\
        }\
    } while(0)


CiplatESP01::CiplatESP01(uint32_t baud, int rx, int tx)
{
    // Using software serial
    SoftwareSerial serial(rx, tx);
    stream = &serial;
    serial.begin(baud);
    
    rx_empty();
}

CiplatESP01::CiplatESP01(uint32_t baud, int serialType)
{
    // Using hardware serial
    switch(serialType){
        case IPLAT_ESP01_HS0:
        stream = &Serial;
        Serial.begin(baud);
        break;
        case IPLAT_ESP01_HS1:
        stream = &Serial1;
        Serial1.begin(baud);
        break;
        case IPLAT_ESP01_HS2:
        stream = &Serial2;
        Serial2.begin(baud);
        break;
        case IPLAT_ESP01_HS3:
        stream = &Serial3;
        Serial3.begin(baud);
        break;
        default:
        stream = &Serial1;
        Serial1.begin(baud);
        break;
    }

    rx_empty();
}

bool CiplatESP01::kick(void){ return eAT(); }

bool CiplatESP01::restart(void)
{
    unsigned long start;
    if (eATRST()) {
        delay(2000);
        start = millis();
        while (millis() - start < 3000) {
            if (eAT()) {
                delay(1500); /* Waiting for stable */
                return true;
            }
            delay(100);
        }
    }
    return false;
}

String CiplatESP01::getVersion(void)
{
    String version;
    eATGMR(version);
    return version;
}

bool CiplatESP01::setOprToStation(void)
{
    uint8_t mode;
    if (!qATCWMODE(&mode)) {
        return false;
    }
    if (mode == 1) {
        return true;
    } else {
        if (sATCWMODE(1) && restart()) {
            return true;
        } else {
            return false;
        }
    }
}

bool CiplatESP01::setOprToSoftAP(void)
{
    uint8_t mode;
    if (!qATCWMODE(&mode)) {
        return false;
    }
    if (mode == 2) {
        return true;
    } else {
        if (sATCWMODE(2) && restart()) {
            return true;
        } else {
            return false;
        }
    }
}

bool CiplatESP01::setOprToStationSoftAP(void)
{
    uint8_t mode;
    if (!qATCWMODE(&mode)) {
        return false;
    }
    if (mode == 3) {
        return true;
    } else {
        if (sATCWMODE(3) && restart()) {
            return true;
        } else {
            return false;
        }
    }
}

String CiplatESP01::getAPList(void)
{
    String list;
    eATCWLAP(list);
    return list;
}

bool CiplatESP01::joinAP(String ssid, String pwd){  return sATCWJAP(ssid, pwd); }
bool CiplatESP01::enableClientDHCP(uint8_t mode, boolean enabled){  return sATCWDHCP(mode, enabled);  }
bool CiplatESP01::leaveAP(void) { return eATCWQAP();  }
bool CiplatESP01::setSoftAPParam(String ssid, String pwd, uint8_t chl, uint8_t ecn) { return sATCWSAP(ssid, pwd, chl, ecn); }

String CiplatESP01::getJoinedDeviceIP(void)
{
    String list;
    eATCWLIF(list);
    return list;
}

String CiplatESP01::getIPStatus(void)
{
    String list;
    eATCIPSTATUS(list);
    return list;
}

String CiplatESP01::getLocalIP(void)
{
    String list;
    eATCIFSR(list);
    return list;
}

bool CiplatESP01::enableMUX(void){  return sATCIPMUX(1);  }
bool CiplatESP01::disableMUX(void){ return sATCIPMUX(0);  }
bool CiplatESP01::createTCP(String addr, uint32_t port){  return sATCIPSTARTSingle("TCP", addr, port);  }
bool CiplatESP01::releaseTCP(void){ return eATCIPCLOSESingle(); }
bool CiplatESP01::createTCP(uint8_t mux_id, String addr, uint32_t port) { return sATCIPSTARTMultiple(mux_id, "TCP", addr, port);  }
bool CiplatESP01::releaseTCP(uint8_t mux_id)  { return sATCIPCLOSEMulitple(mux_id); }

bool CiplatESP01::send(const uint8_t *buffer, uint32_t len)
{
    return sATCIPSENDSingle(buffer, len);
}

bool CiplatESP01::send(uint8_t mux_id, const uint8_t *buffer, uint32_t len)
{
    return sATCIPSENDMultiple(mux_id, buffer, len);
}

uint32_t CiplatESP01::recv(uint8_t *buffer, uint32_t buffer_size, uint32_t timeout)
{
    return recvPkg(buffer, buffer_size, NULL, timeout, NULL);
}

uint32_t CiplatESP01::recv(uint8_t mux_id, uint8_t *buffer, uint32_t buffer_size, uint32_t timeout)
{
    uint8_t id;
    uint32_t ret;
    ret = recvPkg(buffer, buffer_size, NULL, timeout, &id);
    if (ret > 0 && id == mux_id) {
        return ret;
    }
    return 0;
}

uint32_t CiplatESP01::recv(uint8_t *coming_mux_id, uint8_t *buffer, uint32_t buffer_size, uint32_t timeout)
{
    return recvPkg(buffer, buffer_size, NULL, timeout, coming_mux_id);
}

/*----------------------------------------------------------------------------*/
/* +IPD,<id>,<len>:<data> */
/* +IPD,<len>:<data> */

uint32_t CiplatESP01::recvPkg(uint8_t *buffer, uint32_t buffer_size, uint32_t *data_len, uint32_t timeout, uint8_t *coming_mux_id)
{
    String data;
    char a;
    int32_t index_PIPDcomma = -1;
    int32_t index_colon = -1; /* : */
    int32_t index_comma = -1; /* , */
    int32_t len = -1;
    int8_t id = -1;
    bool has_data = false;
    uint32_t ret;
    unsigned long start;
    uint32_t i;
    
    if (buffer == NULL) {
        return 0;
    }
    
    start = millis();
    while (millis() - start < timeout) {
        if(stream->available() > 0) {
            a = stream->read();
            data += a;
        }
        
        index_PIPDcomma = data.indexOf("+IPD,");
        if (index_PIPDcomma != -1) {
            index_colon = data.indexOf(':', index_PIPDcomma + 5);
            if (index_colon != -1) {
                index_comma = data.indexOf(',', index_PIPDcomma + 5);
                /* +IPD,id,len:data */
                if (index_comma != -1 && index_comma < index_colon) { 
                    id = data.substring(index_PIPDcomma + 5, index_comma).toInt();
                    if (id < 0 || id > 4) {
                        return 0;
                    }
                    len = data.substring(index_comma + 1, index_colon).toInt();
                    if (len <= 0) {
                        return 0;
                    }
                } else { /* +IPD,len:data */
                    len = data.substring(index_PIPDcomma + 5, index_colon).toInt();
                    if (len <= 0) {
                        return 0;
                    }
                }
                has_data = true;
                break;
            }
        }
    }
    
    if (has_data) {
        i = 0;
        ret = len > buffer_size ? buffer_size : len;
        start = millis();
        while (millis() - start < 3000) {
            while(stream->available() > 0 && i < ret) {
                a = stream->read();
                buffer[i++] = a;
            }
            if (i == ret) {
                rx_empty();
                if (data_len) {
                    *data_len = len;    
                }
                if (index_comma != -1 && coming_mux_id) {
                    *coming_mux_id = id;
                }
                return ret;
            }
        }
    }
    return 0;
}

void CiplatESP01::rx_empty(void) 
{
    while(stream->available() > 0) {
        stream->read();
    }
}

String CiplatESP01::recvString(String target, uint32_t timeout)
{
    String data;
    char a;
    unsigned long start = millis();
    while (millis() - start < timeout) {
        while(stream->available() > 0) {
            a = stream->read();
      if(a == '\0') continue;
            data += a;
        }
        if (data.indexOf(target) != -1) {
            break;
        }   
    }
    return data;
}

String CiplatESP01::recvString(String target1, String target2, uint32_t timeout)
{
    String data;
    char a;
    unsigned long start = millis();
    while (millis() - start < timeout) {
        while(stream->available() > 0) {
            a = stream->read();
      if(a == '\0') continue;
            data += a;
        }
        if (data.indexOf(target1) != -1) {
            break;
        } else if (data.indexOf(target2) != -1) {
            break;
        }
    }
    return data;
}

String CiplatESP01::recvString(String target1, String target2, String target3, uint32_t timeout)
{
    String data;
    char a;
    unsigned long start = millis();
    while (millis() - start < timeout) {
        while(stream->available() > 0) {
            a = stream->read();
      if(a == '\0') continue;
            data += a;
        }
        if (data.indexOf(target1) != -1) {
            break;
        } else if (data.indexOf(target2) != -1) {
            break;
        } else if (data.indexOf(target3) != -1) {
            break;
        }
    }
    return data;
}

bool CiplatESP01::recvFind(String target, uint32_t timeout)
{
    String data_tmp;
    data_tmp = recvString(target, timeout);
    if (data_tmp.indexOf(target) != -1) {
        return true;
    }
    return false;
}

bool CiplatESP01::recvFindAndFilter(String target, String begin, String end, String &data, uint32_t timeout)
{
    String data_tmp;
    data_tmp = recvString(target, timeout);
    if (data_tmp.indexOf(target) != -1) {
        int32_t index1 = data_tmp.indexOf(begin);
        int32_t index2 = data_tmp.indexOf(end);
        if (index1 != -1 && index2 != -1) {
            index1 += begin.length();
            data = data_tmp.substring(index1, index2);
            return true;
        }
    }
    data = "";
    return false;
}

bool CiplatESP01::eAT(void)
{
    rx_empty();
    stream->println("AT");
    return recvFind("OK");
}

bool CiplatESP01::eATRST(void) 
{
    rx_empty();
    stream->println("AT+RST");
    return recvFind("OK");
}

bool CiplatESP01::eATGMR(String &version)
{
    rx_empty();
    stream->println("AT+GMR");
    return recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", version); 
}

bool CiplatESP01::qATCWMODE(uint8_t *mode) 
{
    String str_mode;
    bool ret;
    if (!mode) {
        return false;
    }
    rx_empty();
    stream->println("AT+CWMODE?");
    ret = recvFindAndFilter("OK", "+CWMODE:", "\r\n\r\nOK", str_mode); 
    if (ret) {
        *mode = (uint8_t)str_mode.toInt();
        return true;
    } else {
        return false;
    }
}

bool CiplatESP01::sATCWMODE(uint8_t mode)
{
    String data;
    rx_empty();
    stream->print("AT+CWMODE=");
    stream->println(mode);
    
    data = recvString("OK", "no change");
    if (data.indexOf("OK") != -1 || data.indexOf("no change") != -1) {
        return true;
    }
    return false;
}

bool CiplatESP01::sATCWJAP(String ssid, String pwd)
{
    String data;
    rx_empty();
    stream->print("AT+CWJAP=\"");
    stream->print(ssid);
    stream->print("\",\"");
    stream->print(pwd);
    stream->println("\"");
    
    data = recvString("OK", "FAIL", 10000);
    if (data.indexOf("OK") != -1) {
        return true;
    }
    return false;
}

bool CiplatESP01::sATCWDHCP(uint8_t mode, boolean enabled)
{
  String strEn = "0";
  if (enabled) {
    strEn = "1";
  }
  
  
    String data;
    rx_empty();
    stream->print("AT+CWDHCP=");
    stream->print(strEn);
    stream->print(",");
    stream->println(mode);
    
    data = recvString("OK", "FAIL", 10000);
    if (data.indexOf("OK") != -1) {
        return true;
    }
    return false;
}

bool CiplatESP01::eATCWLAP(String &list)
{
    String data;
    rx_empty();
    stream->println("AT+CWLAP");
    return recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", list, 10000);
}

bool CiplatESP01::eATCWQAP(void)
{
    String data;
    rx_empty();
    stream->println("AT+CWQAP");
    return recvFind("OK");
}

bool CiplatESP01::sATCWSAP(String ssid, String pwd, uint8_t chl, uint8_t ecn)
{
    String data;
    rx_empty();
    stream->print("AT+CWSAP=\"");
    stream->print(ssid);
    stream->print("\",\"");
    stream->print(pwd);
    stream->print("\",");
    stream->print(chl);
    stream->print(",");
    stream->println(ecn);
    
    data = recvString("OK", "ERROR", 5000);
    if (data.indexOf("OK") != -1) {
        return true;
    }
    return false;
}

bool CiplatESP01::eATCWLIF(String &list)
{
    String data;
    rx_empty();
    stream->println("AT+CWLIF");
    return recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", list);
}
bool CiplatESP01::eATCIPSTATUS(String &list)
{
    String data;
    delay(100);
    rx_empty();
    stream->println("AT+CIPSTATUS");
    return recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", list);
}
bool CiplatESP01::sATCIPSTARTSingle(String type, String addr, uint32_t port)
{
    String data;
    rx_empty();
    stream->print("AT+CIPSTART=\"");
    stream->print(type);
    stream->print("\",\"");
    stream->print(addr);
    stream->print("\",");
    stream->println(port);
    
    data = recvString("OK", "ERROR", "ALREADY CONNECT", 10000);
    if (data.indexOf("OK") != -1 || data.indexOf("ALREADY CONNECT") != -1) {
        return true;
    }
    return false;
}
bool CiplatESP01::sATCIPSTARTMultiple(uint8_t mux_id, String type, String addr, uint32_t port)
{
    String data;
    rx_empty();
    stream->print("AT+CIPSTART=");
    stream->print(mux_id);
    stream->print(",\"");
    stream->print(type);
    stream->print("\",\"");
    stream->print(addr);
    stream->print("\",");
    stream->println(port);
    
    data = recvString("OK", "ERROR", "ALREADY CONNECT", 10000);
    if (data.indexOf("OK") != -1 || data.indexOf("ALREADY CONNECT") != -1) {
        return true;
    }
    return false;
}
bool CiplatESP01::sATCIPSENDSingle(const uint8_t *buffer, uint32_t len)
{
    rx_empty();
    stream->print("AT+CIPSEND=");
    stream->println(len);
    if (recvFind(">", 5000)) {
        rx_empty();
        for (uint32_t i = 0; i < len; i++) {
            stream->write(buffer[i]);
        }
        return recvFind("SEND OK", 10000);
    }
    return false;
}
bool CiplatESP01::sATCIPSENDMultiple(uint8_t mux_id, const uint8_t *buffer, uint32_t len)
{
    rx_empty();
    stream->print("AT+CIPSEND=");
    stream->print(mux_id);
    stream->print(",");
    stream->println(len);
    if (recvFind(">", 5000)) {
        rx_empty();
        for (uint32_t i = 0; i < len; i++) {
            stream->write(buffer[i]);
        }
        return recvFind("SEND OK", 10000);
    }
    return false;
}
bool CiplatESP01::sATCIPCLOSEMulitple(uint8_t mux_id)
{
    String data;
    rx_empty();
    stream->print("AT+CIPCLOSE=");
    stream->println(mux_id);
    
    data = recvString("OK", "link is not", 5000);
    if (data.indexOf("OK") != -1 || data.indexOf("link is not") != -1) {
        return true;
    }
    return false;
}
bool CiplatESP01::eATCIPCLOSESingle(void)
{
    rx_empty();
    stream->println("AT+CIPCLOSE");
    return recvFind("OK", 5000);
}
bool CiplatESP01::eATCIFSR(String &list)
{
    rx_empty();
    stream->println("AT+CIFSR");
    return recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", list);
}
bool CiplatESP01::sATCIPMUX(uint8_t mode)
{
    String data;
    rx_empty();
    stream->print("AT+CIPMUX=");
    stream->println(mode);
    
    data = recvString("OK", "Link is builded");
    if (data.indexOf("OK") != -1) {
        return true;
    }
    return false;
}
bool CiplatESP01::sATCIPSERVER(uint8_t mode, uint32_t port)
{
    String data;
    if (mode) {
        rx_empty();
        stream->print("AT+CIPSERVER=1,");
        stream->println(port);
        
        data = recvString("OK", "no change");
        if (data.indexOf("OK") != -1 || data.indexOf("no change") != -1) {
            return true;
        }
        return false;
    } else {
        rx_empty();
        stream->println("AT+CIPSERVER=0");
        return recvFind("\r\r\n");
    }
}
bool CiplatESP01::sATCIPSTO(uint32_t timeout)
{
    rx_empty();
    stream->print("AT+CIPSTO=");
    stream->println(timeout);
    return recvFind("OK");
}

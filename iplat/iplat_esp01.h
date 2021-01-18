#include "Arduino.h"
#include "SoftwareSerial.h"

#define IPLAT_ESP01_SS -1   // Software Serial
#define IPLAT_ESP01_HS0 0   // Hardware Serial 0
#define IPLAT_ESP01_HS1 1   // Hardware Serial 1
#define IPLAT_ESP01_HS2 2   // Hardware Serial 2
#define IPLAT_ESP01_HS3 3   // Hardware Serial 3

class CiplatESP01 {
private:
public:
  Stream * stream;

  CiplatESP01(uint32_t baud, int rx, int tx);
  CiplatESP01(uint32_t baud, int serialType);

  
  bool kick(void);
  bool restart(void);
  String getVersion(void);
  bool setOprToStation(void);
  bool setOprToSoftAP(void);
  bool setOprToStationSoftAP(void);
  String getAPList(void);
  bool joinAP(String ssid, String pwd);
  bool enableClientDHCP(uint8_t mode, boolean enabled);
  bool leaveAP(void);
  bool setSoftAPParam(String ssid, String pwd, uint8_t chl = 7, uint8_t ecn = 4);
  String getJoinedDeviceIP(void);
  String getIPStatus(void);
  String getLocalIP(void);
  bool enableMUX(void);
  bool disableMUX(void);
  bool createTCP(String addr, uint32_t port);
  bool releaseTCP(void);
  bool createTCP(uint8_t mux_id, String addr, uint32_t port);
  bool releaseTCP(uint8_t mux_id);
  
  bool send(const uint8_t *buffer, uint32_t len);
  bool send(uint8_t mux_id, const uint8_t *buffer, uint32_t len);
  
  uint32_t recv(uint8_t *buffer, uint32_t buffer_size, uint32_t timeout = 1000);
  uint32_t recv(uint8_t mux_id, uint8_t *buffer, uint32_t buffer_size, uint32_t timeout = 1000);
  uint32_t recv(uint8_t *coming_mux_id, uint8_t *buffer, uint32_t buffer_size, uint32_t timeout = 1000);

private:
  void rx_empty(void);
  String recvString(String target, uint32_t timeout = 1000);
  String recvString(String target1, String target2, uint32_t timeout = 1000);
  String recvString(String target1, String target2, String target3, uint32_t timeout = 1000);
  
  bool recvFind(String target, uint32_t timeout = 1000);
  bool recvFindAndFilter(String target, String begin, String end, String &data, uint32_t timeout = 1000);
  uint32_t recvPkg(uint8_t *buffer, uint32_t buffer_size, uint32_t *data_len, uint32_t timeout, uint8_t *coming_mux_id);
  
  
  bool eAT(void);
  bool eATRST(void);
  bool eATGMR(String &version);
  
  bool qATCWMODE(uint8_t *mode);
  bool sATCWMODE(uint8_t mode);
  bool sATCWJAP(String ssid, String pwd);
  bool sATCWDHCP(uint8_t mode, boolean enabled);
  bool eATCWLAP(String &list);
  bool eATCWQAP(void);
  bool sATCWSAP(String ssid, String pwd, uint8_t chl, uint8_t ecn);
  bool eATCWLIF(String &list);
  
  bool eATCIPSTATUS(String &list);
  bool sATCIPSTARTSingle(String type, String addr, uint32_t port);
  bool sATCIPSTARTMultiple(uint8_t mux_id, String type, String addr, uint32_t port);
  bool sATCIPSENDSingle(const uint8_t *buffer, uint32_t len);
  bool sATCIPSENDMultiple(uint8_t mux_id, const uint8_t *buffer, uint32_t len);
  bool sATCIPCLOSEMulitple(uint8_t mux_id);
  bool eATCIPCLOSESingle(void);
  bool eATCIFSR(String &list);
  bool sATCIPMUX(uint8_t mode);
  bool sATCIPSERVER(uint8_t mode, uint32_t port = 333);
  bool sATCIPSTO(uint32_t timeout);
};

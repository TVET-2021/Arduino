// Start of frame
#define IPLAT_PT_SOF 0xAA

// Request a connection
#define IPLAT_PT_REQC 0x10

// Control
#define IPLAT_PT_ACK 0xF0
#define IPLAT_PT_NACK 0xF1

// Data
#define IPLAT_PT_DATA_F 0x30 // Fast mode, Only single sensor
#define IPLAT_PT_DATA_B 0x31 // Byte mode, multi sensor
#define IPLAT_PT_DATA_H 0x32 // Half-word mode, multi sensor
#define IPLAT_PT_DATA_W 0x34 // Word mode, multi sensor
#define IPLAT_PT_DATA_K 0x3F // Block mode, Only single sensor

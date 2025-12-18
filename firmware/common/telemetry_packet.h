#pragma once

#include <stdint.h>

// Binary packet v1 for ESP-NOW transport between nodes and gateway.
// Packed to avoid padding differences across compilers.
typedef struct __attribute__((packed)) {
    uint8_t  version;        // = 1
    uint8_t  node_id;        // optional small ID (0..255)
    uint8_t  mac[6];         // STA MAC of node
    uint32_t seq;            // monotonically increasing sequence

    int16_t  distance_cm;    // measured distance sensor->water
    int16_t  level_cm;       // computed water level
    uint8_t  percentual;     // 0..100
    uint32_t volume_l;       // liters
    int16_t  vin_mv;         // supply voltage in mV (optional)

    // Anomaly detection flags
    uint8_t  flags;          // Bit 0: is_alert, Bit 1-7: reserved
    uint8_t  alert_type;     // 0=none, 1=rapid_drop, 2=rapid_rise, 3=sensor_stuck

    // Fields set/overwritten by the gateway upon reception
    int8_t   rssi;           // RSSI if available, else 0
    uint32_t ts_ms;          // gateway timestamp in ms
} SensorPacketV1;

#define SENSOR_PACKET_VERSION 1

// Packet flags
#define FLAG_IS_ALERT  0x01  // Bit 0: Anomaly alert triggered

// Alert types
#define ALERT_NONE         0
#define ALERT_RAPID_DROP   1  // Leak detected (rapid water level drop)
#define ALERT_RAPID_RISE   2  // Pump failure / flood (rapid water level rise)
#define ALERT_SENSOR_STUCK 3  // Sensor reading unchanged for long period

// ACK packet sent from gateway to node confirming receipt
typedef struct __attribute__((packed)) {
    uint8_t  magic;          // 0xAC (magic byte for validation)
    uint8_t  version;        // = 1
    uint8_t  node_id;        // Node ID being acknowledged
    uint32_t ack_seq;        // Sequence number being acknowledged
    int8_t   rssi;           // RSSI measured by gateway
    uint8_t  status;         // 0=OK, 1=parsed but not sent, 2=error
    uint8_t  gateway_id;     // Which gateway sent this ACK (0-2)
    uint8_t  reserved;       // Future use
} AckPacket;

#define ACK_MAGIC 0xAC
#define ACK_VERSION 1
#define ACK_STATUS_OK 0
#define ACK_STATUS_QUEUED 1
#define ACK_STATUS_ERROR 2

// ============================================================================
// AGUADA ULTRASONIC 01 - Ultra-minimal telemetry packet
// ============================================================================
// Design philosophy:
// - Node sends ONLY: node_id + distance_cm (7 bytes core data)
// - Server maintains node configuration table (MAC, offsets, tank geometry)
// - Server performs all calculations (level, percentage, volume)
// - Gateway adds: RSSI + timestamp (6 bytes metadata)
// - Total: 13 bytes (vs 28 bytes in SensorPacketV1)
// - 54% size reduction, ideal for high-frequency sampling or battery operation

typedef struct __attribute__((packed)) {
    // Header (identification)
    uint8_t  magic;          // 0xA1 (magic byte for aguadaUltrasonic01)
    uint8_t  version;        // = 1
    uint8_t  node_id;        // Node identifier (0-255)
    
    // Core telemetry (node-provided)
    int16_t  distance_cm;    // Raw ultrasonic distance measurement
    
    // Optional fields for future compatibility (reserved)
    uint8_t  flags;          // Bit 0: low_battery, Bit 1: sensor_error, Bit 2-7: reserved
    uint8_t  reserved;       // Future use
    
    // Gateway-populated fields (added upon reception)
    int8_t   rssi;           // Signal strength (dBm)
    uint32_t ts_ms;          // Gateway timestamp (milliseconds since epoch or boot)
} aguadaUltrasonic01Packet;

#define AGUADA_ULTRA01_MAGIC   0xA1
#define AGUADA_ULTRA01_VERSION 1

// Flags for aguadaUltrasonic01Packet
#define ULTRA01_FLAG_LOW_BATTERY  0x01  // Bit 0: Battery below threshold
#define ULTRA01_FLAG_SENSOR_ERROR 0x02  // Bit 1: Sensor reading failed/timeout
#define ULTRA01_FLAG_RESERVED2    0x04  // Bit 2: Reserved
#define ULTRA01_FLAG_RESERVED3    0x08  // Bit 3: Reserved

// Server-side configuration structure (NOT transmitted, stored in database)
// Each node_id maps to this configuration for server-side processing
typedef struct {
    uint8_t  node_id;
    uint8_t  mac[6];             // Node's MAC address (for reference/debugging)
    char     location[32];       // Human-readable location (e.g., "Reservoir A")
    
    // Tank geometry parameters
    int16_t  sensor_offset_cm;   // Distance from sensor to full level (e.g., 20cm)
    int16_t  level_max_cm;       // Maximum water level in tank (e.g., 450cm)
    uint32_t vol_max_l;          // Tank capacity in liters (e.g., 80000L)
    
    // Calibration
    int16_t  distance_offset;    // Calibration offset for sensor (cm)
    
    // Anomaly detection thresholds (server-side)
    int16_t  rapid_change_threshold_cm;  // Alert if |delta| > threshold
    uint16_t no_change_minutes;          // Alert if no change for X minutes
} NodeConfig;

// Server-side processing functions (pseudo-code, implemented in backend)
// inline int16_t calculate_level_cm(int16_t distance_cm, const NodeConfig *cfg) {
//     int16_t adjusted = distance_cm + cfg->distance_offset;
//     int16_t level = cfg->level_max_cm + cfg->sensor_offset_cm - adjusted;
//     return (level < 0) ? 0 : ((level > cfg->level_max_cm) ? cfg->level_max_cm : level);
// }
//
// inline uint8_t calculate_percentual(int16_t level_cm, const NodeConfig *cfg) {
//     return (uint8_t)((level_cm * 100) / cfg->level_max_cm);
// }
//
// inline uint32_t calculate_volume_l(int16_t level_cm, const NodeConfig *cfg) {
//     return (uint32_t)(((int64_t)level_cm * (int64_t)cfg->vol_max_l) / (int64_t)cfg->level_max_cm);
// }

// ============================================================================
// GENERIC DATA PACKET - Variable length key-value pairs
// ============================================================================

// Data types for generic values
typedef enum {
    DATA_TYPE_INT8    = 0x01,  // 1 byte signed integer
    DATA_TYPE_UINT8   = 0x02,  // 1 byte unsigned integer
    DATA_TYPE_INT16   = 0x03,  // 2 bytes signed integer
    DATA_TYPE_UINT16  = 0x04,  // 2 bytes unsigned integer
    DATA_TYPE_INT32   = 0x05,  // 4 bytes signed integer
    DATA_TYPE_UINT32  = 0x06,  // 4 bytes unsigned integer
    DATA_TYPE_FLOAT   = 0x07,  // 4 bytes float
    DATA_TYPE_BOOL    = 0x08,  // 1 byte boolean (0/1)
    DATA_TYPE_STRING  = 0x09   // Variable length null-terminated string
} DataType;

// Key-value pair (variable size)
// Layout: [label_len:1][label:label_len][type:1][value_len:1][value:value_len]
typedef struct __attribute__((packed)) {
    uint8_t  label_len;      // Length of label string (max 31)
    char     label[31];      // Label string (not null-terminated to save space)
    uint8_t  type;           // DataType enum
    uint8_t  value_len;      // Length of value in bytes
    uint8_t  value[32];      // Value bytes (interpreted based on type)
} DataPair;

// Generic packet header + variable data
#define MAX_DATA_PAIRS 10
#define GENERIC_PACKET_VERSION 2

typedef struct __attribute__((packed)) {
    uint8_t  magic;          // 0xDA (magic byte for validation)
    uint8_t  version;        // = 2 (GENERIC_PACKET_VERSION)
    uint8_t  node_id;        // Node ID
    uint8_t  mac[6];         // STA MAC of node
    uint32_t seq;            // Sequence number
    uint8_t  pair_count;     // Number of key-value pairs (0-10)
    
    // Variable data section follows (not included in struct, handled dynamically)
    // Format: [pair1][pair2]...[pairN]
    // Each pair: [label_len:1][label:N][type:1][value_len:1][value:M]
    
    // Gateway adds these at the end
    int8_t   rssi;           // RSSI (filled by gateway)
    uint32_t ts_ms;          // Timestamp (filled by gateway)
} GenericPacketHeader;

#define GENERIC_PACKET_MAGIC 0xDA

// Helper macros for calculating sizes
#define DATA_PAIR_HEADER_SIZE 3  // label_len + type + value_len
#define GENERIC_HEADER_SIZE (sizeof(GenericPacketHeader))
#define MAX_GENERIC_PACKET_SIZE 250  // ESP-NOW limit is 250 bytes

// Helper functions for building generic packets (inline for header-only)

// Calculate size of a data pair
static inline uint16_t data_pair_size(const char* label, uint8_t value_len) {
    uint8_t label_len = strlen(label);
    if (label_len > 31) label_len = 31;
    return 1 + label_len + 1 + 1 + value_len;  // label_len + label + type + value_len + value
}

// Add int32 pair to buffer
static inline uint16_t add_int32_pair(uint8_t* buffer, const char* label, int32_t value) {
    uint8_t label_len = strlen(label);
    if (label_len > 31) label_len = 31;
    
    uint16_t offset = 0;
    buffer[offset++] = label_len;
    memcpy(&buffer[offset], label, label_len);
    offset += label_len;
    buffer[offset++] = DATA_TYPE_INT32;
    buffer[offset++] = 4;  // value_len
    memcpy(&buffer[offset], &value, 4);
    offset += 4;
    
    return offset;
}

// Add uint32 pair to buffer
static inline uint16_t add_uint32_pair(uint8_t* buffer, const char* label, uint32_t value) {
    uint8_t label_len = strlen(label);
    if (label_len > 31) label_len = 31;
    
    uint16_t offset = 0;
    buffer[offset++] = label_len;
    memcpy(&buffer[offset], label, label_len);
    offset += label_len;
    buffer[offset++] = DATA_TYPE_UINT32;
    buffer[offset++] = 4;
    memcpy(&buffer[offset], &value, 4);
    offset += 4;
    
    return offset;
}

// Add float pair to buffer
static inline uint16_t add_float_pair(uint8_t* buffer, const char* label, float value) {
    uint8_t label_len = strlen(label);
    if (label_len > 31) label_len = 31;
    
    uint16_t offset = 0;
    buffer[offset++] = label_len;
    memcpy(&buffer[offset], label, label_len);
    offset += label_len;
    buffer[offset++] = DATA_TYPE_FLOAT;
    buffer[offset++] = 4;
    memcpy(&buffer[offset], &value, 4);
    offset += 4;
    
    return offset;
}

// Add int16 pair to buffer
static inline uint16_t add_int16_pair(uint8_t* buffer, const char* label, int16_t value) {
    uint8_t label_len = strlen(label);
    if (label_len > 31) label_len = 31;
    
    uint16_t offset = 0;
    buffer[offset++] = label_len;
    memcpy(&buffer[offset], label, label_len);
    offset += label_len;
    buffer[offset++] = DATA_TYPE_INT16;
    buffer[offset++] = 2;
    memcpy(&buffer[offset], &value, 2);
    offset += 2;
    
    return offset;
}

// Add uint8 pair to buffer
static inline uint16_t add_uint8_pair(uint8_t* buffer, const char* label, uint8_t value) {
    uint8_t label_len = strlen(label);
    if (label_len > 31) label_len = 31;
    
    uint16_t offset = 0;
    buffer[offset++] = label_len;
    memcpy(&buffer[offset], label, label_len);
    offset += label_len;
    buffer[offset++] = DATA_TYPE_UINT8;
    buffer[offset++] = 1;
    buffer[offset++] = value;
    
    return offset;
}

// Add bool pair to buffer
static inline uint16_t add_bool_pair(uint8_t* buffer, const char* label, bool value) {
    uint8_t label_len = strlen(label);
    if (label_len > 31) label_len = 31;
    
    uint16_t offset = 0;
    buffer[offset++] = label_len;
    memcpy(&buffer[offset], label, label_len);
    offset += label_len;
    buffer[offset++] = DATA_TYPE_BOOL;
    buffer[offset++] = 1;
    buffer[offset++] = value ? 1 : 0;
    
    return offset;
}

// Add string pair to buffer
static inline uint16_t add_string_pair(uint8_t* buffer, const char* label, const char* value) {
    uint8_t label_len = strlen(label);
    if (label_len > 31) label_len = 31;
    
    uint8_t value_len = strlen(value);
    if (value_len > 31) value_len = 31;
    
    uint16_t offset = 0;
    buffer[offset++] = label_len;
    memcpy(&buffer[offset], label, label_len);
    offset += label_len;
    buffer[offset++] = DATA_TYPE_STRING;
    buffer[offset++] = value_len;
    memcpy(&buffer[offset], value, value_len);
    offset += value_len;
    
    return offset;
}


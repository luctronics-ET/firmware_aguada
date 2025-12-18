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


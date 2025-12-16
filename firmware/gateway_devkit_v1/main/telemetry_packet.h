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

    // Fields set/overwritten by the gateway upon reception
    int8_t   rssi;           // RSSI if available, else 0
    uint32_t ts_ms;          // gateway timestamp in ms
} SensorPacketV1;

#define SENSOR_PACKET_VERSION 1

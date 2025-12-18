/**
 * EXAMPLE: Generic Packet Usage
 * 
 * This example shows how to use GenericPacket for flexible telemetry
 * Demonstrates sending various sensor types with dynamic key-value pairs
 */

#include "telemetry_packet.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_mac.h"

static const char* TAG = "generic_example";

// Example: Multi-sensor node sending diverse data
void send_multisensor_telemetry() {
    uint8_t packet_buffer[MAX_GENERIC_PACKET_SIZE];
    uint16_t offset = 0;
    
    // 1. Fill header
    GenericPacketHeader* header = (GenericPacketHeader*)packet_buffer;
    header->magic = GENERIC_PACKET_MAGIC;
    header->version = GENERIC_PACKET_VERSION;
    header->node_id = 2;  // Node Ultra 2
    
    uint8_t dev_mac[6];
    esp_read_mac(dev_mac, ESP_MAC_WIFI_STA);
    memcpy(header->mac, dev_mac, 6);
    
    static uint32_t seq = 0;
    header->seq = ++seq;
    header->pair_count = 0;
    header->rssi = 0;   // Gateway fills
    header->ts_ms = 0;  // Gateway fills
    
    offset = sizeof(GenericPacketHeader);
    
    // 2. Add sensor readings
    
    // DHT22: Temperature
    float temp_c = 23.5f;  // From DHT22 sensor
    offset += add_float_pair(&packet_buffer[offset], "temp", temp_c);
    header->pair_count++;
    
    // DHT22: Humidity
    float humidity = 65.2f;
    offset += add_float_pair(&packet_buffer[offset], "humid", humidity);
    header->pair_count++;
    
    // HC-SR04: Distance
    int16_t distance_cm = 123;
    offset += add_int16_pair(&packet_buffer[offset], "dist", distance_cm);
    header->pair_count++;
    
    // Calculated: Water level
    int16_t level_cm = 450 - distance_cm + 20;
    offset += add_int16_pair(&packet_buffer[offset], "level", level_cm);
    header->pair_count++;
    
    // ADC: Battery voltage
    uint16_t battery_mv = 3300;
    offset += add_uint16_pair(&packet_buffer[offset], "bat_mv", battery_mv);
    header->pair_count++;
    
    // Calculated: Battery percentage
    uint8_t battery_pct = 85;
    offset += add_uint8_pair(&packet_buffer[offset], "bat_pct", battery_pct);
    header->pair_count++;
    
    // BMP280: Pressure
    uint16_t pressure_hpa = 1013;
    offset += add_uint16_pair(&packet_buffer[offset], "press", pressure_hpa);
    header->pair_count++;
    
    // Status flags
    bool pump_active = false;
    offset += add_bool_pair(&packet_buffer[offset], "pump", pump_active);
    header->pair_count++;
    
    bool alert = false;
    offset += add_bool_pair(&packet_buffer[offset], "alert", alert);
    header->pair_count++;
    
    // System info
    offset += add_string_pair(&packet_buffer[offset], "state", "ok");
    header->pair_count++;
    
    // 3. Validate size
    if (offset > MAX_GENERIC_PACKET_SIZE) {
        ESP_LOGE(TAG, "❌ Packet too large: %u bytes (max %u)", 
                 offset, MAX_GENERIC_PACKET_SIZE);
        return;
    }
    
    // 4. Send via ESP-NOW
    uint8_t gateway_mac[6] = {0x80, 0xf3, 0xda, 0x62, 0xa7, 0x84};
    esp_err_t err = esp_now_send(gateway_mac, packet_buffer, offset);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ Generic packet sent: %u bytes, %u pairs", 
                 offset, header->pair_count);
        
        // Log summary
        ESP_LOGI(TAG, "  temp=%.1f°C humid=%.1f%% dist=%dcm level=%dcm", 
                 temp_c, humidity, distance_cm, level_cm);
        ESP_LOGI(TAG, "  bat=%umV (%u%%) press=%uhPa pump=%s", 
                 battery_mv, battery_pct, pressure_hpa, 
                 pump_active ? "ON" : "OFF");
    } else {
        ESP_LOGE(TAG, "❌ Failed to send: %s", esp_err_to_name(err));
    }
}

// Example: Calculate actual packet size before building
void calculate_packet_size_example() {
    uint16_t size = sizeof(GenericPacketHeader);
    
    // Estimate size for each field
    size += data_pair_size("temp", 4);      // float = 4 bytes
    size += data_pair_size("humid", 4);     // float = 4 bytes
    size += data_pair_size("dist", 2);      // int16 = 2 bytes
    size += data_pair_size("level", 2);     // int16 = 2 bytes
    size += data_pair_size("bat_mv", 2);    // uint16 = 2 bytes
    size += data_pair_size("bat_pct", 1);   // uint8 = 1 byte
    size += data_pair_size("press", 2);     // uint16 = 2 bytes
    size += data_pair_size("pump", 1);      // bool = 1 byte
    size += data_pair_size("alert", 1);     // bool = 1 byte
    size += data_pair_size("state", 2);     // string "ok" = 2 bytes
    
    ESP_LOGI(TAG, "📏 Estimated packet size: %u bytes", size);
    
    if (size > MAX_GENERIC_PACKET_SIZE) {
        ESP_LOGW(TAG, "⚠️ Packet will exceed limit! Reduce fields.");
    }
}

// Example: Minimal packet (only essential data)
void send_minimal_telemetry() {
    uint8_t packet_buffer[MAX_GENERIC_PACKET_SIZE];
    uint16_t offset = sizeof(GenericPacketHeader);
    
    GenericPacketHeader* header = (GenericPacketHeader*)packet_buffer;
    header->magic = GENERIC_PACKET_MAGIC;
    header->version = GENERIC_PACKET_VERSION;
    header->node_id = 2;
    // ... (fill rest of header)
    
    header->pair_count = 0;
    
    // Only 3 essential fields
    int16_t level = 327;
    offset += add_int16_pair(&packet_buffer[offset], "lvl", level);
    header->pair_count++;
    
    uint16_t bat = 3300;
    offset += add_uint16_pair(&packet_buffer[offset], "bat", bat);
    header->pair_count++;
    
    bool ok = true;
    offset += add_bool_pair(&packet_buffer[offset], "ok", ok);
    header->pair_count++;
    
    ESP_LOGI(TAG, "📦 Minimal packet: %u bytes (only %u fields)", 
             offset, header->pair_count);
    
    // Send...
}

// Example: High-frequency lightweight updates
void send_heartbeat() {
    uint8_t packet_buffer[MAX_GENERIC_PACKET_SIZE];
    uint16_t offset = sizeof(GenericPacketHeader);
    
    GenericPacketHeader* header = (GenericPacketHeader*)packet_buffer;
    // ... fill header ...
    
    header->pair_count = 0;
    
    // Just status and uptime
    offset += add_string_pair(&packet_buffer[offset], "st", "up");
    header->pair_count++;
    
    uint32_t uptime_s = esp_timer_get_time() / 1000000ULL;
    offset += add_uint32_pair(&packet_buffer[offset], "up", uptime_s);
    header->pair_count++;
    
    ESP_LOGI(TAG, "💓 Heartbeat: %u bytes", offset);
    // Send every 60s for keepalive
}

// Example: Agricultural sensor node
void send_agriculture_data() {
    uint8_t packet_buffer[MAX_GENERIC_PACKET_SIZE];
    uint16_t offset = sizeof(GenericPacketHeader);
    
    GenericPacketHeader* header = (GenericPacketHeader*)packet_buffer;
    // ... fill header ...
    
    header->pair_count = 0;
    
    // Soil moisture (capacitive sensor)
    uint8_t soil_moisture_pct = 45;
    offset += add_uint8_pair(&packet_buffer[offset], "soil", soil_moisture_pct);
    header->pair_count++;
    
    // Soil temperature (DS18B20)
    float soil_temp = 18.5f;
    offset += add_float_pair(&packet_buffer[offset], "s_temp", soil_temp);
    header->pair_count++;
    
    // pH sensor
    float ph = 6.5f;
    offset += add_float_pair(&packet_buffer[offset], "ph", ph);
    header->pair_count++;
    
    // EC sensor (electrical conductivity)
    uint16_t ec_us_cm = 1200;
    offset += add_uint16_pair(&packet_buffer[offset], "ec", ec_us_cm);
    header->pair_count++;
    
    // Light sensor (lux)
    uint16_t light_lux = 850;
    offset += add_uint16_pair(&packet_buffer[offset], "lux", light_lux);
    header->pair_count++;
    
    // Ambient temp & humidity
    float air_temp = 23.5f;
    offset += add_float_pair(&packet_buffer[offset], "a_temp", air_temp);
    header->pair_count++;
    
    float air_humid = 65.2f;
    offset += add_float_pair(&packet_buffer[offset], "a_hum", air_humid);
    header->pair_count++;
    
    ESP_LOGI(TAG, "🌱 Agriculture data: %u bytes, %u sensors", 
             offset, header->pair_count);
}

// Example: Weather station
void send_weather_data() {
    uint8_t packet_buffer[MAX_GENERIC_PACKET_SIZE];
    uint16_t offset = sizeof(GenericPacketHeader);
    
    GenericPacketHeader* header = (GenericPacketHeader*)packet_buffer;
    // ... fill header ...
    
    header->pair_count = 0;
    
    // BME280: Temperature, Humidity, Pressure
    float temp = 23.5f;
    offset += add_float_pair(&packet_buffer[offset], "temp", temp);
    header->pair_count++;
    
    float humid = 65.2f;
    offset += add_float_pair(&packet_buffer[offset], "humid", humid);
    header->pair_count++;
    
    uint16_t press = 1013;
    offset += add_uint16_pair(&packet_buffer[offset], "press", press);
    header->pair_count++;
    
    // Wind sensor
    float wind_speed = 3.2f;
    offset += add_float_pair(&packet_buffer[offset], "wind", wind_speed);
    header->pair_count++;
    
    offset += add_string_pair(&packet_buffer[offset], "dir", "NW");
    header->pair_count++;
    
    // Rain gauge (mm accumulated)
    float rain_mm = 2.5f;
    offset += add_float_pair(&packet_buffer[offset], "rain", rain_mm);
    header->pair_count++;
    
    // UV index
    uint8_t uv_index = 3;
    offset += add_uint8_pair(&packet_buffer[offset], "uv", uv_index);
    header->pair_count++;
    
    ESP_LOGI(TAG, "🌤️ Weather data: %u bytes, %u measurements", 
             offset, header->pair_count());
}

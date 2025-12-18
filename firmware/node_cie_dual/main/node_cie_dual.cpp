// node_cie_dual.cpp
// ESP-IDF firmware for ESP32-C3 Supermini with DUAL HC-SR04 sensors
// - Sensor 1: TRIG=GPIO1, ECHO=GPIO0 → node_id=4 (CIE1)
// - Sensor 2: TRIG=GPIO3, ECHO=GPIO2 → node_id=5 (CIE2)
// - Single ESP32, dual virtual node_ids
// - Independent error handling (each sensor can fail independently)
// - Sends 2 separate packets to gateway
//
// Use case: CIE cistern with 2 independent side-by-side reservoirs

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_task_wdt.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "sdkconfig.h"

// Modules
#include "components/ultrasonic01/ultrasonic01.h"
#include "components/level_calculator/level_calculator.h"
#include "common/telemetry_packet.h"

static const char *TAG = "node_cie_dual";

/* ====== CONFIGURABLE PARAMETERS ====== */
/* Hardware pins - SENSOR 1 (CIE1) */
#define TRIG_GPIO_1       GPIO_NUM_1
#define ECHO_GPIO_1       GPIO_NUM_0
#define NODE_ID_1         4  // Virtual node ID for CIE1

/* Hardware pins - SENSOR 2 (CIE2) */
#define TRIG_GPIO_2       GPIO_NUM_3
#define ECHO_GPIO_2       GPIO_NUM_2
#define NODE_ID_2         5  // Virtual node ID for CIE2

/* Shared ADC and LED */
#define ADC_CHANNEL_VIN   ADC_CHANNEL_4  // ADC1 channel 4 = GPIO4
#define ADC_UNIT_VIN      ADC_UNIT_1
#define LED_GPIO          GPIO_NUM_8     // built-in LED on ESP32-C3 Supermini
#ifdef CONFIG_LED_ACTIVE_HIGH
#define LED_ON_LEVEL      1
#else
#define LED_ON_LEVEL      0
#endif

/* Telemetry / tank model (BOTH RESERVOIRS USE SAME GEOMETRY) */
#define VOL_MAX_L         80000    // liters (vol_max)
#define LEVEL_MAX_CM      450      // cm (level_max)
#define SENSOR_OFFSET_CM  20       // sensor_offset (sensor is 20cm above level_max)

/* Sampling and retries */
#define SAMPLE_INTERVAL_S     30    // loop interval in seconds
#define ULTRA_SAMPLE_RETRIES  3     // number of ultrasonic readings to take
#define ULTRA_MEASURE_DELAY_MS 60   // delay between raw ultrasonic attempts
#define ESPNOW_SEND_RETRIES   2
#define INTER_SENSOR_DELAY_MS 100   // delay between sensor 1 and sensor 2 measurement

/* Ultrasonic validation */
#define MIN_VALID_CM    5
#define MAX_VALID_CM    450

/* Anomaly detection thresholds */
#define RAPID_CHANGE_THRESHOLD_CM  50   // 50cm change triggers alert
#define NO_CHANGE_MINUTES          120  // 2 hours without change triggers sensor stuck alert
#define NO_CHANGE_THRESHOLD_CM     2    // Within ±2cm considered "no change"

/* ADC configuration */
#define ADC_ATTEN        ADC_ATTEN_DB_12
#define ADC_BITWIDTH     ADC_BITWIDTH_12

/* ESP-NOW configuration */
#define MAX_GATEWAYS 3
static const uint8_t GATEWAY_MACS[MAX_GATEWAYS][6] = {
    {0x80, 0xf3, 0xda, 0x62, 0xa7, 0x84},  // Gateway 1 (ESP32 DevKit V1)
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  // Gateway 2 (configure with real MAC)
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}   // Gateway 3 (configure with real MAC)
};

#define ESPNOW_CHANNEL 11
/* ===================================== */

/* NVS keys */
#define NVS_NAMESPACE "node_cfg"
#define NVS_SEQ_KEY_1 "seq1"  // Sequence for CIE1
#define NVS_SEQ_KEY_2 "seq2"  // Sequence for CIE2
#define NVS_LAST_GW_KEY "last_gw"

/* ADC handles (global) */
static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t adc1_cali_handle = NULL;

/* Anomaly detection state - PER SENSOR */
struct SensorState {
    int16_t last_level_cm;
    uint32_t last_change_seq;
    bool initialized;
};

static SensorState sensor1_state = {0, 0, false};
static SensorState sensor2_state = {0, 0, false};

/* ACK tracking - SHARED */
static uint32_t successful_acks = 0;
static uint32_t total_attempts = 0;
static int last_successful_gateway = 0;  // 0-2 (index into GATEWAY_MACS)

/* ====== LED PATTERNS ====== */
static void led_set(bool on) {
    gpio_set_level(LED_GPIO, on ? LED_ON_LEVEL : !LED_ON_LEVEL);
}

static void led_pattern_boot(void) {
    for (int i = 0; i < 3; i++) {
        led_set(true);
        vTaskDelay(pdMS_TO_TICKS(200));
        led_set(false);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

static void led_pattern_tx(void) {
    for (int i = 0; i < 3; i++) {
        led_set(true);
        vTaskDelay(pdMS_TO_TICKS(50));
        led_set(false);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void led_pattern_error(void) {
    for (int i = 0; i < 10; i++) {
        led_set(true);
        vTaskDelay(pdMS_TO_TICKS(30));
        led_set(false);
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

/* ====== NVS FUNCTIONS ====== */
static uint32_t nvs_get_seq(const char *key, uint32_t default_val) {
    nvs_handle_t nvs;
    uint32_t seq = default_val;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err == ESP_OK) {
        nvs_get_u32(nvs, key, &seq);
        nvs_close(nvs);
    }
    return seq;
}

static void nvs_set_seq(const char *key, uint32_t seq) {
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_u32(nvs, key, seq);
        nvs_commit(nvs);
        nvs_close(nvs);
    }
}

static int nvs_get_last_gateway(void) {
    nvs_handle_t nvs;
    uint8_t gw_idx = 0;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs) == ESP_OK) {
        nvs_get_u8(nvs, NVS_LAST_GW_KEY, &gw_idx);
        nvs_close(nvs);
    }
    if (gw_idx >= MAX_GATEWAYS) gw_idx = 0;
    return (int)gw_idx;
}

static void nvs_set_last_gateway(int gw_idx) {
    if (gw_idx < 0 || gw_idx >= MAX_GATEWAYS) return;
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_u8(nvs, NVS_LAST_GW_KEY, (uint8_t)gw_idx);
        nvs_commit(nvs);
        nvs_close(nvs);
    }
}

/* ====== ADC FUNCTIONS ====== */
static void init_adc(void) {
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_VIN,
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc1_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_VIN, &chan_cfg));

    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_VIN,
        .chan = ADC_CHANNEL_VIN,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &adc1_cali_handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC calibration initialized");
    } else {
        ESP_LOGW(TAG, "ADC calibration failed, using raw values");
        adc1_cali_handle = NULL;
    }
}

static int read_vin_mv(void) {
    int adc_raw = 0;
    esp_err_t ret = adc_oneshot_read(adc1_handle, ADC_CHANNEL_VIN, &adc_raw);
    if (ret != ESP_OK) {
        return -1;
    }
    int voltage_mv = 0;
    if (adc1_cali_handle) {
        adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw, &voltage_mv);
    } else {
        voltage_mv = adc_raw;
    }
    return voltage_mv * 2;  // voltage divider (Vin = 2 * ADC_reading)
}

/* ====== MAC HELPER ====== */
static void get_device_mac(uint8_t *mac_out) {
    esp_efuse_mac_get_default(mac_out);
}

/* ====== ESP-NOW CALLBACKS ====== */
static volatile bool ack_received = false;
static AckPacket last_ack = {};

static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    if (len == sizeof(AckPacket)) {
        AckPacket *ack = (AckPacket *)data;
        if (ack->magic == ACK_MAGIC && ack->version == ACK_VERSION) {
            memcpy(&last_ack, ack, sizeof(AckPacket));
            ack_received = true;
            ESP_LOGI(TAG, "✅ ACK recebido: node_id=%d, seq=%u, status=%d, rssi=%d, gw=%d",
                     ack->node_id, ack->ack_seq, ack->status, ack->rssi, ack->gateway_id);
        }
    }
}

static void espnow_send_cb(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    // Not used - we wait for ACK packet instead
}

/* ====== ESP-NOW SEND WITH ACK ====== */
static esp_err_t espnow_send_payload(const uint8_t *payload, size_t len, uint32_t seq, uint8_t node_id) {
    total_attempts++;
    
    // Try last successful gateway first
    last_successful_gateway = nvs_get_last_gateway();
    int gw_order[MAX_GATEWAYS];
    gw_order[0] = last_successful_gateway;
    int idx = 1;
    for (int i = 0; i < MAX_GATEWAYS; i++) {
        if (i != last_successful_gateway) {
            gw_order[idx++] = i;
        }
    }
    
    for (int attempt = 0; attempt < ESPNOW_SEND_RETRIES; attempt++) {
        for (int g = 0; g < MAX_GATEWAYS; g++) {
            int gw_idx = gw_order[g];
            const uint8_t *gw_mac = GATEWAY_MACS[gw_idx];
            
            // Skip unconfigured gateways
            bool is_configured = false;
            for (int i = 0; i < 6; i++) {
                if (gw_mac[i] != 0xFF) {
                    is_configured = true;
                    break;
                }
            }
            if (!is_configured) continue;
            
            ESP_LOGI(TAG, "📤 Enviando para Gateway %d (tentativa %d) node_id=%d seq=%u", 
                     gw_idx, attempt + 1, node_id, seq);
            
            ack_received = false;
            esp_err_t send_err = esp_now_send(gw_mac, payload, len);
            
            if (send_err == ESP_OK) {
                // Wait for ACK (500ms timeout)
                int wait_ms = 500;
                int wait_step = 10;
                for (int i = 0; i < (wait_ms / wait_step); i++) {
                    if (ack_received && last_ack.ack_seq == seq && last_ack.node_id == node_id) {
                        successful_acks++;
                        nvs_set_last_gateway(gw_idx);
                        last_successful_gateway = gw_idx;
                        float success_rate = (float)successful_acks / (float)total_attempts * 100.0f;
                        ESP_LOGI(TAG, "✅ ACK confirmado! Taxa de sucesso: %.1f%% (%u/%u)",
                                 success_rate, successful_acks, total_attempts);
                        return ESP_OK;
                    }
                    vTaskDelay(pdMS_TO_TICKS(wait_step));
                }
                ESP_LOGW(TAG, "⏱️ Timeout aguardando ACK do Gateway %d", gw_idx);
            } else {
                ESP_LOGE(TAG, "❌ Falha no envio para Gateway %d: %s", gw_idx, esp_err_to_name(send_err));
            }
            
            // Exponential backoff before next gateway
            vTaskDelay(pdMS_TO_TICKS(100 * (1 << attempt)));
        }
    }
    
    float success_rate = (float)successful_acks / (float)total_attempts * 100.0f;
    ESP_LOGE(TAG, "❌ Falha após %d tentativas. Taxa de sucesso: %.1f%% (%u/%u)",
             ESPNOW_SEND_RETRIES * MAX_GATEWAYS, success_rate, successful_acks, total_attempts);
    return ESP_FAIL;
}

/* ====== ANOMALY DETECTION ====== */
static void detect_anomalies(SensorState *state, int level_cm, uint32_t seq, 
                             uint8_t *flags, uint8_t *alert_type, const char *sensor_name) {
    *flags = 0;
    *alert_type = ALERT_NONE;
    
    if (!state->initialized) {
        state->last_level_cm = level_cm;
        state->last_change_seq = seq;
        state->initialized = true;
        ESP_LOGI(TAG, "🎯 %s anomaly detection initialized (baseline=%dcm)", sensor_name, level_cm);
        return;
    }
    
    int16_t delta_cm = level_cm - state->last_level_cm;
    uint32_t readings_since_change = seq - state->last_change_seq;
    uint32_t minutes_since_change = (readings_since_change * SAMPLE_INTERVAL_S) / 60;
    
    // Check for rapid drop
    if (delta_cm <= -RAPID_CHANGE_THRESHOLD_CM) {
        *flags |= FLAG_IS_ALERT;
        *alert_type = ALERT_RAPID_DROP;
        ESP_LOGW(TAG, "🚨 %s ALERTA: Queda rápida! Δ=%dcm", sensor_name, delta_cm);
    }
    // Check for rapid rise
    else if (delta_cm >= RAPID_CHANGE_THRESHOLD_CM) {
        *flags |= FLAG_IS_ALERT;
        *alert_type = ALERT_RAPID_RISE;
        ESP_LOGW(TAG, "🚨 %s ALERTA: Subida rápida! Δ=%dcm", sensor_name, delta_cm);
    }
    // Check for sensor stuck
    else if (minutes_since_change >= NO_CHANGE_MINUTES && 
             abs(delta_cm) <= NO_CHANGE_THRESHOLD_CM) {
        *flags |= FLAG_IS_ALERT;
        *alert_type = ALERT_SENSOR_STUCK;
        ESP_LOGW(TAG, "🚨 %s ALERTA: Sensor travado! %u minutos sem mudança", sensor_name, minutes_since_change);
    }
    
    // Update baseline if significant change
    if (abs(delta_cm) > NO_CHANGE_THRESHOLD_CM) {
        state->last_level_cm = level_cm;
        state->last_change_seq = seq;
    }
}

/* ====== MEASURE AND SEND SENSOR ====== */
static void measure_and_send_sensor(gpio_num_t trig_gpio, gpio_num_t echo_gpio,
                                    uint8_t node_id, const char *seq_key,
                                    SensorState *state, const char *sensor_name) {
    // Get sequence number
    uint32_t seq = nvs_get_seq(seq_key, 0);
    seq++;
    
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════");
    ESP_LOGI(TAG, "📊 %s (node_id=%d) - Medição #%u", sensor_name, node_id, seq);
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════");
    
    // Kalman filter for this sensor (static, persists across calls)
    static ultrasonic01::KalmanFilter kalman_sensor1(1.0, 2.0);
    static ultrasonic01::KalmanFilter kalman_sensor2(1.0, 2.0);
    ultrasonic01::KalmanFilter *kalman = (node_id == NODE_ID_1) ? &kalman_sensor1 : &kalman_sensor2;
    
    // Measure distance with Kalman filtering
    int distance_cm = -1;
    int valid_readings = 0;
    
    for (int attempt = 0; attempt < ULTRA_SAMPLE_RETRIES; attempt++) {
        ultrasonic01::Pins pins{trig_gpio, echo_gpio};
        int raw_distance = ultrasonic01::measure_cm(pins);
        
        if (raw_distance >= MIN_VALID_CM && raw_distance <= MAX_VALID_CM) {
            kalman->update(raw_distance);
            distance_cm = (int)kalman->get_estimate();
            valid_readings++;
            ESP_LOGI(TAG, "  Tentativa %d/%d: raw=%dcm, kalman=%dcm ✓", 
                     attempt + 1, ULTRA_SAMPLE_RETRIES, raw_distance, distance_cm);
        } else {
            ESP_LOGW(TAG, "  Tentativa %d/%d: INVÁLIDO (raw=%dcm)", 
                     attempt + 1, ULTRA_SAMPLE_RETRIES, raw_distance);
        }
        
        if (attempt < ULTRA_SAMPLE_RETRIES - 1) {
            vTaskDelay(pdMS_TO_TICKS(ULTRA_MEASURE_DELAY_MS));
        }
    }
    
    // Check if all readings failed
    if (valid_readings == 0) {
        ESP_LOGE(TAG, "❌ %s: Todas as leituras falharam! Enviando erro.", sensor_name);
        kalman->reset();  // Reset filter for next attempt
        distance_cm = -1;  // Indicate total failure
    } else {
        ESP_LOGI(TAG, "✅ %s: Distância final (Kalman): %dcm (%d/%d leituras válidas)", 
                 sensor_name, distance_cm, valid_readings, ULTRA_SAMPLE_RETRIES);
    }
    
    // Validate distance
    if (distance_cm >= 0 && (distance_cm < MIN_VALID_CM || distance_cm > MAX_VALID_CM)) {
        ESP_LOGW(TAG, "%s: distance fora de range (%d cm). Clampando.", sensor_name, distance_cm);
        if (distance_cm < MIN_VALID_CM) distance_cm = MIN_VALID_CM;
        if (distance_cm > MAX_VALID_CM) distance_cm = MAX_VALID_CM;
    }
    
    // Read voltage (shared ADC)
    int vin_mv = read_vin_mv();
    if (vin_mv < 0) {
        ESP_LOGW(TAG, "ADC read failed");
        vin_mv = 0;
    }
    
    // Calculate level/volume (if distance valid)
    int level_cm = 0, percentual = 0, volume_l = 0;
    if (distance_cm >= 0) {
        level_calculator::Model model{LEVEL_MAX_CM, SENSOR_OFFSET_CM, VOL_MAX_L};
        auto res = level_calculator::compute(distance_cm, model);
        level_cm = res.level_cm;
        percentual = res.percentual;
        volume_l = res.volume_l;
        
        ESP_LOGI(TAG, "%s: distance=%dcm → level=%dcm, %d%%, %dL, vin=%dmV",
                 sensor_name, distance_cm, level_cm, percentual, volume_l, vin_mv);
    } else {
        ESP_LOGE(TAG, "%s: FALHA TOTAL - enviando valores zerados", sensor_name);
    }
    
    // Anomaly detection
    uint8_t flags = 0;
    uint8_t alert_type = ALERT_NONE;
    if (distance_cm >= 0) {
        detect_anomalies(state, level_cm, seq, &flags, &alert_type, sensor_name);
    } else {
        // Sensor failure - mark as sensor error
        flags |= FLAG_IS_ALERT;
        alert_type = ALERT_SENSOR_STUCK;  // Could add ALERT_SENSOR_FAILURE
        ESP_LOGW(TAG, "%s: Marcando como erro de sensor", sensor_name);
    }
    
    // Build packet
    uint8_t dev_mac[6];
    get_device_mac(dev_mac);
    SensorPacketV1 pkt{};
    pkt.version = SENSOR_PACKET_VERSION;
    pkt.node_id = node_id;
    memcpy(pkt.mac, dev_mac, sizeof(pkt.mac));
    pkt.seq = seq;
    pkt.distance_cm = (int16_t)distance_cm;
    pkt.level_cm = (int16_t)level_cm;
    pkt.percentual = (uint8_t)percentual;
    pkt.volume_l = (uint32_t)volume_l;
    pkt.vin_mv = (int16_t)vin_mv;
    pkt.flags = flags;
    pkt.alert_type = alert_type;
    pkt.rssi = 0;   // gateway fills
    pkt.ts_ms = 0;  // gateway fills
    
    // Send via ESP-NOW with ACK
    esp_err_t send_err = espnow_send_payload((const uint8_t*)&pkt, sizeof(pkt), seq, node_id);
    if (send_err == ESP_OK) {
        ESP_LOGI(TAG, "✅ %s: Pacote enviado com sucesso (seq=%u)", sensor_name, seq);
        nvs_set_seq(seq_key, seq);
        led_pattern_tx();
    } else {
        ESP_LOGE(TAG, "❌ %s: Falha no envio (seq=%u)", sensor_name, seq);
        led_pattern_error();
    }
}

/* ====== MAIN ====== */
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "╔═══════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║       NODE CIE DUAL - ESP32-C3 Dual Sensor System        ║");
    ESP_LOGI(TAG, "║   Sensor 1 (CIE1): GPIO1/0 → node_id=4                   ║");
    ESP_LOGI(TAG, "║   Sensor 2 (CIE2): GPIO3/2 → node_id=5                   ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════════════════════════╝");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");
    
    // Initialize LED
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    led_set(false);
    led_pattern_boot();
    
    // Initialize ADC
    init_adc();
    
    // Print MAC address
    uint8_t mac[6];
    get_device_mac(mac);
    ESP_LOGI(TAG, "Device MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // Initialize WiFi and ESP-NOW
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_LOGI(TAG, "WiFi set to channel %d", ESPNOW_CHANNEL);
    
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    
    // Add gateway peers
    for (int i = 0; i < MAX_GATEWAYS; i++) {
        // Skip unconfigured gateways
        bool is_configured = false;
        for (int j = 0; j < 6; j++) {
            if (GATEWAY_MACS[i][j] != 0xFF) {
                is_configured = true;
                break;
            }
        }
        if (!is_configured) continue;
        
        esp_now_peer_info_t peer_info = {};
        memcpy(peer_info.peer_addr, GATEWAY_MACS[i], 6);
        peer_info.channel = ESPNOW_CHANNEL;
        peer_info.ifidx = WIFI_IF_STA;
        peer_info.encrypt = false;
        
        ret = esp_now_add_peer(&peer_info);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Gateway %d registrado: %02X:%02X:%02X:%02X:%02X:%02X",
                     i, GATEWAY_MACS[i][0], GATEWAY_MACS[i][1], GATEWAY_MACS[i][2],
                     GATEWAY_MACS[i][3], GATEWAY_MACS[i][4], GATEWAY_MACS[i][5]);
        }
    }
    
    ESP_LOGI(TAG, "ESP-NOW initialized");
    ESP_LOGI(TAG, "🚀 Sistema iniciado! Intervalo de medição: %ds", SAMPLE_INTERVAL_S);
    ESP_LOGI(TAG, "");
    
    // Main loop
    while (1) {
        // Measure and send SENSOR 1 (CIE1)
        measure_and_send_sensor(TRIG_GPIO_1, ECHO_GPIO_1, NODE_ID_1, 
                               NVS_SEQ_KEY_1, &sensor1_state, "CIE1");
        
        // Small delay between sensors to avoid GPIO interference
        vTaskDelay(pdMS_TO_TICKS(INTER_SENSOR_DELAY_MS));
        
        // Measure and send SENSOR 2 (CIE2)
        measure_and_send_sensor(TRIG_GPIO_2, ECHO_GPIO_2, NODE_ID_2,
                               NVS_SEQ_KEY_2, &sensor2_state, "CIE2");
        
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "⏳ Aguardando %ds até próxima medição...", SAMPLE_INTERVAL_S);
        ESP_LOGI(TAG, "");
        
        // Wait for next cycle
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_S * 1000));
    }
}

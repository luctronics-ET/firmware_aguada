// node_ultra01.cpp
// ESP-IDF firmware for ESP32-C3 Supermini
// - Ultrasonic HC-SR04 (trig = GPIO_NUM_1, echo = GPIO_NUM_0)
// - ADC Vin (GPIO_NUM_4) with V/2 divider
// - ESP-NOW broadcast send
// - integer-only calculations (1 cm resolution, integer % and liters)
// - seq counter persisted in NVS
//
// Configure macros below as needed.

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

static const char *TAG = "node_ultra01";

/* ====== CONFIGURABLE PARAMETERS ====== */
/* Hardware pins */
#define TRIG_GPIO         GPIO_NUM_1
#define ECHO_GPIO         GPIO_NUM_0
#define ADC_CHANNEL_VIN   ADC_CHANNEL_4  // ADC1 channel 4 = GPIO4
#define ADC_UNIT_VIN      ADC_UNIT_1
#define LED_GPIO          GPIO_NUM_8     // built-in LED on ESP32-C3 Supermini
#ifdef CONFIG_LED_ACTIVE_HIGH
#define LED_ON_LEVEL      1
#else
#define LED_ON_LEVEL      0
#endif

/* Telemetry / tank model */
#define VOL_MAX_L         80000    // liters (vol_max)
#define LEVEL_MAX_CM      450      // cm (level_max)
#define SENSOR_OFFSET_CM  20       // sensor_offset (sensor is 20cm above level_max)

/* Sampling and retries */
#define SAMPLE_INTERVAL_S     30    // loop interval in seconds (was deep sleep; now active loop)
#define ULTRA_SAMPLE_RETRIES  3     // number of ultrasonic readings to take (for median)
#define ULTRA_MEASURE_DELAY_MS 60   // delay between raw ultrasonic attempts
#define ESPNOW_SEND_RETRIES   2

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
/* Multiple gateway MAC addresses for redundancy/failover */
#define MAX_GATEWAYS 3
static const uint8_t GATEWAY_MACS[MAX_GATEWAYS][6] = {
    {0x80, 0xf3, 0xda, 0x62, 0xa7, 0x84},  // Gateway 1 (ESP32 DevKit V1)
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  // Gateway 2 (configure with real MAC)
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}   // Gateway 3 (configure with real MAC)
};

/* ESP-NOW channel - must match gateway */
#define ESPNOW_CHANNEL 11
/* ===================================== */

/* NVS keys */
#define NVS_NAMESPACE "node_cfg"
#define NVS_SEQ_KEY   "seq"
#define NVS_LAST_GW_KEY "last_gw"  // Last successful gateway index

// Use ultrasonic01 module instead of local implementation

/* ADC handles (global) */
static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t adc1_cali_handle = NULL;

/* ACK tracking */
static volatile bool ack_received = false;
static volatile uint32_t ack_seq_received = 0;
static volatile uint8_t ack_gateway_id = 0xFF;

/* Anomaly detection state (persistent across measurements) */
static int16_t last_level_cm = -1;  // Previous water level
static uint32_t last_change_seq = 0;  // Sequence when last level change detected
static bool anomaly_detection_initialized = false;

/* Transmission statistics */
static struct {
    uint32_t total_attempts;
    uint32_t successful_acks;
    uint32_t failed_acks;
} tx_stats = {0, 0, 0};

/* Read ADC and convert to mV, considering V/2 divider
   Returns integer mV value (vin_mv), or -1 on failure.
*/
static int read_vin_mv(void) {
    if (!adc1_handle) return -1;
    
    int raw = 0;
    esp_err_t err = adc_oneshot_read(adc1_handle, ADC_CHANNEL_VIN, &raw);
    if (err != ESP_OK) return -1;
    
    int voltage_mv = 0;
    if (adc1_cali_handle) {
        adc_cali_raw_to_voltage(adc1_cali_handle, raw, &voltage_mv);
    } else {
        // fallback sem calibração (aproximado para 3.3V ref, 12-bit)
        voltage_mv = (raw * 3300) / 4095;
    }
    
    // account divider (V/2) => Vin_mv = voltage_mv * 2
    int vin_mv = voltage_mv * 2;
    return vin_mv;
}

/* NVS helpers for seq */
static esp_err_t nvs_get_seq(uint32_t *seq_out) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &h);
    if (err == ESP_OK) {
        uint32_t val = 0;
        err = nvs_get_u32(h, NVS_SEQ_KEY, &val);
        if (err == ESP_OK) *seq_out = val;
        nvs_close(h);
    }
    return err;
}
static esp_err_t nvs_set_seq(uint32_t seq) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err == ESP_OK) {
        err = nvs_set_u32(h, NVS_SEQ_KEY, seq);
        if (err == ESP_OK) err = nvs_commit(h);
        nvs_close(h);
    }
    return err;
}

/* NVS helpers for last successful gateway */
static uint8_t nvs_get_last_gateway(void) {
    nvs_handle_t h;
    uint8_t gw_idx = 0;  // Default to first gateway
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &h);
    if (err == ESP_OK) {
        nvs_get_u8(h, NVS_LAST_GW_KEY, &gw_idx);
        nvs_close(h);
    }
    // Validate index
    if (gw_idx >= MAX_GATEWAYS) gw_idx = 0;
    return gw_idx;
}

static void nvs_set_last_gateway(uint8_t gw_idx) {
    if (gw_idx >= MAX_GATEWAYS) return;
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err == ESP_OK) {
        nvs_set_u8(h, NVS_LAST_GW_KEY, gw_idx);
        nvs_commit(h);
        nvs_close(h);
    }
}

/* Check if gateway MAC is configured (not all 0xFF) */
static bool is_gateway_valid(uint8_t gw_idx) {
    if (gw_idx >= MAX_GATEWAYS) return false;
    const uint8_t *mac = GATEWAY_MACS[gw_idx];
    return !(mac[0] == 0xFF && mac[1] == 0xFF && mac[2] == 0xFF &&
             mac[3] == 0xFF && mac[4] == 0xFF && mac[5] == 0xFF);
}

/* ESP-NOW send with automatic gateway failover and ACK wait */
static esp_err_t espnow_send_payload(const uint8_t *data, size_t len, uint32_t expected_seq) {
    tx_stats.total_attempts++;
    
    // Try last successful gateway first
    uint8_t start_gw = nvs_get_last_gateway();
    
    // Round-robin through all configured gateways
    for (uint8_t attempt = 0; attempt < MAX_GATEWAYS; attempt++) {
        uint8_t gw_idx = (start_gw + attempt) % MAX_GATEWAYS;
        
        // Skip unconfigured gateways (0xFF:FF:FF:FF:FF:FF)
        if (!is_gateway_valid(gw_idx)) {
            continue;
        }
        
        const uint8_t *gw_mac = GATEWAY_MACS[gw_idx];
        ESP_LOGI(TAG, "Trying gateway %d: %02X:%02X:%02X:%02X:%02X:%02X",
                 gw_idx, gw_mac[0], gw_mac[1], gw_mac[2], gw_mac[3], gw_mac[4], gw_mac[5]);
        
        // Retry sending to this gateway
        for (int retry = 0; retry < ESPNOW_SEND_RETRIES; retry++) {
            // Reset ACK flag
            ack_received = false;
            ack_seq_received = 0;
            
            esp_err_t err = esp_now_send(gw_mac, data, len);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Gateway %d retry %d send failed: %s", gw_idx, retry, esp_err_to_name(err));
                vTaskDelay(pdMS_TO_TICKS(100 * (1 << retry)));  // Exponential backoff
                continue;
            }
            
            // Wait for ACK with timeout
            #define ACK_TIMEOUT_MS 500
            int64_t start_time = esp_timer_get_time();
            bool ack_ok = false;
            
            while ((esp_timer_get_time() - start_time) < (ACK_TIMEOUT_MS * 1000)) {
                if (ack_received && ack_seq_received == expected_seq) {
                    ack_ok = true;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(10));  // Check every 10ms
            }
            
            if (ack_ok) {
                tx_stats.successful_acks++;
                int success_rate = (tx_stats.successful_acks * 100) / tx_stats.total_attempts;
                
                ESP_LOGI(TAG, "✓ Sent successfully to gateway %d (retry %d) with ACK confirmation", gw_idx, retry);
                ESP_LOGI(TAG, "📊 Stats: %u/%u successful (%.1f%% success rate)", 
                         tx_stats.successful_acks, tx_stats.total_attempts, success_rate / 10.0);
                
                // Save this gateway as last successful
                if (gw_idx != start_gw) {
                    ESP_LOGI(TAG, "Gateway failover: %d -> %d", start_gw, gw_idx);
                    nvs_set_last_gateway(gw_idx);
                }
                return ESP_OK;
            } else {
                ESP_LOGW(TAG, "Gateway %d retry %d: packet sent but no ACK received", gw_idx, retry);
                vTaskDelay(pdMS_TO_TICKS(100 * (1 << retry)));  // Exponential backoff
            }
        }
        
        ESP_LOGE(TAG, "✗ Gateway %d failed after %d retries", gw_idx, ESPNOW_SEND_RETRIES);
    }
    
    tx_stats.failed_acks++;
    ESP_LOGE(TAG, "All gateways failed!");
    ESP_LOGE(TAG, "📊 Stats: %u/%u successful (%.1f%% success rate)", 
             tx_stats.successful_acks, tx_stats.total_attempts, 
             (tx_stats.successful_acks * 100.0) / tx_stats.total_attempts);
    
    return ESP_FAIL;
}

/* ====== LED status helpers (non-blocking via esp_timer) ====== */
static void led_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_GPIO, !LED_ON_LEVEL); // Start OFF
}

static inline void led_set(bool on) {
    gpio_set_level(LED_GPIO, on ? LED_ON_LEVEL : !LED_ON_LEVEL);
}

typedef struct {
    int remaining;
    int on_ms;
    int off_ms;
    bool active;
    bool led_on_phase;
} led_pattern_t;

static led_pattern_t led_pat = {};
static esp_timer_handle_t led_timer = nullptr;

static void led_timer_cb(void *arg) {
    (void)arg;
    if (!led_pat.active) return;
    if (led_pat.led_on_phase) {
        led_set(false);
        led_pat.led_on_phase = false;
        led_pat.remaining--;
        if (led_pat.remaining <= 0) {
            led_pat.active = false;
            return;
        }
        esp_timer_start_once(led_timer, (uint64_t)led_pat.off_ms * 1000ULL);
    } else {
        led_set(true);
        led_pat.led_on_phase = true;
        esp_timer_start_once(led_timer, (uint64_t)led_pat.on_ms * 1000ULL);
    }
}

static void led_pattern_start(int count, int on_ms, int off_ms) {
    if (count <= 0) return;
    if (!led_timer) {
        esp_timer_create_args_t args = {
            .callback = led_timer_cb,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "ledpat",
            .skip_unhandled_events = false
        };
        if (esp_timer_create(&args, &led_timer) != ESP_OK) {
            return;
        }
    }
    esp_timer_stop(led_timer);
    led_pat = {count, on_ms, off_ms, true, false};
    // start immediately with ON phase
    esp_timer_start_once(led_timer, 1000); // 1 ms
}

static inline void led_pattern_searching(void) {
    led_pattern_start(3, 150, 150);
}

static inline void led_pattern_tx(void) {
    led_pattern_start(3, 80, 80);
}

static inline void led_pattern_error(void) {
    led_pattern_start(6, 70, 70);
}

// JSON builder removed in favor of a compact binary packet (SensorPacketV1)

// Use level_calculator module instead of local implementation

/* Get device MAC (STA MAC) as ID */
static void get_device_mac(uint8_t mac_out[6]) {
    esp_read_mac(mac_out, ESP_MAC_WIFI_STA);
}

/* ESP-NOW receive callback - handles ACK packets from gateway */
static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    // Check if it's an ACK packet
    if (len == sizeof(AckPacket)) {
        AckPacket *ack = (AckPacket*)data;
        
        // Validate ACK
        if (ack->magic == ACK_MAGIC && ack->version == ACK_VERSION) {
            ack_received = true;
            ack_seq_received = ack->ack_seq;
            ack_gateway_id = ack->gateway_id;
            
            ESP_LOGI(TAG, "✓ ACK recebido: seq=%u, rssi=%d, gateway=%u, status=%u",
                     ack->ack_seq, ack->rssi, ack->gateway_id, ack->status);
        } else {
            ESP_LOGW(TAG, "ACK inválido: magic=0x%02X, version=%u", ack->magic, ack->version);
        }
    } else {
        ESP_LOGD(TAG, "espnow recv len=%d from " MACSTR, len, MAC2STR(recv_info->src_addr));
    }
}

/* Initialize ADC */
static void init_adc(void) {
    // Configurar ADC oneshot
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_VIN,
        .clk_src = (adc_oneshot_clk_src_t)0,  // Default clock source
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
    
    // Configurar canal
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_VIN, &config));
    
    // Tentar calibração (opcional, pode falhar em alguns chips)
    #if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_VIN,
        .chan = ADC_CHANNEL_VIN,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_handle);
    #else
    // No calibration available
    esp_err_t ret = ESP_ERR_NOT_SUPPORTED;
    adc1_cali_handle = NULL;
    #endif
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC calibration OK");
    } else {
        ESP_LOGW(TAG, "ADC calibration not available, using raw values");
        adc1_cali_handle = NULL;
    }
}

/* Initialize ESP-NOW (WiFi must be initialized) */
static esp_err_t init_espnow(void) {
    esp_err_t err;
    
    // Create default event loop (required for WiFi)
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(err));
        return err;
    }
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(err));
        return err;
    }
    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_storage failed: %s", esp_err_to_name(err));
        return err;
    }
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(err));
        return err;
    }
    // Disable power save for better ESP-NOW responsiveness
    err = esp_wifi_set_ps(WIFI_PS_NONE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_ps failed: %s", esp_err_to_name(err));
        return err;
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(err));
        return err;
    }
    
    // Set fixed WiFi channel for ESP-NOW (must match gateway)
    err = esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_channel failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "WiFi channel set to %d", ESPNOW_CHANNEL);
    
    err = esp_now_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_now_init failed: %s", esp_err_to_name(err));
        return err;
    }
    esp_now_register_recv_cb(espnow_recv_cb);
    
    // Register all configured gateways as peers
    int peers_added = 0;
    for (uint8_t i = 0; i < MAX_GATEWAYS; i++) {
        if (!is_gateway_valid(i)) {
            ESP_LOGW(TAG, "Gateway %d not configured (skipping)", i);
            continue;
        }
        
        esp_now_peer_info_t peer_info = {};
        memcpy(peer_info.peer_addr, GATEWAY_MACS[i], 6);
        peer_info.channel = ESPNOW_CHANNEL;
        peer_info.ifidx = WIFI_IF_STA;
        peer_info.encrypt = false;
        
        err = esp_now_add_peer(&peer_info);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add gateway %d: %s", i, esp_err_to_name(err));
            continue;
        }
        
        ESP_LOGI(TAG, "Gateway %d peer added: %02X:%02X:%02X:%02X:%02X:%02X (channel %d)",
                 i, GATEWAY_MACS[i][0], GATEWAY_MACS[i][1], GATEWAY_MACS[i][2],
                 GATEWAY_MACS[i][3], GATEWAY_MACS[i][4], GATEWAY_MACS[i][5], ESPNOW_CHANNEL);
        peers_added++;
    }
    
    if (peers_added == 0) {
        ESP_LOGE(TAG, "No gateways configured!");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Total gateways configured: %d", peers_added);
    uint8_t last_gw = nvs_get_last_gateway();
    if (is_gateway_valid(last_gw)) {
        ESP_LOGI(TAG, "Last successful gateway: %d (%02X:%02X:%02X:%02X:%02X:%02X)", 
                 last_gw, GATEWAY_MACS[last_gw][0], GATEWAY_MACS[last_gw][1], 
                 GATEWAY_MACS[last_gw][2], GATEWAY_MACS[last_gw][3],
                 GATEWAY_MACS[last_gw][4], GATEWAY_MACS[last_gw][5]);
    }
    
    return ESP_OK;
}

/* Main measurement + send task executed once per boot cycle (then deep sleep) */
extern "C" void app_main(void) {
    esp_err_t err;

    // Disable task watchdog to avoid noisy WDT logs in this periodic loop
    esp_task_wdt_deinit();

    // init NVS
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // init pins & adc
    ultrasonic01::Pins us_pins = {TRIG_GPIO, ECHO_GPIO};
    ultrasonic01::init_pins(us_pins);
    init_adc();
    led_init();  // Initialize LED GPIO

    // Initialize Kalman filter for ultrasonic sensor (persistent across measurements)
    static ultrasonic01::KalmanFilter kalman_filter(1.0f, 2.0f);  // process_noise=1, measurement_noise=2
    
    // Indica inicialização / procurando gateway (rádio subindo)
    led_pattern_searching();

    // init wifi & espnow
    ESP_LOGI(TAG, "Initializing Wi-Fi and ESP-NOW...");
    ESP_ERROR_CHECK(init_espnow());

    while (true) {
        // get seq
        uint32_t seq = 0;
        if (nvs_get_seq(&seq) != ESP_OK) seq = 0;
        seq += 1; // increment for this send

        // perform ultrasonic measurements with Kalman filtering
        int readings[ULTRA_SAMPLE_RETRIES];
        int valid_readings = 0;
        ultrasonic01::Pins pins{TRIG_GPIO, ECHO_GPIO};
        
        // Take multiple readings and apply Kalman filter
        for (int i=0;i<ULTRA_SAMPLE_RETRIES;i++) {
            int d = ultrasonic01::measure_cm(pins);
            if (d < 0) {
                ESP_LOGW(TAG, "ultra read %d = timeout/error", i);
                readings[i] = -1;
            } else {
                // Apply Kalman filter to raw measurement
                float filtered = kalman_filter.update(d);
                readings[i] = (int)(filtered + 0.5f);  // Round to nearest integer
                valid_readings++;
                ESP_LOGI(TAG, "Reading %d: raw=%dcm filtered=%dcm", i, d, readings[i]);
            }
            vTaskDelay(pdMS_TO_TICKS(ULTRA_MEASURE_DELAY_MS));
        }

        // choose median among valid — if some invalid, prefer any valid, else mark invalid
        int distance_cm = -1;
        if (valid_readings == 0) {
            ESP_LOGW(TAG, "No valid ultrasonic readings");
            kalman_filter.reset();  // Reset filter on total failure
        } else {
            int a = readings[0] < 0 ? 10000 : readings[0];
            int b = readings[1] < 0 ? 10000 : readings[1];
            int c = readings[2] < 0 ? 10000 : readings[2];
            int med = ultrasonic01::median3(a,b,c);
            if (med > 9999) {
                distance_cm = -1;
                kalman_filter.reset();
            } else {
                distance_cm = med;
            }
        }

        // validate range
        if (distance_cm < 0 || distance_cm < MIN_VALID_CM || distance_cm > MAX_VALID_CM) {
            ESP_LOGW(TAG, "distance invalid (%d cm). Clamping/flagging.", distance_cm);
            if (distance_cm < MIN_VALID_CM) distance_cm = MIN_VALID_CM;
            if (distance_cm > MAX_VALID_CM) distance_cm = MAX_VALID_CM;
        }

        // read vin mv
        int vin_mv = read_vin_mv();
        if (vin_mv < 0) {
            ESP_LOGW(TAG, "ADC read failed");
            vin_mv = 0;
        }

        // compute level, percentual, volume
        level_calculator::Model model{LEVEL_MAX_CM, SENSOR_OFFSET_CM, VOL_MAX_L};
        auto res = level_calculator::compute(distance_cm, model);
        int level_cm = res.level_cm;
        int percentual = res.percentual;
        int volume_l = res.volume_l;

        ESP_LOGI(TAG, "meas: distance=%d cm, level=%d cm, pct=%d%%, vol=%d L, vin=%d mV, seq=%u",
                 distance_cm, level_cm, percentual, volume_l, vin_mv, seq);

        // ============================================================================
        // ANOMALY DETECTION
        // ============================================================================
        uint8_t flags = 0;
        uint8_t alert_type = ALERT_NONE;
        
        if (!anomaly_detection_initialized) {
            // First measurement - just store baseline
            last_level_cm = level_cm;
            last_change_seq = seq;
            anomaly_detection_initialized = true;
            ESP_LOGI(TAG, "🎯 Anomaly detection initialized (baseline=%dcm)", level_cm);
        } else {
            int16_t delta_cm = level_cm - last_level_cm;
            uint32_t readings_since_change = seq - last_change_seq;
            uint32_t minutes_since_change = (readings_since_change * SAMPLE_INTERVAL_S) / 60;
            
            // Check for rapid drop (leak detection)
            if (delta_cm <= -RAPID_CHANGE_THRESHOLD_CM) {
                flags |= FLAG_IS_ALERT;
                alert_type = ALERT_RAPID_DROP;
                ESP_LOGW(TAG, "🚨 ALERTA: Queda rápida detectada! Δ=%dcm (possível vazamento)", delta_cm);
            }
            // Check for rapid rise (pump failure / flood)
            else if (delta_cm >= RAPID_CHANGE_THRESHOLD_CM) {
                flags |= FLAG_IS_ALERT;
                alert_type = ALERT_RAPID_RISE;
                ESP_LOGW(TAG, "🚨 ALERTA: Subida rápida detectada! Δ=%dcm (falha de bomba/inundação)", delta_cm);
            }
            // Check for sensor stuck (no significant change for long period)
            else if (minutes_since_change >= NO_CHANGE_MINUTES && 
                     abs(delta_cm) <= NO_CHANGE_THRESHOLD_CM) {
                flags |= FLAG_IS_ALERT;
                alert_type = ALERT_SENSOR_STUCK;
                ESP_LOGW(TAG, "🚨 ALERTA: Sensor travado! Sem mudança por %u minutos", minutes_since_change);
            }
            
            // Update baseline if significant change detected
            if (abs(delta_cm) > NO_CHANGE_THRESHOLD_CM) {
                last_level_cm = level_cm;
                last_change_seq = seq;
            }
            
            if (flags & FLAG_IS_ALERT) {
                ESP_LOGI(TAG, "⚠️ Pacote marcado como alerta (tipo=%u)", alert_type);
            }
        }

        // build payload (binary packet)
        uint8_t dev_mac[6];
        get_device_mac(dev_mac);
        SensorPacketV1 pkt{};
        pkt.version = SENSOR_PACKET_VERSION;
        pkt.node_id = 3; // Node 3 - RCB3 - Casa de Bombas 03
        memcpy(pkt.mac, dev_mac, sizeof(pkt.mac));
        pkt.seq = seq;
        pkt.distance_cm = (int16_t)distance_cm;
        pkt.level_cm = (int16_t)level_cm;
        pkt.percentual = (uint8_t)percentual;
        pkt.volume_l = (uint32_t)volume_l;
        pkt.vin_mv = (int16_t)vin_mv;
        pkt.flags = flags;
        pkt.alert_type = alert_type;
        pkt.rssi = 0;   // gateway will overwrite
        pkt.ts_ms = 0;  // gateway will overwrite

        esp_err_t send_err = espnow_send_payload((const uint8_t*)&pkt, sizeof(pkt), seq);
        if (send_err == ESP_OK) {
            ESP_LOGI(TAG, "espnow send OK (binary packet v%d) with ACK", pkt.version);
            nvs_set_seq(seq);
            led_pattern_tx();
        } else {
            ESP_LOGE(TAG, "espnow send failed: %s", esp_err_to_name(send_err));
            led_pattern_error();
        }

        // wait interval instead of deep sleep
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_S * 1000));
    }
}
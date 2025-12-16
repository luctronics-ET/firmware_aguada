// node_ultra02.cpp
// ESP-IDF firmware for ESP32-C3 Supermini
// - Two Ultrasonic HC-SR04 sensors (reservatórios independentes)
//   * Sensor A: trig = GPIO_NUM_1, echo = GPIO_NUM_0
//   * Sensor B: trig = GPIO_NUM_3, echo = GPIO_NUM_2
// - ADC Vin (GPIO_NUM_4) with V/2 divider
// - ESP-NOW broadcast send
// - integer-only calculations (1 cm resolution, integer % and liters)
// - seq counter persisted in NVS

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

static const char *TAG = "node_ultra02";

/* ====== CONFIGURABLE PARAMETERS ====== */
/* Hardware pins */
#define TRIG_A_GPIO       GPIO_NUM_1
#define ECHO_A_GPIO       GPIO_NUM_0
#define TRIG_B_GPIO       GPIO_NUM_3
#define ECHO_B_GPIO       GPIO_NUM_2
#define ADC_CHANNEL_VIN   ADC_CHANNEL_4  // ADC1 channel 4 = GPIO4
#define ADC_UNIT_VIN      ADC_UNIT_1
#define LED_GPIO          GPIO_NUM_8     // built-in LED on ESP32-C3 Supermini
#ifdef CONFIG_LED_ACTIVE_HIGH
#define LED_ON_LEVEL      1
#else
#define LED_ON_LEVEL      0
#endif

/* Telemetry / tank model */
#define VOL_MAX_L         245000    // liters (vol_max)
#define LEVEL_MAX_CM      450      // cm (level_max)
#define SENSOR_OFFSET_CM  20       // sensor_offset (sensor is 20cm above level_max)

/* Sampling and retries */
#define SAMPLE_INTERVAL_S     30    // loop interval in seconds (active loop)
#define ULTRA_SAMPLE_RETRIES  3     // number of ultrasonic readings to take (for median)
#define ULTRA_MEASURE_DELAY_MS 60   // delay between raw ultrasonic attempts
#define ESPNOW_SEND_RETRIES   2

/* Ultrasonic validation */
#define MIN_VALID_CM    5
#define MAX_VALID_CM    450

/* ADC configuration */
#define ADC_ATTEN        ADC_ATTEN_DB_12
#define ADC_BITWIDTH     ADC_BITWIDTH_12

/* ESP-NOW configuration */
/* Gateway MAC address - must match the actual gateway device */
static const uint8_t GATEWAY_MAC[6] = {0x80, 0xf3, 0xda, 0x62, 0xa7, 0x84};  // ESP32 DevKit V1
static const uint8_t SENSOR_B_MAC[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xC2};

/* ESP-NOW channel - must match gateway */
#define ESPNOW_CHANNEL 11
/* ===================================== */

/* NVS keys */
#define NVS_NAMESPACE "node_cfg"
#define NVS_SEQ_KEY   "seq"

/* ADC handles (global) */
static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t adc1_cali_handle = NULL;

struct DistanceResult {
    int value_cm;
    bool valid;
    int valid_samples;
};

struct TelemetryValues {
    int distance_cm;
    int level_cm;
    int percentual;
    int volume_l;
    int vin_mv;
};

/* Read ADC and convert to mV, considering V/2 divider */
static int read_vin_mv(void) {
    if (!adc1_handle) return -1;
    int raw = 0;
    esp_err_t err = adc_oneshot_read(adc1_handle, ADC_CHANNEL_VIN, &raw);
    if (err != ESP_OK) return -1;

    int voltage_mv = 0;
    if (adc1_cali_handle) {
        adc_cali_raw_to_voltage(adc1_cali_handle, raw, &voltage_mv);
    } else {
        voltage_mv = (raw * 3300) / 4095;
    }
    return voltage_mv * 2; // account divider (V/2)
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

/* ESP-NOW send wrapper (to gateway) */
static esp_err_t espnow_send_payload(const uint8_t *peer_mac, const uint8_t *data, size_t len) {
    esp_err_t err = ESP_FAIL;
    const uint8_t *dst = peer_mac ? peer_mac : GATEWAY_MAC;
    for (int r=0; r<ESPNOW_SEND_RETRIES; ++r) {
        err = esp_now_send(dst, data, len);
        if (err == ESP_OK) break;
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return err;
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
    esp_timer_start_once(led_timer, 1000); // 1 ms
}

static inline void led_pattern_searching(void) { led_pattern_start(3, 150, 150); }
static inline void led_pattern_tx(void) { led_pattern_start(3, 80, 80); }
static inline void led_pattern_error(void) { led_pattern_start(6, 70, 70); }

/* Get device MAC (STA MAC) as ID */
static void get_device_mac(uint8_t mac_out[6]) {
    esp_read_mac(mac_out, ESP_MAC_WIFI_STA);
}

/* ESP-NOW receive callback (not used but necessary to initialize) */
static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    ESP_LOGD(TAG, "espnow recv len=%d from " MACSTR, len, MAC2STR(recv_info->src_addr));
}

/* Initialize ADC */
static void init_adc(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_VIN,
        .clk_src = (adc_oneshot_clk_src_t)0,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_VIN, &config));

    #if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_VIN,
        .chan = ADC_CHANNEL_VIN,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_handle);
    #else
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
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_LOGI(TAG, "WiFi channel set to %d", ESPNOW_CHANNEL);

    ESP_ERROR_CHECK(esp_now_init());
    esp_now_register_recv_cb(espnow_recv_cb);

    esp_now_peer_info_t peer_info = {};
    memcpy(peer_info.peer_addr, GATEWAY_MAC, 6);
    peer_info.channel = ESPNOW_CHANNEL;
    peer_info.ifidx = WIFI_IF_STA;
    peer_info.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer_info));
    ESP_LOGI(TAG, "Gateway peer added: %02X:%02X:%02X:%02X:%02X:%02X (channel %d)",
             GATEWAY_MAC[0], GATEWAY_MAC[1], GATEWAY_MAC[2],
             GATEWAY_MAC[3], GATEWAY_MAC[4], GATEWAY_MAC[5], ESPNOW_CHANNEL);
    return ESP_OK;
}

static DistanceResult measure_sensor(const ultrasonic01::Pins &pins, const char *label) {
    int readings[ULTRA_SAMPLE_RETRIES];
    int valid = 0;
    for (int i=0;i<ULTRA_SAMPLE_RETRIES;i++) {
        int d = ultrasonic01::measure_cm(pins);
        if (d < 0) {
            ESP_LOGW(TAG, "%s read %d = timeout/error", label, i);
            readings[i] = -1;
        } else {
            readings[i] = d;
            valid++;
        }
        vTaskDelay(pdMS_TO_TICKS(ULTRA_MEASURE_DELAY_MS));
    }

    DistanceResult res{.value_cm = -1, .valid = false, .valid_samples = valid};
    if (valid == 0) {
        ESP_LOGW(TAG, "%s: sem leituras válidas", label);
        return res;
    }

    int a = readings[0] < 0 ? 10000 : readings[0];
    int b = readings[1] < 0 ? 10000 : readings[1];
    int c = readings[2] < 0 ? 10000 : readings[2];
    int med = ultrasonic01::median3(a,b,c);
    if (med > 9999) {
        res.value_cm = -1;
        return res;
    }

    res.value_cm = med;
    if (res.value_cm < MIN_VALID_CM || res.value_cm > MAX_VALID_CM) {
        ESP_LOGW(TAG, "%s: valor fora da faixa (%d cm). Clamping.", label, res.value_cm);
        if (res.value_cm < MIN_VALID_CM) res.value_cm = MIN_VALID_CM;
        if (res.value_cm > MAX_VALID_CM) res.value_cm = MAX_VALID_CM;
    } else {
        res.valid = true;
    }
    ESP_LOGI(TAG, "%s: dist=%d cm (amostras válidas %d/%d)", label, res.value_cm, valid, ULTRA_SAMPLE_RETRIES);
    return res;
}

static TelemetryValues compute_values(int distance_cm, int vin_mv) {
    level_calculator::Model model{LEVEL_MAX_CM, SENSOR_OFFSET_CM, VOL_MAX_L};
    auto res = level_calculator::compute(distance_cm, model);
    TelemetryValues v{};
    v.distance_cm = distance_cm;
    v.level_cm = res.level_cm;
    v.percentual = res.percentual;
    v.volume_l = res.volume_l;
    v.vin_mv = vin_mv;
    return v;
}

extern "C" void app_main(void) {
    esp_task_wdt_deinit();

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ultrasonic01::Pins sensorA{TRIG_A_GPIO, ECHO_A_GPIO};
    ultrasonic01::Pins sensorB{TRIG_B_GPIO, ECHO_B_GPIO};
    ultrasonic01::init_pins(sensorA);
    ultrasonic01::init_pins(sensorB);
    init_adc();
    led_init();
    led_pattern_searching();

    ESP_LOGI(TAG, "Initializing Wi-Fi and ESP-NOW...");
    ESP_ERROR_CHECK(init_espnow());

    while (true) {
        uint32_t seq = 0;
        if (nvs_get_seq(&seq) != ESP_OK) seq = 0;

        // Sensor A (MAC do próprio ESP)
        DistanceResult dA = measure_sensor(sensorA, "ultraA");
        int distA = dA.valid ? dA.value_cm : MIN_VALID_CM;
        int vin_mv = read_vin_mv();
        if (vin_mv < 0) {
            ESP_LOGW(TAG, "ADC read failed");
            vin_mv = 0;
        }
        TelemetryValues tA = compute_values(distA, vin_mv);

        seq += 1;
        uint8_t macA[6];
        get_device_mac(macA);
        SensorPacketV1 pktA{};
        pktA.version = SENSOR_PACKET_VERSION;
        pktA.node_id = 2; // Node Ultra02
        memcpy(pktA.mac, macA, sizeof(pktA.mac));
        pktA.seq = seq;
        pktA.distance_cm = (int16_t)tA.distance_cm;
        pktA.level_cm = (int16_t)tA.level_cm;
        pktA.percentual = (uint8_t)tA.percentual;
        pktA.volume_l = (uint32_t)tA.volume_l;
        pktA.vin_mv = (int16_t)tA.vin_mv;
        pktA.rssi = 0;
        pktA.ts_ms = 0;

        ESP_LOGI(TAG, "A: dist=%d cm (v%d/%d) level=%d cm pct=%d%% vol=%dL vin=%d seq=%u MAC=%02X:%02X:%02X:%02X:%02X:%02X",
                 distA, dA.valid_samples, ULTRA_SAMPLE_RETRIES, tA.level_cm, tA.percentual, tA.volume_l, tA.vin_mv, seq,
                 macA[0], macA[1], macA[2], macA[3], macA[4], macA[5]);

        esp_err_t send_errA = espnow_send_payload(NULL, (const uint8_t*)&pktA, sizeof(pktA));
        if (send_errA == ESP_OK) {
            led_pattern_tx();
        } else {
            ESP_LOGE(TAG, "espnow send A failed: %s", esp_err_to_name(send_errA));
            led_pattern_error();
        }
        nvs_set_seq(seq);

        // Sensor B (MAC fixo AA:BB:CC:DD:EE:C2)
        DistanceResult dB = measure_sensor(sensorB, "ultraB");
        int distB = dB.valid ? dB.value_cm : MIN_VALID_CM;
        TelemetryValues tB = compute_values(distB, vin_mv); // usa mesma leitura de VIN

        seq += 1;
        SensorPacketV1 pktB{};
        pktB.version = SENSOR_PACKET_VERSION;
        pktB.node_id = 2; // Node Ultra02
        memcpy(pktB.mac, SENSOR_B_MAC, sizeof(pktB.mac));
        pktB.seq = seq;
        pktB.distance_cm = (int16_t)tB.distance_cm;
        pktB.level_cm = (int16_t)tB.level_cm;
        pktB.percentual = (uint8_t)tB.percentual;
        pktB.volume_l = (uint32_t)tB.volume_l;
        pktB.vin_mv = (int16_t)tB.vin_mv;
        pktB.rssi = 0;
        pktB.ts_ms = 0;

        ESP_LOGI(TAG, "B: dist=%d cm (v%d/%d) level=%d cm pct=%d%% vol=%dL vin=%d seq=%u MAC=%02X:%02X:%02X:%02X:%02X:%02X",
                 distB, dB.valid_samples, ULTRA_SAMPLE_RETRIES, tB.level_cm, tB.percentual, tB.volume_l, tB.vin_mv, seq,
                 SENSOR_B_MAC[0], SENSOR_B_MAC[1], SENSOR_B_MAC[2], SENSOR_B_MAC[3], SENSOR_B_MAC[4], SENSOR_B_MAC[5]);

        esp_err_t send_errB = espnow_send_payload(NULL, (const uint8_t*)&pktB, sizeof(pktB));
        if (send_errB == ESP_OK) {
            led_pattern_tx();
        } else {
            ESP_LOGE(TAG, "espnow send B failed: %s", esp_err_to_name(send_errB));
            led_pattern_error();
        }
        nvs_set_seq(seq);

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_S * 1000));
    }
}

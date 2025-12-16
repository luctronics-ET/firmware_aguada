/**
 * AGUADA - Gateway Simplified (ESP32 DevKit V1)
 * ESP-NOW Receiver with Serial Output
 * 
 * Features:
 * - Canal fixo 11
 * - Lightweight (no WiFi, no HTTP)
 * - Binary telemetry_packet.h parsing
 * - Serial output + LED heartbeat
 * 
 * ESP32 DevKit V1
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_http_client.h"
#include "esp_event.h"

#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "telemetry_packet.h"

#define TAG "AGUADA_GATEWAY"

// ============================================================================
// CONFIGURAÇÕES
// ============================================================================

#define ESPNOW_CHANNEL 11
#define LED_BUILTIN GPIO_NUM_2  // ESP32 DevKit V1 uses GPIO2 for LED
#define HEARTBEAT_INTERVAL_MS 2000
#define MAX_PAYLOAD_SIZE 256

// Wi-Fi STA credentials for HTTP POST to backend
#define WIFI_SSID "luciano"
#define WIFI_PASS "Luciano19852012"

// HTTP endpoint for SensorPacket ingest (ajuste para o IP/porta do backend PHP)
#define INGEST_URL "http://192.168.0.117:8080/ingest_sensorpacket.php"

// ============================================================================
// TYPES
// ============================================================================

typedef struct {
    uint8_t src_addr[6];
    SensorPacketV1 data;
} espnow_packet_t;

// ============================================================================
// GLOBALS
// ============================================================================

static uint8_t gateway_mac[6];
static int64_t last_heartbeat = 0;
static bool led_state = false;
static QueueHandle_t espnow_queue = NULL;
static QueueHandle_t http_queue = NULL;
static bool wifi_got_ip = false;
static esp_event_handler_instance_t wifi_any_id_inst;
static esp_event_handler_instance_t ip_got_ip_inst;

// Métricas simples
static struct {
    uint32_t packets_received;
    uint32_t packets_parsed;
    uint32_t parse_errors;
} gateway_metrics = {0};

// ============================================================================
// UTILITIES
// ============================================================================

static void mac_to_string(const uint8_t *mac, char *str) {
    snprintf(str, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void log_current_channel(void) {
    uint8_t primary = 0;
    wifi_second_chan_t sc = WIFI_SECOND_CHAN_NONE;
    if (esp_wifi_get_channel(&primary, &sc) == ESP_OK) {
        ESP_LOGI(TAG, "Canal atual WiFi/ESP-NOW: %u (sec=%d)", primary, (int)sc);
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_got_ip = false;
        ESP_LOGW(TAG, "WiFi desconectado, tentando reconectar...");
        esp_wifi_connect();
    }
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "WiFi associado ao AP");
    }
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        wifi_got_ip = true;
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi IP: " IPSTR, IP2STR(&evt->ip_info.ip));
        log_current_channel();
    }
}

static esp_err_t http_post_packet(const SensorPacketV1 *pkt) {
    char json[256];
    int n = snprintf(json, sizeof(json),
        "{\"version\":%u,\"node_id\":%u,\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"seq\":%u,"
        "\"distance_cm\":%d,\"level_cm\":%d,\"percentual\":%u,\"volume_l\":%u,\"vin_mv\":%d,\"rssi\":%d,\"ts_ms\":%u}",
        (unsigned)pkt->version, (unsigned)pkt->node_id,
        pkt->mac[0], pkt->mac[1], pkt->mac[2], pkt->mac[3], pkt->mac[4], pkt->mac[5],
        (unsigned)pkt->seq,
        (int)pkt->distance_cm, (int)pkt->level_cm, (unsigned)pkt->percentual, (unsigned)pkt->volume_l,
        (int)pkt->vin_mv, (int)pkt->rssi, (unsigned)pkt->ts_ms);

    if (n <= 0 || n >= (int)sizeof(json)) {
        ESP_LOGW(TAG, "json truncado (%d)", n);
        return ESP_FAIL;
    }

    esp_http_client_config_t cfg = {0};
    cfg.url = INGEST_URL;
    cfg.method = HTTP_METHOD_POST;
    cfg.timeout_ms = 3000;
    cfg.transport_type = HTTP_TRANSPORT_OVER_TCP;

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        ESP_LOGW(TAG, "http_client init falhou");
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json, n);

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "HTTP post erro: %s", esp_err_to_name(err));
    } else {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP status: %d", status);
    }

    esp_http_client_cleanup(client);
    return err;
}

// ============================================================================
// ESP-NOW CALLBACK
// ============================================================================

static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    if (!recv_info || !espnow_queue) {
        return;
    }

    // Validate packet size
    if (len != sizeof(SensorPacketV1)) {
        ESP_LOGW(TAG, "⚠ Tamanho inválido de pacote: %d (esperado %d)", len, sizeof(SensorPacketV1));
        gateway_metrics.parse_errors++;
        return;
    }

    espnow_packet_t packet = {0};
    memcpy(packet.src_addr, recv_info->src_addr, 6);
    memcpy(&packet.data, data, sizeof(SensorPacketV1));

    // Enrich with gateway-side info
    memcpy(packet.data.mac, recv_info->src_addr, 6);
    if (recv_info->rx_ctrl) {
        packet.data.rssi = recv_info->rx_ctrl->rssi;
    }
    packet.data.ts_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);

    gateway_metrics.packets_received++;
    
    // Enqueue without blocking
    BaseType_t result = xQueueSendFromISR(espnow_queue, &packet, NULL);
    if (result != pdTRUE) {
        ESP_LOGW(TAG, "⚠ Queue cheia - pacote descartado");
    }
}

// ============================================================================
// PACKET PROCESSING TASK
// ============================================================================

static void packet_processing_task(void *pvParameters) {
    espnow_packet_t packet;
    
    while (1) {
        if (xQueueReceive(espnow_queue, &packet, pdMS_TO_TICKS(1000))) {
            char src_mac_str[18];
            mac_to_string(packet.src_addr, src_mac_str);

            // Parse packet
            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "╔════════════════════════════════════════════════════╗");
            ESP_LOGI(TAG, "║ ✓ ESP-NOW recebido de: %s", src_mac_str);
            ESP_LOGI(TAG, "╠════════════════════════════════════════════════════╣");
            
            // Validate packet version
            if (packet.data.version == SENSOR_PACKET_VERSION) {
                gateway_metrics.packets_parsed++;
                
                // Parse sensor data
                ESP_LOGI(TAG, "║ Versão: %u", packet.data.version);
                ESP_LOGI(TAG, "║ Nó ID: %u", packet.data.node_id);
                ESP_LOGI(TAG, "║ Distância: %d cm", packet.data.distance_cm);
                ESP_LOGI(TAG, "║ Nível: %d cm", packet.data.level_cm);
                ESP_LOGI(TAG, "║ Percentual: %u%%", packet.data.percentual);
                ESP_LOGI(TAG, "║ Volume: %lu L", packet.data.volume_l);
                ESP_LOGI(TAG, "║ Tensão: %d mV", packet.data.vin_mv);
                ESP_LOGI(TAG, "║ RSSI: %d dBm", packet.data.rssi);
                ESP_LOGI(TAG, "║ Sequência: %u", packet.data.seq);
                
                // Output JSON to Serial for external processing
                printf("TELEMETRY:{\"mac\":\"%s\",\"distance\":%d,\"level\":%d,\"volume\":%" PRIu32 ",\"voltage\":%d,\"seq\":%" PRIu32 "}\n",
                       src_mac_str, packet.data.distance_cm, packet.data.level_cm, packet.data.volume_l, 
                       packet.data.vin_mv, packet.data.seq);
                fflush(stdout);

                // Enfileira para envio HTTP em worker dedicado
                if (http_queue) {
                    SensorPacketV1 copy = packet.data;
                    if (xQueueSend(http_queue, &copy, 0) != pdTRUE) {
                        ESP_LOGW(TAG, "Fila HTTP cheia - descartando envio");
                    }
                }
            } else {
                gateway_metrics.parse_errors++;
                ESP_LOGW(TAG, "║ ✗ Versão inválida: %u (esperado %u)", packet.data.version, SENSOR_PACKET_VERSION);
            }
            
            ESP_LOGI(TAG, "╚════════════════════════════════════════════════════╝");
        }
    }
}

// ============================================================================
// WIFI/NETWORK - STA + HTTP
// ============================================================================

static void wifi_sta_init(void) {
    ESP_LOGI(TAG, "Inicializando WiFi STA para ESP-NOW + HTTP...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    if (!netif) {
        ESP_LOGE(TAG, "Falha ao criar netif STA");
        return;
    }
    (void)netif;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, &wifi_any_id_inst));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, &ip_got_ip_inst));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    wifi_config_t sta_cfg = {0};
    strncpy((char *)sta_cfg.sta.ssid, WIFI_SSID, sizeof(sta_cfg.sta.ssid));
    strncpy((char *)sta_cfg.sta.password, WIFI_PASS, sizeof(sta_cfg.sta.password));
    sta_cfg.sta.ssid[sizeof(sta_cfg.sta.ssid) - 1] = '\0';
    sta_cfg.sta.password[sizeof(sta_cfg.sta.password) - 1] = '\0';
    sta_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    // Channel follows the AP; keep your AP fixed (e.g., 11) to match nodes.
    ESP_LOGI(TAG, "✓ WiFi STA conectado (canal do AP)");
}

// ============================================================================
// ESP-NOW INIT
// ============================================================================

static void espnow_init(void) {
    ESP_LOGI(TAG, "Inicializando ESP-NOW...");

    // Get gateway MAC
    esp_wifi_get_mac(WIFI_IF_STA, gateway_mac);
    char mac_str[18];
    mac_to_string(gateway_mac, mac_str);
    ESP_LOGI(TAG, "Gateway MAC: %s", mac_str);

    // Initialize ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_LOGI(TAG, "✓ ESP-NOW inicializado");

    // Register receive callback
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    ESP_LOGI(TAG, "✓ Callback ESP-NOW registrado");

    // Add broadcast peer (FF:FF:FF:FF:FF:FF); channel 0 follows current WiFi channel
    esp_now_peer_info_t peer = {0};
    peer.channel = 0;
    peer.encrypt = false;
    memset(peer.peer_addr, 0xFF, 6);

    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    ESP_LOGI(TAG, "✓ Peer broadcast adicionado (canal segue WiFi)");
}

// ============================================================================
// GPIO INIT
// ============================================================================

static void gpio_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_BUILTIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Initial LED state (off)
    gpio_set_level(LED_BUILTIN, 0);

    // Blink 3x fast
    for (int i = 0; i < 3; i++) {
        gpio_set_level(LED_BUILTIN, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LED_BUILTIN, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "✓ GPIO inicializado (LED=%d)", LED_BUILTIN);
}

// ============================================================================
// HEARTBEAT TASK
// ============================================================================

static void heartbeat_task(void *pvParameters) {
    while (1) {
        // LED heartbeat (blink every 2 seconds)
        if (esp_timer_get_time() - last_heartbeat >= HEARTBEAT_INTERVAL_MS * 1000) {
            last_heartbeat = esp_timer_get_time();
            led_state = !led_state;
            gpio_set_level(LED_BUILTIN, led_state ? 1 : 0);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// HTTP WORKER TASK
// ============================================================================

static void http_worker_task(void *pvParameters) {
    SensorPacketV1 pkt;
    while (1) {
        if (xQueueReceive(http_queue, &pkt, portMAX_DELAY)) {
            if (!wifi_got_ip) {
                ESP_LOGW(TAG, "Sem IP - não enviando HTTP");
                continue;
            }
            http_post_packet(&pkt);
        }
    }
}

// ============================================================================
// APP MAIN
// ============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔═══════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║     AGUADA Gateway (ESP32 DevKit V1)                     ║");
    ESP_LOGI(TAG, "║     ESP-NOW Receiver + HTTP forward                      ║");
    ESP_LOGI(TAG, "║     Canal fixo 11 | STA + HTTP POST                      ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    // Create packet queue
    espnow_queue = xQueueCreate(20, sizeof(espnow_packet_t));
    if (!espnow_queue) {
        ESP_LOGE(TAG, "Falha ao criar fila ESP-NOW");
        return;
    }
    ESP_LOGI(TAG, "✓ Fila ESP-NOW criada (20 slots)");

    // Create HTTP queue
    http_queue = xQueueCreate(20, sizeof(SensorPacketV1));
    if (!http_queue) {
        ESP_LOGE(TAG, "Falha ao criar fila HTTP");
        return;
    }
    ESP_LOGI(TAG, "✓ Fila HTTP criada (20 slots)");

    // Initialize GPIO
    gpio_init();

    // Initialize WiFi (STA for HTTP + fixed channel for ESP-NOW)
    wifi_sta_init();
    
    // Initialize ESP-NOW (after WiFi is up)
    espnow_init();

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "✓ Gateway inicializado e pronto!");
    log_current_channel();
    ESP_LOGI(TAG, "  - Modo: Receptor + HTTP POST para backend");
    ESP_LOGI(TAG, "  - Backend: %s", INGEST_URL);
    ESP_LOGI(TAG, "  - Aguardando dados dos sensores...");
    ESP_LOGI(TAG, "");

    // Create heartbeat task
    xTaskCreate(heartbeat_task, "heartbeat", 2048, NULL, 5, NULL);

    // Create packet processing task
    xTaskCreate(packet_processing_task, "packet_proc", 4096, NULL, 5, NULL);

    // Create HTTP worker task
    xTaskCreate(http_worker_task, "http_worker", 4096, NULL, 4, NULL);

    // Keep main task alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

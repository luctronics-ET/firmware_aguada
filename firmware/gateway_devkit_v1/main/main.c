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
#include <time.h>
#include <sys/time.h>

#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_http_client.h"
#include "esp_event.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "nvs.h"

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
static bool sntp_synced = false;
static esp_event_handler_instance_t wifi_any_id_inst;
static esp_event_handler_instance_t ip_got_ip_inst;

// NVS Persistent Queue Configuration
#define NVS_NAMESPACE "gw_queue"
#define NVS_KEY_HEAD  "q_head"
#define NVS_KEY_TAIL  "q_tail"
#define NVS_KEY_COUNT "q_count"
#define NVS_KEY_PKT   "pkt_%02d"  // Format: pkt_00 to pkt_49
#define NVS_QUEUE_SIZE 50

static nvs_handle_t nvs_queue_handle;
static uint8_t queue_head = 0;
static uint8_t queue_tail = 0;
static uint8_t queue_count = 0;

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

// SNTP time sync callback
static void sntp_sync_time_cb(struct timeval *tv) {
    sntp_synced = true;
    time_t now = tv->tv_sec;
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    ESP_LOGI(TAG, "⏰ SNTP sincronizado: %s", strftime_buf);
}

// Get current UNIX timestamp (seconds since epoch)
static uint32_t get_unix_timestamp(void) {
    if (!sntp_synced) {
        return 0; // Not synced yet
    }
    time_t now;
    time(&now);
    return (uint32_t)now;
}

// ============================================================================
// NVS PERSISTENT QUEUE (stores packets when backend offline)
// ============================================================================

// Initialize NVS queue - load head/tail/count from flash
static esp_err_t nvs_queue_init(void) {
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_queue_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ Erro ao abrir NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    // Load queue state (defaults to 0 if not found)
    nvs_get_u8(nvs_queue_handle, NVS_KEY_HEAD, &queue_head);
    nvs_get_u8(nvs_queue_handle, NVS_KEY_TAIL, &queue_tail);
    nvs_get_u8(nvs_queue_handle, NVS_KEY_COUNT, &queue_count);
    
    ESP_LOGI(TAG, "📦 Fila NVS inicializada: %u pacotes pendentes (head=%u tail=%u)",
             queue_count, queue_head, queue_tail);
    return ESP_OK;
}

// Push packet to NVS queue (circular buffer)
static esp_err_t nvs_queue_push(const SensorPacketV1 *pkt) {
    if (queue_count >= NVS_QUEUE_SIZE) {
        ESP_LOGW(TAG, "⚠️ Fila NVS cheia! Descartando pacote mais antigo");
        // Advance head (drop oldest packet)
        queue_head = (queue_head + 1) % NVS_QUEUE_SIZE;
        queue_count--;
    }
    
    // Write packet to NVS
    char key[16];
    snprintf(key, sizeof(key), NVS_KEY_PKT, queue_tail);
    esp_err_t err = nvs_set_blob(nvs_queue_handle, key, pkt, sizeof(SensorPacketV1));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ Erro ao salvar pacote na NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    // Update tail and count
    queue_tail = (queue_tail + 1) % NVS_QUEUE_SIZE;
    queue_count++;
    
    // Persist queue metadata
    nvs_set_u8(nvs_queue_handle, NVS_KEY_HEAD, queue_head);
    nvs_set_u8(nvs_queue_handle, NVS_KEY_TAIL, queue_tail);
    nvs_set_u8(nvs_queue_handle, NVS_KEY_COUNT, queue_count);
    nvs_commit(nvs_queue_handle);
    
    ESP_LOGI(TAG, "💾 Pacote salvo na NVS [%u/%u]", queue_count, NVS_QUEUE_SIZE);
    return ESP_OK;
}

// Pop packet from NVS queue (FIFO)
static esp_err_t nvs_queue_pop(SensorPacketV1 *pkt) {
    if (queue_count == 0) {
        return ESP_ERR_NOT_FOUND; // Queue empty
    }
    
    // Read packet from NVS
    char key[16];
    snprintf(key, sizeof(key), NVS_KEY_PKT, queue_head);
    size_t required_size = sizeof(SensorPacketV1);
    esp_err_t err = nvs_get_blob(nvs_queue_handle, key, pkt, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ Erro ao ler pacote da NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    // Update head and count
    queue_head = (queue_head + 1) % NVS_QUEUE_SIZE;
    queue_count--;
    
    // Persist queue metadata
    nvs_set_u8(nvs_queue_handle, NVS_KEY_HEAD, queue_head);
    nvs_set_u8(nvs_queue_handle, NVS_KEY_TAIL, queue_tail);
    nvs_set_u8(nvs_queue_handle, NVS_KEY_COUNT, queue_count);
    nvs_commit(nvs_queue_handle);
    
    ESP_LOGI(TAG, "📤 Pacote recuperado da NVS [%u restantes]", queue_count);
    return ESP_OK;
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
        
        // Initialize SNTP for time synchronization
        ESP_LOGI(TAG, "Inicializando SNTP...");
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "pool.ntp.br");
        esp_sntp_setservername(1, "a.st1.ntp.br");
        esp_sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
        sntp_set_time_sync_notification_cb(sntp_sync_time_cb);
        
        // Set timezone to Brazil (UTC-3)
        setenv("TZ", "BRT3", 1);
        tzset();
        
        esp_sntp_init();
        ESP_LOGI(TAG, "SNTP iniciado (aguardando sincronização...)");
    }
}

static esp_err_t http_post_packet(const SensorPacketV1 *pkt, bool is_backlog) {
    char json[350];
    int n = snprintf(json, sizeof(json),
        "{\"version\":%u,\"node_id\":%u,\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"seq\":%u,"
        "\"distance_cm\":%d,\"level_cm\":%d,\"percentual\":%u,\"volume_l\":%u,\"vin_mv\":%d,\"rssi\":%d,\"ts_ms\":%u,"
        "\"flags\":%u,\"alert_type\":%u,\"is_backlog\":%s}",
        (unsigned)pkt->version, (unsigned)pkt->node_id,
        pkt->mac[0], pkt->mac[1], pkt->mac[2], pkt->mac[3], pkt->mac[4], pkt->mac[5],
        (unsigned)pkt->seq,
        (int)pkt->distance_cm, (int)pkt->level_cm, (unsigned)pkt->percentual, (unsigned)pkt->volume_l,
        (int)pkt->vin_mv, (int)pkt->rssi, (unsigned)pkt->ts_ms,
        (unsigned)pkt->flags, (unsigned)pkt->alert_type,
        is_backlog ? "true" : "false");

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
        if (is_backlog) {
            ESP_LOGI(TAG, "📤 HTTP backlog status: %d", status);
        } else {
            ESP_LOGI(TAG, "HTTP status: %d", status);
        }
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
    // Use UNIX timestamp if SNTP is synced, otherwise use milliseconds since boot
    uint32_t timestamp = get_unix_timestamp();
    if (timestamp == 0) {
        // Fallback to milliseconds since boot if SNTP not synced
        timestamp = (uint32_t)(esp_timer_get_time() / 1000ULL);
    }
    packet.data.ts_ms = timestamp;

    gateway_metrics.packets_received++;
    
    // Auto-register node as peer if not already registered (for ACK response)
    if (!esp_now_is_peer_exist(recv_info->src_addr)) {
        esp_now_peer_info_t peer = {0};
        memcpy(peer.peer_addr, recv_info->src_addr, 6);
        peer.channel = 0;  // Use current channel
        peer.ifidx = WIFI_IF_STA;
        peer.encrypt = false;
        
        esp_err_t peer_err = esp_now_add_peer(&peer);
        if (peer_err == ESP_OK) {
            ESP_LOGI(TAG, "✓ Nó auto-registrado: %02X:%02X:%02X:%02X:%02X:%02X",
                     recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                     recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
        }
    }
    
    // Send ACK immediately (best effort, non-blocking)
    AckPacket ack_pkt = {
        .magic = ACK_MAGIC,
        .version = ACK_VERSION,
        .node_id = packet.data.node_id,
        .ack_seq = packet.data.seq,
        .rssi = packet.data.rssi,
        .status = ACK_STATUS_OK,
        .gateway_id = 0,  // TODO: Configure gateway ID
        .reserved = 0
    };
    
    // Send ACK without blocking (fire and forget)
    esp_err_t ack_err = esp_now_send(recv_info->src_addr, (const uint8_t*)&ack_pkt, sizeof(ack_pkt));
    if (ack_err == ESP_OK) {
        ESP_LOGD(TAG, "✓ ACK enviado para seq=%u", ack_pkt.ack_seq);
    } else {
        ESP_LOGW(TAG, "✗ Falha ao enviar ACK: %s", esp_err_to_name(ack_err));
    }
    
    // Enqueue for processing
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
                
                // Check for anomaly alerts
                bool is_alert = (packet.data.flags & FLAG_IS_ALERT) != 0;
                const char* alert_names[] = {"NONE", "RAPID_DROP", "RAPID_RISE", "SENSOR_STUCK"};
                
                if (is_alert) {
                    ESP_LOGW(TAG, "╔════════════════════════════════════════════════════╗");
                    ESP_LOGW(TAG, "║          🚨 ALERTA DE ANOMALIA DETECTADO 🚨       ║");
                    ESP_LOGW(TAG, "╠════════════════════════════════════════════════════╣");
                    ESP_LOGW(TAG, "║ Tipo: %s", alert_names[packet.data.alert_type]);
                    ESP_LOGW(TAG, "║ Nó ID: %u | Sequência: %u", packet.data.node_id, packet.data.seq);
                    ESP_LOGW(TAG, "╚════════════════════════════════════════════════════╝");
                }
                
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
                if (is_alert) {
                    ESP_LOGI(TAG, "║ 🚨 Alerta: %s", alert_names[packet.data.alert_type]);
                }
                
                // Output JSON to Serial for external processing
                printf("TELEMETRY:{\"mac\":\"%s\",\"distance\":%d,\"level\":%d,\"volume\":%" PRIu32 ",\"voltage\":%d,\"seq\":%" PRIu32 ",\"alert\":%u}\n",
                       src_mac_str, packet.data.distance_cm, packet.data.level_cm, packet.data.volume_l, 
                       packet.data.vin_mv, packet.data.seq, packet.data.alert_type);
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
    
    // First, try to send any backlog from previous boot
    while (queue_count > 0) {
        if (!wifi_got_ip) {
            ESP_LOGW(TAG, "⏳ Aguardando IP para enviar backlog...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        
        if (nvs_queue_pop(&pkt) == ESP_OK) {
            esp_err_t err = http_post_packet(&pkt, true);
            if (err != ESP_OK) {
                // Backend still offline, push back and wait
                nvs_queue_push(&pkt);
                ESP_LOGW(TAG, "⚠️ Backend offline - aguardando reconexão");
                vTaskDelay(pdMS_TO_TICKS(10000));
            } else {
                ESP_LOGI(TAG, "✓ Pacote do backlog enviado com sucesso");
            }
        }
    }
    
    ESP_LOGI(TAG, "✓ Backlog NVS vazio - processando telemetria em tempo real");
    
    // Now process real-time telemetry
    while (1) {
        if (xQueueReceive(http_queue, &pkt, portMAX_DELAY)) {
            if (!wifi_got_ip) {
                ESP_LOGW(TAG, "⚠️ Sem IP - salvando na NVS");
                nvs_queue_push(&pkt);
                continue;
            }
            
            esp_err_t err = http_post_packet(&pkt, false);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "⚠️ HTTP falhou - salvando na NVS");
                nvs_queue_push(&pkt);
            }
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

    // Initialize NVS first
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS corrompida ou versão nova - apagando...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_err);
    ESP_LOGI(TAG, "✓ NVS Flash inicializada");
    
    // Initialize persistent queue
    if (nvs_queue_init() != ESP_OK) {
        ESP_LOGE(TAG, "❌ Falha ao inicializar fila persistente");
        return;
    }

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

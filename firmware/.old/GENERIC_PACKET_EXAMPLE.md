# 📦 Generic Packet - Pacote de Dados Variável

## 🎯 Visão Geral

O **Generic Packet** (v2) permite enviar dados arbitrários como pares chave-valor, sem precisar modificar a estrutura do pacote. Ideal para:

- ✅ Sensores com tipos de dados diferentes
- ✅ Prototipagem rápida sem recompilar gateway
- ✅ Configuração dinâmica de campos
- ✅ Integração de sensores heterogêneos

---

## 📊 Estrutura do Pacote

### Header Fixo (18 bytes)
```cpp
typedef struct {
    uint8_t  magic;          // 0xDA (identificação)
    uint8_t  version;        // 2
    uint8_t  node_id;        // ID do nó
    uint8_t  mac[6];         // MAC address
    uint32_t seq;            // Sequência
    uint8_t  pair_count;     // Quantidade de pares (0-10)
    int8_t   rssi;           // RSSI (gateway preenche)
    uint32_t ts_ms;          // Timestamp (gateway preenche)
} GenericPacketHeader;
```

### Dados Variáveis (após header)
```
[pair1][pair2][pair3]...

Cada par:
[label_len:1 byte][label:N bytes][type:1 byte][value_len:1 byte][value:M bytes]
```

---

## 🔧 Tipos de Dados Suportados

| Tipo | Código | Tamanho | Descrição |
|------|--------|---------|-----------|
| `DATA_TYPE_INT8` | 0x01 | 1 byte | Inteiro com sinal (-128 a 127) |
| `DATA_TYPE_UINT8` | 0x02 | 1 byte | Inteiro sem sinal (0 a 255) |
| `DATA_TYPE_INT16` | 0x03 | 2 bytes | Inteiro com sinal (-32768 a 32767) |
| `DATA_TYPE_UINT16` | 0x04 | 2 bytes | Inteiro sem sinal (0 a 65535) |
| `DATA_TYPE_INT32` | 0x05 | 4 bytes | Inteiro com sinal |
| `DATA_TYPE_UINT32` | 0x06 | 4 bytes | Inteiro sem sinal |
| `DATA_TYPE_FLOAT` | 0x07 | 4 bytes | Ponto flutuante IEEE 754 |
| `DATA_TYPE_BOOL` | 0x08 | 1 byte | Booleano (0 ou 1) |
| `DATA_TYPE_STRING` | 0x09 | Variável | String (max 31 caracteres) |

---

## 💡 Exemplo de Uso - Node Ultra 1

### Código Completo (node_ultra1.cpp)

```cpp
#include "telemetry_packet.h"

void send_generic_telemetry() {
    // Buffer para construir o pacote
    uint8_t packet_buffer[MAX_GENERIC_PACKET_SIZE];
    uint16_t offset = 0;
    
    // 1. Preencher header
    GenericPacketHeader* header = (GenericPacketHeader*)packet_buffer;
    header->magic = GENERIC_PACKET_MAGIC;
    header->version = GENERIC_PACKET_VERSION;
    header->node_id = 1;
    
    uint8_t dev_mac[6];
    esp_read_mac(dev_mac, ESP_MAC_WIFI_STA);
    memcpy(header->mac, dev_mac, 6);
    
    header->seq = get_sequence_number();
    header->pair_count = 0;  // Incrementaremos para cada par adicionado
    header->rssi = 0;   // Gateway preenche
    header->ts_ms = 0;  // Gateway preenche
    
    offset = sizeof(GenericPacketHeader);
    
    // 2. Adicionar pares de dados
    
    // Temperatura (float)
    float temperature = read_temperature_sensor();
    offset += add_float_pair(&packet_buffer[offset], "temp_c", temperature);
    header->pair_count++;
    
    // Nível da água (int16)
    int16_t level_cm = measure_water_level();
    offset += add_int16_pair(&packet_buffer[offset], "level_cm", level_cm);
    header->pair_count++;
    
    // Volume (uint32)
    uint32_t volume_l = calculate_volume(level_cm);
    offset += add_uint32_pair(&packet_buffer[offset], "volume_l", volume_l);
    header->pair_count++;
    
    // Tensão da bateria (uint16)
    uint16_t battery_mv = read_battery_voltage();
    offset += add_uint16_pair(&packet_buffer[offset], "battery_mv", battery_mv);
    header->pair_count++;
    
    // Status de alerta (bool)
    bool is_alert = check_anomaly_detection();
    offset += add_bool_pair(&packet_buffer[offset], "alert", is_alert);
    header->pair_count++;
    
    // Qualidade do sinal (uint8)
    uint8_t signal_quality = calculate_ack_success_rate();
    offset += add_uint8_pair(&packet_buffer[offset], "signal_pct", signal_quality);
    header->pair_count++;
    
    // Estado operacional (string)
    offset += add_string_pair(&packet_buffer[offset], "status", "operational");
    header->pair_count++;
    
    // 3. Enviar via ESP-NOW
    esp_err_t err = esp_now_send(gateway_mac, packet_buffer, offset);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ Generic packet sent: %u bytes, %u pairs", 
                 offset, header->pair_count);
    } else {
        ESP_LOGE(TAG, "❌ Failed to send generic packet: %s", 
                 esp_err_to_name(err));
    }
}
```

---

## 🌐 Exemplo de Parsing no Gateway

### Código para Decodificar (gateway_devkit_v1/main.c)

```c
#include "telemetry_packet.h"

void parse_generic_packet(const uint8_t* data, int len) {
    if (len < sizeof(GenericPacketHeader)) {
        ESP_LOGW(TAG, "Packet too small for header");
        return;
    }
    
    GenericPacketHeader* header = (GenericPacketHeader*)data;
    
    // Validar magic byte
    if (header->magic != GENERIC_PACKET_MAGIC) {
        ESP_LOGW(TAG, "Invalid magic byte: 0x%02X", header->magic);
        return;
    }
    
    ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║ Generic Packet (v%u) from Node %u         ║", 
             header->version, header->node_id);
    ESP_LOGI(TAG, "╠════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║ Seq: %u | Pairs: %u                       ║", 
             header->seq, header->pair_count);
    ESP_LOGI(TAG, "╠════════════════════════════════════════════╣");
    
    // Parse data pairs
    uint16_t offset = sizeof(GenericPacketHeader);
    
    for (int i = 0; i < header->pair_count && offset < len; i++) {
        uint8_t label_len = data[offset++];
        
        if (offset + label_len + 2 > len) break;
        
        char label[32] = {0};
        memcpy(label, &data[offset], label_len);
        offset += label_len;
        
        uint8_t type = data[offset++];
        uint8_t value_len = data[offset++];
        
        if (offset + value_len > len) break;
        
        // Decode value based on type
        switch (type) {
            case DATA_TYPE_INT32: {
                int32_t value;
                memcpy(&value, &data[offset], 4);
                ESP_LOGI(TAG, "║ %s: %d (int32)", label, value);
                break;
            }
            case DATA_TYPE_UINT32: {
                uint32_t value;
                memcpy(&value, &data[offset], 4);
                ESP_LOGI(TAG, "║ %s: %u (uint32)", label, value);
                break;
            }
            case DATA_TYPE_FLOAT: {
                float value;
                memcpy(&value, &data[offset], 4);
                ESP_LOGI(TAG, "║ %s: %.2f (float)", label, value);
                break;
            }
            case DATA_TYPE_INT16: {
                int16_t value;
                memcpy(&value, &data[offset], 2);
                ESP_LOGI(TAG, "║ %s: %d (int16)", label, value);
                break;
            }
            case DATA_TYPE_UINT16: {
                uint16_t value;
                memcpy(&value, &data[offset], 2);
                ESP_LOGI(TAG, "║ %s: %u (uint16)", label, value);
                break;
            }
            case DATA_TYPE_UINT8: {
                uint8_t value = data[offset];
                ESP_LOGI(TAG, "║ %s: %u (uint8)", label, value);
                break;
            }
            case DATA_TYPE_INT8: {
                int8_t value = (int8_t)data[offset];
                ESP_LOGI(TAG, "║ %s: %d (int8)", label, value);
                break;
            }
            case DATA_TYPE_BOOL: {
                bool value = data[offset] != 0;
                ESP_LOGI(TAG, "║ %s: %s (bool)", label, value ? "true" : "false");
                break;
            }
            case DATA_TYPE_STRING: {
                char str_value[32] = {0};
                memcpy(str_value, &data[offset], value_len);
                ESP_LOGI(TAG, "║ %s: \"%s\" (string)", label, str_value);
                break;
            }
            default:
                ESP_LOGW(TAG, "║ %s: Unknown type 0x%02X", label, type);
        }
        
        offset += value_len;
    }
    
    ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
}
```

---

## 📤 Serialização para JSON (Backend)

### Exemplo de Conversão para HTTP POST

```c
void send_generic_to_backend(const uint8_t* packet, int len) {
    GenericPacketHeader* header = (GenericPacketHeader*)packet;
    
    // Construir JSON dinamicamente
    char json[512] = {0};
    int json_len = sprintf(json, 
        "{\"version\":%u,\"node_id\":%u,\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
        "\"seq\":%u,\"rssi\":%d,\"ts_ms\":%u,\"data\":{",
        header->version, header->node_id,
        header->mac[0], header->mac[1], header->mac[2],
        header->mac[3], header->mac[4], header->mac[5],
        header->seq, header->rssi, header->ts_ms
    );
    
    // Parse pairs e adicionar ao JSON
    uint16_t offset = sizeof(GenericPacketHeader);
    bool first = true;
    
    for (int i = 0; i < header->pair_count && offset < len; i++) {
        uint8_t label_len = packet[offset++];
        
        char label[32] = {0};
        memcpy(label, &packet[offset], label_len);
        offset += label_len;
        
        uint8_t type = packet[offset++];
        uint8_t value_len = packet[offset++];
        
        if (!first) json_len += sprintf(&json[json_len], ",");
        first = false;
        
        json_len += sprintf(&json[json_len], "\"%s\":", label);
        
        switch (type) {
            case DATA_TYPE_INT32: {
                int32_t value;
                memcpy(&value, &packet[offset], 4);
                json_len += sprintf(&json[json_len], "%d", value);
                break;
            }
            case DATA_TYPE_FLOAT: {
                float value;
                memcpy(&value, &packet[offset], 4);
                json_len += sprintf(&json[json_len], "%.2f", value);
                break;
            }
            case DATA_TYPE_STRING: {
                char str_value[32] = {0};
                memcpy(str_value, &packet[offset], value_len);
                json_len += sprintf(&json[json_len], "\"%s\"", str_value);
                break;
            }
            case DATA_TYPE_BOOL: {
                json_len += sprintf(&json[json_len], "%s", 
                                   packet[offset] ? "true" : "false");
                break;
            }
            // ... outros tipos
        }
        
        offset += value_len;
    }
    
    sprintf(&json[json_len], "}}");
    
    // Enviar via HTTP POST
    http_post_json(json);
}
```

**Resultado JSON:**
```json
{
  "version": 2,
  "node_id": 1,
  "mac": "24:62:AB:D5:E7:A0",
  "seq": 42,
  "rssi": -45,
  "ts_ms": 1702752345,
  "data": {
    "temp_c": 23.5,
    "level_cm": 327,
    "volume_l": 58133,
    "battery_mv": 3300,
    "alert": false,
    "signal_pct": 95,
    "status": "operational"
  }
}
```

---

## ⚡ Casos de Uso

### 1. **Estação Meteorológica**
```cpp
add_float_pair(buffer, "temp_c", 23.5);
add_float_pair(buffer, "humidity_pct", 65.2);
add_uint16_pair(buffer, "pressure_hpa", 1013);
add_float_pair(buffer, "wind_speed_ms", 3.2);
add_string_pair(buffer, "direction", "NW");
```

### 2. **Monitor de Energia**
```cpp
add_float_pair(buffer, "voltage_v", 220.5);
add_float_pair(buffer, "current_a", 2.3);
add_uint32_pair(buffer, "power_w", 507);
add_uint32_pair(buffer, "energy_wh", 12345);
add_float_pair(buffer, "power_factor", 0.95);
```

### 3. **Sensor Agrícola**
```cpp
add_float_pair(buffer, "soil_temp_c", 18.5);
add_uint8_pair(buffer, "soil_moisture_pct", 45);
add_uint16_pair(buffer, "light_lux", 850);
add_float_pair(buffer, "ph", 6.5);
add_uint16_pair(buffer, "ec_us_cm", 1200);
```

### 4. **Monitor Industrial**
```cpp
add_bool_pair(buffer, "motor_on", true);
add_uint16_pair(buffer, "rpm", 1450);
add_float_pair(buffer, "vibration_mm_s", 2.3);
add_float_pair(buffer, "bearing_temp_c", 65.5);
add_uint8_pair(buffer, "oil_level_pct", 85);
add_string_pair(buffer, "state", "running");
```

---

## 🔒 Vantagens vs SensorPacketV1

| Característica | SensorPacketV1 | GenericPacket |
|----------------|----------------|---------------|
| Estrutura | Fixa | Flexível |
| Tamanho | 28 bytes fixos | Variável (20-250 bytes) |
| Tipos de dados | Predefinidos | 9 tipos suportados |
| Adição de campos | Requer recompilação | Dinâmico |
| Overhead | Baixo | Moderado (~3-5 bytes/campo) |
| Parsing | Direto | Iterativo |
| Uso ideal | Sensores padronizados | Prototipagem, sensores diversos |

---

## ⚠️ Limitações e Boas Práticas

### Limitações
- ✅ Máximo 250 bytes por pacote (limite ESP-NOW)
- ✅ Máximo 10 pares recomendados
- ✅ Labels limitados a 31 caracteres
- ✅ Strings limitadas a 31 caracteres
- ✅ Overhead de ~3-8 bytes por par (label + metadados)

### Boas Práticas
- 📝 Use labels curtos: `"temp"` ao invés de `"temperature_celsius"`
- 📝 Agrupe dados relacionados em um único pacote
- 📝 Para dados fixos e frequentes, use `SensorPacketV1`
- 📝 Para prototipagem e dados variados, use `GenericPacket`
- 📝 Valide o tamanho total antes de enviar
- 📝 Prefira tipos menores quando possível (uint8 vs uint32)

---

## 🚀 Próximas Extensões Possíveis

1. **Compressão**: Adicionar flag de compressão Zlib/LZ4
2. **Arrays**: Suportar arrays de valores (ex: histórico de 10 leituras)
3. **Nested objects**: Suportar estruturas aninhadas
4. **Checksums**: Adicionar CRC16 para validação de integridade
5. **Schemas**: Backend pode enviar schema esperado via configuração remota

---

## 📚 Referências

- ESP-NOW: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html
- IEEE 754 Float: https://en.wikipedia.org/wiki/IEEE_754
- TLV Encoding: https://en.wikipedia.org/wiki/Type-length-value

**Pronto para usar! Compile e teste com seus sensores personalizados! 🎉**

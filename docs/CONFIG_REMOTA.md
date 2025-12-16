# Configuração Remota de Sensores - Documentação

## Visão Geral

Sistema de configuração remota que permite alterar parâmetros dos sensores **sem necessidade de reflash**, através de interface web ou API HTTP.

## Características

### ✅ Configurações Persistentes
- Armazenadas em **EEPROM** (Arduino Nano) ou **NVS** (ESP32)
- Sobrevivem a reinicializações e quedas de energia
- Validação por checksum

### ✅ Configurações Disponíveis
- **Rede:** IP do dispositivo, gateway, subnet
- **Backend:** IP e porta do servidor
- **Sensor:** Node ID, offset do sensor, altura/volume do reservatório
- **Timing:** Intervalo entre POSTs, intervalo de debug serial
- **Sampling:** Número de amostras para média, delay entre amostras

### ✅ Interface Web
- Acesso via browser: `http://192.168.0.117:8080/config_sensores.html`
- Cards individuais por dispositivo
- Status online/offline em tempo real
- Telemetria atual exibida
- Validação de campos

---

## API HTTP - Endpoints

### 1. GET / (Telemetria)
Retorna leitura atual do sensor

**Request:**
```bash
curl http://192.168.0.222/
```

**Response:**
```json
{
  "version": 1,
  "node_id": 3,
  "mac": "de:ad:be:ef:fe:ed",
  "seq": 123,
  "distance_cm": 110,
  "level_cm": 360,
  "percentual": 80,
  "volume_l": 64000,
  "vin_mv": 0,
  "rssi": 0,
  "ts_ms": 456789
}
```

---

### 2. GET /config (Consultar Configuração)
Retorna configuração atual do dispositivo

**Request:**
```bash
curl http://192.168.0.222/config
```

**Response:**
```json
{
  "node_id": 3,
  "ip": "192.168.0.222",
  "backend_ip": "192.168.0.117",
  "backend_port": 8080,
  "sensor_offset_cm": 20,
  "res_height_cm": 450,
  "res_volume_l": 80000,
  "post_interval_s": 30,
  "sample_count": 3,
  "sample_delay_ms": 100
}
```

---

### 3. POST /config (Atualizar Configuração)
Atualiza configuração do dispositivo

**Request:**
```bash
curl -X POST http://192.168.0.222/config \
  -H "Content-Type: application/json" \
  -d '{
    "node_id": 5,
    "backend_ip": "192.168.0.200",
    "backend_port": 9090,
    "sensor_offset_cm": 25,
    "res_height_cm": 500,
    "res_volume_l": 90000,
    "post_interval_s": 60,
    "sample_count": 5,
    "sample_delay_ms": 150
  }'
```

**Response:**
```json
{
  "success": true,
  "message": "Config saved. Restart to apply network changes."
}
```

**⚠️ Importante:** Mudanças de rede (IP, gateway) requerem reinicialização do dispositivo.

---

### 4. GET /reset (Restaurar Defaults)
Restaura configuração de fábrica

**Request:**
```bash
curl http://192.168.0.222/reset
```

**Response:**
```json
{
  "success": true,
  "message": "Config reset to defaults. Restart device."
}
```

---

## Uso da Interface Web

### 1. Acessar Interface
```bash
# Abrir no browser
http://192.168.0.117:8080/config_sensores.html
```

### 2. Visualizar Dispositivos
- Cards mostram status (online/offline)
- Telemetria atual exibida para dispositivos online
- Formulário de configuração por dispositivo

### 3. Alterar Configurações
1. Editar campos desejados
2. Clicar em **"💾 Salvar"**
3. Aguardar mensagem de confirmação
4. Reiniciar dispositivo se alterou IP/rede

### 4. Restaurar Defaults
1. Clicar em **"⚠️ Reset"**
2. Confirmar ação
3. Dispositivo retorna aos valores de fábrica
4. Reiniciar dispositivo

---

## Casos de Uso

### Cenário 1: Alterar Intervalo de POST
**Situação:** Reduzir tráfego de rede de 30s para 60s

```bash
curl -X POST http://192.168.0.222/config \
  -H "Content-Type: application/json" \
  -d '{"post_interval_s": 60}'
```

**Resultado:** Sensor passa a enviar dados a cada 1 minuto (sem reiniciar)

---

### Cenário 2: Corrigir Offset do Sensor
**Situação:** Sensor instalado 25cm acima da referência (era 20cm)

```bash
curl -X POST http://192.168.0.222/config \
  -H "Content-Type: application/json" \
  -d '{"sensor_offset_cm": 25}'
```

**Resultado:** Cálculos de nível ajustados imediatamente

---

### Cenário 3: Trocar Backend
**Situação:** Migrar para novo servidor backend

```bash
curl -X POST http://192.168.0.222/config \
  -H "Content-Type: application/json" \
  -d '{
    "backend_ip": "192.168.1.100",
    "backend_port": 8080
  }'
```

**Resultado:** Próximos POSTs vão para novo servidor (sem reflash)

---

### Cenário 4: Aumentar Precisão
**Situação:** Melhorar precisão com mais amostras

```bash
curl -X POST http://192.168.0.222/config \
  -H "Content-Type: application/json" \
  -d '{
    "sample_count": 5,
    "sample_delay_ms": 150
  }'
```

**Resultado:** Sensor faz média de 5 leituras com 150ms entre elas

---

## Adicionar Novos Dispositivos

Editar `backend/config_sensores.html`:

```javascript
const devices = [
    { ip: '192.168.0.222', name: 'Nano Ethernet', type: 'nano' },
    { ip: '192.168.0.223', name: 'Nano Tanque 2', type: 'nano' },
    // Adicionar ESP32 quando tiverem endpoint /config
    // { ip: '192.168.0.100', name: 'ESP32 Node 1', type: 'esp32' },
];
```

---

## Implementar em ESP32

Para adicionar configuração remota aos nodes ESP32:

1. **Copiar conceito do Nano:**
   - `config_manager.h` adaptado para NVS (no lugar de EEPROM)
   - Endpoints HTTP `/config`, `/reset`

2. **Usar NVS (Non-Volatile Storage):**
```cpp
#include "nvs_flash.h"
#include "nvs.h"

nvs_handle_t nvs_handle;
nvs_open("aguada_cfg", NVS_READWRITE, &nvs_handle);
nvs_set_blob(nvs_handle, "config", &cfg, sizeof(cfg));
nvs_commit(nvs_handle);
```

3. **Adicionar servidor HTTP no node:**
```cpp
// Criar endpoint config no ESP32
httpd_handle_t server = NULL;
httpd_start(&server, &config);
httpd_register_uri_handler(server, &config_uri);
```

---

## Arquitetura de Configuração

```
┌─────────────────┐
│  Interface Web  │ (config_sensores.html)
│ (Browser User)  │
└────────┬────────┘
         │ HTTP GET/POST
         ▼
┌─────────────────┐
│ Sensores (Nano/ │ Endpoints: /, /config, /reset
│  ESP32 Nodes)   │ Storage: EEPROM/NVS
└────────┬────────┘
         │ Leitura/Escrita
         ▼
┌─────────────────┐
│ EEPROM/NVS      │ Configuração Persistente
│ (Flash Memory)  │ Checksum validado
└─────────────────┘
```

---

## Segurança (Produção)

### ⚠️ Implementar Autenticação

**Opção 1: Basic Auth**
```cpp
// Validar header Authorization
String auth = server.header("Authorization");
if (auth != "Basic YWRtaW46c2VuaGE=") { // admin:senha em base64
  client.println("HTTP/1.1 401 Unauthorized");
  return;
}
```

**Opção 2: Token API**
```cpp
// Validar token na query string
String token = server.arg("token");
if (token != "SECRET_TOKEN_12345") {
  client.println("HTTP/1.1 403 Forbidden");
  return;
}
```

**Uso:**
```bash
curl -X POST http://192.168.0.222/config?token=SECRET_TOKEN_12345 \
  -d '{"node_id": 5}'
```

---

## Próximos Passos

1. ✅ Flash `nano_ethernet_ultra_v2.ino` no Nano
2. ⏳ Testar endpoints HTTP (GET /, GET /config, POST /config)
3. ⏳ Abrir interface web e validar funcionalidade
4. ⏳ Implementar configuração remota nos ESP32 nodes
5. ⏳ Adicionar autenticação para produção

---

## Comandos Rápidos

```bash
# Consultar config
curl http://192.168.0.222/config

# Alterar node_id
curl -X POST http://192.168.0.222/config -H "Content-Type: application/json" -d '{"node_id": 5}'

# Alterar intervalo POST
curl -X POST http://192.168.0.222/config -H "Content-Type: application/json" -d '{"post_interval_s": 60}'

# Resetar para defaults
curl http://192.168.0.222/reset

# Abrir interface web
xdg-open http://192.168.0.117:8080/config_sensores.html
```

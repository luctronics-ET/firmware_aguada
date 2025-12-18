# 🔥 Guia de Gravação e Teste - Sistema Aguada

## 📝 Resumo Executivo

Este guia detalha como gravar, testar e expandir o sistema com:
- ✅ Gateway 1 (ESP32 DevKit V1) - **Principal**
- ✅ Gateway 2 (ESP32 DevKit V1) - **Redundância** 
- ✅ Gateway 3 (Arduino Nano Ethernet) - **Integração futura**
- ✅ Node Ultra 1 (ESP32-C3 Supermini)
- ✅ Node Ultra 2 (ESP32-C3 Supermini)

---

## 🎯 Passo 1: Gravar Gateway Principal

### 1.1 Conectar Hardware
```bash
# Identificar porta do Gateway 1 (ESP32 DevKit V1)
ls -la /dev/ttyUSB* /dev/ttyACM*
# Exemplo de saída:
# /dev/ttyUSB0  <- Gateway 1
```

### 1.2 Gravar Firmware
```bash
cd ~/firmware_aguada/firmware/gateway_devkit_v1

# Flash completo (inclui bootloader + partições)
idf.py -p /dev/ttyUSB0 flash

# Monitorar logs (Ctrl+] para sair)
idf.py -p /dev/ttyUSB0 monitor

# OU flash + monitor em um comando
idf.py -p /dev/ttyUSB0 flash monitor
```

### 1.3 Verificar Inicialização
Logs esperados:
```
I (123) AGUADA_GATEWAY: ╔═══════════════════════════════════════════════╗
I (124) AGUADA_GATEWAY: ║     AGUADA Gateway (ESP32 DevKit V1)         ║
I (125) AGUADA_GATEWAY: ╚═══════════════════════════════════════════════╝
I (456) AGUADA_GATEWAY: ✓ NVS Flash inicializada
I (457) AGUADA_GATEWAY: 📦 Fila NVS inicializada: 0 pacotes pendentes
I (458) AGUADA_GATEWAY: ✓ Fila ESP-NOW criada (20 slots)
I (789) AGUADA_GATEWAY: WiFi IP: 192.168.0.117
I (790) AGUADA_GATEWAY: Canal atual WiFi/ESP-NOW: 11
I (1234) AGUADA_GATEWAY: Inicializando SNTP...
I (5678) AGUADA_GATEWAY: ⏰ SNTP sincronizado: 2025-12-16 15:30:45
I (5679) AGUADA_GATEWAY: Gateway MAC: 80:F3:DA:62:A7:84  <-- ANOTAR ESTE MAC!
```

**⚠️ IMPORTANTE**: Anote o MAC address do gateway! Você precisará dele nos nós.

---

## 🎯 Passo 2: Gravar Gateway 2 (Redundância)

### 2.1 Conectar Gateway 2
```bash
# Desconectar Gateway 1, conectar Gateway 2
ls -la /dev/ttyUSB*
# /dev/ttyUSB0  <- Gateway 2
```

### 2.2 Gravar Mesmo Firmware
```bash
cd ~/firmware_aguada/firmware/gateway_devkit_v1
idf.py -p /dev/ttyUSB0 flash monitor
```

### 2.3 Anotar MAC Address do Gateway 2
```
I (5679) AGUADA_GATEWAY: Gateway MAC: 24:0A:C4:9A:58:28  <-- ANOTAR!
```

---

## 🎯 Passo 3: Configurar MACs no Node Ultra 1

### 3.1 Editar Código do Nó
```bash
cd ~/firmware_aguada/firmware/node_ultra1
nano main/node_ultra1.cpp  # ou use VS Code
```

### 3.2 Atualizar Array de MACs
Localize (linha ~80):
```cpp
#define MAX_GATEWAYS 3
static const uint8_t GATEWAY_MACS[MAX_GATEWAYS][6] = {
    {0x80, 0xf3, 0xda, 0x62, 0xa7, 0x84},  // Gateway 1 (SUBSTITUIR COM SEU MAC)
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  // Gateway 2 (SUBSTITUIR COM SEU MAC)
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}   // Gateway 3 (não configurado)
};
```

**Substituir com seus MACs reais** (anotados no Passo 1 e 2):
```cpp
static const uint8_t GATEWAY_MACS[MAX_GATEWAYS][6] = {
    {0x80, 0xf3, 0xda, 0x62, 0xa7, 0x84},  // Gateway 1 (seu MAC aqui)
    {0x24, 0x0a, 0xc4, 0x9a, 0x58, 0x28},  // Gateway 2 (seu MAC aqui)
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}   // Gateway 3 (desabilitado)
};
```

### 3.3 Recompilar
```bash
cd ~/firmware_aguada/firmware/node_ultra1
idf.py build
```

---

## 🎯 Passo 4: Gravar Node Ultra 1

### 4.1 Conectar ESP32-C3 Supermini
```bash
ls -la /dev/ttyACM*
# /dev/ttyACM0  <- Node Ultra 1
```

### 4.2 Gravar Firmware
```bash
cd ~/firmware_aguada/firmware/node_ultra1

# Flash + monitor
idf.py -p /dev/ttyACM0 flash monitor
```

### 4.3 Verificar Logs do Nó
```
I (123) node_ultra01: Initializing Wi-Fi and ESP-NOW...
I (456) node_ultra01: ESP-NOW initialized on channel 11
I (457) node_ultra01: Device MAC: 24:62:AB:D5:E7:A0
I (789) node_ultra01: Total gateways configured: 2
I (790) node_ultra01: Last successful gateway: 0 (80:F3:DA:62:A7:84)
I (1234) node_ultra01: Reading 0: raw=123cm filtered=123cm
I (1300) node_ultra01: Reading 1: raw=125cm filtered=124cm
I (1366) node_ultra01: Reading 2: raw=122cm filtered=123cm
I (1432) node_ultra01: meas: distance=123 cm, level=327 cm, pct=73%, vol=58133 L
I (1500) node_ultra01: Trying gateway 0: 80:F3:DA:62:A7:84
I (1550) node_ultra01: ✓ ACK recebido: seq=1, rssi=-45, gateway=0, status=0
I (1560) node_ultra01: ✓ Sent successfully to gateway 0 (retry 0) with ACK confirmation
I (1570) node_ultra01: 📊 Stats: 1/1 successful (100.0% success rate)
```

---

## 🎯 Passo 5: Testar Redundância de Gateways

### 5.1 Cenário: Gateway 1 Offline
1. **Desligar Gateway 1** (desconectar USB)
2. **Observar logs do Nó**:
```
I (5000) node_ultra01: Trying gateway 0: 80:F3:DA:62:A7:84
W (5500) node_ultra01: ⚠️ Timeout aguardando ACK do gateway 0 (tentativa 1)
I (5600) node_ultra01: Trying gateway 1: 24:0A:C4:9A:58:28
I (5650) node_ultra01: ✓ ACK recebido: seq=2, rssi=-52, gateway=1, status=0
I (5660) node_ultra01: ✓ Sent successfully to gateway 1 (retry 0) with ACK confirmation
I (5670) node_ultra01: ✓ Salvando gateway preferido: 1
```

3. **Religar Gateway 1**
4. **Próxima transmissão**: Nó tentará Gateway 1 primeiro (salvo em NVS)

### 5.2 Verificar Logs do Gateway 2
```
I (8901) AGUADA_GATEWAY: ╔════════════════════════════════════════════════╗
I (8902) AGUADA_GATEWAY: ║ RX Node 1 | Seq: 2 | Src: 24:62:AB:D5:E7:A0  ║
I (8903) AGUADA_GATEWAY: ╠════════════════════════════════════════════════╣
I (8910) AGUADA_GATEWAY: ║ Nível: 327 cm
I (8920) AGUADA_GATEWAY: ║ Volume: 58133 L
I (8930) AGUADA_GATEWAY: ║ RSSI: -52 dBm
D (8940) AGUADA_GATEWAY: ✓ ACK enviado para seq=2
I (9000) AGUADA_GATEWAY: HTTP status: 200
```

---

## 🎯 Passo 6: Gravar Node Ultra 2 (Clone)

### 6.1 Preparar Node Ultra 2
```bash
cd ~/firmware_aguada/firmware/node_ultra2

# Verificar se node_id é diferente (deve ser 2)
grep "node_id = " main/node_ultra2.cpp
# Deve mostrar: pkt.node_id = 2;
```

### 6.2 Atualizar MACs (se necessário)
```bash
nano main/node_ultra2.cpp
# Mesmos MACs do Ultra 1
```

### 6.3 Compilar e Gravar
```bash
cd ~/firmware_aguada/firmware/node_ultra2
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

---

## 🎯 Passo 7: Testar Detecção de Anomalias

### 7.1 Simular Vazamento Rápido
**Método**: Aproximar/afastar sensor da superfície rapidamente (>50cm)

**Logs esperados no Nó**:
```
I (1234) node_ultra01: meas: distance=100 cm, level=370 cm, pct=82%, vol=65777 L
W (31234) node_ultra01: 🚨 ALERTA: Queda rápida detectada! Δ=-52cm (possível vazamento)
I (31244) node_ultra01: ⚠️ Pacote marcado como alerta (tipo=1)
```

**Logs esperados no Gateway**:
```
W (8901) AGUADA_GATEWAY: ╔════════════════════════════════════════════════╗
W (8910) AGUADA_GATEWAY: ║          🚨 ALERTA DE ANOMALIA DETECTADO 🚨   ║
W (8920) AGUADA_GATEWAY: ╠════════════════════════════════════════════════╣
W (8930) AGUADA_GATEWAY: ║ Tipo: RAPID_DROP
W (8940) AGUADA_GATEWAY: ║ Nó ID: 1 | Sequência: 42
W (8950) AGUADA_GATEWAY: ╚════════════════════════════════════════════════╝
```

### 7.2 Verificar JSON no Backend
```bash
# Logs do gateway mostram JSON enviado:
# {"version":1,"node_id":1,"seq":42,"level_cm":275,"flags":1,"alert_type":1,...}
```

---

## 🎯 Passo 8: Testar Fila Persistente NVS

### 8.1 Desligar Backend
```bash
cd ~/firmware_aguada
./stop_services.sh
```

### 8.2 Observar Logs do Gateway
```
W (5678) AGUADA_GATEWAY: HTTP post erro: ESP_ERR_TIMEOUT
W (5680) AGUADA_GATEWAY: ⚠️ HTTP falhou - salvando na NVS
I (5690) AGUADA_GATEWAY: 💾 Pacote salvo na NVS [1/50]
```

### 8.3 Religar Backend
```bash
cd ~/firmware_aguada
./start_services.sh
```

### 8.4 Verificar Envio de Backlog
```
I (8901) AGUADA_GATEWAY: 📤 HTTP backlog status: 200
I (8910) AGUADA_GATEWAY: ✓ Pacote do backlog enviado com sucesso
I (9123) AGUADA_GATEWAY: ✓ Backlog NVS vazio - processando telemetria em tempo real
```

---

## 🎯 Passo 9: Arduino Nano Ethernet (Gateway 3)

### 9.1 Situação Atual
O código Arduino está em:
```bash
cd ~/firmware_aguada/firmware/arduino/nano_ethernet_ultra/
ls -la
# nano_ethernet_ultra_v2.ino  <- Versão atual
```

### 9.2 Limitações
- ❌ **Não suporta ESP-NOW** (Arduino Nano não tem WiFi)
- ✅ **Suporta sensor ultrassônico HC-SR04**
- ✅ **Conexão direta Ethernet → Backend**

### 9.3 Integração Futura (Opções)

**Opção A: Nano como Nó Standalone**
- Arduino Nano lê sensor HC-SR04
- Envia dados direto via Ethernet/HTTP
- **Não interage com ESP-NOW**
- Útil para locais com cabo Ethernet

**Opção B: Nano + ESP32 Bridge**
- Arduino lê sensor
- Envia via Serial para ESP32 gateway próximo
- ESP32 encaminha via ESP-NOW
- Requer hardware adicional

**Opção C: Substituir por ESP32 Ethernet** (Recomendado)
- Trocar Nano por ESP32 com módulo Ethernet (W5500)
- Suporta ESP-NOW + Ethernet simultaneamente
- Exemplo: Olimex ESP32-POE, WT32-ETH01

### 9.4 Atualizar Código do Nano (Se usar Opção A)
```bash
cd ~/firmware_aguada/firmware/arduino/nano_ethernet_ultra/
# Editar nano_ethernet_ultra_v2.ino
# Atualizar URL do backend, MAC address, etc.
```

---

## 📊 Resumo de Gravações

| Dispositivo           | Porta         | Comando                          | Status |
|-----------------------|---------------|----------------------------------|--------|
| Gateway 1 (ESP32)     | /dev/ttyUSB0  | `idf.py -p /dev/ttyUSB0 flash`   | ⏳ TODO |
| Gateway 2 (ESP32)     | /dev/ttyUSB1  | `idf.py -p /dev/ttyUSB1 flash`   | ⏳ TODO |
| Node Ultra 1 (C3)     | /dev/ttyACM0  | `idf.py -p /dev/ttyACM0 flash`   | ⏳ TODO |
| Node Ultra 2 (C3)     | /dev/ttyACM1  | `idf.py -p /dev/ttyACM1 flash`   | ⏳ TODO |
| Gateway 3 (Nano ETH)  | /dev/ttyUSB2  | Via Arduino IDE                  | 🔄 Opcional |

---

## ⚠️ Troubleshooting Comum

### Erro: "Permission denied /dev/ttyUSB0"
```bash
sudo usermod -a -G dialout $USER
sudo reboot  # Necessário para aplicar
```

### Erro: "A fatal error occurred: Failed to connect"
- Pressionar e segurar botão BOOT no ESP32
- Executar `idf.py flash`
- Soltar botão BOOT após "Connecting..."

### Erro: "Channel mismatch"
- Gateway e nós **devem estar no canal 11**
- Verificar router WiFi está no canal 11
- Ou mudar `#define ESPNOW_CHANNEL 11` em ambos

### Nó não recebe ACK
- Verificar MACs configurados no nó
- Ambos devices devem estar no mesmo canal
- Verificar distância (ESP-NOW alcança ~100m)

### Backend não recebe dados
- Verificar `./status_services.sh`
- Verificar URL em `gateway_devkit_v1/main/main.c`:
  ```c
  #define INGEST_URL "http://192.168.0.117:8080/ingest_sensorpacket.php"
  ```
- Ajustar IP se necessário

---

## 🚀 Próximas Melhorias Sugeridas

1. **#4 - Configuração Remota**: Backend pode mudar parâmetros dos nós
2. **#5 - Health Monitoring**: Adicionar uptime, heap, battery%
3. **#12 - Compensação de Temperatura**: DHT22 para corrigir velocidade do som
4. **#9 - OTA Updates**: Atualizar firmware remotamente via ESP-NOW

---

## 📞 Suporte

- **Logs detalhados**: `idf.py -p PORT monitor --print-filter "*"`
- **Limpar build**: `cd projeto && rm -rf build/ && idf.py build`
- **Factory reset NVS**: `idf.py -p PORT erase-flash` (apaga tudo!)

**Pronto para começar? Comece pelo Passo 1! 🎯**

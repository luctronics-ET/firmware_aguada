# 🔍 DIAGNÓSTICO - Gateway sem dados (15 dez 2025)

## ❌ PROBLEMA IDENTIFICADO

### Banco de Dados:
- ✅ **Conectando**: MySQL funcionando
- ✅ **Total de leituras**: 663 registros
- ❌ **Leituras HOJE**: 0 (zero)
- 📅 **Última leitura**: 14 de dezembro de 2025, 16:15:09 (ontem)

### Última atividade:
```
2025-12-14 16:15:09 - Node 1: 65cm, 405cm
2025-12-14 16:15:09 - Node 1: 154cm, 316cm
2025-12-14 16:14:58 - Node 1: 56cm, 414cm
2025-12-14 16:14:56 - Node 2: 38cm, 432cm
```

**➡️ Gateway parou de receber dados há ~28 horas!**

---

## 🔌 HARDWARE DETECTADO

### Dispositivos USB:
- ✅ `/dev/ttyACM0` - Dispositivo conectado (provável gateway ou node)

### Processos:
- ❌ Nenhum processo `idf.py monitor` ativo
- ❌ Gateway não está rodando

---

## 🚨 CAUSA RAIZ

O **gateway NÃO está rodando**! 

Possíveis razões:
1. ❌ Gateway não foi iniciado após reiniciar o sistema
2. ❌ Processo travou/crashou ontem às 16:15
3. ❌ Cabo USB desconectado do gateway (conectado em node?)
4. ❌ Nodes enviando mas gateway não ouvindo (ESP-NOW channel mismatch?)

---

## ✅ SOLUÇÃO: Iniciar Gateway

### Passo 1: Identificar qual dispositivo está em /dev/ttyACM0
```bash
cd ~/firmware_aguada/gateway_devkit_v1
idf.py -p /dev/ttyACM0 monitor --no-reset
```

Você verá logs do dispositivo. Se for um **node**, você verá:
```
ESP-NOW sent successfully
distance_cm=X, level_cm=Y
```

Se for o **gateway**, você verá:
```
Received ESP-NOW packet from XX:XX:XX...
HTTP POST to backend...
```

### Passo 2: Conectar gateway na porta correta

Se `/dev/ttyACM0` for um node, procure `/dev/ttyACM1`:
```bash
ls -lh /dev/ttyACM*
```

### Passo 3: Flashar e iniciar gateway
```bash
cd ~/firmware_aguada/gateway_devkit_v1

# Configurar Wi-Fi se necessário
idf.py menuconfig
# → Example Connection Configuration
# → SSID: [seu_wifi]
# → Password: [sua_senha]

# Flash
idf.py -p /dev/ttyACM0 flash

# Monitor
idf.py -p /dev/ttyACM0 monitor
```

### Passo 4: Verificar nodes transmitindo

Em outro terminal:
```bash
cd ~/firmware_aguada/node_ultra1
idf.py -p /dev/ttyACM1 monitor --no-reset
```

Você deve ver transmissões a cada 30 segundos:
```
[12345] Ultrasonic: 65 cm
[12345] Level: 405 cm (85%)
[12345] ESP-NOW sent to gateway
```

---

## 🎯 CHECKLIST DE DIAGNÓSTICO

### Gateway:
- [ ] Conectado via USB (`/dev/ttyACM0` ou `/dev/ttyACM1`)
- [ ] Firmware flashado (versão atual)
- [ ] Wi-Fi configurado (SSID + senha)
- [ ] Backend URL correta (`INGEST_URL` em `main.c`)
- [ ] Canal ESP-NOW = 11 (mesmo dos nodes)
- [ ] Monitor mostrando "Received ESP-NOW packet..."
- [ ] HTTP POST com status 200 OK

### Nodes:
- [ ] Ligados (bateria ou fonte)
- [ ] LED piscando a cada 30s (transmissão)
- [ ] MAC do gateway correto em `node_ultra1.cpp`
- [ ] Canal ESP-NOW = 11
- [ ] Dentro do alcance (< 100m)

### Backend:
- [ ] Servidor PHP rodando (porta 8080)
- [ ] `ingest_sensorpacket.php` acessível
- [ ] MySQL rodando
- [ ] Tabela `leituras_v2` existe
- [ ] Permissões de escrita OK

---

## 🔧 COMANDOS RÁPIDOS

### Ver dispositivos USB:
```bash
ls -lh /dev/tty{USB,ACM}*
```

### Identificar dispositivo:
```bash
udevadm info -a -n /dev/ttyACM0 | grep -i product
```

### Matar processos travados:
```bash
pkill -f "idf.py monitor"
```

### Reiniciar MySQL (se necessário):
```bash
sudo systemctl restart mysql
```

### Testar backend manualmente:
```bash
curl -X POST http://localhost:8080/ingest_sensorpacket.php \
  -H "Content-Type: application/json" \
  -d '{
    "version": 1,
    "node_id": 99,
    "mac": "AA:BB:CC:DD:EE:FF",
    "seq": 1,
    "distance_cm": 100,
    "level_cm": 300,
    "percentual": 60,
    "volume_l": 30000,
    "vin_mv": 4850,
    "rssi": -50,
    "ts_ms": 12345
  }'
```

Se retornar `{"status":"success"}`, o backend está OK!

---

## 📊 STATUS ATUAL

| Componente | Status | Observação |
|------------|--------|------------|
| MySQL      | ✅ OK  | 663 leituras totais |
| Backend PHP| ✅ OK  | Servidor rodando na porta 8080 |
| Gateway    | ❌ OFF | Não está recebendo pacotes ESP-NOW |
| Nodes      | ❓ ?   | Provavelmente transmitindo (LED?) |
| Última leitura | 📅 | 14/12/2025 16:15 (28h atrás) |

---

## 🚀 PRÓXIMA AÇÃO RECOMENDADA

**1. Verifique se nodes estão ligados e piscando LED**

**2. Conecte gateway via USB e inicie monitor:**
```bash
cd ~/firmware_aguada/gateway_devkit_v1
idf.py -p /dev/ttyACM0 monitor
```

**3. Aguarde 30 segundos e observe se aparecem logs:**
```
I (12345) espnow: Received packet from XX:XX:XX:XX:XX:XX
I (12345) http: POST to http://localhost:8080/ingest_sensorpacket.php
I (12345) http: HTTP Status: 200
```

**4. Se não receber nada:**
- Verifique se nodes estão no canal 11
- Verifique se MAC do gateway está correto nos nodes
- Teste alcance aproximando um node do gateway

**5. Assim que gateway começar a receber, rode:**
```bash
cd ~/firmware_aguada/backend
php -r "\$conn = new mysqli('localhost', 'aguada_user', '', 'sensores_db'); 
\$r = \$conn->query('SELECT * FROM leituras_v2 ORDER BY created_at DESC LIMIT 5'); 
while(\$row = \$r->fetch_assoc()) print_r(\$row);"
```

Deverá mostrar novas leituras com timestamp atual! ✅

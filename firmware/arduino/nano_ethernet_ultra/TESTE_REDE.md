# Teste de Conectividade - Arduino Nano Ethernet

## Status Atual

✅ **Firmware gravado com sucesso**
- Version: 1
- Node ID: 3
- MAC: DE:AD:BE:EF:FE:ED
- IP: 192.168.0.222
- Backend: 192.168.0.117:8080

✅ **Sensor funcionando**
- HC-SR04 lendo distâncias (105-127cm)
- Calculando level, percentual, volume corretamente

✅ **POST implementado**
- Envia para backend a cada 30 segundos
- Mesmo formato dos ESP32 (SensorPacketV1)

❌ **Aguardando conexão ethernet**
- Cabo precisa ser conectado
- Nano desconectado da USB para conectar na rede

---

## Testes Após Conectar na Rede

### 1. Verificar conectividade
```bash
ping -c 3 192.168.0.222
```
**Esperado:** Nano responde ao ping

### 2. Testar servidor HTTP
```bash
curl http://192.168.0.222/
```
**Esperado:** Retorna JSON com telemetria:
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

### 3. Monitorar dados no banco (tempo real)
```bash
watch -n 5 'sudo mysql sensores_db -e "SELECT * FROM leituras_v2 WHERE mac LIKE \"%de:ad%\" ORDER BY created_at DESC LIMIT 3;"'
```
**Esperado:** Novos registros a cada 30 segundos com node_id=3

### 4. Ver estatísticas completas
```bash
sudo mysql sensores_db -e "SELECT mac, node_id, COUNT(*) as total, MAX(created_at) as ultimo FROM leituras_v2 GROUP BY mac, node_id ORDER BY ultimo DESC;"
```
**Esperado:** Linha com MAC `de:ad:be:ef:fe:ed` e node_id `3`

### 5. Dashboard web
Abrir: http://192.168.0.117:8080/dashboard.php

**Esperado:** Ver node_id 3 (Nano) junto com os 4 ESP32

---

## Troubleshooting

### Nano não responde ao ping
- Verificar cabo ethernet conectado
- Verificar LEDs no conector RJ45 piscando
- Verificar switch/roteador reconhece dispositivo
- Resetar Nano (botão físico)

### HTTP retorna erro
- Verificar se IP está correto (192.168.0.222)
- Testar `telnet 192.168.0.222 80`
- Verificar firewall não está bloqueando

### Dados não aparecem no banco
- Verificar se backend está rodando: `ps aux | grep php`
- Verificar logs do PHP: `tail -f /var/log/php*.log`
- Testar POST manual:
  ```bash
  curl -X POST http://192.168.0.117:8080/ingest_sensorpacket.php \
    -H "Content-Type: application/json" \
    -d '{"version":1,"node_id":3,"mac":"de:ad:be:ef:fe:ed","seq":1,"distance_cm":100,"level_cm":350,"percentual":78,"volume_l":62400,"vin_mv":0,"rssi":0,"ts_ms":123456}'
  ```

---

## Próximos Passos

1. ✅ Desconectar Nano da USB
2. ⏳ Conectar cabo ethernet
3. ⏳ Aguardar 1 minuto (Nano fará POST automático)
4. ⏳ Executar testes acima
5. ⏳ Verificar dados no dashboard

---

## Arquitetura Final

```
┌─────────────┐
│ 4x ESP32    │ ──ESP-NOW──┐
│ (node_ultra)│            │
└─────────────┘            ▼
                    ┌──────────────┐
┌─────────────┐    │ ESP32 Gateway│───WiFi───┐
│ 1x Nano     │    │ (ttyACM0)    │          │
│ (ethernet)  │────┘              │          ▼
└─────────────┘     Ethernet      │   ┌─────────────┐
                                  │   │ PHP Backend │
                                  └───│ MySQL       │
                                      │ Dashboard   │
                                      └─────────────┘
```

**Total:** 5 sensores ultrassônicos reportando para backend unificado
- 4x via ESP-NOW → Gateway → HTTP
- 1x via Ethernet → HTTP direto

# Nó Ultra 1 (ESP32-C3 Supermini) – Aguada

Nó de nível d'água com HC-SR04. Mede, calcula nível/percentual/volume e envia `SensorPacketV1` por ESP-NOW.

## Build e Flash
```bash
cd node_ultra1
idf.py set-target esp32c3   # ou outro, conforme hardware
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```
- LED ativo-baixo (GPIO8). Ajuste em `sdkconfig.defaults` se necessário.
- TRIG=GPIO1, ECHO=GPIO0 (Supermini). Ajuste em `node_ultra1.cpp` se mudar hardware.

## Parâmetros do Reservatório
- VOL_MAX_L=80000 L (80 m³)
- LEVEL_MAX_CM=450 cm
- SENSOR_OFFSET_CM=20 cm
- volume proporcional ao nível (modelo cilíndrico simples).

## Pacote Enviado (SensorPacketV1)
- version, node_id, mac[6], seq
- distance_cm, level_cm, percentual, volume_l, vin_mv
- rssi, ts_ms (preenchidos pelo gateway)

## LED e Estados
- Boot: 3 piscadas lentas.
- Transmissão: 3 piscadas curtas.
- Falha de envio: piscadas rápidas.

## Erro de Medição
- HC-SR04 típico ±3 mm + ruído; ruído convertido para ±1 cm em nível.
- Erros adicionais de montagem/alinhamento do sensor podem somar 1–3 cm; offset ajustável em SENSOR_OFFSET_CM.

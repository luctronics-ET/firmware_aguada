# Gateway ESP32 DevKit V1 (Aguada)

Recebe pacotes ESP-NOW dos nós, preenche RSSI/timestamp, enfileira e envia ao backend HTTP.

## Build e Flash
```bash
cd gateway_devkit_v1
idf.py build
idf.py -p /dev/ttyACM0 flash
idf.py -p /dev/ttyACM0 monitor   # Ctrl+] para sair
```
- Target: ESP32 (DevKit V1). LED em GPIO2, ativo-alto.
- Canal segue o AP; fixe o roteador no canal 11 (ou outro) e alinhe os nós no mesmo canal.

## Rede e Envio
- STA com SSID/PASS definidos em `main.c` (`WIFI_SSID`, `WIFI_PASS`).
- Endpoint HTTP em `INGEST_URL` (ex.: `http://<host>:8080/ingest_sensorpacket.php`).
- Callback ESP-NOW só enfileira. Tarefa `packet_processing` valida e envia para fila HTTP. Tarefa `http_worker` consome a fila e chama `esp_http_client` com timeout curto.
- Logs mostram IP, canal e status HTTP.

## Formato do Pacote (SensorPacketV1)
- Campos principais: version, node_id, mac[6], seq, distance_cm, level_cm, percentual, volume_l, vin_mv, rssi, ts_ms.
- `rssi` e `ts_ms` são preenchidos pelo gateway.

## LED / Botão
- LED GPIO2: heartbeat a cada 2s. (Sem botão ainda; estados Config/Operação a implementar.)

## Notas
- Mantenha AP e nós no mesmo canal fixo para estabilidade ESP-NOW.
- Se o backend estiver offline, pacotes são descartados após fila HTTP encher (20 slots). Ajuste tamanhos conforme necessidade.

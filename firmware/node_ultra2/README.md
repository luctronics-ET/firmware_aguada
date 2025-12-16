# Nó Ultra 2 (placeholder)

Segundo nó de telemetria baseado em ESP32 (configurar conforme hardware). Use este diretório como base para clonar a lógica do `node_ultra1` e ajustar pinos/IDs.

## Sugestão de Setup
- Copiar parâmetros de reservatório e pinos conforme o hardware usado.
- Ajustar `node_id` e MAC, alinhar canal ESP-NOW com o gateway (mesmo canal do AP fixo).
- Reutilizar `SensorPacketV1` definido em `common/telemetry_packet.h`.

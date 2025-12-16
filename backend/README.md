# backend_xampp (fallback simples)

Backend PHP/MySQL minimalista para ingestão e visualização de `SensorPacketV1`.

## Estrutura
- `config.php` – credenciais do DB.
- `schema.sql` – tabela `leituras_v2`.
- `ingest_sensorpacket.php` – recebe JSON via POST e insere no DB.
- `dashboard.php` – mostra últimas 30 leituras (auto refresh 15s).

## Uso rápido (XAMPP)
1. Copie esta pasta para o htdocs do XAMPP.
2. Crie DB `sensores_db` e importe `schema.sql`.
3. Ajuste `config.php` (DB host/user/pass) e `INGEST_URL` no gateway.
4. Rode Apache/PHP e acesse `http://localhost:8080/dashboard.php`.

## Formato esperado (JSON)
```json
{
  "version":1,
  "node_id":1,
  "mac":"AA:BB:CC:DD:EE:FF",
  "seq":123,
  "distance_cm":100,
  "level_cm":350,
  "percentual":75,
  "volume_l":60000,
  "vin_mv":3300,
  "rssi":-50,
  "ts_ms":1234567
}
```

## Observações
- Sem autenticação; use apenas em rede confiável.
- Em falha de DB, responde HTTP 500; gateway descarta após fila encher.

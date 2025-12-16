# Telemetria ESP-NOW — Nível de Água (ESP32)

Sistema de telemetria com sensores ultrassônicos ESP32-C3, gateway ESP32 DevKit V1, e backend PHP/MySQL.

## ⚡ Performance & Limpeza

**Computador lento?** O projeto pode ter ~1.5GB em pastas `build/`. Execute:

```bash
cd ~/firmware_aguada
./limpar_builds.sh  # Remove builds antigos, recupera espaço
```

Os builds são recriados automaticamente ao compilar (`idf.py build`). Nunca commit builds no Git!

## 🚀 Inicialização Rápida

**Iniciar todo o sistema automaticamente:**

```bash
cd ~/firmware_aguada
./start_services.sh  # Inicia MySQL + PHP + abre navegador
```

**Verificar status:**
```bash
./status_services.sh  # Mostra status de todos os serviços
```

**Parar serviços:**
```bash
./stop_services.sh   # Para servidor PHP
```

O script `start_services.sh` faz automaticamente:
- ✅ Verifica/inicia MySQL
- ✅ Cria banco `sensores_db` (se não existir)
- ✅ Importa schema (se necessário)
- ✅ Inicia servidor PHP na porta 8080
- ✅ Abre navegador em http://localhost:8080

## 📁 Estrutura
- `node_ultra1/`: firmware do nó (ESP32-C3 por padrão).
- `gateway_devkit_v1/`: firmware do gateway (ESP32 DevKit V1, fila HTTP opcional).
- `node_ultra2/`: segundo nó (clone do Ultra01).
- `components/` e `common/`: código compartilhado (`ultrasonic01`, `level_calculator`, `telemetry_packet.h`).
- `backend/`: Backend PHP/MySQL para ingestão e dashboard.
- `frontend/`: Estrutura preparada para dashboard web (React/Vue/Next.js).
- `database/`: Schemas SQL e migrations.
- `docs/`: Documentação de arquitetura.

**Arquivos importantes:**
- `.gitignore`: Evita commit de builds (configurado automaticamente)
- `limpar_builds.sh`: Script para liberar espaço em disco
- `REORGANIZACAO.md`: Plano de estrutura futura (firmware/backend/frontend separados)

## Parâmetros do reservatório (Node Ultra01)
- `VOL_MAX_L = 80000` (80 m³)
- `LEVEL_MAX_CM = 450`
- `SENSOR_OFFSET_CM = 20`
- Resolução: 1 cm para nível; volume proporcional ao nível.

## Pinos (ESP32-C3 Supermini por padrão)
- `TRIG_GPIO = GPIO_NUM_1`
- `ECHO_GPIO = GPIO_NUM_0`
- `LED_GPIO = GPIO_NUM_8` (LED embutido)
- Ajuste conforme seu hardware.

### Ativo-alto vs ativo-baixo do LED
- Por padrão, considera LED ativo-baixo (Supermini). Se seu LED for ativo-alto, rode `idf.py menuconfig` no projeto `node_ultra1` e habilite `Node Ultra01 Options -> LED embutido ativo em nivel alto`, ou descomente `CONFIG_LED_ACTIVE_HIGH=y` em `node_ultra1/sdkconfig.defaults`.

## Status de LED (nó)
- LED fica apagado na maior parte do tempo (padrão).
- Durante inicialização/radio up ("procurando gateway"), 3 piscadas lentas.
- Ao transmitir, 3 piscadas curtas.
- Se falhar o envio, piscadas rápidas (erro).

## Formato do Pacote (ESP-NOW)
`SensorPacketV1` (compacto, `packed`):
- `version`, `node_id`, `mac[6]`, `seq`
- `distance_cm`, `level_cm`, `percentual`, `volume_l`, `vin_mv`
- `rssi`, `ts_ms` (preenchidos/atualizados pelo gateway)

Tabela simples (raw recebido pelo gateway):

| mac | node_id | seq | version | distance_cm | level_cm | percentual (%) | volume_l | vin_mv | rssi | ts_ms |
|---|---|---|---|---|---|---|---|---|---|---|
| FF:FF:FF:FF:FF:FF | 1 | 42 | 1 | 123 | 321 | 71 | 56890 | 3300 | -60 | 1234567 |

## Build (ESP-IDF)
Apps separados com CMake de projeto:

- Nó: `node_ultra1/`
	```bash
	cd node_ultra1
	idf.py set-target esp32c3   # ou esp32, conforme o hardware
	idf.py menuconfig           # opcional
	idf.py build
	idf.py -p /dev/ttyUSB0 flash monitor
	```

- Gateway: `gateway_devkit_v1/`
	```bash
	cd gateway_devkit_v1
	idf.py set-target esp32     # esp32/esp32c3 conforme hardware
	idf.py build
	idf.py -p /dev/ttyUSB1 flash monitor
	```

Estrutura padrão: `project.cmake` no `CMakeLists.txt` raiz e registros em `main/CMakeLists.txt` via `idf_component_register`. Includes adicionam `../..`, `../../components` e `../../common`.

## Envio e Recepção
- Nó envia por broadcast (MAC FF:FF:FF:FF:FF:FF). Opcionalmente, adicione peer do gateway para unicast.
- Gateway registra callback e imprime os campos do pacote, sobrescrevendo `ts_ms` e (se disponível) `rssi`. No `gateway_devkit_v1` há fila/worker HTTP para envio confiável.

## Backend simples (PHP/MySQL)
- Pasta `backend/` contém ingest PHP (`ingest_sensorpacket.php`) e dashboard (`dashboard.php`).
- Importar `database/schema.sql` em um DB MySQL e ajustar `backend/config.php` (host/user/pass/DB).
- Configure `INGEST_URL` no gateway para apontar para o endpoint de ingestão.
- Não há autenticação; use apenas em rede confiável.

**Frontend:** Estrutura preparada em `frontend/` para integração futura. Veja `frontend/README.md` para opções (React/Vue/Next.js/TailAdmin).

## Próximos Passos (Expansão)
- Filtro de média/mediana (já há mediana de 3 leituras). Pode-se aumentar janela e implementar EMA/Kalman.
- Tratamento de erros (intervalo válido de distância, saturação já aplicada).
- Display I2C no nó (LCD/OLED) usando os resultados de `level_calculator`.
- Integração com servidor via Wi-Fi (HTTP/MQTT) no gateway.

## Observações
- O código do nó prioriza operação autônoma: mede, calcula e envia, depois deep-sleep.
- Se necessário, altere `SAMPLE_INTERVAL_S` para o período desejado.
- Caso queira manter JSON, basta recuperar a versão anterior (antes da migração para pacote binário) ou implementar um `#define` para alternar o formato.

## Monitoramento Serial
Para monitorar a saída serial do nó Ultra01 sem reiniciá-lo, use o seguinte comando:

```bash
cd ~/firmware_aguada/node_ultra1
idf.py -p /dev/ttyACM0 monitor --no-reset
```

Isso é útil para depuração e verificação de dados em tempo real.

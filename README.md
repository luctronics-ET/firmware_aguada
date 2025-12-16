# Telemetria ESP-NOW — Nível de Água (ESP32)

Sistema de telemetria com sensores ultrassônicos ESP32-C3, gateway ESP32 DevKit V1, e backend PHP/MySQL.

## 🏛️ Arquitetura da Rede Hídrica - CMASM

### Definições do Sistema Predial

**Sistema Predial**: Conjunto de equipamentos, infraestrutura, softwares e procedimentos que, instalados e configurados conforme as edificações, atuam de forma integrada para fornecer algum recurso ou produto. *Redes são necessárias para conectar os elementos do sistema.*

**Exemplos**: Sistema elétrico, sistema hídrico, sistema de sensores, sistema de fonia.

**Redes**: Divisão do sistema devido à qualidade/tipo do recurso fornecido.

### Redes Hídricas do CMASM

O CMASM possui 4 redes hídricas distintas:
- 🚰 **Rede de Abastecimento**: Captação externa
- 💧 **Rede de Consumo**: Água potável para uso geral
- 🔥 **Rede de Incêndio**: Água não tratada para combate a incêndio
- 🚽 **Rede de Esgoto**: Coleta e tratamento

---

### 📍 NODE-01: Castelo de Consumo (CON)
**Local**: Castelo de Consumo  
**Rede**: Consumo (água potável)  
**Sensor**: ESP32-C3 node_ultra1

**Elementos**:
- **RCON**: Reservatório elevado de 80 ton para consumo
- **VCON-IN1**: Válvula de entrada - recebe água das cisternas CIE1 e CIE2
- **VCON-OUT1**: Válvula de saída AZ1 - envia água para Área Azul (edifícios administrativos)
- **VCON-OUT2**: Válvula de saída AZ2 - envia água para Área Azul (baixadão)
- **VCON-OUT3**: Válvula de saída AV - envia água para Área Vermelha (oficinas)

---

### 📍 NODE-02: Castelo de Incêndio (CAV)
**Local**: Castelo de Incêndio  
**Rede**: Incêndio (água não tratada)  
**Sensor**: ESP32-C3 node_ultra2

**Elementos**:
- **RCAV**: Reservatório de 80 ton para rede de combate a incêndio
- **VCAV-IN1**: Válvula de entrada VB03 - recebe água da Casa de Bombas N03
- **VCAV-OUT1**: Válvula de saída - envia água para rede incêndio Área Azul (CAVAZ)
- **VCAV-OUT2**: Válvula de saída - envia água para rede incêndio Área Vermelha (CAVAV)

---

### 📍 NODE-03: Casa de Bombas N03 (B03)
**Local**: Casa de Bombas N03  
**Rede**: Bombeamento  
**Sensor**: *Planejado* (não instalado)

**Elementos**:
- **RB03**: Reservatório de 80 ton para bombeamento
- **VB03-IN1**: Válvula de entrada VIE1 - recebe água das cisternas CIE1 e CIE2
- **VB03-OUT1**: Válvula de saída VCAV - envia água para Castelo de Incêndio (CAV)
- **VB03-OUT2**: Válvula de saída VCON - envia água para Castelo de Consumo (CON)

---

### 📍 NODE-04: Cisternas Ilha do Engenho (CIE)
**Local**: Cisternas Ilha do Engenho  
**Rede**: Captação/Armazenamento  
**Sensores**: *Planejado* (2 sensores não instalados)

**Elementos**:

#### Cisterna CIE1 (Sensor 1 - planejado)
- **CIE1**: Cisterna IE 01 - reservatório de 245 ton
- **VCIE1-OUT1**: Válvula de saída VB03 - envia água para Casa de Bombas N03
- **VCIE1-IN1**: Válvula de entrada 01 - recebe água das cisternas IF

#### Cisterna CIE2 (Sensor 2 - planejado)
- **CIE2**: Cisterna IE 02 - reservatório de 245 ton
- **VCIE2-OUT1**: Válvula de saída VB03 - envia água para Casa de Bombas N03
- **VCIE2-IN1**: Válvula de entrada 02 - recebe água das cisternas IF

---

### 📊 Status Atual da Instalação

| Node | Local | Rede | Capacidade | Status Sensor |
|------|-------|------|------------|---------------|
| NODE-01 | CON - Castelo Consumo | Consumo | 80 ton | ✅ **Ativo** (ESP32-C3) |
| NODE-02 | CAV - Castelo Incêndio | Incêndio | 80 ton | ✅ **Ativo** (ESP32-C3) |
| NODE-03 | B03 - Casa Bombas | Bombeamento | 80 ton | ⚠️ Planejado |
| NODE-04 | CIE1 - Cisterna 01 | Captação | 245 ton | ⚠️ Planejado |
| NODE-04 | CIE2 - Cisterna 02 | Captação | 245 ton | ⚠️ Planejado |

**Sensores ativos**: 2 de 5 planejados (NODE-01 e NODE-02)

---

## 🎯 NOVO: Sistema de Balanço Hídrico v2.0

**✅ Correção Fundamental Aplicada (15/12/2025)**

Implementada lógica correta de balanço hídrico com detecção automática de vazamentos:

```
BALANÇO = VOLUME_FINAL - VOLUME_INICIAL

• BALANÇO > 0 → ENTRADA (abastecimento)
• BALANÇO < 0 → SAÍDA (consumo)
• Consumo > Esperado + 20% → 🟡 ALERTA: Possível vazamento
• Consumo > Esperado + 50% → 🔴 CRÍTICO: Vazamento severo!
```

**Features**:
- ✅ Cálculo automático de consumo por reservatório
- ✅ Comparação com média histórica (últimos 7 dias)
- ✅ Detecção de vazamentos com 3 níveis (normal/alerta/crítico)
- ✅ Interface web com indicadores visuais por cor
- ✅ Views SQL para análise diária e alertas
- ✅ Stored procedure otimizada

**Teste rápido**:
```bash
./test_balanco_corrigido.sh  # Valida todo o sistema
```

**Documentação**: `docs/CORRECAO_BALANCO_HIDRICO.md`

---

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
- `backend/`: Backend PHP/MySQL para ingestão, dashboard e **relatórios de serviço**.
  - `relatorio_servico.html`: Interface de relatório operacional ✨ **NOVO**
  - `api/relatorios.php`: API de relatórios e balanço hídrico ✨ **NOVO**
- `frontend/`: Estrutura preparada para dashboard web (React/Vue/Next.js).
- `database/`: Schemas SQL e migrations.
  - `migrations/004_balanco_hidrico.sql`: Tabelas de relatórios ✨ **NOVO**
- `docs/`: Documentação de arquitetura.
  - `RELATORIOS_SERVICO.md`: Doc completa de relatórios (51 KB) ✨ **NOVO**

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

## 📊 Sistema de Relatórios de Serviço ✨ NOVO

Sistema completo para geração de relatórios operacionais com cálculos automáticos de balanço hídrico.

### Acesso Rápido

```bash
# Abrir interface de relatório
xdg-open "http://localhost:8080/relatorio_servico.html"
```

### Funcionalidades

✅ **Preenchimento Automático**: Leituras iniciais dos sensores ESP32  
✅ **Cálculo em Tempo Real**: Consumo calculado automaticamente  
✅ **Balanço Hídrico**: Entrada/Saída/Divergência por período  
✅ **Validação**: Supervisores validam relatórios antes do fechamento  
✅ **API REST**: 10 endpoints para CRUD e cálculos  

### Componentes

- **Interface:** `backend/relatorio_servico.html`
- **API:** `backend/api/relatorios.php` (10 endpoints)
- **Banco:** `database/migrations/004_balanco_hidrico.sql` (4 tabelas, 2 views, 1 stored procedure)
- **Documentação:** `docs/RELATORIOS_SERVICO.md` (51 KB, guia completo)

### Exemplo de Uso

1. Operador abre relatório (manhã 06:00)
2. Sistema preenche volumes iniciais automaticamente
3. Durante turno, registra abastecimentos via API
4. Fim do turno (14:00), atualiza volumes finais
5. Sistema calcula consumo automaticamente
6. Operador preenche observações e finaliza
7. Supervisor valida relatório

**Cálculos:**
- `consumo = volume_inicial + entrada - volume_final`
- `divergencia = variacao_real - balanco_calculado`
- `percentual_divergencia = (divergencia / entrada) * 100`

Veja documentação completa em `docs/RELATORIOS_SERVICO.md`.

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

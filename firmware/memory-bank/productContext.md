# Product Context

Sistema de Telemetria IoT - Aguada

## Overview

Sistema completo de monitoramento em tempo real de nível de água em 4 reservatórios industriais usando:
- **6 sensores**: 5 ESP32-C3 (ESP-NOW) + 1 Arduino Nano Ethernet
- **1 gateway**: ESP32 DevKit V1 (receptor ESP-NOW + HTTP forwarder)
- **Backend**: PHP 8.x + MySQL 8.x com APIs REST
- **Frontend**: Dashboard web TailAdmin (Tailwind CSS + Alpine.js + ApexCharts)

Total de 4 reservatórios monitorados:
1. RCON (Consumo): 80m³ - Node 1 (ESP-NOW) + Node 10 (Ethernet) - **redundância**
2. RCAV (Incêndio): 80m³ - Node 2
3. RCB3 (Casa de Bombas 03): 80m³ - Node 3
4. CIE (Castelo Incêndio Elevado): 245m³ - Node 4 + Node 5 (dual sensor)

## Core Features

### Firmware
- Medição ultrassônica HC-SR04 (precisão 1cm)
- Cálculo automático de nível, percentual e volume
- Detecção de anomalias (sensor travado, queda/subida rápida)
- Transmissão ESP-NOW (nodes 1-5) ou HTTP direto (node 10)
- ACK mechanism com retry exponencial
- Persistência NVS (gateway) e EEPROM (nano)
- LED status indicators

### Backend
- Ingestão JSON via HTTP POST
- Schema MySQL robusto (leituras_v2 + sistema SCADA completo)
- APIs REST para frontend:
  - GET /api/get_sensors_data.php (status atual)
  - GET /api/get_recent_readings.php (últimas N leituras)
  - GET /api/get_history.php (histórico agregado)
- Suporte a flags/alert_type do SensorPacketV1
- Migrations para balanço hídrico

### Frontend (TailAdmin)
- Dashboard em tempo real (auto-refresh 5s)
- Cards de status por node (online/offline, progress bars coloridos)
- Gráficos ApexCharts (histórico, distribuição de volume)
- Tabela de leituras recentes
- Indicadores de alerta visual
- Dark mode support
- Responsive design (mobile-first)

## Technical Stack

### Firmware
- **ESP-IDF v5.x+**: Nodes ESP32-C3 + Gateway ESP32 DevKit V1
- **Arduino AVR**: Node 10 (Nano Ethernet)
- **Protocolos**: ESP-NOW, HTTP POST, SensorPacketV1 (binary packed struct)
- **Persistência**: NVS (gateway), EEPROM (nano)

### Backend
- **PHP 8.x**: Scripts de ingestão e APIs
- **MySQL 8.x**: Database principal
- **Apache/PHP Built-in Server**: Web server
- **JSON**: Formato de comunicação HTTP

### Frontend
- **TailAdmin**: Dashboard template (Tailwind CSS 4.x)
- **Alpine.js 3.14**: Framework reativo JavaScript
- **ApexCharts 3.51**: Biblioteca de gráficos
- **Webpack 5**: Module bundler
- **Auto-refresh**: Polling a cada 5 segundos

### Infrastructure
- **Rede WiFi**: Canal 11 fixo (ESP-NOW + WiFi AP)
- **Ethernet**: Node 10 (192.168.0.210)
- **Backend**: 192.168.0.117:8080
- **Gateway**: 192.168.0.130 (DHCP)
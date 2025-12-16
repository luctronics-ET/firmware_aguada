// config_manager.h - Gerenciamento de configuração persistente via EEPROM
#pragma once

#include <EEPROM.h>

#define CONFIG_MAGIC 0xAC // Magic byte para validar EEPROM
#define CONFIG_VERSION 1
#define CONFIG_EEPROM_ADDR 0

struct DeviceConfig {
  uint8_t magic;           // 0xAC = config válida
  uint8_t version;         // versão da struct
  
  // Network
  uint8_t ip[4];           // IP do dispositivo (ex: 192.168.0.222)
  uint8_t gateway[4];      // Gateway (ex: 192.168.0.1)
  uint8_t subnet[4];       // Subnet (ex: 255.255.255.0)
  
  // Backend
  uint8_t backend_ip[4];   // IP do backend (ex: 192.168.0.117)
  uint16_t backend_port;   // Porta do backend (ex: 8080)
  
  // Sensor
  uint8_t node_id;         // ID do nó (1-255)
  uint16_t sensor_offset_cm; // Offset do sensor em cm
  uint16_t res_height_cm;  // Altura útil do reservatório
  uint32_t res_volume_l;   // Volume total em litros
  
  // Timing
  uint32_t post_interval_ms; // Intervalo entre POSTs (ex: 30000 = 30s)
  uint16_t print_interval_ms; // Intervalo de debug serial (ex: 1000 = 1s)
  
  // Sampling
  uint8_t sample_count;    // Número de amostras para média (1-10)
  uint16_t sample_delay_ms; // Delay entre amostras (ex: 100ms)
  
  uint8_t checksum;        // Checksum simples
};

class ConfigManager {
public:
  DeviceConfig config;
  
  void begin() {
    if (!load()) {
      // Carregar defaults se EEPROM inválida
      loadDefaults();
      save();
    }
  }
  
  bool load() {
    EEPROM.get(CONFIG_EEPROM_ADDR, config);
    
    // Validar magic e checksum
    if (config.magic != CONFIG_MAGIC) {
      return false;
    }
    
    uint8_t calc_checksum = calculateChecksum();
    if (config.checksum != calc_checksum) {
      return false;
    }
    
    return true;
  }
  
  void save() {
    config.magic = CONFIG_MAGIC;
    config.version = CONFIG_VERSION;
    config.checksum = calculateChecksum();
    EEPROM.put(CONFIG_EEPROM_ADDR, config);
  }
  
  void loadDefaults() {
    config.magic = CONFIG_MAGIC;
    config.version = CONFIG_VERSION;
    
    // Network defaults
    config.ip[0] = 192; config.ip[1] = 168; config.ip[2] = 0; config.ip[3] = 222;
    config.gateway[0] = 192; config.gateway[1] = 168; config.gateway[2] = 0; config.gateway[3] = 1;
    config.subnet[0] = 255; config.subnet[1] = 255; config.subnet[2] = 255; config.subnet[3] = 0;
    
    // Backend defaults
    config.backend_ip[0] = 192; config.backend_ip[1] = 168; config.backend_ip[2] = 0; config.backend_ip[3] = 117;
    config.backend_port = 8080;
    
    // Sensor defaults
    config.node_id = 3;
    config.sensor_offset_cm = 20;
    config.res_height_cm = 450;
    config.res_volume_l = 80000;
    
    // Timing defaults
    config.post_interval_ms = 30000; // 30s
    config.print_interval_ms = 1000; // 1s
    
    // Sampling defaults
    config.sample_count = 3;
    config.sample_delay_ms = 100;
    
    config.checksum = calculateChecksum();
  }
  
  uint8_t calculateChecksum() {
    uint8_t sum = 0;
    uint8_t* ptr = (uint8_t*)&config;
    // Soma todos os bytes exceto o checksum
    for (size_t i = 0; i < sizeof(DeviceConfig) - 1; i++) {
      sum += ptr[i];
    }
    return sum;
  }
  
  // Helpers para conversão
  IPAddress getIPAddress() {
    return IPAddress(config.ip[0], config.ip[1], config.ip[2], config.ip[3]);
  }
  
  IPAddress getGateway() {
    return IPAddress(config.gateway[0], config.gateway[1], config.gateway[2], config.gateway[3]);
  }
  
  IPAddress getSubnet() {
    return IPAddress(config.subnet[0], config.subnet[1], config.subnet[2], config.subnet[3]);
  }
  
  IPAddress getBackendIP() {
    return IPAddress(config.backend_ip[0], config.backend_ip[1], config.backend_ip[2], config.backend_ip[3]);
  }
  
  void setIP(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    config.ip[0] = a; config.ip[1] = b; config.ip[2] = c; config.ip[3] = d;
  }
  
  void setBackendIP(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    config.backend_ip[0] = a; config.backend_ip[1] = b; config.backend_ip[2] = c; config.backend_ip[3] = d;
  }
  
  void printConfig(Stream& serial) {
    serial.println(F("\n=== Configuração Atual ==="));
    serial.print(F("Node ID: ")); serial.println(config.node_id);
    serial.print(F("IP: ")); 
    serial.print(config.ip[0]); serial.print('.');
    serial.print(config.ip[1]); serial.print('.');
    serial.print(config.ip[2]); serial.print('.');
    serial.println(config.ip[3]);
    serial.print(F("Backend: "));
    serial.print(config.backend_ip[0]); serial.print('.');
    serial.print(config.backend_ip[1]); serial.print('.');
    serial.print(config.backend_ip[2]); serial.print('.');
    serial.print(config.backend_ip[3]);
    serial.print(':'); serial.println(config.backend_port);
    serial.print(F("POST interval: ")); serial.print(config.post_interval_ms / 1000); serial.println(F("s"));
    serial.print(F("Sensor offset: ")); serial.print(config.sensor_offset_cm); serial.println(F(" cm"));
    serial.print(F("Res. height: ")); serial.print(config.res_height_cm); serial.println(F(" cm"));
    serial.print(F("Res. volume: ")); serial.print(config.res_volume_l); serial.println(F(" L"));
    serial.print(F("Sample count: ")); serial.println(config.sample_count);
    serial.print(F("Sample delay: ")); serial.print(config.sample_delay_ms); serial.println(F(" ms"));
    serial.println(F("========================\n"));
  }
};

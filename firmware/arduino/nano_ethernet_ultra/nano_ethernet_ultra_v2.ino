// Nano Ethernet + HC-SR04 - Versão 2 com Configuração Remota
// Endpoints HTTP:
//   GET  /           - Retorna telemetria atual (JSON)
//   GET  /config     - Retorna configuração atual (JSON)
//   POST /config     - Atualiza configuração (JSON body)
//   GET  /reset      - Restaura configuração padrão

#include <SPI.h>
#include <Ethernet.h>
#include "config_manager.h"

// --- Hardware config (fixo) ---
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
const uint8_t PIN_TRIG = 6;
const uint8_t PIN_ECHO = 5;

EthernetServer server(80);
EthernetClient backendClient;
ConfigManager cfg;

unsigned long lastPrint = 0;
unsigned long lastPost = 0;
uint32_t seq = 0;

struct Telemetry {
  long distance_cm;
  long level_cm;
  float percentual;
  long volume_l;
};

// Medir distância com média de múltiplas amostras
long measure_cm_averaged() {
  long sum = 0;
  uint8_t valid_samples = 0;
  
  for (uint8_t i = 0; i < cfg.config.sample_count; i++) {
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);
    
    unsigned long duration = pulseIn(PIN_ECHO, HIGH, 500000UL);
    if (duration > 0) {
      long cm = (duration + 29) / 58;
      sum += cm;
      valid_samples++;
    }
    
    if (i < cfg.config.sample_count - 1) {
      delay(cfg.config.sample_delay_ms);
    }
  }
  
  if (valid_samples == 0) return -1;
  return sum / valid_samples;
}

Telemetry buildTelemetry(long distance_cm) {
  Telemetry t;
  t.distance_cm = distance_cm;

  float level = cfg.config.res_height_cm - (distance_cm - cfg.config.sensor_offset_cm);
  if (level < 0) level = 0;
  if (level > cfg.config.res_height_cm) level = cfg.config.res_height_cm;
  t.level_cm = lround(level);

  t.percentual = (level / (float)cfg.config.res_height_cm) * 100.0f;
  float volume = (level / (float)cfg.config.res_height_cm) * cfg.config.res_volume_l;
  t.volume_l = lround(volume);
  return t;
}

void printMacHex(String &out) {
  for (int i = 0; i < 6; i++) {
    if (i) out += ':';
    if (mac[i] < 16) out += '0';
    out += String(mac[i], HEX);
  }
}

void sendTelemetryJSON(EthernetClient &client, const Telemetry &t) {
  String body;
  body.reserve(200);
  body += '{';
  body += "\"version\":1";
  body += ",\"node_id\":"; body += cfg.config.node_id;
  body += ",\"mac\":\""; printMacHex(body); body += "\"";
  body += ",\"seq\":"; body += seq;
  body += ",\"distance_cm\":"; body += t.distance_cm;
  body += ",\"level_cm\":"; body += t.level_cm;
  body += ",\"percentual\":"; body += (int)t.percentual;
  body += ",\"volume_l\":"; body += t.volume_l;
  body += ",\"vin_mv\":0";
  body += ",\"rssi\":0";
  body += ",\"ts_ms\":"; body += millis();
  body += '}';
  client.println(body);
}

void sendConfigJSON(EthernetClient &client) {
  String body;
  body.reserve(400);
  body += '{';
  body += "\"node_id\":"; body += cfg.config.node_id;
  body += ",\"ip\":\""; 
  body += cfg.config.ip[0]; body += '.';
  body += cfg.config.ip[1]; body += '.';
  body += cfg.config.ip[2]; body += '.';
  body += cfg.config.ip[3]; body += "\"";
  body += ",\"backend_ip\":\"";
  body += cfg.config.backend_ip[0]; body += '.';
  body += cfg.config.backend_ip[1]; body += '.';
  body += cfg.config.backend_ip[2]; body += '.';
  body += cfg.config.backend_ip[3]; body += "\"";
  body += ",\"backend_port\":"; body += cfg.config.backend_port;
  body += ",\"sensor_offset_cm\":"; body += cfg.config.sensor_offset_cm;
  body += ",\"res_height_cm\":"; body += cfg.config.res_height_cm;
  body += ",\"res_volume_l\":"; body += cfg.config.res_volume_l;
  body += ",\"post_interval_s\":"; body += cfg.config.post_interval_ms / 1000;
  body += ",\"sample_count\":"; body += cfg.config.sample_count;
  body += ",\"sample_delay_ms\":"; body += cfg.config.sample_delay_ms;
  body += '}';
  client.println(body);
}

bool parseConfigJSON(String& body) {
  // Parser JSON simples (procura por campos específicos)
  int idx;
  
  // node_id
  idx = body.indexOf("\"node_id\":");
  if (idx >= 0) {
    cfg.config.node_id = body.substring(idx + 10).toInt();
  }
  
  // backend_port
  idx = body.indexOf("\"backend_port\":");
  if (idx >= 0) {
    cfg.config.backend_port = body.substring(idx + 15).toInt();
  }
  
  // sensor_offset_cm
  idx = body.indexOf("\"sensor_offset_cm\":");
  if (idx >= 0) {
    cfg.config.sensor_offset_cm = body.substring(idx + 19).toInt();
  }
  
  // res_height_cm
  idx = body.indexOf("\"res_height_cm\":");
  if (idx >= 0) {
    cfg.config.res_height_cm = body.substring(idx + 16).toInt();
  }
  
  // res_volume_l
  idx = body.indexOf("\"res_volume_l\":");
  if (idx >= 0) {
    cfg.config.res_volume_l = body.substring(idx + 15).toInt();
  }
  
  // post_interval_s (converter para ms)
  idx = body.indexOf("\"post_interval_s\":");
  if (idx >= 0) {
    cfg.config.post_interval_ms = body.substring(idx + 18).toInt() * 1000UL;
  }
  
  // sample_count
  idx = body.indexOf("\"sample_count\":");
  if (idx >= 0) {
    uint8_t count = body.substring(idx + 15).toInt();
    if (count > 0 && count <= 10) {
      cfg.config.sample_count = count;
    }
  }
  
  // sample_delay_ms
  idx = body.indexOf("\"sample_delay_ms\":");
  if (idx >= 0) {
    cfg.config.sample_delay_ms = body.substring(idx + 18).toInt();
  }
  
  // backend_ip (formato: "192.168.0.117")
  idx = body.indexOf("\"backend_ip\":\"");
  if (idx >= 0) {
    String ip_str = body.substring(idx + 14);
    int end = ip_str.indexOf('"');
    if (end > 0) {
      ip_str = ip_str.substring(0, end);
      int a, b, c, d;
      if (sscanf(ip_str.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) == 4) {
        cfg.setBackendIP(a, b, c, d);
      }
    }
  }
  
  return true;
}

void handleHttpRequest(EthernetClient &client) {
  String request = "";
  String body = "";
  bool isPost = false;
  int contentLength = 0;
  
  // Ler request line
  while (client.connected() && client.available()) {
    String line = client.readStringUntil('\n');
    if (request.length() == 0) {
      request = line;
      isPost = line.startsWith("POST");
    }
    
    // Content-Length header
    if (line.startsWith("Content-Length:")) {
      contentLength = line.substring(15).toInt();
    }
    
    // Linha vazia = fim dos headers
    if (line == "\r" || line.length() == 0) {
      break;
    }
  }
  
  // Ler body se POST
  if (isPost && contentLength > 0) {
    char buf[contentLength + 1];
    int read = client.readBytes(buf, contentLength);
    buf[read] = '\0';
    body = String(buf);
  }
  
  // Processar rotas
  if (request.indexOf("GET / ") >= 0) {
    // GET / - Telemetria
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: application/json"));
    client.println(F("Connection: close"));
    client.println();
    
    long cm = measure_cm_averaged();
    Telemetry t = buildTelemetry(cm);
    sendTelemetryJSON(client, t);
    
  } else if (request.indexOf("GET /config") >= 0) {
    // GET /config - Configuração atual
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: application/json"));
    client.println(F("Connection: close"));
    client.println();
    sendConfigJSON(client);
    
  } else if (request.indexOf("POST /config") >= 0) {
    // POST /config - Atualizar configuração
    parseConfigJSON(body);
    cfg.save();
    
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: application/json"));
    client.println(F("Connection: close"));
    client.println();
    client.println(F("{\"success\":true,\"message\":\"Config saved. Restart to apply network changes.\"}"));
    
    Serial.println(F("[CONFIG] Atualizado via HTTP"));
    cfg.printConfig(Serial);
    
  } else if (request.indexOf("GET /reset") >= 0) {
    // GET /reset - Restaurar defaults
    cfg.loadDefaults();
    cfg.save();
    
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: application/json"));
    client.println(F("Connection: close"));
    client.println();
    client.println(F("{\"success\":true,\"message\":\"Config reset to defaults. Restart device.\"}"));
    
    Serial.println(F("[CONFIG] Reset para defaults"));
    
  } else {
    // 404
    client.println(F("HTTP/1.1 404 Not Found"));
    client.println(F("Connection: close"));
    client.println();
  }
}

bool postToBackend(const Telemetry &t) {
  IPAddress backend = cfg.getBackendIP();
  if (backendClient.connect(backend, cfg.config.backend_port)) {
    String body;
    body.reserve(200);
    body += '{';
    body += "\"version\":1";
    body += ",\"node_id\":"; body += cfg.config.node_id;
    body += ",\"mac\":\""; printMacHex(body); body += "\"";
    body += ",\"seq\":"; body += seq;
    body += ",\"distance_cm\":"; body += t.distance_cm;
    body += ",\"level_cm\":"; body += t.level_cm;
    body += ",\"percentual\":"; body += (int)t.percentual;
    body += ",\"volume_l\":"; body += t.volume_l;
    body += ",\"vin_mv\":0";
    body += ",\"rssi\":0";
    body += ",\"ts_ms\":"; body += millis();
    body += '}';

    backendClient.println(F("POST /ingest_sensorpacket.php HTTP/1.1"));
    backendClient.print(F("Host: "));
    backendClient.print(backend);
    backendClient.print(':');
    backendClient.println(cfg.config.backend_port);
    backendClient.println(F("Content-Type: application/json"));
    backendClient.print(F("Content-Length: "));
    backendClient.println(body.length());
    backendClient.println(F("Connection: close"));
    backendClient.println();
    backendClient.println(body);

    unsigned long timeout = millis() + 5000;
    while (backendClient.connected() && millis() < timeout) {
      if (backendClient.available()) {
        String line = backendClient.readStringUntil('\n');
        if (line.startsWith("HTTP/1.1 200")) {
          Serial.println(F("[✓] POST OK"));
          backendClient.stop();
          return true;
        }
      }
    }
    backendClient.stop();
    Serial.println(F("[✗] POST timeout"));
    return false;
  } else {
    Serial.println(F("[✗] Falha conexão backend"));
    return false;
  }
}

void setup() {
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  Serial.begin(115200);
  while (!Serial) { ; }
  Serial.println(F("\nNano Ethernet + HC-SR04 v2"));
  Serial.println(F("Config remota: http://IP/config"));

  // Carregar configuração da EEPROM
  cfg.begin();
  cfg.printConfig(Serial);

  // Inicializar ethernet com IPs da config
  Ethernet.init(10);
  Ethernet.begin(mac, cfg.getIPAddress(), cfg.getIPAddress(), cfg.getGateway(), cfg.getSubnet());
  delay(1000);

  Serial.print(F("IP: "));
  Serial.println(Ethernet.localIP());
  server.begin();
  Serial.println(F("HTTP server on port 80"));
  Serial.print(F("Backend: "));
  Serial.print(cfg.getBackendIP());
  Serial.print(':');
  Serial.println(cfg.config.backend_port);
}

void loop() {
  // HTTP server
  EthernetClient client = server.available();
  if (client) {
    handleHttpRequest(client);
    delay(1);
    client.stop();
  }

  unsigned long now = millis();
  
  // POST para backend (intervalo configurável)
  if (now - lastPost > cfg.config.post_interval_ms) {
    lastPost = now;
    long cm = measure_cm_averaged();
    seq++;
    Telemetry t = buildTelemetry(cm);
    
    Serial.print(F("[POST] seq=")); Serial.print(seq);
    Serial.print(F(" dist=")); Serial.print(t.distance_cm);
    Serial.print(F(" level=")); Serial.print(t.level_cm);
    Serial.print(F(" pct=")); Serial.print((int)t.percentual);
    Serial.print(F(" vol=")); Serial.print(t.volume_l);
    Serial.print(F(" -> "));
    
    postToBackend(t);
  }
  
  // Debug serial (intervalo configurável)
  if (now - lastPrint > cfg.config.print_interval_ms) {
    lastPrint = now;
    long cm = measure_cm_averaged();
    Telemetry t = buildTelemetry(cm);
    Serial.print(F("[INFO] dist=")); Serial.print(t.distance_cm);
    Serial.print(F(" level=")); Serial.print(t.level_cm);
    Serial.print(F(" pct=")); Serial.print((int)t.percentual);
    Serial.print(F(" vol=")); Serial.print(t.volume_l);
    Serial.println();
  }
}

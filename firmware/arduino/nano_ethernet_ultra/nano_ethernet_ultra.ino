// Nano Ethernet + HC-SR04 - Versão 2 com Configuração Remota
// 
// Node 10 - RCON (Reservatório de Consumo) - Backup do Node 1
// Mede o MESMO reservatório que o Node 1 (ESP32-C3)
// Oferece redundância via conexão Ethernet direta ao backend
//
// Compatível com SensorPacketV1 (common/telemetry_packet.h)
// Envia JSON com campos: version, node_id, mac, seq, distance_cm, level_cm,
//                        percentual, volume_l, vin_mv, flags, alert_type, rssi, ts_ms
//
// Configuração do Reservatório (idêntica ao Node 1):
//   - sensor_offset_cm: 20 cm
//   - res_height_cm: 450 cm
//   - res_volume_l: 80,000 L
//
// Anomaly detection:
//   - SENSOR_STUCK (alert_type=3): Distance unchanged for 5+ consecutive readings
//   - RAPID_DROP (alert_type=1): Level dropped >10cm since last reading
//   - RAPID_RISE (alert_type=2): Level increased >10cm since last reading
//
// Endpoints HTTP:
//   GET  /           - Retorna telemetria atual (JSON)
//   GET  /config     - Retorna configuração atual (JSON)
//   POST /config     - Atualiza configuração (JSON body)
//   GET  /reset      - Restaura configuração padrão

#include <SPI.h>
#include <Ethernet.h>
#include "config_manager.h"

// --- Hardware config (fixo) ---
// MAC address: AA:BB:CC:DD:EE:01 (Node 10 - RCON backup)
byte mac[] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01 };
const uint8_t PIN_TRIG = 6;
const uint8_t PIN_ECHO = 5;

EthernetServer server(80);
EthernetClient backendClient;
ConfigManager cfg;

unsigned long lastPost = 0;
uint32_t seq = 0;

struct Telemetry {
  long distance_cm;
  long level_cm;
  float percentual;
  long volume_l;
  uint8_t flags;        // Bit 0: is_alert
  uint8_t alert_type;   // 0=none, 1=rapid_drop, 2=rapid_rise, 3=sensor_stuck
};

// Alert detection state
struct AlertState {
  long prev_level_cm;
  long prev_distance_cm;
  uint8_t stuck_count;
  unsigned long last_change_ms;
};
AlertState alert_state = {-1, -1, 0, 0};

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
  
  // Anomaly detection
  t.flags = 0;
  t.alert_type = 0; // ALERT_NONE
  
  // Sensor stuck detection (distance unchanged for 5 consecutive readings)
  if (alert_state.prev_distance_cm == distance_cm && distance_cm != -1) {
    alert_state.stuck_count++;
    if (alert_state.stuck_count >= 5) {
      t.flags = 0x01; // FLAG_IS_ALERT
      t.alert_type = 3; // ALERT_SENSOR_STUCK
    }
  } else {
    alert_state.stuck_count = 0;
  }
  
  // Rapid drop detection (level dropped >10cm since last reading)
  if (alert_state.prev_level_cm > 0) {
    long level_change = alert_state.prev_level_cm - t.level_cm;
    if (level_change > 10) {
      t.flags = 0x01;
      t.alert_type = 1; // ALERT_RAPID_DROP
    } else if (level_change < -10) {
      t.flags = 0x01;
      t.alert_type = 2; // ALERT_RAPID_RISE
    }
  }
  
  // Update state
  alert_state.prev_level_cm = t.level_cm;
  alert_state.prev_distance_cm = distance_cm;
  alert_state.last_change_ms = millis();
  
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
  body.reserve(256);
  body += "{\"version\":1,\"node_id\":";
  body += cfg.config.node_id;
  body += ",\"mac\":\""; printMacHex(body); body += "\"";
  body += ",\"seq\":"; body += seq;
  body += ",\"distance_cm\":"; body += t.distance_cm;
  body += ",\"level_cm\":"; body += t.level_cm;
  body += ",\"percentual\":"; body += (int)t.percentual;
  body += ",\"volume_l\":"; body += t.volume_l;
  body += ",\"vin_mv\":0,\"flags\":"; body += t.flags;
  body += ",\"alert_type\":"; body += t.alert_type;
  body += ",\"rssi\":0,\"ts_ms\":"; body += millis();
  body += '}';
  client.println(body);
}

void sendConfigJSON(EthernetClient &client) {
  String body;
  body.reserve(350);
  body += "{\"node_id\":"; body += cfg.config.node_id;
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

void handleHttpRequest(EthernetClient &client) {
  String request = client.readStringUntil('\n');
  
  // Skip remaining headers
  while (client.connected() && client.available()) {
    String line = client.readStringUntil('\n');
    if (line == "\r" || line.length() == 0) break;
  }
  
  // GET / - Telemetria
  if (request.indexOf("GET / ") >= 0) {
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: application/json"));
    client.println(F("Connection: close"));
    client.println();
    
    long cm = measure_cm_averaged();
    Telemetry t = buildTelemetry(cm);
    sendTelemetryJSON(client, t);
    
  } else if (request.indexOf("GET /config") >= 0) {
    // GET /config - Configuração
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: application/json"));
    client.println(F("Connection: close"));
    client.println();
    sendConfigJSON(client);
    
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
    body.reserve(256);
    body += "{\"version\":1,\"node_id\":";
    body += cfg.config.node_id;
    body += ",\"mac\":\""; printMacHex(body); body += "\"";
    body += ",\"seq\":"; body += seq;
    body += ",\"distance_cm\":"; body += t.distance_cm;
    body += ",\"level_cm\":"; body += t.level_cm;
    body += ",\"percentual\":"; body += (int)t.percentual;
    body += ",\"volume_l\":"; body += t.volume_l;
    body += ",\"vin_mv\":0,\"flags\":"; body += t.flags;
    body += ",\"alert_type\":"; body += t.alert_type;
    body += ",\"rssi\":0,\"ts_ms\":"; body += millis();
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
  Serial.println(F("\nN10 RCON"));

  cfg.begin();

  Ethernet.init(10);
  Ethernet.begin(mac, cfg.getIPAddress(), cfg.getIPAddress(), cfg.getGateway(), cfg.getSubnet());
  delay(1000);

  Serial.print(F("IP:")); Serial.println(Ethernet.localIP());
  server.begin();
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
  
  // POST para backend
  if (now - lastPost > cfg.config.post_interval_ms) {
    lastPost = now;
    long cm = measure_cm_averaged();
    seq++;
    Telemetry t = buildTelemetry(cm);
    
    Serial.print(seq); Serial.print(F(":")); Serial.print(t.distance_cm);
    Serial.print(F("cm ")); Serial.print(t.level_cm); Serial.print(F("/")); Serial.print((int)t.percentual);
    Serial.print(F("% "));
    
    postToBackend(t);
  }
}

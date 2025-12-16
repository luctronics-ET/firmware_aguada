// Nano Ethernet + HC-SR04
// Pins: trig=D6, echo=D5 (echo uses interrupt-safe pulseIn)
// Sends telemetry to backend via HTTP POST every 30s
// Also exposes HTTP GET JSON endpoint for manual queries

#include <SPI.h>
#include <Ethernet.h>

// --- Network config ---
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 0, 222);
IPAddress dnsIP(192, 168, 0, 1);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

// --- Backend config ---
IPAddress backendServer(192, 168, 0, 117);
const char* backendPath = "/ingest_sensorpacket.php";
const uint16_t backendPort = 8080;

// --- Packet model (match SensorPacketV1 semantics) ---
const uint8_t PACKET_VERSION = 1;
const uint8_t NODE_ID = 3;
const float RES_HEIGHT_CM = 450.0;
const float RES_OFFSET_CM = 20.0;
const float RES_VOLUME_L = 80000.0;

// --- Ultrasonic pins ---
const uint8_t PIN_TRIG = 6;
const uint8_t PIN_ECHO = 5;

EthernetServer server(80);
EthernetClient backendClient;
unsigned long lastPrint = 0;
unsigned long lastPost = 0;
const unsigned long POST_INTERVAL = 30000; // 30 segundos
uint32_t seq = 0;

struct Telemetry {
  long distance_cm;
  long level_cm;
  float percentual;
  long volume_l;
};

long measure_cm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long duration = pulseIn(PIN_ECHO, HIGH, 500000UL);
  if (duration == 0) return -1;
  long cm = (duration + 29) / 58;
  return cm;
}

Telemetry buildTelemetry(long distance_cm) {
  Telemetry t;
  t.distance_cm = distance_cm;

  float level = RES_HEIGHT_CM - (distance_cm - RES_OFFSET_CM);
  if (level < 0) level = 0;
  if (level > RES_HEIGHT_CM) level = RES_HEIGHT_CM;
  t.level_cm = lround(level);

  t.percentual = (level / RES_HEIGHT_CM) * 100.0f;
  float volume = (level / RES_HEIGHT_CM) * RES_VOLUME_L;
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

void sendHttpResponse(EthernetClient &client, const Telemetry &t) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  
  String body;
  body.reserve(200);
  body += '{';
  body += "\"version\":"; body += PACKET_VERSION;
  body += ",\"node_id\":"; body += NODE_ID;
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

bool postToBackend(const Telemetry &t) {
  if (backendClient.connect(backendServer, backendPort)) {
    String body;
    body.reserve(200);
    body += '{';
    body += "\"version\":"; body += PACKET_VERSION;
    body += ",\"node_id\":"; body += NODE_ID;
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

    backendClient.println("POST " + String(backendPath) + " HTTP/1.1");
    backendClient.print("Host: ");
    backendClient.print(backendServer);
    backendClient.print(":");
    backendClient.println(backendPort);
    backendClient.println("Content-Type: application/json");
    backendClient.print("Content-Length: ");
    backendClient.println(body.length());
    backendClient.println("Connection: close");
    backendClient.println();
    backendClient.println(body);

    // Aguardar resposta (timeout 5s)
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
  Serial.println(F("Nano Ethernet + HC-SR04"));

  Ethernet.init(10);
  Ethernet.begin(mac, ip, dnsIP, gateway, subnet);
  delay(1000);

  Serial.print(F("IP: "));
  Serial.println(Ethernet.localIP());
  server.begin();
  Serial.println(F("HTTP server on port 80"));
  Serial.print(F("Backend: "));
  Serial.print(backendServer);
  Serial.print(F(":"));
  Serial.println(backendPort);
}

void loop() {
  // HTTP server para requests externos
  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        if (line == "\r" || line.length() == 0) {
          long cm = measure_cm();
          Telemetry t = buildTelemetry(cm);
          sendHttpResponse(client, t);
          break;
        }
      }
    }
    delay(1);
    client.stop();
  }

  unsigned long now = millis();
  
  // POST para backend a cada 30s
  if (now - lastPost > POST_INTERVAL) {
    lastPost = now;
    long cm = measure_cm();
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
  
  // Debug serial a cada 1s
  if (now - lastPrint > 1000) {
    lastPrint = now;
    long cm = measure_cm();
    Telemetry t = buildTelemetry(cm);
    Serial.print(F("[INFO] dist=")); Serial.print(t.distance_cm);
    Serial.print(F(" level=")); Serial.print(t.level_cm);
    Serial.print(F(" pct=")); Serial.print((int)t.percentual);
    Serial.print(F(" vol=")); Serial.print(t.volume_l);
    Serial.println();
  }
}

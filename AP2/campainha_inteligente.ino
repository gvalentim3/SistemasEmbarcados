#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <stdint.h>

const char* ssid = "iPhone de Gustavo";
const char* password = "12345678910";

const char* mqtt_server = "test.mosquitto.org";
const int   mqtt_port   = 1883;
const char* mqtt_topic  = "esp8266/teste/tempo";

WiFiClient   espClient;
PubSubClient client(espClient);

const int botaoPin = 0;
bool estadoAnterior = HIGH;
unsigned long ultimoEnvio = 0;
unsigned long debounceDelay = 200;

unsigned long  lastSend = 0;
const unsigned long interval = 1000;  // 1 segundo

String clientId = "ESP32Client-" + String(random(0xffff), HEX);

void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void setup_time() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Sincronizando NTP");
  time_t now = time(nullptr);
  while (now < 100000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  delay(1000);
  Serial.println("\nHora sincronizada!");
}

void reconnect() {
  if (!client.connected()) {
    Serial.print("Tentando conectar ao MQTT... ");
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado!");
      client.publish(mqtt_topic, "ESP32 conectado ao MQTT!");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(botaoPin, INPUT_PULLUP);
  setup_wifi();
  setup_time();
  client.setServer(mqtt_server, mqtt_port);
}

void publicarMensagem(const char* mensagem) {
  Serial.print("Enviando: ");
  Serial.println(mensagem);
  client.publish(mqtt_topic, mensagem);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();


  unsigned long nowMs = millis();

  int leitura = digitalRead(botaoPin);
  if (leitura == LOW && estadoAnterior == HIGH && (millis() - ultimoEnvio) > debounceDelay) {
    //
    if (nowMs - lastSend >= interval) {
      lastSend = nowMs;

      time_t rawSec = time(nullptr);
      uint64_t timestamp_ms = (uint64_t)rawSec * 1000ULL;  // 64 bits

      char msg[64];
      // Note o %llu para unsigned long long
      snprintf(msg, sizeof(msg), "{\"timestamp\":%llu}", timestamp_ms);
      publicarMensagem(msg);
      ultimoEnvio = millis();
    }
  }

  estadoAnterior = leitura;

  delay(10);
}
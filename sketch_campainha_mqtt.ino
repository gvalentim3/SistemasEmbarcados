#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Wifi murilo";
const char* password = "muripp13";

const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/teste/botao";

WiFiClient espClient;
PubSubClient client(espClient);

const int botaoPin = 32;
bool estadoAnterior = HIGH;
unsigned long ultimoEnvio = 0;
unsigned long debounceDelay = 200;

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
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  int leitura = digitalRead(botaoPin);
  if (leitura == LOW && estadoAnterior == HIGH && (millis() - ultimoEnvio) > debounceDelay) {
    Serial.println("Botão pressionado!");
    client.publish(mqtt_topic, "Botao pressionado!");
    ultimoEnvio = millis();
  }

  estadoAnterior = leitura;

  delay(10);
}
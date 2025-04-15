#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "iPhone de Gustavo";
const char* password = "12345678910";
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;

#define SS_PIN 5
#define RST_PIN 4

MFRC522 rfid(SS_PIN, RST_PIN);

WiFiClient espClient;
PubSubClient client(espClient);

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("ESP32RFIDClient")) {
      Serial.println("conectado!");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5s");
      delay(5000);
    }
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando-se ao WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  SPI.begin(18, 19, 23, SS_PIN); // SPI para ESP32
  rfid.PCD_Init();
  Serial.println("Aguardando cartões...");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  // Monta o UID como string
  String uidString = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uidString += "0";
    uidString += String(rfid.uid.uidByte[i], HEX);
  }
  uidString.toUpperCase();

  Serial.print("UID detectado: ");
  Serial.println(uidString);

  // Envia via MQTT
  client.publish("iotbr/esp32", uidString.c_str());
  Serial.println("Enviado via MQTT!");

  rfid.PICC_HaltA(); // Para comunicação
  delay(1000); // Evita leituras repetidas
}

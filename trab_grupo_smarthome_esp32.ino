#include <WiFi.h>
#include <PubSubClient.h>

// Configuração da rede Wi-Fi
const char* ssid = "iPhone de Gustavo";        // Nome da sua rede Wi-Fi
const char* password = "12345678910";   // Senha do Wi-Fi

// Configuração do Broker MQTT
const char* mqtt_server = "test.mosquitto.org";  // Pode usar um broker local ou público
const int mqtt_port = 1883;  // Porta padrão do MQTT

WiFiClient espClient;
PubSubClient client(espClient);

// Função para reconectar ao MQTT caso a conexão caia
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("ESP32_Client")) {
      Serial.println("Conectado!");
      client.subscribe("iotbr/esp32"); // Inscrevendo-se no tópico
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5s");
      delay(5000);
    }
  }
}

// Callback para receber mensagens
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  Serial.print("Mensagem: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  // Conectando ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao Wi-Fi...");
  }
  Serial.println("Wi-Fi conectado!");

  // Configurando o MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Publica uma mensagem a cada 5 segundos
  client.publish("iotbr/esp32", "Grupo SmartHome enviado do ESP32 :)");
  Serial.println("Mensagem enviada!");

  delay(5000);
}
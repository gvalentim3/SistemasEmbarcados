#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ————— Configurações de WiFi e MQTT —————
const char* ssid        = "iPhone de Gustavo";
const char* password    = "12345678910";
const char* mqtt_server = "test.mosquitto.org";
const int   mqtt_port   = 1883;
const char* mqtt_topic  = "iotbr/esp8266/rfid"; // Tópico para publicar o UID

// ————— Pinos do MFRC522 (ESP8266) —————
#define SS_PIN  5 // (D1)
#define RST_PIN 4 // (D2)

// --- Instâncias dos objetos ---
MFRC522 rfid(SS_PIN, RST_PIN);
WiFiClient espClient;
PubSubClient client(espClient);

// ————— Parâmetros do Teste Automático —————
const int NUM_ITENS_TESTE = 100;
unsigned long inicioTeste = 0;

// --- Contadores de Métricas Aprimorados ---
int contIteracoes  = 0; // Total de loops do teste
int contLeiturasOK = 0; // Conta apenas leituras RFID bem-sucedidas
int contMqttOK     = 0; // Conta apenas publicações MQTT bem-sucedidas

// --- Acumuladores para TEML (Tempo de Execução Médio da Leitura) ---
unsigned long somaTempos  = 0;
unsigned long somaTempos2 = 0;
int iteracoesTAML         = 0;

// Função para gerar um Client ID único e reconectar ao MQTT
void reconnect() {
  Serial.println("Tentando conectar ao broker MQTT...");
  while (!client.connected()) {
    String clientId = "ESP8266-RFID-";
    clientId += String(random(0xffff), HEX);
    
    Serial.print("Conectando ao MQTT com o ID: ");
    Serial.println(clientId);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado ao MQTT!");
    } else {
      Serial.print("Falha na conexão MQTT, rc=");
      Serial.print(client.state());
      Serial.println(". Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void setup_wifi() {
  Serial.print("Conectando ao WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  int tent = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (tent++ > 20) { // Timeout de 10 segundos
      Serial.println("\nTimeout ao conectar WiFi. O programa continuará, mas não publicará no MQTT.");
      return;
    }
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== Iniciando Programa ESP8266 + RFID + Métricas (v2) ===");

  // 1) Conecta ao WiFi
  setup_wifi();
  
  // 2) Configura o servidor MQTT
  client.setServer(mqtt_server, mqtt_port);

  // 3) Inicializa SPI e o leitor MFRC522
  Serial.println("Inicializando SPI e leitor MFRC522...");
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("MFRC522 inicializado!");
  Serial.println("Aproxime a tag para iniciar as leituras...");
  
  // 4) Zera contadores e marca o início do primeiro bloco de teste
  inicioTeste = millis();
}

// ==========================================================
// VERSÃO CORRIGIDA (FINAL, ESPERO!) PARA LEITURA ITERATIVA DA MESMA TAG
// ==========================================================
void loop() {
  // 1) Garante conexão MQTT
  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    reconnect();
  }
  client.loop();

  // --- Início da Lógica de Leitura e Métricas ---
  unsigned long t0 = millis();

  bool uidReadSuccess = false;
  
  // Tenta "ativar" ou "re-selecionar" a tag RFID
  // Esta é uma maneira mais direta de tentar obter o UID continuamente
  // Se a tag está presente e já ativa, PICC_ReadCardSerial() deveria funcionar.
  // Se a tag está presente mas em HALT (por exemplo, após uma leitura anterior que chamou HaltA),
  // PICC_IsNewCardPresent() é necessária para reativá-la.
  
  // Lógica para leitura contínua da MESMA tag:
  // Tenta selecionar e ler o UID. Esta função fará a "mágica" de reativar se preciso.
  if (rfid.PICC_ReadCardSerial()) { 
    uidReadSuccess = true;
  } else {
    // Se PICC_ReadCardSerial falhou, pode ser que a tag não esteja selecionada.
    // Tenta detectá-la como "nova" e então lê-la.
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      uidReadSuccess = true;
    }
  }


  if (uidReadSuccess) {
    // Leitura do UID foi bem-sucedida
    contLeiturasOK++; 

    String uidString = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) uidString += "0";
      uidString += String(rfid.uid.uidByte[i], HEX);
    }
    uidString.toUpperCase();

    Serial.print("UID Detectado: ");
    Serial.print(uidString);

    if (client.connected()) {
      if (client.publish(mqtt_topic, uidString.c_str())) {
        Serial.println(" -> Publicado.");
        contMqttOK++;
      } else {
        Serial.println(" -> Falha MQTT.");
      }
    } else {
      Serial.println(" -> Sem Conexão.");
    }
    
    // **Importante:** NÃO chame PICC_HaltA() aqui para leituras contínuas.
    // Isso mantém a tag em um estado selecionado/ativo, permitindo que as próximas
    // tentativas de leitura a encontrem facilmente.

  } else {
    // Nenhuma tag presente ou leitura falhou.
    Serial.println("Nenhuma tag ou erro na leitura.");
    // Aqui é onde você deve chamar HaltA() para limpar o estado
    // e permitir que uma tag seja detectada como "nova" se ela reaparecer.
    rfid.PICC_HaltA(); 
  }

  // Acumula os tempos e contadores (independentemente da leitura RFID)
  unsigned long delta = millis() - t0;
  somaTempos  += delta;
  somaTempos2 += (unsigned long)delta * delta;
  iteracoesTAML++;
  contIteracoes++;
  
  delay(50); // Pequeno atraso para evitar sobrecarga do serial e dar tempo ao RC522

  // Bloco de relatório (sem alterações)
  if (contIteracoes >= NUM_ITENS_TESTE) {
    unsigned long duracaoTotal = millis() - inicioTeste;
    float tlvPct     = (contLeiturasOK / (float)contIteracoes) * 100.0;
    float mqttPct    = (contLeiturasOK > 0) ? (contMqttOK / (float)contLeiturasOK) * 100.0 : 0;
    float mediaTAML  = (iteracoesTAML > 0) ? somaTempos / (float)iteracoesTAML : 0;
    float varianca   = (iteracoesTAML > 0) ? (somaTempos2 / (float)iteracoesTAML) - (mediaTAML * mediaTAML) : 0;
    float desvioTAML = sqrt(abs(varianca));

    Serial.println("\n====================== RELATÓRIO DO BLOCO ======================");
    Serial.printf(" Duração Total do Bloco: %lu ms\n", duracaoTotal);
    Serial.printf(" TLV (Taxa de Leituras Válidas RFID): %.2f%% (%d/%d tentativas)\n",
                  tlvPct, contLeiturasOK, contIteracoes);
    Serial.printf(" Taxa de Sucesso MQTT: %.2f%% (%d/%d publicações)\n",
                  mqttPct, contMqttOK, contLeiturasOK);
    Serial.printf(" TEML (Tempo Médio por Tentativa): %.2f ms (Desvio Padrão: %.2f ms)\n",
                  mediaTAML, desvioTAML);
    Serial.println("================================================================\n");
    Serial.println("Reiniciando contagem para o próximo bloco...");

    contIteracoes  = 0;
    contLeiturasOK = 0;
    contMqttOK     = 0;
    somaTempos     = 0;
    somaTempos2    = 0;
    iteracoesTAML  = 0;
    inicioTeste    = millis();
  }
}
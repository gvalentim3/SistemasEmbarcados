int potenciometroPin = A0; // Potenciômetro conectado ao pino A0
int valor;

void setup() {
  Serial.begin(9600);
}

void loop() {
  valor = analogRead(potenciometroPin);
  Serial.println(valor);
  delay(500);
}
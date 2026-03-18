// ------------------------------------
// Parque da Ciência Newton Freire Maia
// Curso de Formação de Professores
// Atividades Experimentais de Física
// Utilizando a Plataforma Arduino
// ------------------------------------

uint8_t pinLed = 2; // pino do LED

void setup() {
  pinMode(pinLed, OUTPUT);
}

void loop() {
  digitalWrite(pinLed, HIGH);
  delay(1000);
  digitalWrite(pinLed, LOW);
  delay(1000);
}

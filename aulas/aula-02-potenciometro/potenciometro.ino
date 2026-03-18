int pinled = 3;
int pinpot = A0;

void setup() {
  // put your setup code here, to run once:
pinMode(pinled,OUTPUT);
pinMode(pinpot,INPUT);
Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
int valor = 0;
valor = analogRead(pinpot);
Serial.println(valor);
int brilho = map(valor, 0, 1024, 0, 255);
analogWrite(pinled,brilho);
}

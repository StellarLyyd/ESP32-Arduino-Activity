#define C1 A1
#define C2 A2
#define button D10
#define IA D9
#define IB D8

bool buttonstate = false;

void setup() {
  pinMode(C1,INPUT);
  pinMode(C2,INPUT);
  pinMode(button,INPUT);
  pinMode(IA,OUTPUT);
  pinMode(IB,OUTPUT);
  Serial.begin(115200);
}

void loop() {
  if(digitalRead(button)==0&& !buttonstate){
    buttonstate = true;
    analogWrite(IA,200);
    delay(3200);
    analogWrite(IA,0);
    analogWrite(IB,0);
  }
  if(digitalRead(button)==0&& buttonstate){
    buttonstate = false;
    analogWrite(IB,200);
    delay(3200);
    analogWrite(IB,0);
    analogWrite(IA,0);
  }
}

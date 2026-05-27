#include <ESP32Servo.h>
Servo myservo; 
#define pinA 9
#define input1 A1
#define motor 5

int pos = 0;

void setup() {
  pinMode(pinA, OUTPUT);
  pinMode(input1, INPUT);
  
  ESP32PWM::allocateTimer(0);
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(motor, 1000, 2000);
}

void photoControl(){
    if(analogRead(input1)<600){
      digitalWrite(pinA, LOW);
      for (pos = 0; pos <= 180; pos += 1) {
		    myservo.write(pos);    // tell servo to go to position in variable 'pos'
		    delay(15);             // waits 15ms for the servo to reach the position
	    }
      for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
        myservo.write(pos);              // tell servo to go to position in variable 'pos'
        delay(15);                       // waits 15 ms for the servo to reach the position
      }
    }else{
      digitalWrite(pinA, HIGH);
    }
    delay(500);
}

void loop() {
    photoControl();
}

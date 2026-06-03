#include <Servo.h>
Servo myservo;
#define input1 A1
#define motor 5
#define LED 9
int pos = 0;
void setup() {
  // put your setup code here, to run once:
  pinMode(motor,OUTPUT);
  pinMode(LED,OUTPUT);
  pinMode(input1,INPUT);

  myservo.attach(motor);
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
  if(analogRead(input1)<150){ // when it is dark
      digitalWrite(LED, LOW); // turn off the LED
      for (pos = 0; pos <= 180; pos += 1) {
		    myservo.write(pos);    // tell servo to go to position in variable 'pos'
		    delay(15);             // waits 15ms for the servo to reach the position
	    }
      for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
        myservo.write(pos);              // tell servo to go to position in variable 'pos'
        delay(15);                       // waits 15 ms for the servo to reach the position
      }
    }else{ // when it is bright
      digitalWrite(LED, HIGH); // turn on the LED
    }
  delay(500);
}

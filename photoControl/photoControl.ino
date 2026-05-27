#include <ESP32Servo.h> // include the ESP32 servo library. Input Servo.h only for an Arduino
Servo myservo; // create a new Servo object
#define input1 A1
#define motor 5 
#define LED 9
// define the analog input port, the servo signal port, and the LED port

int pos = 0;

void setup() {
  pinMode(motor, OUTPUT); // declare servo signal port as the output
  pinMode(LED, OUTPUT); // declare LED as the output
  pinMode(input1, INPUT); // declare analog input port as the input
  
  ESP32PWM::allocateTimer(0); // set up a pwm timer; not always necessary
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(motor, 1000, 2000); // attach the servo object to the signal port
}

void photoControl(){
    if(analogRead(input1)<600){ // when it is dark
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
    delay(500); // always delay to prevent overly frequent detection than necessary
}

void loop() {
    photoControl();
}

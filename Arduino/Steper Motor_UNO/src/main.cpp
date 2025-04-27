#include <Arduino.h>
#include <Stepper.h>


const int stepsPerRevolution = 2048; 
Stepper myStepper (stepsPerRevolution, 8, 10, 9, 11); //піни
void setup() 
{
  myStepper.setSpeed(10); // швидкість
  Serial.begin (9600);
}

void loop() 
{
  Serial.println("За год. стрілкою");
  myStepper.step(stepsPerRevolution);
  delay(1000); 
  Serial.println("Проти год. стрілкою");
  myStepper.step(-stepsPerRevolution);
  delay(1000); 
}


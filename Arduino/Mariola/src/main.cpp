#include <Arduino.h>
#include <AccelStepper.h>
#include <MultiStepper.h>

// defining the stepper motor bindings
AccelStepper M1(AccelStepper::DRIVER, 2, 3);
AccelStepper M2(AccelStepper::DRIVER, 4, 5);
AccelStepper M3(AccelStepper::DRIVER, 6, 7);
AccelStepper M4(AccelStepper::DRIVER, 8, 9);

int maxSpeed = 3000;
int maxAccl =  500;

void setup() {
  // put your setup code here, to run once:

  // Port operation to disable both stepper motors (to not keep them under current)
  // PB5 (input 13) is for !enable
  PORTB |= 0b00100000;

  // settuping up the stepper motors
  // max speed;
  M1.setMaxSpeed(maxSpeed);
  M2.setMaxSpeed(maxSpeed);
  M3.setMaxSpeed(maxSpeed);
  M4.setMaxSpeed(maxSpeed);
  // acceleration 
  M1.setAcceleration(maxAccl);
  M2.setAcceleration(maxAccl);
  M3.setAcceleration(maxAccl);
  M4.setAcceleration(maxAccl);
  // the speed
  M1.setSpeed(maxSpeed);
  M2.setSpeed(maxSpeed);
  M3.setSpeed(maxSpeed);
  M4.setSpeed(maxSpeed);

  // for the needs of initial test
  M1.move(10000);
  M2.move(10000);
  M3.move(10000);
  M4.move(10000);


}

void loop() {
  // put your main code here, to run repeatedly:

  delay(1000);
  // PB5 (gpio 13) is for !enable
  PORTB &= 0b11011111;

  // kicking off the steppers pulses
  M1.run();
  M2.run();
  M3.run();
  M4.run();

  // PB5 (input 13) is for !enable
  PORTB |= 0b00100000;


}
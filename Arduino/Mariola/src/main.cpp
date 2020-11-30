#include <Arduino.h>
#include <AccelStepper.h>
#include <MultiStepper.h>

// defining the stepper motor bindings
AccelStepper M1(AccelStepper::DRIVER, 2, 3);
AccelStepper M2(AccelStepper::DRIVER, 4, 5);
AccelStepper M3(AccelStepper::DRIVER, 6, 7);
AccelStepper M4(AccelStepper::DRIVER, 8, 9);

float maxSpeed = 4000.0;
float maxAccl = 500.0;
float accl = maxAccl;
float speed = maxSpeed;
const int pulses_per_step = 200 * 8; // for 1/8 step

// mechanical dimensions based data
const float A = 185.0; //[mm] dist between wheels (width)
const float D = 65.0;  //[mm] wheel diameter
const float wheel_lenght = PI * D;
const float wheel_const = 10 * pulses_per_step / wheel_lenght;
const float turn_const = A / (180.0 * D);

// ***********************************
// Stuff for serial communication use
bool newData = false;
const byte numChars = 48;
char receivedChars[numChars];
char tempChars[numChars];
// variables to hold the parsed data
char messageFromPC[numChars] = {0};
// global variables to keep the received data
int param[5];
int command;
// ***********************************
// other general variabes
unsigned long idleTime;
unsigned long idleTimeDelay = 2000; // [ms] of waiting to disengage stepper drivers
bool isIdle = false;

// Functions declarations
void recvWithStartEndMarkers();
void parseData();
void goStop();
float myDivide(float A, float B);

void setup()
{
  // put your setup code here, to run once:

  // kicking off serial
  Serial.begin(9600);

  // Pin modes set
  pinMode(13, OUTPUT);

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
  // M1.move(10000);
  // M2.move(10000);
  // M3.move(10000);
  // M4.move(10000);

  delay(1000);
}

void loop()
{
  // put your main code here, to run repeatedly:

  // temporary to disable speed change
  param[4] = maxSpeed;

  // grabstuff from serial
  recvWithStartEndMarkers();
  // if we have some new data...
  if (newData == true)
  {

    strcpy(tempChars, receivedChars);

    parseData();
    newData = false;

    Serial.print("command: ");
    Serial.println(command);

    switch (command)
    {
    case 0:
      // stop action
      goStop();
      break;

    case 2:
    {
      // executing typical Mariola move
      // reading the desired max speed
      float newSpeed = param[4];
      if (newSpeed <= 0)
        newSpeed = speed;
      speed = newSpeed;
      // limiting the max speed to the predefined range only
      speed = constrain(newSpeed, 0, maxSpeed);

      float maxSteps = 1.0 * max(max(abs(param[0]), abs(param[1])), max(abs(param[2]), abs(param[3])));

      if (maxSteps > 0)
      {
        float A = abs((param[0]) / maxSteps);
        float B = abs((param[1]) / maxSteps);
        float C = abs((param[2]) / maxSteps);
        float D = abs((param[3]) / maxSteps);

        // setting speed with respect to the set distance
        M1.setMaxSpeed(newSpeed * A);
        M2.setMaxSpeed(newSpeed * B);
        M3.setMaxSpeed(newSpeed * C);
        M4.setMaxSpeed(newSpeed * D);
        // setting accel proportionally
        M1.setAcceleration(maxAccl * A);
        M2.setAcceleration(maxAccl * B);
        M3.setAcceleration(maxAccl * C);
        M4.setAcceleration(maxAccl * D);

        // setting distance for each wheel
        M1.move((long)param[0] * wheel_const);
        M2.move((long)param[1] * wheel_const);
        M3.move((long)param[2] * wheel_const);
        M4.move((long)param[3] * wheel_const);

        // turning on the drivers
        // PB5 (gpio 13) is for !enable
        PORTB &= 0b11011111;
        isIdle = false;

        Serial.print(A);
        Serial.print(" ");
        Serial.print(B);
        Serial.print(" ");
        Serial.print(C);
        Serial.print(" ");
        Serial.print(D);
        Serial.println(" ");
      }
    }
    break;

    case 3:
    {
      // execute move command with ramped acceleration
      // solution implemented.

      float newSpeed = param[4];
      if (newSpeed <= 0)
        newSpeed = speed;
      // limiting the max speed to the predefined range only
      speed = constrain(newSpeed, 0, maxSpeed);

      float maxSteps = 1.0 * max(max(abs(param[0]), abs(param[1])), max(abs(param[2]), abs(param[3])));

      if (maxSteps > 0)
      {
        float A = abs((param[0]) / maxSteps);
        float B = abs((param[1]) / maxSteps);
        float C = abs((param[2]) / maxSteps);
        float D = abs((param[3]) / maxSteps);

        // setting speed with respect to the set distance
        M1.setMaxSpeed(newSpeed * A);
        M2.setMaxSpeed(newSpeed * B);
        M3.setMaxSpeed(newSpeed * C);
        M4.setMaxSpeed(newSpeed * D);

        // distances to reavel for each wheel
        long D1 = ((long)param[0] * wheel_const);
        long D2 = ((long)param[1] * wheel_const);
        long D3 = ((long)param[2] * wheel_const);
        long D4 = ((long)param[3] * wheel_const);

        // Working on the acceleration calculations per speed and steps
        // the required acceleration for to keep the speed changes
        // per the triangle shape (ref to video desc on GH)
        // derivated formula is:
        // accl = (Vmax^2)/Steps

        float a1 = sq(newSpeed * A) / D1;
        float a2 = sq(newSpeed * B) / D2;
        float a3 = sq(newSpeed * C) / D3;
        float a4 = sq(newSpeed * D) / D4;

        M1.setAcceleration(a1);
        M2.setAcceleration(a2);
        M3.setAcceleration(a3);
        M4.setAcceleration(a4);

        // setting distance for each wheel
        M1.move(D1);
        M2.move(D2);
        M3.move(D3);
        M4.move(D4);

        // turning on the drivers
        // PB5 (gpio 13) is for !enable
        PORTB &= 0b11011111;
        isIdle = false;

        // confirming the command

        Serial.print(a1);
        Serial.print(" ");
        Serial.print(a2);
        Serial.print(" ");
        Serial.print(a3);
        Serial.print(" ");
        Serial.print(a4);
        Serial.println(" ");
      }
    }

    break;

    default:
      // statements
      break;
    }

    // // PB5 (gpio 13) is for !enable
    // PORTB &= 0b11011111;

    // // PB5 (input 13) is for !enable
    // PORTB |= 0b00100000;
  }

  if (M1.distanceToGo() == 0 && M2.distanceToGo() == 0 && M3.distanceToGo() == 0 && M4.distanceToGo() == 0)
  {

    if (!isIdle)
    {
      isIdle = true;
      idleTime = millis();
    }
    else
    {
      if (millis() - idleTime > idleTimeDelay)
      {
        // PB5 (input 13) is for !enable
        PORTB |= 0b00100000;
      }
    }
  }
  else
  {
    isIdle = false;
  }

  // kicking off the steppers pulses
  M1.run();
  M2.run();
  M3.run();
  M4.run();
}

// *****************
// *** Functions ***
// *****************

void recvWithStartEndMarkers()
{
  //  Read data in this style <C, 1, 2, 3, 4, 5>
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;

  while (Serial.available() > 0 && newData == false)
  {
    rc = Serial.read();

    if (recvInProgress == true)
    {
      if (rc != endMarker)
      {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars)
        {
          ndx = numChars - 1;
        }
      }
      else
      {
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }

    else if (rc == startMarker)
    {
      recvInProgress = true;
    }
  }
}

void parseData()
{ // split the data into its parts

  char *strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(tempChars, ","); // get the first part - the string
  command = atoi(strtokIndx);          // convert this part to an int

  strcpy(messageFromPC, strtokIndx); // copy it to messageFromPC

  strtokIndx = strtok(NULL, ",");
  param[0] = (int)atof(strtokIndx); // convert this part to a int

  strtokIndx = strtok(NULL, ",");
  param[1] = (int)atof(strtokIndx); // convert this part to a int

  strtokIndx = strtok(NULL, ",");
  param[2] = (int)atof(strtokIndx); // convert this part to a int

  strtokIndx = strtok(NULL, ",");
  param[3] = (int)atof(strtokIndx); // convert this part to a int

  strtokIndx = strtok(NULL, ",");
  param[4] = (int)atof(strtokIndx); // convert this part to a int
}

void goStop()
{
  // soft stop asap
  M1.setAcceleration(accl);
  M2.setAcceleration(accl);
  M3.setAcceleration(accl);
  M4.setAcceleration(accl);

  M1.stop();
  M2.stop();
  M3.stop();
  M4.stop();
}

float myDivide(float A, float B)
{
  if (B != 0)
  {
    return A / B;
  }
  else
  {
    return 1.0;
  }
}

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
bool fakeNewData = false;
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
  if (newData == true || fakeNewData == true)
  {

    strcpy(tempChars, receivedChars);

    if (newData == true) parseData();
    newData = false;
    fakeNewData = false;

    Serial.print("command: ");
    Serial.println(command);

    switch (command)
    {
    case 0:
      // stop action
      goStop();
      break;


    case 1:
    {
      // translating the Dz3 command to Mariola command
      // for control intercompatibility 
      // for command 1 the structure is:
      // <1,A,B,C> where:
      // A - the distance
      // B - turn angle
      // C - speed
      //
      // Mariola expects:
      // <2,a,b,c,d,v>
      // a,b,c,d - steps for each wheel
      // v - the speed
      //
      // The idea is to not recreate the entire case 2 statement 
      // functionality but to cheat a little by making fake serial command.
      //
      // Firstly - we need to do some math - to calculate the required steps 
      // for each wheel.
      // 
      // from the math done in python remote control file we know:
      // w4 = int((Dy + Dx - w * dims))
      // w3 = int((Dy - Dx - w * dims))
      // w2 = int((Dy - Dx + w * dims))
      // w1 = int((Dy + Dx + w * dims))
      // in case of this hack the Dx is 0
      // w4 = int((Dy - w * dims))
      // w3 = int((Dy - w * dims))
      // w2 = int((Dy + w * dims))
      // w1 = int((Dy + w * dims))
      // where 
      // dims = 4.5+9 = 13.5
      // w = radians(B) 
      long A = param[0];
      long B = param[1];
      long C = param[2];
      // below gets wonky as need to be in [cm]
      float w = (B * 71.0) / 4068;
      // const float dims = 0.5 * 180 / (180.0 * 65 * wheel_const);
      const float dims = 18.5;

      // faking the new transsmission
      param[3] = (int) A - w * dims;
      param[2] = (int) A - w * dims;
      param[1] = (int) A + w * dims;
      param[0] = (int) A + w * dims;

      command = 2;
      param[4] = C;
      fakeNewData = true;

    } break;

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

        // Serial.print(A);
        // Serial.print(" ");
        // Serial.print(B);
        // Serial.print(" ");
        // Serial.print(C);
        // Serial.print(" ");
        // Serial.print(D);
        // Serial.println(" ");
      }
    } break;

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

        // calculating the max speed values
        float Vmax1 = 1.0 * newSpeed * A;
        float Vmax2 = 1.0 * newSpeed * B;
        float Vmax3 = 1.0 * newSpeed * C;
        float Vmax4 = 1.0 * newSpeed * D;

        // setting speed with respect to the set distance
        M1.setMaxSpeed( Vmax1 );
        M2.setMaxSpeed( Vmax2 );
        M3.setMaxSpeed( Vmax3 );
        M4.setMaxSpeed( Vmax4 );

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

        float a1 = Vmax1 * Vmax1 / abs(D1);
        float a2 = Vmax2 * Vmax2 / abs(D2);
        float a3 = Vmax3 * Vmax3 / abs(D3);
        float a4 = Vmax4 * Vmax4 / abs(D4);

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
        Serial.print(D1);
        Serial.print(" ");
        Serial.print(a1);
        Serial.print(" ");
        Serial.print(Vmax1);
        Serial.print(" : ");
      
        Serial.print(D2);
        Serial.print(" ");
        Serial.print(a2);
        Serial.print(" ");
        Serial.print(Vmax2);
        Serial.print(" : ");

        Serial.print(D3);
        Serial.print(" ");
        Serial.print(a3);
        Serial.print(" ");
        Serial.print(Vmax3);
        Serial.print(" : ");

        Serial.print(D4);
        Serial.print(" ");
        Serial.print(a4);
        Serial.print(" ");
        Serial.print(Vmax4);
        Serial.println("");
      
      }
    } break;

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

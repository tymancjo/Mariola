#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>


// Setting up the CAN communication structure
struct can_frame canMsg;
MCP2515 mcp2515(10); // 10 is the pin for CS

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

// Functions declarations
void recvWithStartEndMarkers();
void recFromCan();
void sendToCan(int address);
void parseData();

void setup()
{
  // put your setup code here, to run once:
  // Preparing the CAN module 
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  // kicking off serial
  Serial.begin(9600);

  delay(1000);
}

void loop()
{
  // put your main code here, to run repeatedly:

  // temporary to disable speed change

  // grabstuff from serial
  recvWithStartEndMarkers();

  // if we have some new data...
  if (newData == true)
  {
    strcpy(tempChars, receivedChars);
    parseData();

    sendToCan(102);
    newData = false;
    Serial.println("Data sent to CAN");

  }
}

// *****************
// *** Functions ***
// *****************

// receiving from the CAN
void sendToCan(int address)
{
  int msgBytes = 7;
  canMsg.can_dlc = msgBytes;
  canMsg.data[0] = (uint8_t)command;

  if (command == 41){
    address = 202;
    canMsg.can_id = address;
    msgBytes = 2;
    canMsg.can_dlc = msgBytes;

    canMsg.data[0] = param[0];
    canMsg.data[1] = param[1];

  }

  else
  {
    canMsg.can_id = address;
    int p = 0;
    for (int i=1; i < msgBytes-1; i+=2)
    {
      // Union use for data conversion
      union tVal 
      {
        /* data */
        int t_int;
        byte t_byte[2];
      } t;

      t.t_int = param[p]; 
      canMsg.data[i] = t.t_byte[1];
      canMsg.data[i+1] = t.t_byte[0];
      Serial.println(canMsg.data[i]);
      Serial.println(canMsg.data[i+1]);
      p++;
    }
  }
  mcp2515.sendMessage(&canMsg);
}

// void sendToCan(int address)
// {
//   canMsg.can_id = address;
//   canMsg.can_dlc = 6;
//   canMsg.data[0] = command;
//   Serial.println(command);

//   for (int i=1; i < 6; i++)
//   {
//     canMsg.data[i] = param[i-1];
//     Serial.println(param[i-1]);
//   }
//   mcp2515.sendMessage(&canMsg);
// }


void recFromCan()
{
   if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
     // checking if the adress is the right one
     // 102 is set as Mariola drive address 
      int msgBytes = 7;
      if (canMsg.can_id == 102 && canMsg.can_dlc > 4){
       // the first data byt is command
       command = canMsg.data[0];

      for (int i=1; i < msgBytes; i+=2)
      {
        // Union use for data conversion
        union tVal 
        {
          /* data */
          int t_int;
          byte t_byte[2];
        } t;

        t.t_byte[1] = canMsg.data[i];
        t.t_byte[0] = canMsg.data[i+1];
        param[i-1] = t.t_int; 
      }
       newData = true;
     }
   }
}

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

  Serial.println(tempChars);

  strtokIndx = strtok(tempChars, ","); // get the first part - the string
  command = atoi(strtokIndx);          // convert this part to an int
  Serial.println(command);
  
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
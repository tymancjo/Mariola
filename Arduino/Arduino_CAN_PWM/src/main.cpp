// # CAN PWM module
#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>
#include <Adafruit_PWMServoDriver.h>



// Setting up the CAN communication structure
struct can_frame canMsg;
MCP2515 mcp2515(10); // 10 is the pin for CS

// kicking of the pwm module definition
// called this way, it uses the default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// defining the values used for PWM servo control
// those were selected after experiment with the real used hw servos
#define SERVOMIN 160  // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX 505  // This is the 'maximum' pulse length count (out of 4096)
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates

// for servos attached to the PWM module
// initial reset position
const int pos0 = 0;
// total servo canals count
const int servo_count = 16;

uint8_t servos_pos[servo_count] = {0};

float servos_curr[servo_count] = {0};

unsigned long now;
unsigned long last = 0;


void recFromCan();

void setup()
{
  // put your setup code here, to run once:
  // Preparing the CAN module 
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  // kicking off serial
  Serial.begin(9600);

  // Servos reset
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ); // Analog servos run at ~50 Hz updates
  delay(100);

  servos_pos[7] = 85;
  servos_pos[6] = 45;
  servos_pos[0] = 150;
  servos_pos[1] = 45;
  servos_pos[15] = 5;
  servos_pos[14] = 99;

  Serial.print("Resetting Servo: ");
  for (int i = 0; i < servo_count; i++)
  {
    servos_curr[i] = servos_pos[i];
    Serial.print(i);
    Serial.print(" ");
    Serial.println(servos_curr[i]);
    pwm.setPWM(i, 0, (int)map(servos_curr[i], 0, 180, SERVOMIN, SERVOMAX));
    delay(10);
  }


  delay(1000);
}

void loop() {
recFromCan();
now = millis();

// handling the servo slope moving
  if (now > last + 10){
    last = now;
    
    for (uint8_t s=0; s < servo_count; s++){
        if  (servos_curr[s] != servos_pos[s]){
            servos_curr[s] = (0.95 * servos_curr[s] + 0.05 * servos_pos[s]);
            pwm.setPWM(s, 0, (int)map(servos_curr[s], 0, 180, SERVOMIN, SERVOMAX));      
          }
    }
  }

}

// receiving from the CAN
void recFromCan()
{
   if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
     // checking if the adress is the right one
     // 102 is set as Mariola drive address 
     Serial.print(" can msg.");
      if (canMsg.can_id == 202 ){
        // the first data byt is command
        
        int msgBytesPairs = canMsg.can_dlc / 2;
        Serial.println(msgBytesPairs);

        for (int i=0; i <= msgBytesPairs; i++)
        {
          uint8_t servo = canMsg.data[i];
          i++;
          uint8_t pos = canMsg.data[i];
          Serial.println(servo);
          Serial.println(pos);


          servos_pos[servo] = pos;
        }
     }
   }
   else {
   }
}
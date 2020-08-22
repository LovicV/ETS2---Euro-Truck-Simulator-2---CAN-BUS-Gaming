//----------------------------------------------------------------

//CAN BUS gaming for Euro Truck Simulator
//(c) by Vlatko Lovic 2020


// Scroll down for MAIN CONFIGURATION

//----------------------------------------------------------------

//Libraries
#include <mcp_can.h>            //CAN Bus Shield Compatibility Library
#include <SPI.h>                //CAN Bus Shield SPI Pin Library
#include <LCD.h>
#include <LiquidCrystal.h>
#include <LiquidCrystal_I2C.h>
#include <mcp_can_dfs.h>

//Definition  
#define lo8(x) ((int)(x)&0xff)
#define hi8(x) ((int)(x)>>8)

//Variables
int incomingByte = 0;
byte speedL = 0;
byte speedH = 0;
byte WaterTempL = 0;    
byte WaterTempH = 0;
  
byte VoltageL = 0;
byte VoltageH = 0;
byte rpmL = 0;
byte rpmH = 0;
  
  
float speedo = 0;
float waterTemp = 0;
int gear = 0;
float BatVoltage = 0;
int rpm = 0;
int rpm1 = 0;
float FuelLevel = 0;
int serial_byte;


const int LEFT_INDICATOR  = A1;
const int RIGHT_INDICATOR = A2;
const int PARKING_BREAK   = A3;
const int LOW_BEAM        = 2;
const int HIGH_BEAM       = 4;

#define PACKET_SYNC 0xFF
#define PACKET_VER  2
  
#define SERVO_DIR_NORMAL false
#define SERVO_DIR_INVERT true

//Variables CAN BUS
//The cs pin of the version after v1.1 is default to D9
//v0.9b and v1.0 is default D10 
const int SPI_CS_PIN = 9;
MCP_CAN CAN(SPI_CS_PIN);  

//Variables LCD
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Addr, En, Rw, Rs, d4, d5, d6, d7, backlightpin, polarity

void setup(){
   //LCD
  lcd.begin(16,2);
  lcd.backlight();
  showLCD("By","Vlatko Lovic");
  delay(2000);
 
  if(Serial){
    showLCD("Connection","Established");
    }
    else{
      return;
    }

  //LEDs
  pinMode(LEFT_INDICATOR, OUTPUT);
  pinMode(RIGHT_INDICATOR, OUTPUT);
  pinMode(PARKING_BREAK, OUTPUT);
  pinMode(LOW_BEAM, OUTPUT);
  pinMode(HIGH_BEAM, OUTPUT);
  
  digitalWrite(LEFT_INDICATOR, 0);
  digitalWrite(RIGHT_INDICATOR, 0);
  digitalWrite(PARKING_BREAK, 0);
  digitalWrite(LOW_BEAM, 0);
  digitalWrite(HIGH_BEAM, 0);
  delay(500);

  digitalWrite(LEFT_INDICATOR, 1);
  digitalWrite(RIGHT_INDICATOR, 1);
  digitalWrite(PARKING_BREAK, 1);
  digitalWrite(LOW_BEAM, 1);
  digitalWrite(HIGH_BEAM, 1);
  delay(500);

  digitalWrite(LEFT_INDICATOR, 0);
  digitalWrite(RIGHT_INDICATOR, 0);
  digitalWrite(PARKING_BREAK, 0);
  digitalWrite(LOW_BEAM, 0);
  digitalWrite(HIGH_BEAM, 0);

  Serial.begin(115200);

  //Begin with CAN Bus Initialization
  START_INIT:
  
      if(CAN_OK == CAN.begin(CAN_1000KBPS)){            //baudrate = 1000k
          Serial.println("CAN BUS Shield init ok!");
      }
      else{
          Serial.println("CAN BUS Shield init fail");
          Serial.println("Init CAN BUS Shield again");
          delay(100);
          goto START_INIT;
      }
}

void skip_serial_byte(){
  (void)Serial.read();
}
// Reload LCD
void showLCD(String line1,String line2){  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

//Send CAN message
void CanSend(uint32_t address, byte a, byte b, byte c, byte d, byte e, byte f, byte g, byte h){
  uint8_t DataToSend[8] = {a,b,c,d,e,f,g,h};
  byte sndStat = CAN.sendMsgBuf(address, 0, 8, DataToSend);

   if(sndStat != CAN_OK){
      Serial.println("Error Sending Message...");
      }
      else{
        Serial.print("Succesfully Sent Message with ID:");
        Serial.println(address);
      }
}

void SetSpeedFromGame(float speed1){
  serial_byte = Serial.read();
  speed1 = (serial_byte * 10) + 14;
  speedL = lo8(speed1);
  speedH = hi8(speed1);

  CanSend(1254, 0x00, 0x00, speedL, speedH, 0x00, 0x00, 0x00, 0x00);
 
  }

  void SetRPMFromGame(float rpm1){
    serial_byte = Serial.read();
    rpm1 = serial_byte *(100/6);
    
    rpmL = lo8(rpm1);
    rpmH = hi8(rpm1);

    CanSend(1250, rpmL, rpmH, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
 
  }

  void PrintFuelFromGame(float Fuel){
    serial_byte = Serial.read();
    Fuel = serial_byte + 20;
    showLCD("Fuel level:",(String)Fuel);
    }

  void digitalWriteFromBit(int port, int value, int shift){
    digitalWrite(port, (value >> shift) & 0x01);
  }


void loop(){

     if (Serial.available() < 16)
     return;
  
    serial_byte = Serial.read();
    if (serial_byte != PACKET_SYNC)
     return;

    serial_byte = Serial.read();
    if (serial_byte != PACKET_VER){
       return;
    }

    
    SetSpeedFromGame(speedo);
    SetRPMFromGame(rpm);
    skip_serial_byte();
    skip_serial_byte();
    PrintFuelFromGame(FuelLevel);
    skip_serial_byte();
    skip_serial_byte();
    skip_serial_byte();
    skip_serial_byte();
    skip_serial_byte();


  // Truck lights byte
  serial_byte = Serial.read();
  digitalWriteFromBit(LEFT_INDICATOR,  serial_byte, 5);  
  digitalWriteFromBit(RIGHT_INDICATOR, serial_byte, 4);
  digitalWriteFromBit(LOW_BEAM,  serial_byte, 3);  
  digitalWriteFromBit(HIGH_BEAM, serial_byte, 2);


  serial_byte = Serial.read();  
  digitalWriteFromBit(PARKING_BREAK, serial_byte, 7);

  // Enabled flags
  serial_byte = Serial.read();
  
  int text_len = Serial.read();
  
  if (0 < text_len && text_len < 127){
    for (int i = 0; i < text_len; ++i){
      while (Serial.available() == 0){
        delay(2);
      }
      serial_byte = Serial.read();
        if (serial_byte == 'a'){
          gear = 11;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
          //showLCD("Brzina: ","1");
          }
        else  if (serial_byte == 'b'){
          gear = 12;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);    
          
        }
        else  if (serial_byte == 'c'){  
          gear = 13;  
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'd'){    
          gear = 14;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'e'){    
          //showLCD("Brzina: ","5");
          gear = 15;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'f'){ 
          gear = 16;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);   
          //showLCD("Brzina: ","6");
        }
        else  if (serial_byte == 'g'){
          gear = 17;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);    
          //showLCD("Brzina: ","7");
        }
        else  if (serial_byte == 'h'){    
          //showLCD("Brzina: ","8");
          gear = 18;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'i'){    
          //showLCD("Brzina: ","9");
          gear = 19;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'j'){    
          //showLCD("Brzina: ","10");
          gear = 20;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'k'){    
          //showLCD("Brzina: ","11");
          gear = 21;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'l'){    
          //showLCD("Brzina: ","12");
          gear = 22;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'm'){    
          //showLCD("Brzina: ","13");
          gear = 23;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'n'){    
          //showLCD("Brzina: ","14");
          gear = 24;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'o'){    
          //showLCD("Brzina: ","15");
          gear = 25;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'p'){    
          //showLCD("Brzina: ","16");
          gear = 26;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'q'){    
          //showLCD("Brzina: ","17");
          gear = 27;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'r'){    
          //showLCD("Brzina: ","18");
          gear = 28;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 's'){
          //showLCD("Brzina: ","R1");
          gear = 9;    
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 't'){    
          //showLCD("Brzina: ","R2");
          gear = 8;    
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'u'){    
          //showLCD("Brzina: ","R3");
          gear = 7;    
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'v'){    
          //showLCD("Brzina: ","R4");
          gear = 6;    
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }
        else  if (serial_byte == 'w'){    
          //showLCD("Brzina: ","Neutral");
          gear = 10;
          CanSend(1251,0x00, 0x00,0x00, 0x00, gear, 0x00, VoltageL, VoltageH);
        }


    }
        }
     delay(20);

}

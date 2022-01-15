#define DEBUG 0
#define UNO_PROTO
//#define MINI

// include the libraries:
#include <avr/pgmspace.h>

#include "PS2Keyboard.h"

#include <Wire.h>
#include "RTClib.h"

#include <SPI.h>
#include <SD.h>

// set up variables using the SD utility library functions:
Sd2Card   card;
SdVolume  volume;
File    root;

File      workFile;

PS2Keyboard keyboard;

RTC_DS1307 RTC;
// call back for file timestamps
void dateTime(uint16_t* date, uint16_t* time) {
 DateTime now = RTC.now();

 // return date using FAT_DATE macro to format fields
 *date = FAT_DATE(now.year(), now.month(), now.day());

 // return time using FAT_TIME macro to format fields
 *time = FAT_TIME(now.hour(), now.minute(), now.second());
}

#ifdef UNO_PROTO

// PS2 Keyboard Pin Setup
#define PS2_CLK   3
#define PS2_DATA  14

// SD Card Pin Setup
#define SD_CS 10
#define SD_CD 8
#define SD_WP 9

// Atari Joystick Pin Setup
#define ATARI_CLK 7         // ATARI ---> ARDUINO
#define ATARI_DA0 2         // ATARI <--> ARDUINO
#define ATARI_DA1 5         // ATARI <--> ARDUINO
#define ATARI_DA2 6         // ATARI <--> ARDUINO
#define ATARI_ACK 4         // ATARI <--- ARDUINO

#define VERSION_STRING  "JOY2SD v1.0 Arduino Shield"

#endif

#ifdef MINI
// PS2 Keyboard Pin Setup
#define PS2_CLK   3
#define PS2_DATA  14

// SD Card Pin Setup
#define SD_CS 10
#define SD_CD 8
#define SD_WP 9

// Atari Joystick Pin Setup
#define ATARI_CLK 2         // ATARI ---> ARDUINO
#define ATARI_DA0 5         // ATARI <--> ARDUINO
#define ATARI_DA1 6         // ATARI <--> ARDUINO
#define ATARI_DA2 7         // ATARI <--> ARDUINO
#define ATARI_ACK 4         // ATARI <--- ARDUINO

#define VERSION_STRING  "JOY2SD v1.0 Arduino Pro Mini"
#endif

#define MENU_SYNC       0
#define MENU_RTC        1
#define MENU_KEYBOARD   2
#define MENU_SERIAL     3
#define MENU_SDCARD     4

#define MENU_GOTOTOP    7

const char interfaceNAME[] PROGMEM  = {VERSION_STRING};  // 20 chars

bool IO_SYNCH = false;
byte IO_MENU = 0;
bool IO_ACK = 0;
bool IO_CLK = 0;
byte IO_FRAME = 0;
byte IO_DATA = 0;
byte IO_BUFFER[63];
byte IO_SIZE = 0;
byte IO_MODE = 0;
long IO_LONG = 0;

String IO_FILENAME;
String IO_PATH = "/";

byte IO_CMD = 0;

void setup() {
  Serial.begin(57600);
//  while (!Serial) {
//    ; // wait for serial port to connect. Needed for Leonardo only
//  }
  Serial.println(F("\nInit communication pins."));
  pinMode(ATARI_CLK, INPUT);
  IO_CLK = 1; //digitalRead(ATARI_CLK);
  pinMode(ATARI_DA0, INPUT);
  pinMode(ATARI_DA1, INPUT);
  pinMode(ATARI_DA2, INPUT);
  pinMode(ATARI_ACK, OUTPUT);
  IO_ACK = !IO_CLK;
  digitalWrite(ATARI_ACK, IO_ACK);

  Serial.print(F("CLK ("));
  Serial.print(IO_CLK); 
  Serial.print(F(") ACK (")); 
  Serial.print(IO_ACK); 
  Serial.println(F(")")); 


    Wire.begin();
    RTC.begin();
    Serial.println(F(__DATE__));
    Serial.println(F(__TIME__));
    
      if (! RTC.isrunning()) {
    Serial.println(F("RTC is not running!"));
    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // following line sets the RTC to the date & time this sketch was compiled
    //RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  // set date time callback function
 SdFile::dateTimeCallback(dateTime); 

  Serial.print(F("\nInit keyboard..."));
  keyboard.begin(PS2_DATA);
  // Open serial communications and wait for port to open:

  Serial.print(F("\nInit SD card..."));

  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, SD_CS)) {
    Serial.println(F("failed."));
    return;
  } else {
    Serial.println(F("OK"));
  }

  // print the type of card
  Serial.print(F("\nCard type: "));
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println(F("SD1"));
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println(F("SD2"));
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println(F("SDHC"));
      break;
    default:
      Serial.println(F("Unknown"));
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println(F("No FAT partition!"));
    return;
  }
  SD.begin(SD_CS);

//  root = SD.open(IO_PATH);

}

void toggleACK() {
  if (IO_ACK == 0) {
    IO_ACK = 1;
    digitalWrite(ATARI_ACK, HIGH);
  } else {
    IO_ACK = 0;
    digitalWrite(ATARI_ACK, LOW);
  }
}

void portMode(bool mode) {
  pinMode(ATARI_DA0, mode);
  pinMode(ATARI_DA1, mode);
  pinMode(ATARI_DA2, mode);

}


byte readFrame() {
  byte value;
    while (digitalRead(ATARI_CLK) == IO_CLK) {  
    }
    portMode(INPUT);
    IO_CLK = digitalRead(ATARI_CLK);
    value = (digitalRead(ATARI_DA2) * 4) + (digitalRead(ATARI_DA1) * 2) + (digitalRead(ATARI_DA0) * 1);
if (DEBUG){
  Serial.print(F("CLK ("));
  Serial.print(IO_CLK); 
  Serial.print(F(") ACK (")); 
  Serial.print(IO_ACK); 
  Serial.print(F(") ")); 
    Serial.print(F("RX frame 0x"));
    Serial.println(value, HEX);
}
//    delay(50);   
    return (value);
}

void sendFrame(int value) {
    portMode(OUTPUT);
    digitalWrite(ATARI_DA2, (value & 4)>>2);
    digitalWrite(ATARI_DA1, (value & 2)>>1);
    digitalWrite(ATARI_DA0, (value & 1));
    toggleACK(); 
//    delay(50);
if (DEBUG){
  Serial.print(F("CLK ("));
  Serial.print(IO_CLK); 
  Serial.print(F(") ACK (")); 
  Serial.print(IO_ACK); 
  Serial.print(F(") ")); 
    Serial.print(F("TX frame 0x"));
    Serial.println(value, HEX);   
}
}

byte readByte() {
  byte DATA;
    DATA = readFrame();
    sendFrame(0); 
    DATA = DATA + (readFrame() << 3) ;
    sendFrame(0);
    DATA = DATA + (readFrame() << 6);
    sendFrame(0);    
if (DEBUG){
    Serial.print(F("RX byte 0x"));
    Serial.println(DATA, HEX);
}
    return DATA;
}

void sendByte(byte value) {
if (DEBUG){
    Serial.print(F("TX byte 0x"));
    Serial.println(value, HEX);
}
    readFrame();
    sendFrame(value&7);
    readFrame(); 
    sendFrame((value & 56) >> 3);
    readFrame();
    sendFrame((value & 192) >> 6);    
  
}

byte hex2dec (byte value) {
  byte i;
  byte j;
  byte k;
    i = value;
    j = 0;
    for (k=value; k>=10; k=k-10) {
      i = i - 10;
      j = j + 16;
    }
    return i+j;
} 

byte convert2ascii (byte value) {
  if (value<0x40) return value+0x20;
  else if (value<0x60) return value-0x40;
  else return value;
}

void updateRTC_buffer() {
  byte i;

    DateTime now = RTC.now();
    IO_BUFFER[0] = hex2dec(now.second());
    IO_BUFFER[1] = hex2dec(now.minute());
    IO_BUFFER[2] = hex2dec(now.hour());
    IO_BUFFER[3] = now.dayOfTheWeek();
    IO_BUFFER[4] = hex2dec(now.day());
    IO_BUFFER[5] = hex2dec(now.month());
    IO_BUFFER[6] = hex2dec(now.year()-2000);
    IO_BUFFER[7] = RTC.readSqwPinMode();
    for (i=0; i < 55; ++i) {
      IO_BUFFER[i+8] = RTC.readnvram(i);
      }
}

void menu_level0() {
  
    switch(readFrame()) {
      case 0:             // synchronize device
        if (!IO_SYNCH) {  
  if (DEBUG) Serial.println(F("SYNC CMD"));
          IO_SYNCH = true;
          sendFrame(0);
        } else {
  if (DEBUG) Serial.println(F("SYNC CMD - GET STATUS"));
            int len = strlen_P(interfaceNAME);
            sendFrame(0);
            sendByte(len);
            for (int k = 0; k < len; k++) {
              IO_DATA =  pgm_read_byte_near(interfaceNAME + k);
              sendByte(IO_DATA);
            }
        }
        break;
      case 1:             // read RTC command         - %001
  if (DEBUG) Serial.println(F("RTC CMD"));
        if (IO_SYNCH) {
          sendFrame(0);
          IO_MENU = 1;
          updateRTC_buffer();
        } else {
          sendFrame(7);
          sendByte(0x80); // bit 7 device is not synchronized
        }
        break;
      case 2:             // read KEYBOARD command    - %010
  if (DEBUG) Serial.println(F("KEYBOARD CMD"));
        if (IO_SYNCH) {
          sendFrame(0);
          IO_MENU = 2;
        } 
        break;
      case 3:             // read SERIAL command         - %011
  if (DEBUG) Serial.println(F("SERIAL CMD"));
        if (IO_SYNCH) {
          sendFrame(0);
          IO_MENU = 3;
        } 

        break;
      case 4:             // read SD command            - &100 
  if (DEBUG) Serial.println(F("SDCARD CMD"));
        if (IO_SYNCH) {
          sendFrame(0);
          IO_MENU = 4;
        } 

        break;
      case 5:
  if (DEBUG) Serial.println(F("COMMUNICAION TEST CMD"));
        if (IO_SYNCH) {
          sendFrame(0);
          IO_MENU = 5;
        } 
      
        break;
      case 7:             // unsynchronize device     - %111
        if (IO_SYNCH) {
  if (DEBUG) Serial.println(F("UNSYNC CMD"));
          if (IO_MENU) IO_MENU=0;
          else IO_SYNCH = false;
          
          sendFrame(0);
        } else {
  if (DEBUG) Serial.println(F("UNSYNC CMD - RESTART DEVICE"));
//          sendFrame(0);
//          sendByte(0x00);
          softReset();      
        }
       break;   
      default:
        break;

    }      
}

void menu_rtc() {
  byte i;
  byte j;
    switch (readFrame()) {
      case 0:                         // read byte from rtc
  if (DEBUG) Serial.println(F("RTC READ BYTE"));
        sendFrame(0);
        IO_DATA = readByte();
        sendByte(IO_BUFFER[IO_DATA]);
       break;
      case 1:                         // read next byte from rtc
  if (DEBUG) Serial.println(F("RTC READ NEXT BYTE"));

       break;
      case 2:
        sendFrame(0);
        break;
      case 7:                         // back to top level
  if (DEBUG) Serial.println(F("BACK TO TOP LEVEL"));
        if (IO_SYNCH) {
          IO_MENU=0;       
          sendFrame(0);
        } else {
          sendFrame(0);
          sendByte(0x80);
      
        }

       break;
      default:
        break;

    }
}

void menu_keyboard() {
  byte temp_b = 0;
    switch (readFrame()) {
      case 0:
        if (DEBUG) Serial.println(F("KEYBOARD SCAN"));
        sendFrame(0);
        if (keyboard.available()) {
          sendByte(keyboard.read_extra());
          sendByte(keyboard.read());
        } else {
          sendByte(keyboard.read_extra());
          sendByte(0);
        }
        break;
      case 1:
        if (DEBUG) Serial.println(F("KEYBOARD SEND AT COMMAND"));
        sendFrame(0);
//        keyboard.kbd_set_lights(readByte());
        break;
      default:
        sendFrame(0);
        break;        
    }
        IO_MENU = 0;  // return to menu top level
}

void menu_serial() {
  
}

void sendDirRecord(){
 workFile = root.openNextFile();
  if (!workFile) {
    sendFrame(1);
    return;
  } else {
    sendFrame(0);
    String helper = workFile.name();
    for ( int count = 0; count < helper.length(); ++count ) {
      sendByte(helper.charAt(count)); 
    }
    if (helper.length() < 12){
      for ( int count = helper.length(); count < 12; ++count ) sendByte(0);
    }
    if (workFile.isDirectory()) {
      sendByte(1);
      sendByte(0);
      sendByte(0);
      sendByte(0);
    } else {
      sendByte(0);
      sendByte((workFile.size()) & 0xff);
      sendByte((workFile.size() >> 8) & 0xff);
      sendByte((workFile.size() >> 16) & 0xff);
    }
    
    workFile.close();
  }
}

void menu_sdcard() {
        long temp = 0;
        char path4SD[IO_PATH.length()];
    switch (readByte()) {
      case 0x00:
        if (DEBUG) Serial.println(F("SDCARD GET STATUS CMD"));
        
        sendByte(0);
        break;
      case 0x01:
        if (DEBUG) Serial.println(F("SDCARD GET DIRECTORY RECORD CMD"));
        switch (readFrame()){
         case 0:
         if (DEBUG) Serial.println(F("MODE: first record"));
            if (root) root.close();
            IO_PATH.toCharArray(path4SD, IO_PATH.length()+1);
            root = SD.open(path4SD);
            root.rewindDirectory();
            sendDirRecord();
            break;          
          case 1:
         if (DEBUG) Serial.println(F("MODE: next record"));          
            sendDirRecord();
            break;
          case 2:
         if (DEBUG) Serial.println(F("MODE: skip records"));          
            sendFrame(0);
            if (root) root.close();
//            char path4SD[IO_PATH.length()];
            IO_PATH.toCharArray(path4SD, IO_PATH.length()+1);
            root = SD.open(path4SD);
            root.rewindDirectory();
            byte helper = readByte();
            for (int count=0; count < helper; count++) {
              workFile = root.openNextFile();
              workFile.close();
            }
            break;
          
        }
        break;  
      case 0x02:
        if (DEBUG) Serial.println(F("SDCARD CHANGE DIRECTORY CMD"));
        IO_FILENAME = String();
        for (byte count=0; count<12; ++count) {
          IO_FILENAME = IO_FILENAME + char(convert2ascii(readByte()));
        }
          IO_FILENAME.trim();
          IO_PATH = IO_PATH + IO_FILENAME + "/";
          if (DEBUG) Serial.println(IO_PATH);  
        readFrame();
        sendFrame(0);
        break;  
      case 0x03:
        if (DEBUG) Serial.println(F("SDCARD CHANGE DIR TO PARRENT CMD"));
        if (IO_PATH.lastIndexOf('/')) {
          String helper = IO_PATH.substring(0,IO_PATH.lastIndexOf('/') - 1);
        if (DEBUG) Serial.println(helper);
          IO_PATH = IO_PATH.substring(0,helper.lastIndexOf('/')) + "/";
        }
        if (DEBUG) Serial.println(IO_PATH);
        readFrame();
        sendFrame(0);
        break;  
      case 0x04:
        if (DEBUG) Serial.println(F("SDCARD CHANGE DIR TO ROOT CMD"));
        sendByte(0);
        IO_PATH = "/";
        readFrame();
        sendFrame(0);
        break;  
      case 0x05:
        if (DEBUG) Serial.println(F("SDCARD GET PATH CMD"));
        sendByte(IO_PATH.length() & 0xff );
        for ( int count = 0; count < IO_PATH.length(); ++count ) {
          sendByte(IO_PATH.charAt(count)); 
        }
        break;  
      case 0x06:
        if (DEBUG) Serial.println(F("SDCARD COUNT DIR ITEMS CMD"));
        IO_LONG = 0;
        if (root) root.close();
        char path4SD[IO_PATH.length()];
        IO_PATH.toCharArray(path4SD, IO_PATH.length()+1);
        root = SD.open(path4SD);
        root.rewindDirectory();
        while (true){
          workFile = root.openNextFile();
          if (! workFile) break;
          workFile.close();
          IO_LONG++;
        }
        if (DEBUG) {
          Serial.print(IO_LONG,DEC);
          Serial.print(F(" items in directory "));
          Serial.println(IO_PATH);
        }
        sendByte(IO_LONG & 0xff);
//        sendByte((IO_LONG >> 8) & 0xff);
//        root.close()
        break;  
      case 0x07:
        if (DEBUG) Serial.println(F("BACK TO TOP MENU CMD"));
        root.close();
        IO_MENU = 0;
        break;  
      case 0x08:
        if (DEBUG) Serial.println(F("SDCARD CREATE OBJECT CMD"));
      
        break;  
      case 0x09:
        if (DEBUG) Serial.println(F("SDCARD REMOVE OBJECT CMD"));
      
        break;  

      case 0x010:
        if (DEBUG) Serial.println(F("OPEN FILE CMD"));
        IO_FILENAME = String();
        for (byte count=0; count<12; ++count) {
          IO_FILENAME = IO_FILENAME + char(convert2ascii(readByte()));
        }
          IO_FILENAME.trim();
          IO_FILENAME = IO_PATH + IO_FILENAME;
          
          char fileName4SD[IO_FILENAME.length()];
          
          IO_FILENAME.toCharArray(fileName4SD, IO_FILENAME.length()+1);
          if (DEBUG){ Serial.print(F("Recived filename: "));
          Serial.println(IO_FILENAME);
          //Serial.println(F("|"));
          }
        IO_MODE = readByte(); // 1= read, 2= write
          if (DEBUG){ Serial.print(F("Recived file mode: "));
          Serial.println(IO_MODE, DEC);
          }
        if (SD.exists(fileName4SD)) Serial.println(F("Opening the file."));
          
        workFile = SD.open(fileName4SD,IO_MODE);
        if (DEBUG){
          Serial.print(F("Size of opened file: "));
        
        Serial.print(workFile.size(),DEC);
        Serial.println(F(" bytes"));
        }
        sendByte(0);
        break;  
      case 0x011:
        if (DEBUG) Serial.println(F("GET FILE OFFSET CMD"));
        IO_LONG = workFile.position();
        sendByte(IO_LONG & 0xff);
        sendByte((IO_LONG >> 8) & 0xff);
        sendByte((IO_LONG >> 16) & 0xff);        
        break;  
      case 0x012:
        if (DEBUG) Serial.println(F("SET FILE OFFSET CMD"));
        IO_LONG = readByte();
        temp = readByte();
        IO_LONG = IO_LONG + (temp << 8);
        temp = readByte();
        IO_LONG = IO_LONG + (temp << 16);
        workFile.seek(IO_LONG);
        break;  
      case 0x013:
        if (DEBUG){
          Serial.println(F("READ FILE CMD"));
        }
        IO_LONG = readByte();
        temp = readByte();
        IO_LONG = IO_LONG+(temp << 8);
        temp = readByte();
        IO_LONG = IO_LONG+(temp << 16);

        if (DEBUG){
          Serial.print(F("Recived block size: 0x"));
        Serial.println(IO_LONG,HEX);
        }
        for (long count=0; count<IO_LONG; ++count) {
          sendByte(workFile.read());
//          Serial.print(count,HEX);
//          Serial.print(" ");
          
        }
Serial.println(F("End of block")); 
        break;  
      case 0x014:
        if (DEBUG) Serial.println(F("WRITE FILE CMD"));

        break;  
      case 0x015:
        if (DEBUG) Serial.println(F("GET FILE SIZE CMD"));
        IO_LONG = workFile.size();
        sendByte(IO_LONG & 0xff);
        sendByte((IO_LONG >> 8) & 0xff);
        sendByte((IO_LONG >> 16) & 0xff);
        break;  

      case 0x01f:
        if (DEBUG) Serial.println(F("CLOSE FILE CMD"));
        workFile.close();
        IO_MENU = 0;
        break;

      default:
        break;

    }
  
}

void menu_tests() {
    switch (readFrame()) {
      case 0:                         // BYTE ECHO TEST
        sendFrame(0);
        IO_CMD = readByte();
        sendByte(IO_CMD);
       break;
      case 1:                         // SERIAL ECHO TEST

       break;

      case 7:                         // back to top level
  if (DEBUG) Serial.println(F("BACK TO TOP LEVEL"));
        if (IO_SYNCH) {
          IO_MENU=0;       
          sendFrame(0);
        } 

        break;
      default:
        break;

    }
  
}

void loop() {
//  while(true) {
//    toggleACK();
//    delay(2000);
//    Serial.println("Blink");
//  }
  switch (IO_MENU) {
        case 0:
          menu_level0();
          break;
        case 1:
          menu_rtc();
          break;
        case 2:
          menu_keyboard();
        case 3:
          menu_serial();
          break;
        case 4:
          menu_sdcard();
          break;
        case 5:
          menu_tests();
          break;
        default:
          break;

      }
}

void softReset(){
asm volatile ("  jmp 0");
}

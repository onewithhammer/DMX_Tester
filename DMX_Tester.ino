
// DMX Tester
// Author: Tony Keith
// Description: DMX Tester
//
// Supports the following input protocol:
// XXX@III#
// XXX-XXX@III#
// *@*#
// XXX@*#
// XXX-*@III#
// XXX-*@*#
//
// XXX = 1 to 512 Channel
// III = 1 to 256 Intensity
// * = Wildcard value: 512 for channel and 256 for Full intensity
//
// A = @ (at sign)
// D = - (dash)
// C = Clear
// # = Execute
// B = Bump (no supported)

// Comment out for Serial but not DMX
//#define SERIAL_DEBUG_ENABLED

#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Wire.h>

// Comment out for Serial - include for DMX
#include <Conceptinetics.h>

// States
#define INITIAL       1
#define START1        2
#define START2        3
#define START3        4
#define END1          5
#define END2          6
#define END3          7
#define ATSIGN        8
#define WILDCARD1     9
#define WILDCARD2     10
#define INTENSITY1    12
#define INTENSITY2    13
#define INTENSITY3    14
#define POUNDSIGN     15
#define EXECUTE       16
#define CLEAR         17
#define BUMP          18

const byte ROWS = 4; // define four rows
const byte COLS = 4; // define four
char keys [ROWS] [COLS] = {
{'1', '2', '3','@'},
{'4', '5', '6','B'},
{'7', '8', '9','C'},
{'*', '0', '#','-'}
};

// 16 switches on PC board keypad
// Pin  R/C Port
// 8    C4  13
// 7    C3  12
// 6    C2  11
// 5    C1  10
// 4    R1  6
// 3    R2  7
// 2    R3  8
// 1    R4  9

// 4x4 membrane keypad
// Pin  R/C Port
// 8    C4  13
// 7    C3  12
// 6    C2  11
// 5    C1  10
// 4    R4  9
// 3    R3  8
// 2    R2  7
// 1    R1  6

// Connect 4 * 4 keypad row-bit port, the corresponding digital IO ports panel
byte rowPins [ROWS] = {6,7,8,9};

// Connect 4 * 4 buttons faithfully port, the corresponding digital IO ports panel
byte colPins [COLS] = {10,11,12,13};

// Call the function library function Keypad
Keypad keypad = Keypad (makeKeymap (keys), rowPins, colPins, ROWS, COLS);

LiquidCrystal_I2C lcd(0x3F,20,4);

#define MAXCMDBUFFER  10

static char buffer[MAXCMDBUFFER];
static int index=0;
static int pos = 0;

static int startChannel = 0;
static int endChannel = 0;
static int intensity = -1;

static int state=INITIAL;

#define VERSION              "V0.30"

#define MIN_START_CHANNEL    1
#define MAX_END_CHANNEL      512
#define MIN_INTENSITY        0
#define MAX_INTENSITY        255

#ifndef SERIAL_DEBUG_ENABLED

#define DMX_MASTER_CHANNELS  512

// Pin number to change read or write mode on the shield
#define RXEN_PIN                2

// Configure a DMX master controller, the master controller
// will use the RXEN_PIN to control its write operation 
// on the bus
DMX_Master        dmx_master ( DMX_MASTER_CHANNELS, RXEN_PIN );
#endif

void setup () {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("DMX Tester ");
  lcd.print(VERSION);
  lcd.setCursor(0,1);
  lcd.print("Enter Cmd:");
// Serial or DMX but not both
#ifdef SERIAL_DEBUG_ENABLED
  Serial.begin (9600);
#else
  dmx_master.enable ();
#endif

}

void loop () {
  int tmpState;
  
  char key = keypad.getKey ();
  if (key != NO_KEY) {
    
    // Clear
    if(key == 'C') {
      state = CLEAR;
    }
    
    // Bump - Not Implemented
    if(key == 'B') {
      tmpState = state;
      state = BUMP;
    }
    
    displayState(state);
    switch(state) {
      case INITIAL:
        // X
        if(validKeys0to9(key)) {
          pos = displayKey(key, pos);
          storeKey(key);
          state = START1;
        } else if (validWildCard(key)) {
          // *@
          pos = displayKey(key, pos);
          state = WILDCARD1;          
        } else {
          invalidFormat();
          clearAll();
        }
      break;
      case START1:
        // XX
        if(validKeys0to9(key)) {
          pos = displayKey(key, pos);
          storeKey(key);
          state =  START2;
        } else if (validDash(key)) {
          // X-
          state = END1;
          pos = displayKey(key, pos);
          startChannel = getInt();
          if(! validateChannel(startChannel)) {
            invalidStartChannel();
            clearAll();
          } else {
            clearBuffer();
          }
        } else if(validAtSign(key)) {
          // X@
          state = INTENSITY1;
          pos = displayKey(key, pos);
          startChannel = getInt();
         if(! validateChannel(startChannel)) {
            invalidStartChannel();
            clearAll();
          } else {
            endChannel = startChannel;
            clearBuffer();
          }         
        } else {
          invalidFormat();
          clearAll();
        }
      break;
      case START2:
        // XXX
        if(validKeys0to9(key)) {
          pos = displayKey(key, pos);
          storeKey(key);
          state =  START3;
        } else if (validDash(key)) {
          // XX-
          state = END1;
          pos = displayKey(key, pos);
          startChannel = getInt();
          if(! validateChannel(startChannel)) {
            invalidStartChannel();
            clearAll();
          } else {
            clearBuffer();
          }
        } else if(validAtSign(key)) {
          // XX@
          state = INTENSITY1;
          pos = displayKey(key, pos);
          startChannel = getInt();
          endChannel = startChannel; 
          clearBuffer();         
        } else {
          invalidFormat();
          clearAll();
        }
      break;
      case START3:
        if (validDash(key)) {
          // XXX-
          state = END1;
          pos = displayKey(key, pos);
          startChannel = getInt();
          if(! validateChannel(startChannel)) {
            invalidStartChannel();
            clearAll();
          } else {
            clearBuffer(); 
          }
        } else if(validAtSign(key)) {
          // XXX@
          state = INTENSITY1;
          pos = displayKey(key, pos);
          startChannel = getInt();
          if(! validateChannel(startChannel)) {
            invalidStartChannel();
            clearAll();
          } else {
            endChannel = startChannel;
            clearBuffer(); 
          }          
        } else {
          invalidFormat();
          clearAll();
        }
      break;
      case END1:
        // X-X, XX-X, XXX-X
        if(validKeys0to9(key)) {
          pos = displayKey(key, pos);
          storeKey(key);
          state =  END2;
        } else if (validWildCard(key)) {
          // X-*, XX-*, XXX-*
          state = WILDCARD2;
          pos = displayKey(key, pos);
          endChannel = MAX_END_CHANNEL;        
        } else {
          invalidFormat();
          clearAll();
        }
      break;
      case END2:
        // X-XX, XX-XX, XXX-XX
        if(validKeys0to9(key)) {
          pos = displayKey(key, pos);
          storeKey(key);
          state =  END3;
        } else if (validAtSign(key)) {
          // X-X@, XX-X@, XXX-X@
          state = INTENSITY1;
          pos = displayKey(key, pos);
          // endChannel = X
          endChannel = getInt();
          // validate endChannel
          if(! validateChannel(endChannel) || ! validateChannelRange(startChannel, endChannel)) {
            invalidEndChannel();
            clearAll();
          } else {
            clearBuffer(); 
          }  
        } else {
          invalidFormat();
          clearAll();
        }
      break;
      case END3:
        // X-XXX, XX-XXX, XXX-XXX
        if(validKeys0to9(key)) {
          pos = displayKey(key, pos);
          storeKey(key);
          state =  ATSIGN;
        } else if (validAtSign(key)) {
          // X-XX@, XX-XX@, XXX-XX@
          state = INTENSITY1;
          pos = displayKey(key, pos);
          // endChannel = XX
          endChannel = getInt();
          // validate End Channel      
         if(! validateChannel(endChannel) || ! validateChannelRange(startChannel, endChannel)) {
            invalidEndChannel();
            clearAll();
          } else {
            clearBuffer(); 
          }       
        } else {
          invalidFormat();
          clearAll();
        }
      break;
      case ATSIGN:
        // X-XXX@, XX-XXX@, XXX-XXX@
        if(validAtSign(key)) {
          state = INTENSITY1;
          pos = displayKey(key, pos);
          // endChannel = XX
          endChannel = getInt();
          // validate End Channel
         if(! validateChannel(endChannel)) {
            invalidEndChannel();
            clearAll();
          } else {
            clearBuffer(); 
          }
        } else {
          invalidFormat();
          clearAll();
        }
      break;
      case WILDCARD1:
        // *@
        if(validAtSign(key)) {
          state = INTENSITY1;
          pos = displayKey(key, pos);
          startChannel = MIN_START_CHANNEL;
          endChannel = MAX_END_CHANNEL;
        } else {
          invalidFormat();
          clearAll();
        }
        
      break;
      case WILDCARD2:
        // X-*@, XX-*@, XXX-*@
        if(validAtSign(key)) {
          state = INTENSITY1;
          pos = displayKey(key, pos);
          endChannel = MAX_END_CHANNEL;
        } else {
          invalidFormat();
          clearAll();
        }        
      break;
      case INTENSITY1:
        // @X
        if(validKeys0to9(key)) {
          pos = displayKey(key, pos);
          storeKey(key);
          state =  INTENSITY2;
        } else if (validWildCard(key)) {
          // X-XX@*, XX-XX@*, XXX-XX@*
          state = POUNDSIGN;
          pos = displayKey(key, pos);
          // intensity = MAX
          intensity = MAX_INTENSITY;
          // validate Intensity
         if(! validateIntensity(intensity)) {
            invalidIntensity();
            clearAll();
          } else {
            clearBuffer(); 
          }    
        } else {
          invalidFormat();
          clearAll();
        }
      break;
      case INTENSITY2:
        // @XX
        if(validKeys0to9(key)) {
          pos = displayKey(key, pos);
          storeKey(key);
          state =  INTENSITY3;
        } else if (validPoundSign(key)) {
          // X-XX@XI#, XX-XX@I#, XXX-XX@I#
          state = EXECUTE;
          pos = displayKey(key, pos);
          // intensity = I
          intensity = getInt();
          // validate Intensity
         if(! validateIntensity(intensity)) {
            invalidIntensity();
            clearAll();
          } else {
            clearBuffer(); 
          }
        } else {
          invalidFormat();
          clearAll();
        }
      break;
      case INTENSITY3:
        // @XXX
        if(validKeys0to9(key)) {
          state =  POUNDSIGN;
          pos = displayKey(key, pos);
          storeKey(key);
          intensity = getInt();
          // validate Intensity
         if(! validateIntensity(intensity)) {
            invalidIntensity();
            clearAll();
          } else {
            clearBuffer(); 
          }
        } else if (validPoundSign(key)) {
          // @XXX#, @XXX#, @XXX#
          state = EXECUTE;
          pos = displayKey(key, pos);
          // intensity = XXX
          intensity = getInt();
          // validate Intensity
         if(! validateIntensity(intensity)) {
            invalidIntensity();
            clearAll();
          } else {
            clearBuffer(); 
          }     
        } else {
          invalidFormat();
          clearAll();
        }
      break;
      case POUNDSIGN:
        if (validPoundSign(key)) {
          state = EXECUTE;
        } else {
          invalidFormat();
          clearAll();
        }
      break;
      case CLEAR:
        clearAll();
      break;
      case BUMP:
        state = tmpState;
      break;
      default:
      break;      
    } // swtich
        
#ifdef SERIAL_DEBUG_ENABLED          
    Serial.print("Key:");
    Serial.print(key);
    Serial.print(" Idx:");
    Serial.print(index);
    Serial.print(" S:");
    Serial.print(startChannel);
    Serial.print(" E:");
    Serial.print(endChannel);
    Serial.print(" I:");
    Serial.println(intensity);
#endif
    // EXECUTE
    if(state == EXECUTE) {   
        sendDMX(startChannel, endChannel, intensity);
        clearAll();
    }
  } // if 
} // loop

// Send DMX
void sendDMX(int start, int end, unsigned char intensity) {
  lcd.setCursor(0,2);
  lcd.print("S:");
  lcd.print(start);
  lcd.print(" E:");
  lcd.print(end);
  lcd.print(" I:");
  lcd.print(intensity);
  delay(2000);
  
#ifdef SERIAL_DEBUG_ENABLED

#else
  // add DMX control
  if(start == end) {
    dmx_master.setChannelValue(start, intensity);    
  } else {
    dmx_master.setChannelRange(start, end, intensity );
  }
#endif
  return;
}
// Clears resets everything
void clearAll(void) {
  buffer[0] = '\0';
  index=0;
  pos=0;
  startChannel = 0;
  endChannel = 0;
  intensity = -1;
  state = INITIAL;
  clearDisplay();
}
// Display key on Cmd line
int displayKey(char key, int pos) {
  lcd.setCursor(pos,2);
  lcd.print(key);
  pos++;
  return(pos);
}
// Store key in buffer
void storeKey(char key) {
  buffer[index] = key;
  index++;
}
// Clear buffer
void clearBuffer(void) {
  buffer[0] = '\0';
  index=0;
}
// Convert buffer into Integer
int getInt() {
   buffer[index] = '\0';
   return(atoi(buffer));
}

// Display state name
void displayState(int state) {
#ifdef SERIAL_DEBUG_ENABLED 
    char stateName[15];
    
    switch(state) {
      case INITIAL:
        strcpy(stateName, "INITIAL");
      break;
      case START1:
        strcpy(stateName, "START1");
      break;
      case START2:
       strcpy(stateName, "START2");
      break;
      case START3:
       strcpy(stateName, "START3");
      break;
      case END1:
       strcpy(stateName, "END1");
      break;
      case END2:
       strcpy(stateName, "END2");
      break;
      case END3:
       strcpy(stateName, "END3");
      break;
      case ATSIGN:
       strcpy(stateName, "ATSIGN");
      break;
      case WILDCARD1:
       strcpy(stateName, "WILDCARD1");
      break;
      case WILDCARD2:
       strcpy(stateName, "WILDCARD2");
      break;
      case INTENSITY1:
       strcpy(stateName, "INTENSITY1");
      break;
      case INTENSITY2:
       strcpy(stateName, "INTENSITY2");
      break;
      case INTENSITY3:
       strcpy(stateName, "INTENSITY3");
      break;
      case POUNDSIGN:
        strcpy(stateName, "POUNDSIGN");
      break;
      case EXECUTE:
       strcpy(stateName, "EXECUTE");
      break;
      case CLEAR:
       strcpy(stateName, "CLEAR");
      break;
      case BUMP:
       strcpy(stateName, "BUMP");
      break;
      default:
        strcpy(stateName, "UNKNOWN");
      break;
  } // switch
  Serial.print("State:");
  Serial.println(stateName);
#endif
}

// Validate channel
// return true if valid, else false
int validateChannel(int channel) {
  int valid=1;
  if(channel < MIN_START_CHANNEL || channel > MAX_END_CHANNEL) {
    valid = 0;
  }
  return(valid);
}

// Validate channel range
// retun true if valid, else false
int validateChannelRange(int startCh, int endCh) {
  int valid=1;
  if(startCh < MIN_START_CHANNEL || startCh > MAX_END_CHANNEL) {
    valid = 0;
  } else if(endCh <= MIN_START_CHANNEL || endCh > MAX_END_CHANNEL) {
    valid = 0;
  } else if(startCh > endCh) {
    valid = 0;
  }
  return(valid);
}
// Validate intensity
// return true if valid, else false
int validateIntensity(int intensity) {
  int valid=1;
  if(intensity < MIN_INTENSITY || intensity > MAX_INTENSITY) {
    valid = 0;
  }
  return(valid);
}
// Clear the Cmd and execute area of the display
void clearDisplay(void) {
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,3);
  lcd.print("                    ");
}
// Display invalid intensity error
void invalidIntensity(void) {
  lcd.setCursor(0,2);
  lcd.print("Invalid Intensity:");
  lcd.setCursor(0,3);
  lcd.print("0-255 or *");
  delay(2000);
  clearDisplay();
}
// Display invalid start channel error
void invalidStartChannel(void) {
  lcd.setCursor(0,2);
  lcd.print("Invalid Start Chan:");
  lcd.setCursor(0,3);
  lcd.print("1-512");
  delay(2000);
  clearDisplay();
}
// Display invalid end channel error
void invalidEndChannel(void) {
  lcd.setCursor(0,2);
  lcd.print("Invalid End Chan:");
  lcd.setCursor(0,3);
  lcd.print("1-512 or *");
  delay(2000);
  clearDisplay();
}
// Display format error
void invalidFormat(void) {
  lcd.setCursor(0,2);
  lcd.print("Invalid Format:");
  lcd.setCursor(0,3);
  lcd.print("CCC@PPP");
  delay(1000); 
  lcd.setCursor(0,3);
  lcd.print("CCC-CCC@PPP");
  delay(1000); 
  lcd.setCursor(0,3);
  lcd.print("use * for CCC or PPP");
  delay(1000);
  clearDisplay();
}
// Validate keys: * (asterisk)
// return true if valid, else false
int validWildCard(int key) {
  int valid=0;
  switch(key) {
    case '*':
      valid=1;
    break;
  }
  return valid;
}
// Validate keys: @ (at sign)
// return true if valid, else false
int validAtSign(int key) {
  int valid=0;
  switch(key) {
    case '@':
      valid=1;
    break;
  }
  return valid;
}
// Validate keys: - (dash)
// return true if valid, else false
int validDash(int key) {
  int valid=0;
  switch(key) {
    case '-':
      valid=1;
    break;
  }
  return valid;
}
// Validate keys: # (pound sign)
// return true if valid, else false
int validPoundSign(int key) {
  int valid=0;
  switch(key) {
    case '#':
      valid=1;
    break;
  }
  return valid;
}
// Validate keys: 0-9
// return true if valid, else false
int validKeys0to9(int key) {
  int valid=0;
  switch(key) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      valid=1;
    break;
  }
  return valid;
}

// End of File

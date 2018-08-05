#include <Time.h>	//https://github.com/PaulStoffregen/Time
#include <TimeLib.h>	//https://github.com/PaulStoffregen/Time

#include <Wire.h>
#include "DFRobot_LCD.h"
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>

//#if ARDUINO >= 100
// Wiring Instructions

// Connect the GPS Power pin to 5V
// Connect the GPS Ground pin to ground
// If using software serial (sketch example default):
//   Connect the GPS TX (transmit) pin to Digital 3
//   Connect the GPS RX (receive) pin to Digital 2

// Global vars
SoftwareSerial mySerial(3, 2);
Adafruit_GPS GPS(&mySerial);  //TX,RX
#define GPSECHO  false
DFRobot_LCD lcd(16,2);  //16 characters and 2 lines of show
static float TIMEZONES[] = { 5.5, 10 }; //IST, SYDNEY
static float TIMEZONE = TIMEZONES[1];  // SYDNEY
static char MONTH_SHORT[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static int DAYLIGHT=0;
// this keeps track of whether we're using the interrupt
// off by default!
boolean usingInterrupt = true;
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy
uint32_t timer = millis();
uint32_t GPSSYNC_TIME;

char DATETIME_DISPLAY[16];// = "DDMMMYYYY HH:MM";
char TMP_DISPLAY[16];
boolean isTimeChanged=false;

void updateTime() {
  if(gpsSync()) {
    //nothing to do
  }
  
  if (millis() - GPSSYNC_TIME >= 5000) {
    GPSSYNC_TIME = millis();
    lcd.setCursor(0, 1);
    lcd.print("                ");//Clear second line.
  }
  if (millis() - timer >= 1000) {
    sprintf(TMP_DISPLAY,"%02d%s%d %02d:%02d ",day(),MONTH_SHORT[(month()-1)],year(),hour(),minute());
  }
  
  isTimeChanged=(strcmp(TMP_DISPLAY, DATETIME_DISPLAY) != 0);
  if(isTimeChanged) {
    //uint32_t rawtime = TIME;
    sprintf(DATETIME_DISPLAY,"%02d%s%d %02d:%02d ",day(),MONTH_SHORT[(month()-1)],year(),hour(),minute());
    isTimeChanged = false;
  }
  return;
}

void displayDate() {
  lcd.setCursor(0, 0);
  lcd.print(DATETIME_DISPLAY);
}

boolean gpsSync() {
  if (GPS.newNMEAreceived()) {
    if (GPS.parse(GPS.lastNMEA())) {   // this also sets the newNMEAreceived() flag to false
      //lcd.clear();
      setTime(GPS.hour, GPS.minute, GPS.seconds, GPS.day, GPS.month, GPS.year);
      adjustTime((TIMEZONE+DAYLIGHT) * SECS_PER_HOUR);
      lcd.setCursor(0, 1);
      lcd.print("GPS Synced");
      GPSSYNC_TIME = millis();
      isTimeChanged = true;
      return true;
    }
  }
  return false;
}

void setup() {
  // 9600 NMEA is the default baud rate for MTK - some use 4800
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  delay(1000);
  GPS.sendCommand(PMTK_SET_BAUD_4800);
  delay(1000);
  Serial.begin(4800);

  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_ALLDATA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  GPS.sendCommand(PMTK_API_SET_FIX_CTL_1HZ);

  GPS.sendCommand(PGCMD_ANTENNA);
  useInterrupt(true);
  
  delay(1000);
  lcd.init();
}

// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if (GPSECHO)
    if (c) UDR0 = c;  
    // writing direct to UDR0 is much much faster than Serial.print 
    // but only one character can be written at a time. 
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

void loop() {
  updateTime();
  displayDate();
}

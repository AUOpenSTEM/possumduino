/*
   PossumDuino - wearable datalogging

   by Arjen Lentz & Jonathan Oxer
   - 2014-08-30 first full prototype assembly
   - 2014-10-01 live prototype tested

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version. http://www.gnu.org/licenses/

*/


// ----------------------------------------
/*
  Arduino pins and interrupts

  - PulseSensor (measuring human heartbeat)
    BPM  - pin A0
    Timer2 interrupt every 2ms

  - Freetronics 3-axis accelerometer
    X    - pin A1
    Y    - pin A2
    Z    - pin A3

  - Freetronics Real Time Clock + temperature sensor via I2C
    SDA	 - pin A4
    SCL  - pin A5

  - SD card on SPI bus:
    MOSI - pin D11
    MISO - pin D12
    CLK  - pin D13
    CS   - pin D4

  - Ethernet and other shields
    CS   - pin D10  just always output and set high to keep it happy

*/
#include <Arduino.h>



// ----------------------------------------
// PulseSensor
#include "PulseSensor.h"

void sendDataToProcessing(char symbol, int data)
{
  Serial.print(symbol);                // symbol prefix tells Processing what type of data is coming
  Serial.println(data);                // the data to send culminating in a carriage return
}



// ----------------------------------------
// 3-axis accelerometer
#define xAxis  A1
#define yAxis  A2
#define zAxis  A3


// ----------------------------------------
// Freetronics RTC module (DS3232)
#include <Wire.h>

#define DS3232_I2C_ADDRESS 0x68

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return ( (val / 16 * 10) + (val % 16) );
}


char *Dec2s(byte value)
{
  static char s[3] = "\0";

  s[0] = (char)('0' + (value / 10));
  s[1] = (char)('0' + (value % 10));
  s[2] = '\0';

  return (s);
}



// ----------------------------------------
// SD card slot
#include <SPI.h>
#include <SD.h>

#define SPI_SD_CS_PIN 			 4
#define SPI_ETHER_CS_PIN		10

File dataFile;



// ----------------------------------------
// Main stuff

void setup() {
  Serial.begin(9600);

  Serial.println();
  Serial.println("PossumDuino");

  // PulseSensor
  PulseSensorInterruptSetup();                 // sets up to read Pulse Sensor signal every 2mS
  // UN-COMMENT THE NEXT LINE IF YOU ARE POWERING The Pulse Sensor AT LOW VOLTAGE,
  // AND APPLY THAT VOLTAGE TO THE A-REF PIN
  //analogReference(EXTERNAL);

  // SPI for RTC/temp
  Wire.begin();


  // SD card slot
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(SPI_ETHER_CS_PIN, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(SPI_SD_CS_PIN)) {
    Serial.println("SD card failed, or not present");
  }

  delay(5000);  // sleep for a bit to get everything settled
}



void loop()
{
  // ----------------------------------------
  // RTC

  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0); // set DS3232 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3232_I2C_ADDRESS, 7);

  String date_str = "20", time_str;

  {
    byte second, minute, hour, dayofweek, day, month, year;

    second     = bcdToDec(Wire.read() & 0x7f);
    minute     = bcdToDec(Wire.read());
    hour       = bcdToDec(Wire.read() & 0x3f);
    dayofweek  = bcdToDec(Wire.read());
    day        = bcdToDec(Wire.read());
    month      = bcdToDec(Wire.read());
    year       = bcdToDec(Wire.read());

    date_str += Dec2s(year);
    date_str += '-';
    date_str += Dec2s(month);
    date_str += '-';
    date_str += Dec2s(day);

    time_str = Dec2s(hour);
    time_str += ':';
    time_str += Dec2s(minute);
    time_str += ':';
    time_str += Dec2s(second);

    Serial.print("RTC  ");
    Serial.print(date_str);
    Serial.print(' ');
    Serial.println(time_str);
  }


  // PulseSensor
  int bpm;

  if (PulseSensorGetQS() == true) {        // Quantified Self flag is true when arduino finds a heartbeat
    bpm = PulseSensorGetBPM();
    // sanity for junk readings
    if (bpm > 20 && bpm < 180)
      sendDataToProcessing('B', bpm);
    else
      bpm = -1;  // bad signal
  }
  else
    bpm = -2;  // no signal


  // Accelerometer
  int XaxisValue, YaxisValue, ZaxisValue;

  XaxisValue = analogRead(xAxis);
  YaxisValue = analogRead(yAxis);
  ZaxisValue = analogRead(zAxis);
  Serial.print("X=");
  Serial.print(XaxisValue);
  Serial.print(",Y=");
  Serial.print(YaxisValue);
  Serial.print(",Z=");
  Serial.println(ZaxisValue);


  // SD
  dataFile = SD.open("DATALOG.CSV", FILE_WRITE);
  if (dataFile) {
    dataFile.print("PD1,\"");
    dataFile.print(date_str);
    dataFile.print(" ");
    dataFile.print(time_str);
    dataFile.print("\",");
    dataFile.print(bpm);
    dataFile.print(",");
    dataFile.print(XaxisValue);
    dataFile.print(",");
    dataFile.print(YaxisValue);
    dataFile.print(",");
    dataFile.print(ZaxisValue);
    dataFile.println();

    dataFile.close();
  }
  else
    Serial.println("! Can't write to SD file DATALOG.CSV");

  delay(5000);                             //  take a break

}



/* end of file */

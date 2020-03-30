#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

//Sensor Libraries
#include "Adafruit_SHT31.h"                         //https://github.com/adafruit/Adafruit_SHT31
#include "SparkFun_VEML6030_Ambient_Light_Sensor.h" //https://github.com/sparkfun/SparkFun_Ambient_Light_Sensor_Arduino_Library
//https://github.com/ControlEverythingCommunity/BH1715/blob/master/Arduino/BH1715.ino
#include "Adafruit_CCS811.h" //https://github.com/adafruit/Adafruit_CCS811
#include "Adafruit_BME280.h" //https://github.com/adafruit/Adafruit_BME280_Library

//I2C Addresses
#define addrSHT31D_L 0x44
#define addrSHT31D_H 0x45
#define addrBH1715_L 0x23
#define addrBH1715_H 0x5C
#define addrVEML6030 0x10
#define addrCCS811B 0x5A
#define addrBME280 0x76

//Define Sensors
Adafruit_SHT31 sht31_L = Adafruit_SHT31();
Adafruit_SHT31 sht31_H = Adafruit_SHT31();
SparkFun_Ambient_Light veml6030(addrVEML6030);
Adafruit_CCS811 ccs811B;
Adafruit_BME280 bme280;

//VEML6030 settings
float gain = .125;
int integTime = 100;

//BME280 settings
#define SEALEVELPRESSURE_HPA (1013.25)

//SD setting
int ID;
String dataMessage;

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("File written");
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message))
  {
    Serial.println("Message appended");
  }
  else
  {
    Serial.println("Append failed");
  }
  file.close();
}

void printValue(float value, const char *type, const char *unit)
{
  Serial.print(type);
  Serial.print(": ");
  !isnan(value) ? Serial.print(value)
                : Serial.println("ERROR");
  Serial.print(unit);
}

void beginBH1715(uint8_t addr)
{
  Wire.beginTransmission(addr);
  Wire.write(0x01);
  Wire.endTransmission();
  //Set to Continuous Mode
  Wire.beginTransmission(addr);
  Wire.write(0x10);
  Wire.endTransmission();
  delay(300);
}

float readBH1715(int addr)
{
  unsigned int data[2];
  Wire.requestFrom(addr, 2);
  if (Wire.available() == 2)
  {
    data[0] = Wire.read();
    data[1] = Wire.read();
  }
  delay(300);
  return ((data[0] * 256) + data[1]) / 1.2;
}

void setup()
{
  Wire.begin();
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  !SD.begin(13) ? Serial.println("Card Mount Failed")
                : Serial.println("Card Initialized");

  File file = SD.open("/data.txt");

  if (!file)
  {
    Serial.println("File doesn't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/data.txt", "ID, temp1, temp2, temp3, avgTemp, hum1, hum2, hum3, avgHum, pres, alt, lux1, lux2, lux3, avgLux, eCO2, tVOC \r\n");
  }
  else
  {
    Serial.println("File already exists");
  }
  file.close();

  Serial.println("---------------------SHT31 Test---------------------");
  !sht31_L.begin(addrSHT31D_L) ? Serial.println("ERROR while loading SHT31_L")
                               : Serial.println("SHT31_L is loaded");

  !sht31_H.begin(addrSHT31D_H) ? Serial.println("ERROR while loading SHT31_H")
                               : Serial.println("SHT31_H is loaded");
  Serial.println("---------------------VEML6030 Test---------------------");
  if (!veml6030.begin())
  {
    Serial.println("ERROR while loading VEML6030");
  }
  else
  {
    veml6030.setGain(gain);
    veml6030.setIntegTime(integTime);
    Serial.println("VEML6030 is loaded");
  };
  Serial.println("---------------------CCS811B Test---------------------");
  !ccs811B.begin(addrCCS811B) ? Serial.println("ERROR while loading CCS811B")
                              : Serial.println("CCS811B is loaded");
  Serial.println("---------------------BME280 Test---------------------");
  !bme280.begin() ? Serial.println("ERROR while loading BME280")
                  : Serial.println("BME280 is loaded");
  Serial.println("---------------------BH1715 Test---------------------");
  beginBH1715(addrBH1715_L);
  beginBH1715(addrBH1715_H);
}

void loop()
{
  float temp1 = sht31_L.readTemperature();
  float temp2 = sht31_H.readTemperature();
  float temp3 = bme280.readTemperature();
  float avgTemp = (temp1 + temp2 + temp3) / 3;
  float hum1 = sht31_L.readHumidity();
  float hum2 = sht31_H.readHumidity();
  float hum3 = bme280.readHumidity();
  float avgHum = (hum1 + hum2 + hum3) / 3;
  float pres = bme280.readPressure();
  float alt = bme280.readAltitude(SEALEVELPRESSURE_HPA);
  float lux1 = veml6030.readLight();
  float lux2 = readBH1715(addrBH1715_H);
  float lux3 = readBH1715(addrBH1715_L);
  float avgLux = (lux1 + lux2 + lux3) / 3;
  float eCO2;
  float tVOC;
  if (ccs811B.available())
  {
    if (!ccs811B.readData())
    {
      eCO2 = ccs811B.geteCO2();
      tVOC = ccs811B.getTVOC();
    } 
  } else {
      Serial.println("ERROR reading CCS881B");
      eCO2 = 0;
      tVOC = 0;
    }

  Serial.println("------------TEMP--------------");
  printValue(temp1, "Temperature (SHT31-L)", "C");
  printValue(temp2, "Temperature (SHT31-H)", "C");
  printValue(temp3, "Temperature (BME280)", "C");
  printValue(avgTemp, "Average Temperature", "C");
  Serial.println("------------HUM--------------");
  printValue(hum1, "Humidity (SHT31-L)", "%");
  printValue(hum2, "Humidity (SHT31-H)", "%");
  printValue(hum3, "Humidity (BME280)", "%");
  printValue(avgHum, "Average Humidity", "%");
  Serial.println("------------PRES-------------");
  printValue(pres, "Pressure (BME280)", "hPa");
  printValue(alt, "Altitude (BME280)", "m");
  Serial.println("-----------LIGHT-------------");
  printValue(lux1, "Luminance (CCS811B)", "lux");
  printValue(lux2, "Luminance (BH1715_H)", "lux");
  printValue(lux2, "Luminance (BH1715_L)", "lux");
  Serial.println("------------AIR--------------");
  printValue(eCO2, "CO2 Level (CCS811B)", "ppm");
  printValue(tVOC, "Volatile Organic Compounds Level (CCS811B)", "ppm");

  dataMessage = String(temp1) + ", " + String(temp2) + ", " + String(temp3) + ", " + String(avgTemp) + ", " + String(hum1) + ", " + String(hum2) + ", " + String(hum3) + ", " + String(avgHum) + ", " + String(pres) + ", " + String(alt) + ", " + String(lux1) + ", " + String(lux2) + ", " + String(lux3) + ", " + String(avgLux) + ", " + String(eCO2) + ", " + String(tVOC) + "\r\n";
  Serial.print("Save data: ");
  Serial.println(dataMessage);
  appendFile(SD, "/data.txt", dataMessage.c_str());

  ID++;

  delay(60000);
}
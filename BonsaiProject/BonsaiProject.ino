#include <EEPROM.h>
#include <Wire.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <LordLedPanel.h>
#include <Rainer.h>

// Pins Assignment
#define SOIL_PIN A2
#define LIGHT_PIN 10
#define FAN_PIN 11
#define RAIN_PIN 12
#define ONE_WIRE_BUS 13

// Time between measure
#define TEMP_SECOND 5
#define SOIL_SECOND 60

// LordLedPannel Settings
#define PWM_TIME    60          // Nombre de seconde pour passer le pwm de 0 a 255
#define MAX_TEMP    60          // temperature max
#define TEMP_MARGE  15          // marge temperature pour reallumer apres arret d'urgence
#define TIMEZONE    2           // UTC france = +2
const float LATITUDE =  43.70;  // Nice = latitude = 43.7000000
const float LONGITUDE = 7.25;   // Nice = longitude = 7.250000

// Rainer Settings
#define SOIL_MIN    60          // valeur min pour le declenchement de l'arrosage
#define RAIN_TIME   15          // duree du l'arrosage en sec
#define RAIN_BREAK  3600        // duree min entre 2 arrosages en sec

// Debug Settings
#define TEST_DEBUG 1                  // 1 = debug test actif, 0 = debug test inactif
const int lordOn = (16 * 60) + 17;   // Ex 13h40
const int lordOff = (16 * 60) + 20;  // Ex 13h43

// Clock
RTC_DS1307 RTC;       // Instance DS1307 RTC
DateTime now;         // Variable pour recuperer l'heure de l'horloge
int DST = 0;          // Heure été/hiver
void changeDST(void)  // Changement automatique de l'heure été / hiver  --- A refaire ---
{
  if (now.dayOfTheWeek() == 0 && now.month() == 3 && now.day() >= 25 && now.day() <= 31 && now.hour() == 2 && now.minute() == 0 && now.second() == 0 && DST == 0)
  {
    RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour() + 1, now.minute(), now.second()));
    DST = 1;
    EEPROM.put(0, DST);
    now = RTC.now(); //get time from RTC
  }
  else if (now.dayOfTheWeek() == 0 && now.month() == 10 && now.day() >= 25 && now.day() <= 31 && now.hour() == 3 && now.minute() == 0 && now.second() == 0 && DST == 1)
  {
    RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour() - 1, now.minute(), now.second()));
    DST = 0;
    EEPROM.put(0, DST);
    now = RTC.now(); //get time from RTC
  }
}

// DS18B20
#define TEMPERATURE_PRECISION 12
OneWire oneWire(ONE_WIRE_BUS);        // Instance OneWire
DallasTemperature sensors(&oneWire);  // Instance Dallas Temperature a partir de l'instance OneWire.
DeviceAddress insideThermometer = { 0x28, 0xEB, 0x75, 0x2D, 0x9, 0x0, 0x0, 0x13 };    // adresses des sondes DS18B20
DeviceAddress outsideThermometer = { 0x28, 0xFF, 0xC3, 0x20, 0xA8, 0x15, 0x1, 0xCA }; // adresses des sondes DS18B20
unsigned long last_temperature = 0;   // heure de la dernière mesure
float insideTemp = 0;                 // Variable pour recuperer la temperature
float outsideTemp = 0;                // Variable pour recuperer la temperature

// Soil Moisture
int soilMoisture = 0;         // Variable pour recuperer l'humidite du sol
unsigned long last_soil = 0;  // heure de la dernière mesure

// Objects
LordLedPanel light(LIGHT_PIN, FAN_PIN);
Rainer arrosage(RAIN_PIN);

// ### MAIN ###
void initObject()
{
  // Initialize DST
  EEPROM.get(0, DST);
  if (DST != 0 && DST != 1)
  {
    DST = 1;
    EEPROM.put(0, DST);
  }

  // Initialize light
  light.setValue(LORD_LED_PWM_TIME, PWM_TIME);
  light.setValue(LORD_LED_TEMP, MAX_TEMP);
  light.setValue(LORD_LED_MARGE, TEMP_MARGE);
  light.setValue(LORD_LED_TZ, TIMEZONE);
  light.setValue(LORD_LED_LAT, LATITUDE);
  light.setValue(LORD_LED_LON, LONGITUDE);
  light.setLord();
  light.enable();

  // Initialze rainer
  arrosage.setValue(RAINER_MIN,   SOIL_MIN);
  arrosage.setValue(RAINER_TIME,  RAIN_TIME);
  arrosage.setValue(RAINER_BREAK, RAIN_BREAK);
  arrosage.enable();
}
void measure()
{
  // get time
  now = RTC.now();
  changeDST();

  // DS18B20
  if (now.unixtime() >= (last_temperature + TEMP_SECOND))
  {
    sensors.requestTemperatures();
    insideTemp = sensors.getTempC(insideThermometer);
    outsideTemp = sensors.getTempC(outsideThermometer);
    last_temperature = now.unixtime();
  }

  // Soil Moisture
  if (now.unixtime() >= (last_soil + SOIL_SECOND))
  {
    soilMoisture = analogRead(SOIL_PIN);
    last_soil = now.unixtime();
  }
}
void runObject()
{
  light.run(now, soilMoisture);
  arrosage.run(now.unixtime(), soilMoisture);
}
void runDebug()
{
  if (TEST_DEBUG)
  {
    light.setValue(LORD_LED_ON, lordOn);
    light.setValue(LORD_LED_OFF, lordOff);
  }
}
int lastSec = 0;

void setup ()
{
  // affichage temporaire en Serial
  // sera remplacer par l'affichage lcd
  Serial.begin(9600);

  // Initialize RTC
  Wire.begin(); //start I2C interface
  RTC.begin(); //start RTC interface
  if (! RTC.isrunning()) RTC.adjust(DateTime(__DATE__, __TIME__));

  // Initialize DS18B20
  sensors.setResolution(insideThermometer, TEMPERATURE_PRECISION);
  sensors.setResolution(outsideThermometer, TEMPERATURE_PRECISION);

  // Initialize SOIL Moisture
  pinMode(SOIL_PIN, INPUT);

  initObject();
}
void loop ()
{
  measure();
  runObject();
  affichage();
  lastSec = now.second();
  runDebug();
}

// ### Affichage ###
void affichage(void)
{
  if (now.second() != lastSec)
  {
    Serial.println("####################");
    printTime();
    printMeasure();
    printLight();
    if (TEST_DEBUG) printDebug();
    printArrosage();
  }
}
void printTime()
{
  if (now.hour() < 10) Serial.print(0);
  Serial.print(now.hour());
  Serial.print("h");
  if (now.minute() < 10) Serial.print(0);
  Serial.print(now.minute());
  Serial.print(":");
  if (now.second() < 10) Serial.print(0);
  Serial.println(now.second());
}
void printMeasure()
{
  Serial.print("T1: ");
  Serial.print(insideTemp);
  Serial.print("°C - T2: ");
  Serial.print(outsideTemp);
  Serial.print("°C - Soil: ");
  Serial.println(soilMoisture);
}
void printLight()
{
  int lordOn = light.getValue(LORD_LED_ON);
  int minuteOn = lordOn % 60;
  int heureOn = (lordOn - minuteOn) / 60;
  int lordOff = light.getValue(LORD_LED_OFF);
  int minuteOff = lordOff % 60;
  int heureOff = (lordOff - minuteOff) / 60;
  Serial.print("-Lumiere- On: ");
  if (heureOn < 10) Serial.print(0);
  Serial.print(heureOn);
  Serial.print("h");
  if (minuteOn < 10) Serial.print(0);
  Serial.print(minuteOn);
  Serial.print(" - Off: ");
  if (heureOff < 10) Serial.print(0);
  Serial.print(heureOff);
  Serial.print("h");
  if (minuteOff < 10) Serial.print(0);
  Serial.print(minuteOff);
  Serial.print(" - Pwm: ");
  Serial.println(light.getPwm());
}
void printDebug(void)
{
  Serial.println("--------------------");
  unsigned long timeSec = (unsigned long)(now.hour()) * 3600 + (now.minute() * 60) + now.second();
  unsigned long startOn = (unsigned long)(light.getValue(LORD_LED_ON)) * 60;
  unsigned long endOn = startOn + light.getValue(LORD_LED_PWM_TIME);
  unsigned long startOff = (unsigned long)(light.getValue(LORD_LED_OFF)) * 60;
  unsigned long endOff = startOff + light.getValue(LORD_LED_PWM_TIME);
  Serial.print(timeSec);
  Serial.print(" - ");
  Serial.print(startOn);
  Serial.print(" - ");
  Serial.print(endOn);
  Serial.print(" - ");
  Serial.print(startOff);
  Serial.print(" - ");
  Serial.println(endOff);
  if (timeSec < startOn) Serial.println("before sunrise");
  else if ((timeSec >= startOn) && (timeSec <= endOn)) Serial.println("running sunrise");
  else if ((timeSec > endOn) && (timeSec < startOff)) Serial.println("between sunrise and sunset");
  else if ((timeSec >= startOff) && (timeSec <= endOff)) Serial.println("running sunset");
  else if (timeSec > endOff) Serial.println("after sunset");
  Serial.println("--------------------");
}
void printArrosage()
{
  Serial.print("-Arrosage: ");
  Serial.print(arrosage.isWorking()?"On":"Off");
  Serial.print(" - limite: ");
  Serial.println(arrosage.getValue(RAINER_MIN));
}

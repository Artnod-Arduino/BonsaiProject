#include <EEPROM.h>
#include <Wire.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <LordPwmTimer.h>

#define TIMEZONE 2            // UTC france = +2
const float LATITUDE = 43.70; // Nice = latitude = 43.7000000
const float LONGITUDE = 7.25; // Nice = longitude = 7.250000
#define PWM_TIME 900          // Nombre de seconde pour passer le pwm de 0 a 255
#define LIGHT_PIN 11          // Broche utilise pour la lumiere
#define ONE_WIRE_BUS 4        // Broche utilise pour les sondes DS18B20

// ### RTC ###
RTC_DS1307 RTC;               // Instance DS1307 RTC
DateTime now;                 // Variable pour recuperer l'heure de l'horloge
int DST = 0;                  // Heure été/hiver
void changeDST(void)          // Changement automatique de l'heure été / hiver
{
  if (now.dayOfTheWeek() == 0 && now.month() == 3 && now.day() >= 25 && now.day() <= 31 && now.hour() == 2 && now.minute() == 0 && now.second() == 0 && DST == 0)
  {
    RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour() + 1, now.minute(), now.second()));
    DST = 1;
    EEPROM.put(0, DST);
  }
  else if (now.dayOfTheWeek() == 0 && now.month() == 10 && now.day() >= 25 && now.day() <= 31 && now.hour() == 3 && now.minute() == 0 && now.second() == 0 && DST == 1)
  {
    RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour() - 1, now.minute(), now.second()));
    DST = 0;
    EEPROM.put(0, DST);
  }
}

// ### DS18B20 ###
#define TEMPERATURE_PRECISION 9
OneWire oneWire(ONE_WIRE_BUS);        // Instance OneWire 
DallasTemperature sensors(&oneWire);  // Instance Dallas Temperature a partir de l'instance OneWire.
DeviceAddress insideThermometer = { 0x28, 0xEB, 0x75, 0x2D, 0x9, 0x0, 0x0, 0x13 };    // adresses des sondes DS18B20
DeviceAddress outsideThermometer = { 0x28, 0xFF, 0xC3, 0x20, 0xA8, 0x15, 0x1, 0xCA }; // adresses des sondes DS18B20
float insideTemp = 0;                 // Variable pour recuperer la temperature
float outsideTemp = 0;                // Variable pour recuperer la temperature

// ### Light ###
LordPwmTimer light(LIGHT_PIN);

// ### MAIN ###
void setup ()
{
  // affichage temporaire en Serial
  // sera remplacer par l'affichage lcd
  Serial.begin(9600);

  // Initialize RTC
  Wire.begin(); //start I2C interface
  RTC.begin(); //start RTC interface
  if (! RTC.isrunning()) RTC.adjust(DateTime(__DATE__, __TIME__));
  EEPROM.get(0, DST);
  if (DST != 0 && DST != 1)
  {
    DST = 1;
    EEPROM.put(0, DST);
  }

  // Initialize DS18B20
  sensors.setResolution(insideThermometer, TEMPERATURE_PRECISION);
  sensors.setResolution(outsideThermometer, TEMPERATURE_PRECISION);
  
  // Initialize light
  light.setLord(TIMEZONE, LATITUDE, LONGITUDE);
  light.setValue(LORDTIMER_PWM_TIME, PWM_TIME);
  light.enable();
}

int lastSec = 0;
void loop ()
{
  // ***************** Recuperation de l'heure ********************************
  now = RTC.now(); //get time from RTC
  changeDST(); // changement automatique de l'heure été / hiver

  // ***************** Running every sec ********************************
  if (now.second() != lastSec)
  {
    sensors.requestTemperatures();
    insideTemp = sensors.getTempC(insideThermometer);
    outsideTemp = sensors.getTempC(outsideThermometer);
    light.run(now);
    Serial.println("####################");
    printTime();
    printTemp();
    printLight();
    lastSec = now.second();
  }
}

// ### Affichage ###
void printTime()
{
  Serial.print(now.hour());
  Serial.print("h");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.println(now.second());
}
void printTemp()
{
  Serial.print("Temp1: ");
  Serial.print(insideTemp);
  Serial.print("°C - Temp2: ");
  Serial.print(outsideTemp);
  Serial.println("°C");
  
}
void printLight()
{
  int lordOn = light.getValue(LORDTIMER_ON);
  int minuteOn = lordOn % 60;
  int heureOn = (lordOn - minuteOn) /60;
  int lordOff = light.getValue(LORDTIMER_OFF);
  int minuteOff = lordOff % 60;
  int heureOff = (lordOff - minuteOn) /60;
  Serial.print("pwm: ");
  Serial.print(light.getPwm());
  Serial.print(" - on: ");
  Serial.print(heureOn);
  Serial.print("h");
  Serial.print(minuteOn);
  Serial.print(" - off: ");
  Serial.print(heureOff);
  Serial.print("h");
  Serial.println(minuteOff);
}

#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <LordPwmTimer.h>

#define TIMEZONE 2            // UTC france = +2
const float LATITUDE = 43.70; // Nice = latitude = 43.7000000
const float LONGITUDE = 7.25; // Nice = longitude = 7.250000
#define LIGHT_PIN 11          // pwm pin id for light
#define PWM_TIME 900          // nb of sec for pwm (0 to 255)

// ### RTC ###
RTC_DS1307 RTC;               // Tells the RTC library that we're using a DS1307 RTC
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

// ### Light ###
LordPwmTimer light(LIGHT_PIN);

// ### MAIN ###
void setup ()
{
  // affichage temporaire en Serial
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
    light.run(now);
    Serial.println("####################");
    printTime();
    printLight();
    lastSec = now.second();
  }
}

// ### Affichage ###
void printTime()
{
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.print(now.second());
  Serial.print(" - Time: ");
  Serial.println((now.hour() * 60) + now.minute());
}
void printLight()
{
  Serial.print("pwm: ");
  Serial.println(light.getPwm());
  Serial.print(" - on: ");
  Serial.println(light.getValue(LORDTIMER_ON));
  Serial.print(" - off: ");
  Serial.println(light.getValue(LORDTIMER_OFF));
}

/* **************************** Garden Manager ****************************
   Criado por: Ronaldo Meneguite
   Rev.: 01
   Data: 12.11.2019

   Guia de conexão:
   LCD RS: pino 12
   LCD Enable: pino 11
   LCD D4: pino 5
   LCD D5: pino 4
   LCD D6: pino 3
   LCD D7: pino 2
   LCD R/W: GND
   LCD VSS: GND
   LCD VCC: VCC (5V)
   Potenciômetro de 10K terminal 1: GND
   Potenciômetro de 10K terminal 2: V0 do LCD (Contraste)
   Potenciômetro de 10K terminal 3: VCC (5V)
   Sensor de umidade do solo A0: Pino A0
   Módulo Relé (Válvula): Pino 10
 ***************************************************************************** */

// inclui a biblioteca:
#include <LiquidCrystal.h>
#include "RTClib.h"

RTC_DS1307 rtc;

// define os pinos de conexão entre o Arduino e o Display LCD
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Sensors
const int moistureSensor = A0;
const int rtcBatterySensor = A3;
const int rtcSDASensor = A4;
const int rtcSLCSensor = A5;

const int valveSensor = 10;

// Settings
const long wateringTime = 1800;
const int moistureLimit = 70;
const float batteryLevelMinLimit = 2.0;
const int wateringHours[] = { 10, 19 }; 
const boolean debug = false;
const boolean wateringOnFirstRecovery = false;


// Started values
boolean isWatering = false;
float batteryLevel = 0;
int currentMoisture = 0;
DateTime currentDateTime;
DateTime lastWatering = DateTime(2000, 1, 1, 0, 0, 0);


void setup() {
  while (!Serial); // for Leonardo/Micro/Zero

  // Start Valve
  pinMode(valveSensor, OUTPUT);
  digitalWrite(valveSensor, HIGH);

  Serial.begin(9600);
  startRtcModule();
}

void startRtcModule() {
  if (! rtc.begin()) {
    printLn("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning()) {
    printLn("RTC is NOT running!");
    rtc.adjust(DateTime(2019, 11, 20, 17, 35, 0));
  }
}

int updateMoisture() {
  int mapLow = 1018;
  int mapHigh = 365;
  int moistureValue = analogRead(moistureSensor);

  int moisture = map(moistureValue, mapLow, mapHigh, 0, 100);
  printLn("Moisture: " + String(moisture) + '%');
  return moisture;
}

float checkBatteryLevel() {
  int batteryLevel = analogRead(rtcBatterySensor);
  return (float(batteryLevel) / 1023 ) * 5;
}

String formatDateTime(DateTime &date) {
  char bf[] = "DD/MM hh:mm";
  return date.toString(bf);
}

void updateView () {
  lcd.begin(16, 2);
  String line1 = (batteryLevel < batteryLevelMinLimit ? "*Alerta -> " : formatDateTime(currentDateTime));
  lcd.print(line1);
  lcd.print(" " + String(batteryLevel, 1) + "v");
 

  lcd.setCursor(0, 1);
  lcd.print("Umidade: " +  String(currentMoisture) + " %");
}

boolean itsTimeForWatering() {
  printLn(formatDateTime(currentDateTime) + ": Check for its time for watering");
  int diference = currentDateTime.unixtime() - lastWatering.unixtime();

  if (wateringOnFirstRecovery && diference > 10000) {
      printLn("Watering first time");
      return true;
  }
  
  if (diference < 3600 ) {
    return false;
  }
  
  int currentHour = currentDateTime.hour();
  int arraySize = sizeof(wateringHours) / sizeof(wateringHours[0]);
  for (int i = 0; i < arraySize; i++) {
    if (String(wateringHours[i]) == String(currentHour)) {
      return true;
    }
  }
  
  return false;
}

boolean shouldWater() {
  if (isWatering) {
    printLn("Not should Watering.");
    return false;
  }
  
  if (currentMoisture > moistureLimit) {
    return false;
  }

  return itsTimeForWatering();
}

void startWatering() {
  isWatering = true;
  lcd.begin(16, 2);
  lcd.print("UR: " + formatDateTime(lastWatering));
  lcd.setCursor(0, 1);
  lcd.print("Aguando...        ");
  printLn(formatDateTime(currentDateTime) + ": Starting Watering..");
  digitalWrite(valveSensor, LOW);
  delay(wateringTime);
  digitalWrite(valveSensor, HIGH);
  lastWatering = rtc.now();
  isWatering = false;
  printLn(formatDateTime(lastWatering) + ": Finished Watering..");
}

void printLn(String message) {
  if (debug) {
    Serial.println(message);
  }
}


void loop() {
  currentDateTime = rtc.now();
  currentMoisture = updateMoisture();

  batteryLevel = checkBatteryLevel();
  updateView();
  
  if (shouldWater()) {
    startWatering();
  }

  updateView();
  delay(5000);
}

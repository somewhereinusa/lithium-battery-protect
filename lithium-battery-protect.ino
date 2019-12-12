/*
Program to turn relays on and off based on voltage and temperature
*/
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_RA8875.h"

// NOT GUARANTEED TO BE BUG FREE! I AM NOT A C PROGRAMMER!

/*
 * @kazetsukaimiko:
 * Maybe in C it is bad practice to name non-constants all uppercase.
 * The point here is that if you have business logic in code, your variables
 * should reflect that business logic if they are specific to your application.
 * If you wanted to write generic code usable in other scenarios, you'd avoid
 * this kind of specificity, but its not bad here. Helps reading the code.
 */

// Should be 15
static float BATTERY_HIGH_VOLTS = 2.3;
// Should be 11
static float BATTERY_LOW_VOLTS = 1.0;
// Should be 158
static int BATTERY_HIGH_TEMP = 80;
// Should be 34
static int BATTERY_LOW_TEMP = 65;
// Should be 34
static int HEATER_MAX_TEMP = 65;
// Should be 90
static int CABIN_HIGH_TEMP = 80;

static int MAIN_BATTERY_CUTOFF_PIN = 2;
static int CABIN_COOLING_FAN_PIN = 3;
static int BATTERY_HEATING_PAD_PIN = 4;
static int TBD_PIN = 5;

static int BATTERY_TEMP_INDEX = 0;
static int CABIN_TEMP_INDEX = 1;
static int HEATER_TEMP_INDEX = 2;
static int OTHER_TEMP_INDEX = 3;

float BATTERY_TEMP_VALUE = 0.0;
float CABIN_TEMP_VALUE = 0.0;
float HEATER_TEMP_VALUE = 0.0;
float OTHER_TEMP_VALUE = 0.0;

#define RA8875_INT 8
#define RA8875_CS 10
#define RA8875_RESET 9

/* @kazetsukaimiko: I like comments above code, not next to code.
 * ...even if they interrupt a block like this set of #defines.
 * Preference.
 */
// pin for DS18B20
#define ONE_WIRE_BUS 6
Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS, RA8875_RESET);
uint16_t tx, ty;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
//================================================================VOLTS 1

/*
 * Name the analogInput variable by its function. You're writing code to solve
 * your problems, not someone else's.
 */
int analogInput1 = A1;
float vout = 0.0;
float vin = 0.0;
/*
 * @kazetsukaimiko: Resistor values? I'd start these as lowercase or make them
 * explicitly static.
 */
float R1 = 35000.0;
float R2 = 7500.0;
int value = 0;
//================================================================VOLTS 2
int analogInput2 = A3;
float vout2 = 0.0;
float vin2 = 0.0;
float R12 = 35000.0;
float R22 = 7500.0;
int value2 = 0;

//=============================
void setup() {
  /*
   * @kazetsukaimiko: Would up this value a little bit. If you rely on serial
   * communication to give instructions or read values, Serial.setTimeout(50)
   * can reduce the response delay.
   */
  Serial.begin(9600);

  /*
   * @kazetsukaimiko: Your code has sensors.begin(); twice. You may only have to
   * do this once.
   */
  sensors.begin();

  setupTFT();
  sensors.begin();
  setupPins();
}

void setupTFT() {
  if (!tft.begin(RA8875_480x272)) {
    Serial.println("RA8875 Not Found!");
    while (1);
  }

  tft.displayOn(true);
  tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX
  tft.PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
  tft.PWM1out(255);
  tft.textTransparent(RA8875_WHITE);
  tft.textMode();
}

void setupPins() {
  pinMode(MAIN_BATTERY_CUTOFF_PIN, OUTPUT);      //Output Mode for Relay1
  pinMode(CABIN_COOLING_FAN_PIN, OUTPUT);      //Output Mode for Relay2
  pinMode(BATTERY_HEATING_PAD_PIN, OUTPUT);      //Output Mode for Relay3
  pinMode(TBD_PIN, OUTPUT);      //Output Mode for Relay4

  pinMode(analogInput1, INPUT);
  pinMode(analogInput2, INPUT);
}

void getTemperatures() {
  sensors.requestTemperatures();
  BATTERY_TEMP_VALUE = sensors.getTempFByIndex(BATTERY_TEMP_INDEX);
  CABIN_TEMP_VALUE = sensors.getTempFByIndex(CABIN_TEMP_INDEX);
  HEATER_TEMP_VALUE = sensors.getTempFByIndex(HEATER_TEMP_INDEX);
  OTHER_TEMP_VALUE = sensors.getTempFByIndex(3);
}

void getVoltages() {
  /*
        +++++++++++++++++++++++++++++++++++++++++++++++++++++
        +                    NOTE                           +
        +     Temp and voltage values used are for testing  +
        +      final values are commented.                  +
        +++++++++++++++++++++++++++++++++++++++++++++++++++++
    */
    //============================================================Volt sense 1
    // read the value at analog input
    value = analogRead(analogInput1);
    vout = (value * 5.0) / 1024.0;
    vin = vout / (R2 / (R1 + R2));

    //============================================================Volt sense 2
    value2 = analogRead(analogInput2);
    vout2 = (value2 * 5.0) / 1024.0;
    vin2 = vout2 / (R22 / (R12 + R22));
}

boolean cabinIsTooHot() {
  return (CABIN_TEMP_VALUE > CABIN_HIGH_TEMP);
}

boolean batteryIsTooHot() {
  return (BATTERY_TEMP_VALUE > BATTERY_HIGH_TEMP);
}

boolean batteryIsTooCold() {
  return (BATTERY_TEMP_VALUE < BATTERY_LOW_TEMP);
}

boolean batteryAtMaxVoltage() {
  return (vin > BATTERY_HIGH_VOLTS);
}

boolean batteryAtMinVoltage() {
  return (vin < BATTERY_LOW_VOLTS);
}

boolean batteryShouldBeCutOff() {
  return batteryIsTooHot() || batteryIsTooCold() || batteryAtMaxVoltage() || batteryAtMinVoltage();
}

boolean heaterAtDesiredTemperature() {
  return (HEATER_TEMP_VALUE >= HEATER_MAX_TEMP);
}


String floatToString(float input) {
   String valueString = "";
   char buff[10];
   dtostrf(input, 4, 6, buff);
   valueString += buff;
   return valueString;
}

void sendMetricsToSerial() {
  Serial.println("Battery temp " + floatToString(BATTERY_TEMP_VALUE) + "F   ");
  Serial.println("Cabin temp " + floatToString(CABIN_TEMP_VALUE) + "F   ");
  Serial.println("Heater temp " + floatToString(HEATER_TEMP_VALUE) + "F   ");
  Serial.println("Other temp " + floatToString(OTHER_TEMP_VALUE) + "F   ");

  Serial.println(cabinIsTooHot() ? "Cabin temp High" : "Cabin temp OK");

  Serial.println(
    !batteryShouldBeCutOff() ? "Battery ON." :
    "Battery OFF: " +
      batteryAtMaxVoltage()? "Volts High; " : batteryAtMinVoltage()? "Volts Low; " : "" +
      batteryIsTooHot()? "Battery Temp High" : batteryIsTooCold()? "Battery Temp Low" : ""
  );

  Serial.println(heaterAtDesiredTemperature() ? "Heater Off" : "Heater On");

  Serial.println((digitalRead(TBD_PIN) == HIGH) ? "Relay 4 OFF (unused)" : "Relay 4 ON (unused)");

  Serial.println("Battery Volts1 " + floatToString(vin));
  Serial.println("Battery Volts2 " + floatToString(vin2));
}

void sendMetricsToTFT() {
  tft.fillScreen(RA8875_BLACK);

  //Depending on the temperature switch============= Rel 1 Battery Temp high and low unhooks batteries from everything
  tft.textTransparent(RA8875_WHITE);
  tft.textSetCursor(10, 0);
  tft.textEnlarge(1);
  tft.print ("Battery Volts " + floatToString(vin));
  tft.textSetCursor(10, 30);
  tft.print ("Battery Temp  " + floatToString(BATTERY_TEMP_VALUE));
  tft.textSetCursor(10, 60);
  tft.print ("Room Temp     " + floatToString(CABIN_TEMP_VALUE));
  tft.textSetCursor(10, 90);
  tft.print ("Heater        " + floatToString(HEATER_TEMP_VALUE));

  //==============================================================TFT battery temperature
  tft.textSetCursor(350, 30);
  tft.textTransparent((batteryIsTooHot() || batteryIsTooCold())? RA8875_RED : RA8875_GREEN);
  tft.print(batteryIsTooHot() ? "  HIGH" : batteryIsTooCold() ? "  LOW" : "  OK");

  //==============================================================TFT Volts
  tft.textSetCursor(350, 0);
  tft.textTransparent((batteryAtMaxVoltage() || batteryAtMinVoltage()) ? RA8875_RED : RA8875_GREEN);
  tft.print(batteryAtMaxVoltage() ? "  HIGH" : batteryAtMinVoltage() ? "  LOW" : "  OK");

  tft.textSetCursor(350, 60);
  tft.textTransparent(cabinIsTooHot()? RA8875_RED : RA8875_GREEN);
  tft.print(cabinIsTooHot()? "Fan ON" : "Fan Off");

  tft.textSetCursor(350, 90);
  tft.textTransparent(heaterAtDesiredTemperature()? RA8875_GREEN: RA8875_RED);
  tft.print(heaterAtDesiredTemperature()? "Heat Off" : "Heat On");

  tft.textSetCursor(350, 120);
  tft.textTransparent(RA8875_RED);
  tft.print(vin2);
}

void printMetrics() {
  sendMetricsToSerial();
  sendMetricsToTFT();
}


void writePins() {
  digitalWrite(MAIN_BATTERY_CUTOFF_PIN, batteryShouldBeCutOff() ? LOW : HIGH);
  digitalWrite(CABIN_COOLING_FAN_PIN, cabinIsTooHot() ? LOW : HIGH);
  digitalWrite(BATTERY_HEATING_PAD_PIN, heaterAtDesiredTemperature() ? HIGH : LOW);
  digitalWrite(TBD_PIN, HIGH);
}

/*
 * @kazetsukaimiko: Your setup() and loop() methods, you should try to keep small.
 * Break anything longer than 20-30 lines into separate methods and delegate.
 * I didn't go as far as I could have, I did the bare minimum.
 */
void loop() {
  // Read stuff from pins, sensors, etc
  getTemperatures();
  getVoltages();


  // Print all metrics to the TFT and to Serial.
  printMetrics();

  // Set the pins.)
  writePins();

  delay(5000);
}
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
float BATTERY_HIGH_VOLTS = 2.3;
// Should be 11
float BATTERY_LOW_VOLTS = 1.0;
// Should be 158
int BATTERY_HIGH_TEMP = 80;
// Should be 34
int BATTERY_LOW_TEMP = 65;
// Should be 34
int HEATER_MAX_TEMP = 65;
// Should be 90
int CABIN_HIGH_TEMP = 80;

int MAIN_BATTERY_CUTOFF_PIN = 2;
int CABIN_COOLING_FAN_PIN = 3;
int BATTERY_HEATING_PAD_PIN = 4;
int TBD_PIN = 5;

int BATTERY_TEMP_INDEX = 0;
int CABIN_TEMP_INDEX = 1;
int HEATER_TEMP_INDEX = 2;
int OTHER_TEMP_INDEX = 3;

float BATTERY_TEMP_VALUE = 0.0;
float CABIN_TEMP_VALUE = 0.0;
float HEATER_TEMP_VALUE = 0.0;
float OTHER_TEMP_VALUE = 0.0;

// @kazetsukaimiko:
// Honestly I wouldn't do this. Since these pins should be set as output, you
// can read their state directly rather than having a variable mirror them.
// Will provide an example in another branch.
int MAIN_BATTERY_CUTOFF_PIN_VALUE;
int CABIN_COOLING_FAN_PIN_VALUE;
int BATTERY_HEATING_PAD_PIN_VALUE;
int TBD_PIN_VALUE;


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


/*
 * SETUP
 */

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

/*
 * RUNTIME
 */

void getTemperatures() {
  sensors.requestTemperatures();
  BATTERY_TEMP_VALUE = sensors.getTempFByIndex(BATTERY_TEMP_INDEX);
  CABIN_TEMP_VALUE = sensors.getTempFByIndex(CABIN_TEMP_INDEX);
  HEATER_TEMP_VALUE = sensors.getTempFByIndex(HEATER_TEMP_INDEX);
  OTHER_TEMP_VALUE = sensors.getTempFByIndex(OTHER_TEMP_INDEX);
}

void getVoltages() {
    value = analogRead(analogInput1);
    vout = (value * 5.0) / 1024.0;
    vin = vout / (R2 / (R1 + R2));

    value2 = analogRead(analogInput2);
    vout2 = (value2 * 5.0) / 1024.0;
    vin2 = vout2 / (R22 / (R12 + R22));
}


// @kazetsukaimiko: Possible to break this method into smaller ones!
void sendMetricsToSerial() {
  Serial.print ("Battery temp ");
  Serial.print(BATTERY_TEMP_VALUE);
  Serial.println ("F   ");

  Serial.print ("Cabin temp ");
  Serial.print(CABIN_TEMP_VALUE);
  Serial.println ("F   ");

  Serial.print ("Heater temp ");
  Serial.print(HEATER_TEMP_VALUE);
  Serial.println ("F   ");

  Serial.print ("Other temp ");
  Serial.print(OTHER_TEMP_VALUE);
  Serial.println ("F   ");
  Serial.println();

  if (CABIN_COOLING_FAN_PIN_VALUE == LOW) {
    Serial.println("Cabin temp High");
  } else if (CABIN_COOLING_FAN_PIN_VALUE == HIGH) {
    Serial.println ("Cabin temp OK");
  }

  if (MAIN_BATTERY_CUTOFF_PIN_VALUE == LOW) {
    Serial.println("Battery OFF: ");
    if (vin > BATTERY_HIGH_VOLTS) {
      Serial.println("Volts high.");
    } else if (vin < BATTERY_LOW_VOLTS) {
      Serial.println("Volts low.");
    }
    if (BATTERY_TEMP_VALUE > BATTERY_HIGH_TEMP) {
      Serial.println("Battery Temp High.");
    } else if (BATTERY_TEMP_VALUE < BATTERY_LOW_TEMP) {
      Serial.println("Battery Temp Low.");
    }
  } else {
    Serial.println("Battery ON.");
  }

  if (BATTERY_HEATING_PAD_PIN_VALUE == HIGH) {
    Serial.println("Heater Off");
  } else if (BATTERY_HEATING_PAD_PIN_VALUE == LOW) {
    Serial.println ("Heater On");
  }


  if (TBD_PIN_VALUE == HIGH) {
    Serial.println("Relay 4 OFF (unused)");
  } else {
    Serial.println("Relay 4 ON (unused)");
  }

  Serial.print("Battery Volts1 ");
  Serial.println(vin);

  Serial.print("Battery Volts2 ");
  Serial.println(vin2);

}

// @kazetsukaimiko: Possible to break this method into smaller ones!
void sendMetricsToTFT() {
  tft.fillScreen(RA8875_BLACK);

  //Depending on the temperature switch============= Rel 1 Battery Temp high and low unhooks batteries from everything
  tft.textTransparent(RA8875_WHITE);
  tft.textSetCursor(10, 0);
  tft.textEnlarge(1);
  tft.print ("Battery Volts ");
  tft.print(vin);
  tft.textSetCursor(10, 30);
  tft.print ("Battery Temp  ");
  tft.print(sensors.getTempFByIndex(0));
  tft.textSetCursor(10, 60);
  tft.print ("Room Temp     ");
  tft.print(sensors.getTempFByIndex(1));
  tft.textSetCursor(10, 90);
  tft.print ("Heater        ");
  tft.print(sensors.getTempFByIndex(2));

  //==============================================================TFT battery temperature
  tft.textSetCursor(350, 30);
  if (BATTERY_TEMP_VALUE > BATTERY_HIGH_TEMP) { //should be 158
   tft.textTransparent(RA8875_RED);
   tft.print("  HIGH");
 } else if (BATTERY_TEMP_VALUE > BATTERY_LOW_TEMP && BATTERY_TEMP_VALUE < BATTERY_HIGH_TEMP) { // should be 34 and 158
   tft.textTransparent(RA8875_GREEN);
   tft.print("  OK");
 } else if (BATTERY_TEMP_VALUE < BATTERY_LOW_TEMP) {//should be 34
   tft.textTransparent(RA8875_RED);
   tft.print("  LOW");
  }
  //==============================================================TFT Volts
  tft.textSetCursor(350, 0);
  if (vin > BATTERY_HIGH_VOLTS) {//should be 15
   tft.textTransparent(RA8875_RED);
   tft.print("  HIGH");
 } else if (vin > BATTERY_LOW_VOLTS && vin < BATTERY_HIGH_VOLTS) {// should be 11 and 15
   tft.textTransparent(RA8875_GREEN);
   tft.print("  OK");
 } else if (vin < BATTERY_LOW_VOLTS) { //should be 11
   tft.textTransparent(RA8875_RED);
   tft.print("  LOW");
  }

  tft.textSetCursor(350, 60);
  if (CABIN_COOLING_FAN_PIN_VALUE == LOW) {
   tft.textTransparent(RA8875_RED);
   tft.print("Fan ON");
  } else if (CABIN_COOLING_FAN_PIN_VALUE == HIGH) {
   tft.textTransparent(RA8875_GREEN);
   tft.print ("Fan Off");
  }

  tft.textSetCursor(350, 90);
  if (BATTERY_HEATING_PAD_PIN_VALUE == HIGH) {
    tft.textTransparent(RA8875_GREEN);
    tft.print("Heat Off");
  } else if (BATTERY_HEATING_PAD_PIN_VALUE == LOW) {
    tft.textTransparent(RA8875_RED);
    tft.print ("Heat On");
  }

  tft.textSetCursor(350, 120);
  tft.textTransparent(RA8875_RED);
  tft.print(vin2);
}

void printMetrics() {
  sendMetricsToSerial();
  sendMetricsToTFT();
}

/*
 * @kazetsukaimiko : Same logic bug. >=.
 */
void setupCabinCoolingFan() {
  if (CABIN_TEMP_VALUE > CABIN_HIGH_TEMP) {
    CABIN_COOLING_FAN_PIN_VALUE = LOW;
  } else {
    CABIN_COOLING_FAN_PIN_VALUE = HIGH;
  }
}

/*
 * @kazetsukaimiko: Previously there was a bug in this logic.
 * You were checking for x > y and x < z but not x == z or x == y.
 */
void setupBattery() {
  if ((BATTERY_TEMP_VALUE > BATTERY_HIGH_TEMP || vin > BATTERY_HIGH_VOLTS) ||
   (BATTERY_TEMP_VALUE < BATTERY_LOW_TEMP || vin < BATTERY_LOW_VOLTS)) {
    MAIN_BATTERY_CUTOFF_PIN_VALUE = LOW;
  } else {// should be 34 and 158 and 11 and 15
    MAIN_BATTERY_CUTOFF_PIN_VALUE = HIGH;
  }
}

/*
 * @kazetsukaimiko: Previously there was a bug in this logic.
 * You handled the greater than and less than cases, but what about equals?
 * Probably existed in other checks as well.
 * Never do:
 if (x > y) {
 } else if (x < y) {
 }

 * Always handle the equivalence by doing either:
  if (x >= y) {
  } else if (x < y) {
  }
 * Or:
  if (x > y) {
  } else if (x <= y) {
  }
 * Or if possible, use just else with no condition, then equals is handled by exclusion.
  if (x > y) {
  } else {
  }
 */
void setupHeater() {
  if (HEATER_TEMP_VALUE >= HEATER_MAX_TEMP) {
    BATTERY_HEATING_PAD_PIN_VALUE = HIGH;
  } else {
    BATTERY_HEATING_PAD_PIN_VALUE = LOW;
  }
}

void setupUnused() {
  TBD_PIN_VALUE = HIGH;
  Serial.println("Relay 4 unused");
}

void writePins() {
  digitalWrite(MAIN_BATTERY_CUTOFF_PIN, MAIN_BATTERY_CUTOFF_PIN_VALUE);
  digitalWrite(CABIN_COOLING_FAN_PIN, CABIN_COOLING_FAN_PIN_VALUE);
  digitalWrite(BATTERY_HEATING_PAD_PIN, BATTERY_HEATING_PAD_PIN_VALUE);
  digitalWrite(TBD_PIN, TBD_PIN_VALUE);
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

  // Setup the appliance relays
  setupCabinCoolingFan();
  setupBattery();
  setupHeater();
  setupUnused();

  // Print all metrics to the TFT and to Serial.
  printMetrics();

  // Set the pins.)
  writePins();

  delay(5000);
}

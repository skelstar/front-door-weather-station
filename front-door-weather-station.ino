#include <DHT.h>
#include <ESP8266WiFi.h>
extern "C" {
  #include "user_interface.h"
}
#include <MyBatteryMonitor.h>
#include <UbiDotsVariable.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <SparkFunTSL2561.h>

/////////////////////////////////////////////////////////////////////

#define   TIMEOUTMINUTE     60 * 1000
#define   TIMEOUTMINUTES    1 * TIMEOUTMINUTE
#define   SLEEPTIMEMINUTES  60 * 1000 * 1000    // usecs
#define   SLEEPMINUTES      30
#define   SLEEPTIME         SLEEPMINUTES * SLEEPTIMEMINUTES
#define   LOW_BATT_SLEEPTIME  2 * SLEEPMINUTES * SLEEPTIMEMINUTES

/////////////////////////////////////////////////////////////////////

// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library

#define   DS1820Pin   0    // what pin we're connected to
#define   DHTPIN      12

#define   SENSORS_POWER_PIN   14


//// sensors
OneWire  ds(DS1820Pin); 
DallasTemperature dallas(&ds);

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

MyBatteryMonitor batt(0, 776, 4.11);
#define   BATTERY_MAX  4.1
#define   BATTERY_MIN  3.9

// barometric pressure
Adafruit_BMP280 baro; // I2C

// light sensor
// For more information see the hookup guide at: https://learn.sparkfun.com/tutorials/getting-started-with-the-tsl2561-luminosity-sensor

SFE_TSL2561 light;

boolean gain;     // Gain setting, 0 = X1, 1 = X16;
unsigned int ms;  // Integration ("shutter") time in milliseconds

//// wifi
const char* ssid = "LeilaNet2";
const char* password = "ec1122%f*&";

WiFiClient client;

//////////////////////////////////////////////////////////////


#define VARIABLE_TEMP   0
#define VARIABLE_HUMID  1
#define VARIABLE_PRESS  2
#define VARIABLE_LIGHT  3
#define VARIABLE_BATT   4

bool sendVariable[5];

//// udiBots

#define VARIABLE_COUNT  5

UbiDotsVariable variable[VARIABLE_COUNT] = {
  UbiDotsVariable("56a3053876254213d7dcf918", "temperature", 0, 35),  //  VARIABLE_TEMP
  UbiDotsVariable("569134ec76254215afed7070", "humidity", 0, 100),  //  VARIABLE_HUMID
  UbiDotsVariable("5696c82e7625423fc86a4952", "pressure", 95000, 105000),  //  VARIABLE_PRESS
  UbiDotsVariable("5696d81e76254266cbddbd39", "light", 0, 1000),  //  VARIABLE_LIGHT
  UbiDotsVariable("569134fa76254215afed7093", "battery", 2.8, 4.3)   //  VARIABLE_BATT
};

String temperatureVariable = "569134db76254213753a108f";
String humidityVariable = "569134ec76254215afed7070";
String pressureVariable = "5696c82e7625423fc86a4952";
String lightVariable = "5696d81e76254266cbddbd39";
String batteryVariable = "569134fa76254215afed7093";

String token = "GioE85MkDexLpsbE1Tt37F3TY2p3Fa6XM4bKvftz9OfC87MrH5bQGceJrVgF";

const char* server = "things.ubidots.com";

void turnSensorsOn();
void turnSensorsOff();

// ========================================================================

void setup() {
  Serial.begin(115200);
  delay(10);

  turnSensorsOn();

  dallas.begin();

  dht.begin();

  if (!baro.begin()) {
     Serial.println("Could not find a valid BMP280 sensor, check wiring!");   
  }

  light.begin();
  gain = 1;   // default was 0 (low)
  unsigned char time = 2;
  light.setTiming(gain, time, ms);
  light.setPowerUp();

   
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFiServer server(80);
  
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  server.begin();
}

////////////////////////////////////////////////////////////////////////////////

void loop() {

  turnSensorsOn();

  // TEMPERATURE
  dallas.requestTemperatures(); // Send the command to get temperatures
  float temp = dallas.getTempCByIndex(0);  //DHT.temperature;
  variable[VARIABLE_TEMP].value = String(temp);
    
  // HMIDITY
  float humidity = dht.readHumidity();
  variable[VARIABLE_HUMID].value = String(humidity);

  // BATTERY
  batt.ReadBatteryVoltage();  
  float battery = batt.GetBatteryRemaining(); // analogRead(A0);
  variable[VARIABLE_BATT].value = String(battery);

  // BARO
  int pressure = baro.readPressure();
  variable[VARIABLE_PRESS].value = String(pressure/100);

  // LUX
  unsigned int data0, data1;
  double lux;    // Resulting lux value
  boolean lightgood = false;  // True if neither sensor is saturated 
  
  if (light.getData(data0,data1)) {
    lightgood = light.getLux(gain, ms, data0, data1, lux);
    variable[VARIABLE_LIGHT].value = String(lux);
  }

  turnSensorsOff();


  if (variable[VARIABLE_TEMP].ValueValid(temp))
      variable[VARIABLE_TEMP].SendVariableToUbidots(token);

  if (variable[VARIABLE_HUMID].ValueValid(humidity))
      variable[VARIABLE_HUMID].SendVariableToUbidots(token);

  if (variable[VARIABLE_BATT].ValueValid(battery))
      variable[VARIABLE_BATT].SendVariableToUbidots(token);

  if (variable[VARIABLE_PRESS].ValueValid(pressure))
      variable[VARIABLE_PRESS].SendVariableToUbidots(token);

  if (lightgood)
      variable[VARIABLE_LIGHT].SendVariableToUbidots(token);

  // -----------

  for (int i = 0; i < VARIABLE_COUNT; i++) {
      variable[i].SerialPrintValue();
  }
  Serial.println();

  Serial.print(".. sleeping for: ");

  if (battery >= 3.9) {
    Serial.print(SLEEPMINUTES); Serial.println(" mins");
    system_deep_sleep(SLEEPTIME); // usecs
  } else {
    Serial.print(LOW_BATT_SLEEPTIME); Serial.println(" mins");
    system_deep_sleep(LOW_BATT_SLEEPTIME); // usecs
  }
}

void turnSensorsOn() {
    pinMode(SENSORS_POWER_PIN, OUTPUT);
    digitalWrite(SENSORS_POWER_PIN, HIGH);
    delay(500);
}

void turnSensorsOff() {
    digitalWrite(SENSORS_POWER_PIN, LOW);
}




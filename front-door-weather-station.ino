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

/////////////////////////////////////////////////////////////////////

// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library

#define   DS1820Pin   0    // what pin we're connected to
#define   DHTPIN      2


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
  UbiDotsVariable("569134db76254213753a108f", "temperature", 0, 35),  //  VARIABLE_TEMP
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

// ========================================================================

// void ubiSendVariables(UbiDotsVariable var[VARIABLE_COUNT]);
// void ubiSaveValue(UbiDotsVariable var);
// void ubiSendAllVariables(UbiDotsVariable var[VARIABLE_COUNT]);


void setup() {
  Serial.begin(115200);
  delay(10);

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

  // TEMPERATURE
  dallas.requestTemperatures(); // Send the command to get temperatures
  float temp = dallas.getTempCByIndex(0);  //DHT.temperature;
  variable[VARIABLE_TEMP].value = String(temp);
  variable[VARIABLE_TEMP].send = variable[VARIABLE_TEMP].ValueValid(temp);
  variable[VARIABLE_TEMP].SendVariableToUbidots(token);
    
  // HMIDITY
  float humidity = dht.readHumidity();
  variable[VARIABLE_HUMID].value = String(humidity);
  variable[VARIABLE_HUMID].send = variable[VARIABLE_HUMID].ValueValid(humidity);
  variable[VARIABLE_HUMID].SendVariableToUbidots(token);

  // BATTERY
  batt.ReadBatteryVoltage();  
  float battery = batt.GetBatteryRemaining(); // analogRead(A0);
  variable[VARIABLE_BATT].value = String(battery);
  variable[VARIABLE_BATT].send = variable[VARIABLE_BATT].ValueValid(battery);
  variable[VARIABLE_BATT].SendVariableToUbidots(token);


  // BARO
  int pressure = baro.readPressure();
  variable[VARIABLE_PRESS].value = String(pressure/100);
  variable[VARIABLE_PRESS].send = variable[VARIABLE_PRESS].ValueValid(pressure);
  variable[VARIABLE_PRESS].SendVariableToUbidots(token);

  // LUX
  unsigned int data0, data1;
  double lux;    // Resulting lux value
  boolean lightgood;  // True if neither sensor is saturated 
  
  if (light.getData(data0,data1)) {
    lightgood = light.getLux(gain, ms, data0, data1, lux);
    variable[VARIABLE_LIGHT].value = String(lux);
    variable[VARIABLE_LIGHT].send = lightgood;
    variable[VARIABLE_LIGHT].SendVariableToUbidots(token);
  }

  // -----------

  for (int i = 0; i < VARIABLE_COUNT; i++) {
      variable[i].SerialPrintValue();
  }
  Serial.println();

  // ubiSendVariables(variable);
  // //ubiSendAllVariables(variable);

  Serial.print(".. sleeping for: ");
  Serial.print(SLEEPMINUTES);
  Serial.println(" mins");

  system_deep_sleep(SLEEPTIME); // usecs
}

// /// getting 404 (doesn't work)
// void ubiSendAllVariables(UbiDotsVariable var[VARIABLE_COUNT]) {

//   String payload = "";
//   const String payloadDef = "{\"variable\": \"{VARIABLE}\", \"value\":{VALUE}}";

//   for (int i = 0; i < VARIABLE_COUNT; i++) {
//       String thisPayload = payloadDef;
//       thisPayload.replace("{VARIABLE}", var[i].GetId());
//       thisPayload.replace("{VALUE}", var[i].value);

//       payload += thisPayload;
//       if (i < VARIABLE_COUNT - 1) {
//         payload += ",";
//       }
//   }

//   Serial.print("Payload: "); Serial.println(payload);

//   delay(200);
  
//   if (client.connect(server, 80)) {
    
//     Serial.println("connected ubidots");
//     delay(200);

//     client.println("POST /api/v1.6/collections/values HTTP/1.1");
//     client.println("Content-Type: application/json");
//     client.println("Content-Length: "+String(payload.length()));
//     client.println("X-Auth-Token: "+token);
//     client.println("Host: things.ubidots.com\n");
//     client.print(payload);

//     Serial.println("POST /api/v1.6/collections/values HTTP/1.1");
//     Serial.println("Content-Type: application/json");
//     Serial.println("Content-Length: "+String(payload.length()));
//     Serial.println("X-Auth-Token: "+token);
//     Serial.println("Host: things.ubidots.com\n");
//     Serial.print(payload);

//   }
//   else {
//     Serial.println("Unable to connect?");
//   }
// }

// void ubiSendVariables(UbiDotsVariable var[VARIABLE_COUNT]) {

//   delay(100);
  
//   for (int i = 0; i < VARIABLE_COUNT; i++) {

//     if (var[i].send) {
//       ubiSaveValue(var[i]);
//     } else {
//       var[i].SerialPrintValue();
//     }
//   }  
// }

// void ubiSaveValue(UbiDotsVariable var) {

//   delay(200);
  
//   int retriesRemain = 3;

//   do {
//     if (client.connect(server, 80)) {
      
//       String payload = "{\"value\": " + var.value + "}";

//       Serial.println("connected ubidots, sending " + var.GetLabel() + ": (" + var.value + ")");
//       delay(200);

//       client.println("POST /api/v1.6/variables/" + var.GetId() + "/values HTTP/1.1");
//       client.println("Content-Type: application/json");
//       client.println("Content-Length: "+String(payload.length()));
//       client.println("X-Auth-Token: "+token);
//       client.println("Host: things.ubidots.com\n");
//       client.print(payload);

//       break;
//     }
//     else {
//       Serial.println("retrying (" + var.GetLabel() + "): " + retriesRemain--);
//     }
//   } while(retriesRemain > 0);
// }



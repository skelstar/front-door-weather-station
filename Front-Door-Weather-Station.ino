#include <DHT.h>

#include <ESP8266WiFi.h>
extern "C" {
  #include "user_interface.h"
}
#include <MyBatteryMonitor.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <EEPROM.h>

#define   TIMEOUTMINUTE     60 * 1000
#define   TIMEOUTMINUTES    1 * TIMEOUTMINUTE
#define   SLEEPTIMEMINUTES  60 * 1000 * 1000    // usecs
#define   SLEEPMINUTES      30
#define   SLEEPTIME         SLEEPMINUTES * SLEEPTIMEMINUTES


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

MyBatteryMonitor batt(0, 660, 3.65);
#define   BATTERY_MAX  4.1
#define   BATTERY_MIN  3.9

//// wifi
const char* ssid = "LeilaNet2";
const char* password = "ec1122%f*&";

WiFiClient client;

/// EEPROM
bool batteryBelowThreshold = false;
int batteryBelowThresholdAddr = 0;

#define STATE_NORMAL                  0
#define STATE_BATTERY_BELOW_THRESHOLD 1

//// udiBots

String temperatureVariable = "569134db76254213753a108f";
String humidityVariable = "569134ec76254215afed7070";
String batteryVariable = "569134fa76254215afed7093";

#define state
String stateVariable = "56930cbd76254254bd747c6e";

String token = "GioE85MkDexLpsbE1Tt37F3TY2p3Fa6XM4bKvftz9OfC87MrH5bQGceJrVgF";

const char* server = "things.ubidots.com";
// ========================================================================

void ubiSaveValue(String idvariable, String value);

void setup() {
  Serial.begin(115200);
  delay(10);

  dallas.begin();

  //dht.setup(dht_dpin, dht.AM2302);    // DHT22
  dht.begin();
   
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

  dallas.requestTemperatures(); // Send the command to get temperatures
  float temp = dallas.getTempCByIndex(0);  //DHT.temperature;
    
  float humidity = dht.readHumidity();

  batt.ReadBatteryVoltage();  
  float battery = batt.GetBatteryRemaining(); // analogRead(A0);
    
  Serial.print("temperature: " + String(temp) + "C,  ");
  Serial.print("humidity: " + String(humidity) + "%");
  Serial.print("battery: " + String(battery) + "v");
  Serial.println();

  ubiSaveValue(temperatureVariable, String(temp));
  ubiSaveValue(humidityVariable, String(humidity));
  ubiSaveValue(batteryVariable, String(battery));

  int eepromReportedBatteryThreshold = EEPROM.read(batteryBelowThresholdAddr);

  if (battery > BATTERY_MAX) {
    Serial.println("Battery is above MAX");
    EEPROM.write(batteryBelowThresholdAddr, false);
  }
  else {
    if (battery <= BATTERY_MIN) {
      if (!eepromReportedBatteryThreshold) {
        Serial.println("Battery is <= MIN && being reported");
        ubiSaveValue(stateVariable, String(STATE_BATTERY_BELOW_THRESHOLD));
        EEPROM.write(batteryBelowThresholdAddr, true);
      }
      else {
        Serial.println("Battery is <= MIN");        
        ubiSaveValue(stateVariable, String(STATE_NORMAL));
      }
    }
    else {
      Serial.println("Battery is normal");
      ubiSaveValue(stateVariable, "0");
    }
  }
  
  Serial.print(".. sleeping for: ");
  Serial.print(SLEEPMINUTES);
  Serial.println(" mins");

  system_deep_sleep(SLEEPTIME); // usecs
}

void ubiSaveValue(String idvariable, String value) {

  int num = 0;
  String var = "{\"value\": " + String(value) + "}";
  num = var.length();

  delay(100);
  
  if (client.connect(server, 80)) {
    Serial.println("connected ubidots");
    delay(100);

    client.println("POST /api/v1.6/variables/"+idvariable+"/values HTTP/1.1");
    Serial.println("POST /api/v1.6/variables/"+idvariable+"/values HTTP/1.1");
    client.println("Content-Type: application/json");
    Serial.println("Content-Type: application/json");
    client.println("Content-Length: "+String(num));
    Serial.println("Content-Length: "+String(num));
    client.println("X-Auth-Token: "+token);
    Serial.println("X-Auth-Token: "+token);
    client.println("Host: things.ubidots.com\n");
    Serial.println("Host: things.ubidots.com\n");
    client.print(var);
    Serial.print(var+"\n");
  }
  else {
    Serial.println("Unable to connect?");
  }

  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
}



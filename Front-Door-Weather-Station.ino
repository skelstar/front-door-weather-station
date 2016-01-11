#include <ESP8266WiFi.h>
extern "C" {
  #include "user_interface.h"
}
#include <MyBatteryMonitor.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <dht.h>

#define TIMEOUTMINUTE  60 * 1000
#define TIMEOUTMINUTES  1 * TIMEOUTMINUTE
#define SLEEPTIMEMINUTES 60 * 1000 * 1000    // usecs
#define SLEEPMINUTES    30
#define SLEEPTIME     SLEEPMINUTES * SLEEPTIMEMINUTES

// wifi
const char* ssid = "LeilaNet2";
const char* password = "ec1122%f*&";
 
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library

#define DS1820Pin 4 // what pin we're connected to


OneWire  ds(DS1820Pin); 
DallasTemperature dallas(&ds);

// DHT
#define dht_dpin 2 //no ; here. Set equal to channel sensor is on
dht DHT;

WiFiClient client;

MyBatteryMonitor batt(0, 660, 3.65);

// uidBots

String temperatureVariable = "569134db76254213753a108f";
String humidityVariable = "569134ec76254215afed7070";
String batteryVariable = "569134fa76254215afed7093";

String token = "GioE85MkDexLpsbE1Tt37F3TY2p3Fa6XM4bKvftz9OfC87MrH5bQGceJrVgF";

const char* server = "things.ubidots.com";
// ========================================================================

void ubiSaveValue(String idvariable, String value);

void setup() {
  Serial.begin(115200);
  delay(10);

  dallas.begin();
   
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
  float temp = 0;
  float humidity = 0;
  float battery = 0;

  // request
  dallas.requestTemperatures(); // Send the command to get temperatures
  temp = dallas.getTempCByIndex(0);  //DHT.temperature;
  
  DHT.read11(dht_dpin);
  
  humidity = DHT.humidity;

  batt.ReadBatteryVoltage();  
  battery = batt.GetBatteryRemaining(); // analogRead(A0);
    
  Serial.print("temperature: " + String(temp) + "C,  ");
  Serial.print("humidity: " + String(humidity) + "%");
  Serial.print("battery: " + String(battery) + "v");
  Serial.println();

  ubiSaveValue(temperatureVariable, String(temp));
  ubiSaveValue(humidityVariable, String(humidity));
  ubiSaveValue(batteryVariable, String(battery));

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



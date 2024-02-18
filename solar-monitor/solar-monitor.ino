///////////////////////////////////////////////////////////////////////////////
// solar-monitor.ino 
//  ESP8266 on NodeMCU board to report DHT11 temp and humidity
//  along with solar luminosity and UV levels
//  Voltage monitor on input pin.
///////////////////////////////////////////////////////////////////////////////
/******************************************************************************/
// solar: BH1750FVI
// UV: VEML6070
// temp/humidity: DHT11
//voltage divider --
//  22k from battery to A0
//  47k from A0 to ground
//  MOSFET control on D6/pin 12
//  D0 to RST for sleep
// SCK on SD2
// SCL on SD3
// DHT11 on D2/ pin 4
// 1wire on D2/ pin 5
/******************************************************************************/

#include <ESP8266WiFi.h>
#include "myconfig.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h> //pubsubclient - nick oleary
//use esp8266 2.7.3 version or suffer the errors
//board mgr url: http://arduino.esp8266.com/stable/package_esp8266com_index.json

uint8_t dataPin  = 9; //SD2 / white / SCK
uint8_t clockPin = 10; //SD3 / yellow / DT


#include <Wire.h>

/******************************************************************************/
#include <DallasTemperature.h> //dallastemperature miles burton
#include <OneWire.h>

#define ONE_WIRE_BUS 5
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

/******************************************************************************/
//BH1750
//https://github.com/RobTillaart/BH1750FVI_RT
#include <BH1750FVI.h>
BH1750FVI myLux(0x23);
float lux_val;

/******************************************************************************/
//VEML6070 UV detector
//https://github.com/gty77663/arduino-VEML6070/blob/main/examples/UVS/UVS.ino
#include <VEML6070.h>

/******************************************************************************/
//DHT11
//https://github.com/adafruit/DHT-sensor-library/blob/master/examples/DHT_Unified_Sensor/DHT_Unified_Sensor.ino
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#define DHTPIN 4     // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)
DHT_Unified dht(DHTPIN, DHTTYPE);


/******************************************************************************/
volatile unsigned long i = 0;
char charBuffer[32];

WiFiClient espClient;
PubSubClient client(espClient);

//this is D6
#define MOSFET_PIN D6



/******************************************************************************/
void setup() {
  Serial.begin(115200);
  Serial.println("foo");
  delay(10);

  pinMode(MOSFET_PIN, OUTPUT);



  // We start by connecting to a WiFi network
  if (debugOutput) {
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
  }
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int maxWait = 500;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (debugOutput) Serial.print(".");
    if (maxWait <= 0)
      ESP.restart();
    maxWait--;
  }
  if (debugOutput) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  delay(500);
  do_update();
  client.setServer(mqtt_host, mqtt_port);
  reconnect();

  //enable power
  digitalWrite(MOSFET_PIN, HIGH);

  //1wire
  sensors.begin();


  //BH1750 light meter
  Wire.begin(dataPin,clockPin);
  myLux.powerOn();
  myLux.setContLowRes();

  //veml6070
  VEML.begin();

  //dht11
  dht.begin();
  sensor_t sensor;

}


void loop() {
  int nVoltageRaw;
  float fVoltage;
  String strBuffer;
  int sleepval;

  //turn on the power and wait 5s
  digitalWrite(MOSFET_PIN, HIGH);
  delay(5000);

  Serial.println("==========");

  uint16_t uvs = VEML.read_uvs_step();
  int risk_level = VEML.convert_to_risk_level(uvs);
  char* risk = VEML.convert_to_risk_char(risk_level);

  Serial.print("Current UVS is: ");
  Serial.print(uvs);
  Serial.print(", with current risk level: ");
  Serial.println(*risk);
  strBuffer = String(uvs);
  strBuffer.toCharArray(charBuffer, 10);
  if (!client.publish(mqtt_topic_prefix_uvs, charBuffer, false)) {
    ESP.restart();
    delay(100);
  }
  strBuffer = String(risk);
  strBuffer.toCharArray(charBuffer, 10);
  if (!client.publish(mqtt_topic_prefix_uvrisk, charBuffer, false)) {
    ESP.restart();
    delay(100);
  }


  Serial.println("==========");
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
    strBuffer = String(event.temperature);
    strBuffer.toCharArray(charBuffer, 10);
    if (!client.publish(mqtt_topic_prefix_dhttemperature, charBuffer, false)) {
      ESP.restart();
      delay(100);
    }
  }


  Serial.println("==========");
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
    strBuffer = String(event.relative_humidity);
    strBuffer.toCharArray(charBuffer, 10);
    if (!client.publish(mqtt_topic_prefix_humidity, charBuffer, false)) {
      ESP.restart();
      delay(100);
    }
  }



  Serial.println("==========");
  sensors.requestTemperatures();
  Serial.print("1wire temperature: ");
  Serial.println(sensors.getTempCByIndex(0));
  strBuffer = String(sensors.getTempCByIndex(0));
  strBuffer.toCharArray(charBuffer, 10);
  if (!client.publish(mqtt_topic_prefix_1wire, charBuffer, false)) {
    ESP.restart();
    delay(100);
  }

  Serial.println("==========");
  long rssi = WiFi.RSSI();
  Serial.print("RSSI:");
  Serial.println(rssi);

  strBuffer = String(rssi);
  strBuffer.toCharArray(charBuffer, 10);
  if (!client.publish(mqtt_topic_prefix_rssi, charBuffer, false)) {
    ESP.restart();
    delay(100);
  }

  Serial.println("==========");
  myLux.setOnceHigh2Res();
  delay(200);
  lux_val = myLux.getLux();
  Serial.print("lux: ");
  Serial.println(lux_val, 1);
  strBuffer = String(lux_val);
  strBuffer.toCharArray(charBuffer, 10);
  if (!client.publish(mqtt_topic_prefix_lux, charBuffer, false)) {
    ESP.restart();
    delay(100);
  }

  delay(1000);

  Serial.println("==========");

  //determine how long to sleep
  //get ADC
  //voltage divider --
  //  22k from battery to A0
  //  47k from A0 to ground
  //  my 4-wire setup measured 46.958k and 21.547k
  nVoltageRaw = analogRead(A0);
  fVoltage = (float)nVoltageRaw * 0.00460474;
  //4.71v is the maximum to read
  //will operate down to 2.5v but needs reset after
  //fVoltage = (float)nVoltageRaw;
  Serial.print("Battery voltage: ");
  Serial.println(fVoltage);

  //report voltage
  strBuffer = String(fVoltage);
  strBuffer.toCharArray(charBuffer, 10);
  if (!client.publish(mqtt_topic_prefix_voltage, charBuffer, false)) {
    ESP.restart();
    delay(100);
  }

  if (fVoltage < 3.7) {
    //600 seconds
    Serial.println("low volt: 600s sleep");
    sleepval = 6e8;
  } else {
    //50 seconds
    //Serial.println("50s sleep");
    //sleepval = 5e7;
    //120sec
    Serial.println("60s sleep");
    sleepval = 6e7;
  }

  //testing 5s sleep
  //sleepval = 5e6;

  //turn off power
  digitalWrite(MOSFET_PIN, LOW);
  delay(1000);

  ESP.deepSleep(sleepval); /* Sleep for 50 seconds */

  if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();
    delay(100);
  }
}

void reconnect() {
  int maxWait = 0;
  while (!client.connected()) {
    if (debugOutput) Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_id)) {
      if (debugOutput) Serial.println("connected");
    } else {
      if (debugOutput) {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
      }
      delay(5000);
      if (maxWait > 10)
        ESP.restart();
      maxWait++;
    }
  }
}





/******************************************************************************/
/******************************************************************************/
void do_update() {
  if (debugOutput) Serial.println("do update");
  t_httpUpdate_return ret = ESPhttpUpdate.update(update_server, 80, update_uri, firmware_version);
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      if (debugOutput) Serial.println("[update] Update failed.");
      break;
    case HTTP_UPDATE_NO_UPDATES:
      if (debugOutput) Serial.println("[update] no Update needed");
      break;
    case HTTP_UPDATE_OK:
      if (debugOutput) Serial.println("[update] Update ok.");  // may not called we reboot the ESP
      break;
  }
}

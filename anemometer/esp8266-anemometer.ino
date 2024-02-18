#include <ESP8266WiFi.h>
#include "myconfig.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>

/******************************************************************************/
#define DHTPIN 5 //this is D1 on the silkscreen

#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;
/******************************************************************************/

/******************************************************************************/
//wind direction stuff
#define D0 16
#define D1 5
#define D2 4
#define D3 0
int windDir[] = {22, 0, 90, 292, 45, 337, 67, 315, 180, 202, 112, 270, 157, 225, 135, 247};
/******************************************************************************/


unsigned long  next_timestamp = 0;
volatile unsigned long i = 0;
float wind = 0;
float last_wind = 0;
int count = 0;
volatile unsigned long last_micros;
long debouncing_time = 5; //in millis
int input_pin = 13;
char charBuffer[32];

WiFiClient espClient;
PubSubClient client(espClient);

void ICACHE_RAM_ATTR Interrupt()
{
  if ((long)(micros() - last_micros) >= debouncing_time * 1000) {
    i++;
    last_micros = micros();
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(input_pin, INPUT_PULLUP);//D7
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
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
  attachInterrupt(input_pin, Interrupt, RISING);
}


void loop()
{
  if (millis() > next_timestamp )
  {
    detachInterrupt(input_pin);
    count++;
    float rps = i / number_reed; //computing rounds per second
    if (i == 0)
      wind = 0.0;
    else
      wind = 1.761 / (1 + rps) + 3.013 * rps;// found here: https://www.amazon.de/gp/customer-reviews/R3C68WVOLJ7ZTO/ref=cm_cr_getr_d_rvw_ttl?ie=UTF8&ASIN=B0018LBFG8 (in German)
    if (last_wind - wind > 0.8 || last_wind - wind < -0.8 || count >= 10) {
      if (debugOutput) {
        Serial.print("Wind: ");
        Serial.print(wind);
        Serial.println(" km/h");
        //not going to put this in production, don't have great way to expose
        //the sensor and it's not well-sited anyway
        //read_DHT();
        read_direction();

      }
      String strBuffer;
      strBuffer =  String(wind);
      strBuffer.toCharArray(charBuffer, 10);
      if (!client.publish(mqtt_topic_prefix_wind, charBuffer, false))
      {
        ESP.restart();
        delay(100);
      }
      count = 0;
    }
    i = 0;
    last_wind = wind;




    if (WiFi.status() != WL_CONNECTED) {
      ESP.restart();
      delay(100);
    }
    next_timestamp  = millis() + 1000; //intervall is 1s
    attachInterrupt(input_pin, Interrupt, RISING);

  }




  yield();
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
//read_direction
// this function needs to cycle the individual bits on and check the results
// LSB is the outer-most, MSB is closest to the hub
/******************************************************************************/
void read_direction() {
  int b0 = 0;
  int b1 = 0;
  int b2 = 0;
  int b3 = 0;
  int analog_raw = 0;
  int bit_set = 500;


  digitalWrite(D0, HIGH);
  delay(10);
  if (analogRead(A0) > bit_set) {
    b0 = 1;
  }
  digitalWrite(D0, LOW);
  //
  digitalWrite(D1, HIGH);
  delay(10);
  if (analogRead(A0) > bit_set) {
    b1 = 1;
  }
  digitalWrite(D1, LOW);
  //
  digitalWrite(D2, HIGH);
  delay(10);
  if (analogRead(A0) > bit_set) {
    b2 = 1;
  }
  digitalWrite(D2, LOW);
  //
  digitalWrite(D3, HIGH);
  delay(10);
  if (analogRead(A0) > bit_set) {
    b3 = 1;
  }
  digitalWrite(D3, LOW);

  //Serial.print("results: ");
  //Serial.print(b3);
  //Serial.print(" ");
  //Serial.print(b2);
  //Serial.print(" ");
  //Serial.print(b1);
  //Serial.print(" ");
  //Serial.println(b0);

  //convert discrete bits to a 4-bit int
  int number = (b3 & 0x1) | ((b2 & 0x1) << 1) | ((b1 & 0x1) << 2) | ((b0 & 0x1) << 3);

  

  Serial.print("Wind: ");
  Serial.print(windDir[number]);
  Serial.println(" degrees");


  //report via mqtt
  String strBuffer;
  strBuffer =  String(windDir[number]);
  strBuffer.toCharArray(charBuffer, 10);
  if (!client.publish(mqtt_topic_prefix_wdir, charBuffer, false))
  {
    ESP.restart();
    delay(100);
  }


}



/******************************************************************************/

/******************************************************************************/
void read_DHT() {
  float newT = dht.readTemperature(true);
  //if temperature read failed, don't change t value
  if (isnan(newT)) {
    Serial.println("Failed to read from DHT sensor!");
  }
  else {
    t = newT;
    Serial.print("temperature: ");
    Serial.println(t);
    String strBuffer;
    strBuffer =  String(t);
    strBuffer.toCharArray(charBuffer, 10);
    if (!client.publish(mqtt_topic_prefix_temperature, charBuffer, false))
    {
      ESP.restart();
      delay(100);
    }
  }
  //read Hhumidity
  float newH = dht.readHumidity();
  if (isnan(newH)) {
    Serial.println("Failed to read from DHT sensor!");
  }
  else {
    h = newH;
    Serial.print("RH: ");
    Serial.println(h);
    String strBuffer;
    strBuffer =  String(h);
    strBuffer.toCharArray(charBuffer, 10);
    if (!client.publish(mqtt_topic_prefix_relh, charBuffer, false))
    {
      ESP.restart();
      delay(100);
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
      if (debugOutput )Serial.println("[update] no Update needed");
      break;
    case HTTP_UPDATE_OK:
      if (debugOutput) Serial.println("[update] Update ok."); // may not called we reboot the ESP
      break;
  }
}

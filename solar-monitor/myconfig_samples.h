/*************************************************/
/* Debugging                                     */
/*************************************************/
const bool debugOutput = true;  // set to true for serial OUTPUT

/*************************************************/
/* Settings for WLAN                             */
/*************************************************/
const char* ssid = "SSID";
const char* password = "ergerg123erg";

/*************************************************/
/* Update settings                               */
/*************************************************/ 
const char* firmware_version = "sunnyday_0.0.1";
const char* update_server = "updateserver";
const char* update_uri = "/path/update.php";

/*************************************************/
/* MQTTCloud data                               */
/*************************************************/
const char* mqtt_host = "192.168.1.101";
const char* mqtt_id = "ESP8266-Solar";
const char* mqtt_topic_prefix_voltage = "/solar1/voltage";
const char* mqtt_topic_prefix_rssi = "/solar1/rssi";
const char* mqtt_topic_prefix_1wire = "/solar1/1wire";
const char* mqtt_topic_prefix_humidity = "/solar1/humidity";
const char* mqtt_topic_prefix_dhttemperature = "/solar1/dhttemp";
const char* mqtt_topic_prefix_uvs = "/solar1/uvs";
const char* mqtt_topic_prefix_uvrisk = "/solar1/uvrisk";
const char* mqtt_topic_prefix_lux = "/solar1/lux";

const int mqtt_port = 1883;
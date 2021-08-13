# Description
This project measures the wind in km/h and reports the value via MQTT.

This version is (will be) specifically modified to replace the control
in a LaCrosse Technology TX63U-IT where the complete system just died
after a year.

Will include: 4-bit windvane

DHT22 temperature / humidity

Light intensity via the solar cell.

# compiling
It should be noted that the ESP8266 board library rev 2.7.4 is required
due to a deprecated function somewhere in the code.

# Hardware
The following hardware is used for building this project:
* ESP8266 (Wemos D1)
* Eltako Windsensor (http://amzn.to/2ps1Tte - German Amazon)

# Wiring
I connected the anemometer with the D7 and GND Pin of my Wemos D1. But it can be all other suitable Pins used.

# Software
For using this software you have to copy the myconfig_samples.h to myconfig.h and change the values for the MQTT server and the WLAN settings.

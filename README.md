# Description
This project measures the wind in km/h and reports the value via MQTT.

This version is specifically modified to replace the control
in a LaCrosse Technology TX63U-IT where the complete system just died
after a year.

Will include: 4-bit windvane

DHT11 temperature / humidity

Light intensity via the solar cell.

Wind direction and speed

# compiling
It should be noted that the ESP8266 board library rev 2.7.4 is required
due to a deprecated function somewhere in the code.

# Hardware
The following hardware is used for building this project:
* ESP8266 -- cheap amazon nodemcu clone
* Junk LaCrosse TX63U

# Wiring
* speed -- D7 to ground
* DHT11 -- D1
* LED control output: D0/D1/D2/D3 or GPIO 16/5/4/0
* photodiode input: A0

# Software
For using this software you have to copy the myconfig_samples.h to myconfig.h and change the values for the MQTT server and the WLAN settings.


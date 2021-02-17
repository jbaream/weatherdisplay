# Weather Display
An application for LILYGO TTGO T5 e-ink board to display weather for specified city from openweather.

This application will query openweather api for forecast and display it on the e-ink display.
The user specific configuration (line WiFi ssid, password, location etc) are not stored in the codebase. You just need to create simple configuration file wifi_config.txt (see example) and put it into the root of Micro SD card. Before first launch card is inserted into the module. 
Application will read configuration and store it into EEROM, so the SD card can be removed for the next launches.

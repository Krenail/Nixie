# Nixie Tube Clock

This project aims to replicate Dalibor Farny PURI Nixie Clock 4 using an ESP8266 as a controller, K155ID1 drivers, 74HC595 shift registers and a DHT22 sensor for temp/humidity measurement.

The clock is automatically syncronised using NTP and on startup it requires to be connected to a Wi-fi network in order to update the time and date.

There are 5 different "modes" in which the clock can operate: time, date, temp/humidity, IP, night.
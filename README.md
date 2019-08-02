# CleverNet library


## About

A library to simplify the communication between the cleverpet hub and external ESP32 based sensors


## Prerequisits

* particle-cli
* arduino ide (with ESP32 board support installed)
* ESP32 board with Wifi
* Cleverpet hub converted to a [Hackerpet](https://hackerpet.com/)

For the example 

* Tilt switch module from [Joy-it](http://sensorkit.en.joy-it.net/index.php?title=KY-017_Tilt_switch_module) (any other switch will work for testing just as well)

## How to run the example

compile the hub binary by

    particle compile photon src
    particle flash <your device id goes here> /home/developer/Development/c++/hackerpet-1/games/photon_firmware_<check the output of the previous command for the correct timestamp>.bin

Modify the SSID and Wifi Password in examples/hackerpet_remote_sensor/hackerpet_remote_sensor.ino to match your configuration.

Compile the example from examples/hackerpet_remote_sensor and install it onto the ESP32

A video of the remote button in action can be found here (https://youtu.be/Dw1Pwbul5oc)



## How it works

When both devices are connected to the network the esp32 will continuously announce itself.
As soon as the hub discovers those messages it will try to connect to the esp32 and poll for button state changes.


## ATTENTION

**This is only a proof of concept demonstration and not a full library yet.**

## Todo

* Refactor the code for the particle and the esp32 into a single library
* Keep an internal cache of all known devices
* Allow the user to select to which devices it needs to connect






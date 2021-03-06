# Smart agriculture with arduino

This arduino project implements an intelligent plant caring and monitoring system. Features include:

* monitoring of temperature, soil moisture and received light
* automatic watering and heating
* water level warning, when water is low in reservoir

## Automatic mode

In this mode arduino will automatically try to keep temperature and soil moisture above given levels. This levels can be set through sending commands to arduino. See [list of commands](#list-of-commands) for details. This setting will be preserved in persistent memory through reboots.

If you want to control heating and pump manually first disable automatic mode (by default disabled).


## Water level warning

Arduino automatically checks if water level in reservoir is low. This will be reported by red LED. You can reservoir capacity via appropriate command. Every time capacity is set the reservoir will be assumed full, so you can use `W` command to reset state after refilling.

Remaining water level is stored in EEPROM memory, which means it will be preserved during reboots.


## Serial communication

Sensors measurements and commands to arduino are transferred via serial in binary form. You can use scripts `reader.py` and `ctl.py` to read/write to serial in required format (see below). Binary format is as follows:

Arduino output:
```
// sensors
uint16_t temperature
uint16_t moisture
uint16_t light
uint8_t button
// devices
uint8_t pump_on
uint8_t heating_on
// threshold for engaging heating in auto mode
uint16_t temperature_level
// threshold for engaging pump in auto mode
uint16_t moisture_level
// volume of attached water tank in dl
uint16_t water_capacity
// water in the reservoir
uint16_t remaining_water
// if auto mode is enabled
uint8_t auto_mode
```

Arduino input:
```
uint16_t cmd
uint16_t value
```

See [list of commands](#list-of-commands).

Note that all binary messages are without padding. You might want to use packed structures to read them.

## reader.py

Reads sensors and settings data from serial and outputs csv on stdout. Format is:

```
datetime(ISO 8601),thermistor(temperature),moisture,photoresitor(light),button,pump_on,heating_on,temperature_level,moisture_level,water_capacity,remaining_water,auto_mode
```

This data includes both current readings from sensors (`thermistor(temperature),moisture,photoresitor(light)`), state of attached devices (`button,pump_on,heating_on`) and current settings and data that are preserved through reboots (`temperature_level,moisture_level,water_capacity,remaining_water,auto_mode`).

## ctl.py

Controls the pump attached to arduino. Reads commands on stdin and sends them to arduino in binary format.

Command format is :


```
CMD ARG
```

CMD is one letter identifier of command and ARG is *mandatory* unsigned 16-bit integer argument (as text).

### List of commands

#### Actuators

* `P [0-1]` - starts (1) or stops (0) the pump
* `R <duration is seconds>` - runs the pump for given duration. You can use this to precisely control watering.
* `H [0-1]` - starts (1) or stops (0) the heating

#### Settings

All settings will be preserved through reboots.

* `T [0-1023]` - sets `temperature_level`. Threshold for engaging heating in auto mode
* `M [0-1023]` - sets `moisture_level`. Threshold for engaging pump in auto mode
* `W [0-1023]` - sets `water_capacity`. Volume of attached water tank in dl
* `A [0-1]` - sets `auto_mode`. Enables or disables automatic mode.

## Building

You should be able to build `agriculture.ino` with recent Arduino IDE. Furthermore there is a makefile for [Arduino-Makefile](https://github.com/sudar/Arduino-Makefile). Use

```
make upload
```

to build and upload to Arduino UNO.

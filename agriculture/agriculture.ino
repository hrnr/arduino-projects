// use backported EEPROM library from arduino 1.8.1 with new `get` and `put`
// methods
#include "EEPROM.h"

// pin numbers for digital pins
#define button_pin 11
#define green_led_pin 10
#define heating_pin 9
#define relay_pin 6
// pin numbers for analog pins
#define moisture_pin 2
#define thermistor_pin 1
#define photoresistor_pin 0

// magic value to mark our content in eeprom
#define EEPROM_MAGIC 0xce

struct Measurements {
  uint16_t temperature = 0;
  uint16_t moisture = 0;
  uint16_t light = 0;
  uint16_t button = 0;
};

struct Command {
  uint16_t cmd;
  uint16_t value;
};

struct Settings {
  // threshold for engaging heating in auto mode
  uint16_t temperature_level = 800;
  // threshold for engaging pump in auto mode
  uint16_t moisture_level = 1020;
  // volume of attached water tank in dl
  uint16_t water_capacity = 3;
  // if auto mode is enabled
  char auto_mode = 0;

  /* load from EEPROM if possible load defaults otherwise */
  void load() {
    if (EEPROM[0] != EEPROM_MAGIC) {
      *this = Settings();
      return;
    }
    EEPROM.get(1, *this);
  }

  /* save to EEPROM */
  void save() {
    EEPROM[0] = EEPROM_MAGIC;
    EEPROM.put(1, *this);
  }

  /**
   * @brief Sets setting identified by its letter and saves all settings
   *
   * @param cmd letter identifying setting
   * @param val value to set
   */
  void set(char cmd, uint16_t val) {
    switch (cmd) {
    case 'T':
      temperature_level = val;
      break;
    case 'M':
      moisture_level = val;
      break;
    case 'W':
      water_capacity = val;
      pump_remaining_water = water_capacity;
      break;
    case 'A':
      auto_mode = val;
      break;
    default:
      return;
    }
    save();
  }
};

Measurements sensors;
Command cmd;
Settings settings;

// timers
unsigned long time = 0;
unsigned long serial_timer = 0;
unsigned long pump_start_time = 0;
unsigned long pump_duration = 0;

// water in the reservoir
uint16_t pump_remaining_water = 0;

/* blinks an arduino built-in led */
void blinkLed(int led_pin, unsigned char repeats = 1) {
  for (unsigned char i = 0; i < repeats; ++i) {
    digitalWrite(led_pin, HIGH);
    delay(250);
    digitalWrite(led_pin, LOW);
    delay(250);
  }
}

void readSensors() {
  sensors.temperature = analogRead(thermistor_pin);
  sensors.moisture = analogRead(moisture_pin);
  sensors.light = analogRead(photoresistor_pin);
  sensors.button = digitalRead(button_pin);
}

void updateTime() { time = millis(); }

void sendMeasurements() {
  Serial.write((const uint8_t *)&sensors, sizeof(sensors));
}

/* returns true if duration from start_time passed. if duration is 0 this timer
 * will never trigger. */
bool timerDuration(unsigned long start_time, unsigned long duration) {
  return duration && (time < start_time || time > duration + start_time);
}

void startPump() {
  digitalWrite(relay_pin, HIGH);
  pump_duration = 0;
  pump_start_time = millis();
}

void stopPump() {
  digitalWrite(relay_pin, LOW);
  pump_duration = 0;

  // calculate used water
  unsigned long duration = time - pump_start_time;
  uint16_t consumed_water = (duration / 1024) * 1 /* pumping ratio */;
  // protect underflow
  pump_remaining_water -= std::min(pump_remaining_water, consumed_water);
}

void startStopPump(bool start) {
  if (start) {
    startPump();
  } else {
    stopPump();
  }
}

/* will run pump for specified time in s */
void runPumpForTime(int time) {
  startPump();
  pump_duration = time * 1000;
}

void startStopHeating(bool start) {
  if (start) {
    // start
    digitalWrite(heating_pin, HIGH);
  } else {
    // stop
    digitalWrite(heating_pin, LOW);
  }
}

void doCommand() {
  switch (cmd.cmd) {
  case 'P':
    startStopPump(cmd.value);
    break;
  case 'R':
    runPumpForTime(cmd.value);
    break;
  case 'H':
    startStopHeating(cmd.value);
    break;
  default:
    // we try if it is settings command
    settings.set(cmd.cmd, cmd.value);
    break;
  }
}

/* automates watering and heating. run actuator if sensor measurement is below
 * threshold. stop otherwise.*/
void autoMode() {
  // higher reading from moisture sensor means less moisture
  startStopPump(sensors.moisture > settings.moisture_level);
  startStopHeating(sensors.temperature < settings.temperature_level);
}

void warnLowWaterLevel() {
  // signal with LED when water level is low
  // TODO change this led to something else to be more visible.
  if (pump_remaining_water < settings.water_capacity / 8) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void setup() {
  // setup all leds
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(green_led_pin, OUTPUT);
  pinMode(heating_pin, OUTPUT);
  pinMode(relay_pin, OUTPUT);

  pinMode(button_pin, INPUT);
  // setup serial speed
  Serial.begin(115200);
  // stop for safety on start
  stopPump();
}

void loop() {
  updateTime();

  readSensors();

  if (settings.auto_mode) {
    autoMode();
  }

  warnLowWaterLevel();

  /* stop pump if  predefined time is over */
  if (timerDuration(pump_start_time, pump_duration)) {
    stopPump();
  }

  if (sensors.button == HIGH) {
    runPumpForTime(2);
  }

  /* receive commands */
  if (Serial.available() >= (int)sizeof(cmd)) {
    Serial.readBytes((char *)&cmd, sizeof(cmd));
    doCommand();
  }

  /* send measurements over serial every 1s */
  if (timerDuration(serial_timer, 1000)) {
    serial_timer = time;
    sendMeasurements();
  }
}

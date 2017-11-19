// use backported EEPROM library from arduino 1.8.1 with new `get` and `put`
// methods
#include "EEPROM.h"

// pin numbers for digital pins
#define button_pin 11
#define green_led_pin 10
#define heating_pin 9
#define relay_pin 6
#define red_led_pin 5
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
  uint8_t button = 0;
  uint8_t pump_on = false;
  uint8_t heating_on = false;
} __attribute__((packed));

struct Command {
  uint16_t cmd;
  uint16_t value;
};

struct Settings {
  // threshold for engaging heating in auto mode
  uint16_t temperature_level = 800;
  // threshold for engaging pump in auto mode
  uint16_t moisture_level = 1000;
  // volume of attached water tank in dl
  uint16_t water_capacity = 3;
  // water in the reservoir
  uint16_t remaining_water = water_capacity;
  // if auto mode is enabled
  uint8_t auto_mode = 0;

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

  /* setters that also save the settings */

  void setAutoMode(bool val) {
    auto_mode = val;
    save();
  }

  void setWaterCapacity(uint16_t val) {
    water_capacity = val;
    save();
  }

  void setMoistureLevel(uint16_t val) {
    moisture_level = val;
    save();
  }

  void setTemperatureLevel(uint16_t val) {
    temperature_level = val;
    save();
  }

  void setRemainingWater(uint16_t val) {
    remaining_water = val;
    save();
  }
} __attribute__((packed));

void stopAutoMode();

Measurements sensors;
Command cmd;
Settings settings;

// timers
unsigned long time = 0;
unsigned long serial_timer = 0;
unsigned long pump_start_time = 0;
unsigned long pump_duration = 0;

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
  Serial.write((const uint8_t *)&settings, sizeof(settings));
}

/* returns true if duration from start_time passed. if duration is 0 this timer
 * will never trigger. */
bool timerDuration(unsigned long start_time, unsigned long duration) {
  return duration && (time < start_time || time > duration + start_time);
}

void startPump() {
  digitalWrite(relay_pin, HIGH);
  sensors.pump_on = true;

  // reset timer
  pump_duration = 0;
  pump_start_time = millis();
}

void stopPump() {
  digitalWrite(relay_pin, LOW);
  pump_duration = 0;
  sensors.pump_on = false;

  // calculate used water
  unsigned long duration = time - pump_start_time;
  uint16_t consumed_water = (duration / 1024) * 1 /* pumping ratio */;
  // protect underflow
  uint16_t remaining_water =
      settings.remaining_water - min(settings.remaining_water, consumed_water);
  // save to EEPROM
  settings.setRemainingWater(remaining_water);
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

void startHeating() {
  digitalWrite(heating_pin, HIGH);
  sensors.heating_on = true;
}

void stopHeating() {
  digitalWrite(heating_pin, LOW);
  sensors.heating_on = false;
}

void startStopHeating(bool start) {
  if (start) {
    startHeating();
  } else {
    stopHeating();
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
  case 'W':
    // refill water tank, reset capacity
    settings.setWaterCapacity(cmd.value);
    settings.setRemainingWater(cmd.value);
    break;
  case 'A':
    settings.setAutoMode(cmd.value);
    // we can always disable, it will be re-enabled automatically if needed
    stopAutoMode();
    break;
  case 'T':
    settings.setTemperatureLevel(cmd.value);
    break;
  case 'M':
    settings.setMoistureLevel(cmd.value);
    break;
  default:
    // unknown command
    return;
  }
}

/* automates watering and heating. run actuator if sensor measurement is below
 * threshold. stop otherwise.*/
void autoMode() {
  if (!settings.auto_mode) {
    digitalWrite(green_led_pin, LOW);
    return;
  }

  // signal we are in auto mode
  digitalWrite(green_led_pin, HIGH);

  // used to prevent cycling near threshold level. This value should be big
  // enough to deal with noise in measurements
  constexpr int sensors_tresh = 25;

  /* carefully engage pump and heating to prevent cycling, which can harm
   * devices */

  // higher reading from moisture sensor means less moisture
  if (sensors.pump_on) {
    startStopPump(sensors.moisture + sensors_tresh > settings.moisture_level);
  } else {
    startStopPump(sensors.moisture > settings.moisture_level);
  }

  if (sensors.heating_on) {
    startStopHeating(sensors.temperature - sensors_tresh <
                     settings.temperature_level);
  } else {
    startStopHeating(sensors.temperature < settings.temperature_level);
  }
}

/* handles disabling auto mode safely */
void stopAutoMode() {
  // safety measure, if we have turned heating and pump on
  stopPump();
  stopHeating();
}

void warnLowWaterLevel() {
  // signal with LED when water level is low
  if (settings.remaining_water <= settings.water_capacity / 8) {
    digitalWrite(red_led_pin, HIGH);
  } else {
    digitalWrite(red_led_pin, LOW);
  }
}

void setup() {
  // setup all leds
  pinMode(red_led_pin, OUTPUT);
  pinMode(green_led_pin, OUTPUT);
  pinMode(heating_pin, OUTPUT);
  pinMode(relay_pin, OUTPUT);

  pinMode(button_pin, INPUT);
  // setup serial speed
  Serial.begin(115200);
  // stop for safety on start
  stopAutoMode();

  // read settings from EEPROM
  settings.load();
}

void loop() {
  updateTime();

  readSensors();

  // controls pump and heating if running in auto mode
  autoMode();

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

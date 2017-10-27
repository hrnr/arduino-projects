// pin numbers for digital pins
#define button_pin 11
#define green_led_pin 10
#define blue_led_pin 9
#define relay_pin 6
// pin numbers for analog pins
#define moisture_pin 2
#define thermistor_pin 1
#define photoresistor_pin 0

#define MSG_SIZE 6
char msg_buffer[MSG_SIZE];
unsigned long serial_timer = 0;
unsigned long time = 0;

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

Measurements sensors;
Command cmd;

/* blinks an arduino builtin led */
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

/* visualise sensor measurements via leds */
void visualiseMeasurements() {
  // analogRead values go from 0 to 1023, analogWrite values from 0 to 255
  analogWrite(blue_led_pin, sensors.light / 4);
  analogWrite(green_led_pin, sensors.temperature / 4);
}

void updateTime() { time = millis(); }

void sendMeasurements() {
  Serial.write((const uint8_t *)&sensors, sizeof(sensors));
}

bool timerEvery(unsigned long prev_time, unsigned long duration) {
  return prev_time < time && time - prev_time > duration;
}

void startPump() { digitalWrite(relay_pin, HIGH); }

void stopPump() { digitalWrite(relay_pin, LOW); }

/* will run pump for specified time */
void runPumpForTime(int time) {}

void doCommand() {
  switch (cmd.cmd) {
  case 'R':
    startPump();
    break;
  case 'S':
    stopPump();
    break;
  case 'T':
    runPumpForTime(cmd.value);
    break;
  default:
    // ignore uknown commands
    break;
  }
}

void setup() {
  // setup all leds
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(green_led_pin, OUTPUT);
  pinMode(blue_led_pin, OUTPUT);
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

  visualiseMeasurements();

  if (sensors.button == HIGH) {
    runPumpForTime(1);
  }

  /* receive commands */
  if (Serial.available() >= (int)sizeof(cmd)) {
    Serial.readBytes((char *)&cmd, sizeof(cmd));
    doCommand();
  }

  /* send measurements over serial every 1s */
  if (timerEvery(serial_timer, 1000)) {
    serial_timer = time;
    sendMeasurements();
  }
}

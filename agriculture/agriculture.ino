// pin numbers for digital pins
#define button_pin 11
#define green_led_pin 10
#define blue_led_pin 9
// pin numbers for analog pins
#define thermistor_pin 1
#define photoresistor_pin 0

#define MSG_SIZE 6
char msg_buffer[MSG_SIZE];
unsigned long serial_timer = 0;
unsigned long time = 0;

char button_state = LOW;
int thermistor_state = 0;
int photoresistor_state = 0;

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
  button_state = digitalRead(button_pin);
  thermistor_state = analogRead(thermistor_pin);
  photoresistor_state = analogRead(photoresistor_pin);
}

/* visualise sensor measurements via leds */
void visualiseMeasurements() {
  // analogRead values go from 0 to 1023, analogWrite values from 0 to 255
  analogWrite(blue_led_pin, photoresistor_state / 4);
  analogWrite(green_led_pin, thermistor_state / 4);
}

void updateTime() {
  time = millis();
}

void sendMeasurements() {
  Serial.print(thermistor_state);
  Serial.print(' ');
  Serial.println(photoresistor_state);
}

bool timerEvery(unsigned long prev_time, unsigned long duration) {
  return prev_time < time && time - prev_time > duration;
}

void setup() {
  // setup all leds
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(green_led_pin, OUTPUT);
  pinMode(blue_led_pin, OUTPUT);

  pinMode(button_pin, INPUT);
  // setup serial speed
  Serial.begin(115200);
}

void loop() {
  updateTime();

  readSensors();

  visualiseMeasurements();

  // shine full on button press
  if (button_state == HIGH) {
    analogWrite(green_led_pin, 255);
  }

  /* serial echo */
  if (Serial.available() >= MSG_SIZE) {
    Serial.readBytes(msg_buffer, MSG_SIZE);
    Serial.write(msg_buffer, MSG_SIZE);
  }

  /* send measurements over serial every 1s */
  if (timerEvery(serial_timer, 1000)) {
    serial_timer = time;
    sendMeasurements();
  }
}

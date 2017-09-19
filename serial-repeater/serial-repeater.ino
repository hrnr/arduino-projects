#define MSG_SIZE 6
char msg_buffer[MSG_SIZE];

/* blinks an arduino builtin led */
void blinkLed(unsigned char repeats = 1) {
  for (unsigned char i = 0; i < repeats; ++i) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
  }
}

void setup() {
  // we will blink on message
  pinMode(LED_BUILTIN, OUTPUT);
  // setup serial speed
  Serial.begin(57600);
}

void loop() {
  if (Serial.available() >= MSG_SIZE) {
    Serial.readBytes(msg_buffer, MSG_SIZE);
    blinkLed(2);
    Serial.write(msg_buffer, MSG_SIZE);
  }
}

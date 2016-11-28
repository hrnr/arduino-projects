#include <Servo.h>

Servo left;
Servo right;
int button_pin = 2;

void setup()
{
  // initialize serial and wait for port to open:
  Serial.begin(115200);
  left.attach(12);
  right.attach(13);
  // button
  pinMode(button_pin, INPUT_PULLUP);
  
  stopWheels();
}

bool buttonDown() {
  return !digitalRead(button_pin);
}

void goForward() {
  left.write(1700);
  right.write(1300);
}

void stopWheels() {
  left.write(1500);
  right.write(1500);
}

void debugInfra() {
  for (int i = 0; i < 5; ++i) {
    Serial.print(analogRead(i));
    Serial.print(' ');
  }
  Serial.println();
}

bool lineAhead() {
  bool res = 0;
  for (int i = 0; i < 5; ++i) {
    res |= analogRead(i) < 200 ? 1 : 0;
  }
  return res;
}

void loop()
{
//  Serial.println(buttonDown());
//  debugInfra();
  if (buttonDown()) {
    goForward();
  }
  if (lineAhead()) {
    stopWheels();
  }
  delay(50);
}


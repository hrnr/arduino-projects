#include <Servo.h>

#define button_pin 2
#define infra_low 3
#define infra_high 8

Servo left;
Servo right;
int go = 0;
bool infras[infra_high - infra_low];

void setup()
{
  // initialize serial and wait for port to open:
  Serial.begin(115200);
  left.attach(12);
  right.attach(13);
  // button
  pinMode(button_pin, INPUT_PULLUP);
  
  stopWheels();
  // setup infaarray
  for (int i = infra_low; i < infra_high; ++i) {
    pinMode(i, INPUT);
  }
}

bool buttonDown() {
  return !digitalRead(button_pin);
}

void goForward(int speed = 200) {
  left.write(1500 + speed);
  right.write(1500 - speed);
}

void turn(int slope) {
  left.write(1500 + slope);
  right.write(1500 + slope);
}

void stopWheels() {
  left.write(1500);
  right.write(1500);
}

void debugInfra() {
  for (int i = 0; i < sizeof(infras); ++i) {
    Serial.print(infras[i]);
    Serial.print(' ');
  }
  Serial.println();
}

void readInfra() {
  for (int i = infra_low, j = 0; i < infra_high; ++i,++j) {
    infras[j] = digitalRead(i);
  }
}

bool lineAhead() {
  bool res = 0;
  for (int i = infra_low; i < infra_high; ++i) {
    res |= !infras[i];
  }
  return res;
}

bool lineLeft() {
  return !infras[1];
}

bool lineRight() {
  return !infras[3];
}

bool lineBeneath() {
  return !infras[2];
}

void loop()
{
  readInfra();
  // Serial.println(buttonDown());
  // debugInfra();

  if (buttonDown()) {
    go ^= 1;
  }
  if (!go) {
    stopWheels();
    return;
  }

  if (lineBeneath()) {
    goForward(50);
  } else if (lineLeft()){
    turn(-50);
  } else if (lineRight()) {
    turn(50);
  } else { // finding line
    turn(-100);
    delay(5);
  }

  delay(5);
}

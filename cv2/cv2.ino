#include <Servo.h>

/* simple line follower that uses 3-state logic. */

#define button_pin 2
#define infra_low 3
#define infra_high 8

Servo left;
Servo right;
int go = 0;
// readings from infrared sensors
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
  // setup infa array
  for (int i = infra_low; i < infra_high; ++i) {
    pinMode(i, INPUT);
  }
}

bool buttonDown() {
  return !digitalRead(button_pin);
}

/* going forward, defaults to full speed */
void goForward(int speed = 200) {
  left.write(1500 + speed);
  right.write(1500 - speed);
}

/* inplace turning */
void turn(int slope) {
  left.write(1500 + slope);
  right.write(1500 + slope);
}

/* turning on smoother curve */
void slopeTurn(int slope, int speed = 200) {
  left.write(1500 + speed + slope);
  right.write(1500 - speed + slope);
}

/* stops robot */
void stopWheels() {
  left.write(1500);
  right.write(1500);
}

/* reading from infra red sensors */
void debugInfra() {
  for (int i = 0; i < sizeof(infras); ++i) {
    Serial.print(infras[i]);
    Serial.print(' ');
  }
  Serial.println();
}

/* reads infra red sensors to infras array */
void readInfra() {
  for (int i = infra_low, j = 0; i < infra_high; ++i,++j) {
    infras[j] = digitalRead(i);
  }
}

/* indicates if any of the sensors is on the line */
bool lineAhead() {
  bool res = 0;
  for (int i = infra_low; i < infra_high; ++i) {
    res |= !infras[i];
  }
  return res;
}

/* line left to the robt */
bool lineLeft() {
  return !infras[1];
}

/* line right to the robt */
bool lineRight() {
  return !infras[3];
}

/* line is properly aligned under robot */
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

  // correct position according to line position
  if (lineBeneath()) {
    goForward(200);
  } else if (lineLeft()){
    turn(-70);
  } else if (lineRight()) {
    turn(70);
  } else { // finding line
    slopeTurn(-30);
  }
  delay(1);
}

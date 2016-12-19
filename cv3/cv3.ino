#include <Servo.h>

/* line follower that follows hints */

#define button_pin 2
#define infra_low 3
#define infra_high 8

Servo left;
Servo right;
bool go_same_side = true;
signed char mark = 0;
signed char crossroad_count = 0; // how many crossroads we have met yet (0 or 1). this
                         // will be reseted after we are out of junction
// readings from infrared sensors
bool infras[infra_high - infra_low];

typedef unsigned int uint;

bool buttonDown() { return !digitalRead(button_pin); }

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
  delay(500);
}

/* reads infra red sensors to infras array */
void readInfra() {
  for (int i = infra_low, j = 0; i < infra_high; ++i, ++j) {
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
bool lineLeft() { return !infras[1]; }

/* line right to the robt */
bool lineRight() { return !infras[3]; }

/* line is properly aligned under robot */
bool lineBeneath() { return !infras[2]; }

bool markRight() { return !infras[4]; }

bool markLeft() { return !infras[0]; }

void maskLeft() { infras[1] = 1; }

void maskRight() { infras[3] = 1; }

void maskDirection(int direction) {
  if (mark == -1) {
    maskRight();
  } else if (mark == 1) {
    maskLeft();
  }
}

bool stopLine() { return markLeft() && markRight(); }

bool crossRoad() {
  return (lineBeneath() && lineRight()) || (lineBeneath() && lineLeft());
}

void setup() {
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

const static int mark_turn_ratio = 40;
const static int mark_delay = 100;
const static int mark_speed = 40;

const static int basic_turn_ratio = 70;
const static int forward_speed = 40;
const static uint crossroad_delay = 25000;

void lineFollow() {
  // correct position according to line position
  if (lineBeneath()) {
    goForward(forward_speed);
  } else if (lineLeft()) {
    slopeTurn(-1 * basic_turn_ratio, forward_speed);
  } else if (lineRight()) {
    slopeTurn(basic_turn_ratio, forward_speed);
  } else { // finding line
    slopeTurn(-30);
  }
}

void loop() {
  readInfra();
  // Serial.println(buttonDown());
  // debugInfra();

  if (buttonDown()) {
    stopWheels();
    delay(8000);
    // go_same_side = false;
  }

  if (stopLine()) {
    stopWheels();
    // return;
  }

  // marker
  if (markLeft() && !crossroad_count) {
    mark = -1;
  } else if (markRight() && !crossroad_count) {
    mark = 1;
  }

  // first cross road
  if ((crossRoad() && mark) && !crossroad_count) {
    crossroad_count = 1;

    slopeTurn(mark * mark_turn_ratio, mark_speed);
    delay(mark_delay);
    for(uint i = 0; i < crossroad_delay; ++i) {
      readInfra();
      maskDirection(mark);
      lineFollow();
    }
    readInfra();
    stopWheels();
    delay(5000);
  }

  // join junction
  if (((markLeft() || markRight()) && mark) && crossroad_count) {
    crossroad_count = 0;
    for(uint i = 0; i < crossroad_delay/2; ++i) {
      readInfra();
      maskDirection(-1 * mark);
      lineFollow();
    }
    mark = 0;
    readInfra();
    stopWheels();
    delay(5000);
  }

  lineFollow();
}

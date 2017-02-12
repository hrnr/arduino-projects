#include <Servo.h>
// use backported EEPROM library from arduino 1.8.1 with new `get` and `put`
// methods
#include "EEPROM.h"

#define button_pin 2
#define infra_low 3
#define infra_high 8
#define led_pin 11

#define LENGTH(x) (sizeof x / sizeof x[0])

// magic value to mark our content in eeprom
#define eeprom_magic 0xce

struct Command {
  // where to go first
  bool row_first;
  // coordinates
  unsigned char x;
  unsigned char y;
  // absolute time from start
  unsigned int time;

  void print() {
    Serial.print(row_first);
    Serial.print(" ");
    Serial.print(x);
    Serial.print(" ");
    Serial.print(y);
    Serial.print(" ");
    Serial.print(time);
    Serial.println();
  }

  bool empty() { return time == 0; }

  static Command createEmpty() { return {0, 0, 0, 0}; }
};

struct Status {
  // robot's heading
  enum Orientation { NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3 };

  unsigned char x;
  unsigned char y;
  Orientation orientation;

  bool empty() { return x == 255 && y == 255 && orientation == 5; }

  static Status createEmpty() { return {255, 255, Orientation(5)}; }
};

/* robot setup */
const static int basic_turn_ratio = 70;
const static int forward_speed = 100;

/* globals */
Servo left;
Servo right;
// readings from infrared sensors
bool infras[infra_high - infra_low];
Status status;
Status starting_position;
Command schedule[128];

// predefined schedule used as fallback
Status default_status = {0, 0, Status::NORTH};
Command default_schedule[] = {
    {0, 4, 0, 150}, {0, 1, 1, 350}, {1, 0, 2, 450},
    {1, 2, 3, 567}, {0, 3, 1, 700},
};

/**
 * robot controls
 */

/* stops robot */
void stopWheels() {
  left.write(1500);
  right.write(1500);
}

/* reads infra red sensors to infras array */
void readInfra() {
  for (int i = infra_low, j = 0; i < infra_high; ++i, ++j) {
    infras[j] = digitalRead(i);
  }
}

/* going forward, defaults to full speed */
void go(int speed = 200) {
  left.write(1500 + speed);
  right.write(1500 - speed);
}

/* inplace turning */
void turn(int slope) {
  left.write(1500 + slope);
  right.write(1500 + slope);
}

/* returns true if line is left to the robot */
bool lineLeft() { return !infras[1]; }

/* returns true if line is right to the robot */
bool lineRight() { return !infras[3]; }

/* returns true if line is properly aligned under robot */
bool lineBeneath() { return !infras[2] && infras[1] && infras[3]; }

/* returns true if line (crossroad) is in front of the robot */
bool lineInFront() { return !infras[0] || !infras[4]; }

/* turning on smoother curve */
void slopeTurn(int slope, int speed = 200) {
  left.write(1500 + speed + slope);
  right.write(1500 - speed + slope);
}

/* commands robots either to go forward or turn in such a way that robot will
 * follow the line in the next step */
void lineFollow() {
  // correct position according to line position
  if (lineBeneath()) {
    go(forward_speed);
  } else if (lineLeft()) {
    slopeTurn(-1 * basic_turn_ratio, forward_speed);
  } else if (lineRight()) {
    slopeTurn(basic_turn_ratio, forward_speed);
  } else { // finding line
    slopeTurn(-30);
  }
}

/* blinks a led on top of the robot */
void blinkLed(unsigned char repeats = 1) {
  for (unsigned char i = 0; i < repeats; ++i) {
    digitalWrite(led_pin, HIGH);
    delay(250);
    digitalWrite(led_pin, LOW);
    delay(250);
  }
}

/* returns true if button is pressed */
bool buttonDown() { return !digitalRead(button_pin); }

/* waits until the button is pressed */
void waitButtonPress() {
  while (!buttonDown()) {
  }
  // wait a bit to allow user put his finger up
  delay(1000);
}

/**
 * serial communication
 */

/* read 1 command from serial line. on error returns empty Command */
Command parseCommand() {
  // read first 2 characters - coordinates
  char a1 = Serial.read();
  char a2 = Serial.read();
  // read time
  unsigned int time = Serial.parseInt();
  // handle invalid commands
  if (time <= 0 || !(isAlpha(a1) ^ isAlpha(a2)) ||
      !(isDigit(a1) ^ isDigit(a2))) {
    return Command::createEmpty();
  }
  bool row_first = !isAlpha(a1);
  // normalize letters to upper case
  a1 = toupper(a1);
  a2 = toupper(a2);

  // create structure
  unsigned char x, y;
  // save coordinates at proper order
  if (!row_first) {
    x = a1 - 'A'; // convert row coordinate to number
    y = a2 - '1'; // we want to count cols from 0
  } else {
    x = a2 - 'A';
    y = a1 - '1';
  }

  return {row_first, x, y, time};
}

/* reads serial input until there is a character available other than whitespace
 */
void skipWhiteSpace() {
  // skip whitespace
  while (Serial.available() && isspace(Serial.peek())) {
    Serial.read();
  }
}

/* read special status line from serial line */
Status parseStatus() {
  char a1 = Serial.read();
  char a2 = Serial.read();
  char a3 = Serial.read();

  if (!isAlpha(a1) || !isDigit(a2) || !isAlpha(a3)) {
    return Status::createEmpty();
  }
  // normalize to uppercase
  a1 = toupper(a1);
  a3 = toupper(a3);

  Status status;
  status.x = a1 - 'A';
  status.y = a2 - '1';
  switch (a3) {
  case 'N':
    status.orientation = Status::NORTH;
    break;
  case 'S':
    status.orientation = Status::SOUTH;
    break;
  case 'E':
    status.orientation = Status::EAST;
    break;
  case 'W':
    status.orientation = Status::WEST;
    break;
  default:
    return Status::createEmpty();
  }

  return status;
}

/* parses schedule send via serial. returns false if shedule is invalid. */
bool readSchedule() {
  skipWhiteSpace();
  // first line is special
  status = parseStatus();
  if (status.empty()) {
    return false;
  }
  // zero out current schedule
  memset(schedule, 0, sizeof(schedule));
  unsigned char i = 0;
  while (Serial.available()) {
    if (i >= LENGTH(schedule)) {
      Serial.println("maximum length of schedule exceeded. some commands might "
                     "be stripped.");
      return true;
    }
    skipWhiteSpace();
    // read command
    if (Serial.available()) {
      schedule[i++] = parseCommand();
      if (schedule[i - 1].empty()) {
        return false;
      }
    }
  }

  return true;
}

/**
 * schedule persisting
 */

/* saves schedule to EEPROM */
void saveSchedule() {
  int address = 0;
  // magic to know if we have valid schedule in eeprom
  EEPROM[address++] = eeprom_magic;
  EEPROM.put(address, status);
  address += sizeof(status);
  EEPROM.put(address, schedule);
}

/* loads schedule either from EEPROM or from built-in failback schedule. If
 * button is down it always loads failback schedule. */
void loadSchedule() {
  if (EEPROM[0] != eeprom_magic || buttonDown()) {
    // we don't have our schedule in EEPROM, fall back to precompiled schedule
    status = default_status;
    memset(schedule, 0, sizeof(schedule));
    memcpy(schedule, default_schedule, sizeof(default_schedule));
    Serial.println("no schedule present in EEPROM (or manual override), "
                   "falling back to precompiled schedule.");
    blinkLed();
    return;
  }
  int address = 1;
  EEPROM.get(address, status);
  address += sizeof(status);
  EEPROM.get(address, schedule);
}

/**
 * schedule execution and planning
 */

/* robot robot left (direction = -1)  or right (direction = 1) 90 deg */
void rotateRobot(signed char direction) {
  turn(100 * direction);
  delay(400);
  do {
    readInfra();
  } while (!lineBeneath());
  stopWheels();
}

/* rotate robot right 90 deg */
void rotateRight() {
  Serial.println("R");
  rotateRobot(1);
}

/* rotate robot right 90 deg */
void rotateLeft() {
  Serial.println("L");
  rotateRobot(-1);
}

/* go forward 1 cell */
void goForward() {
  Serial.println("F");
  go(forward_speed);
  do {
    readInfra();
    lineFollow();
  } while (!lineInFront());
  go(forward_speed);
  delay(260);
  stopWheels();
}

/* rotate robot. after rotation is finished robot will face desired `orientation` */
void rotate(Status::Orientation orientation) {
  int diff = status.orientation - orientation;
  int dist = abs(diff) % 2;
  if (diff == 0) {
    // we are already rotated in desired direction
    return;
  }
  if (dist == 0) {
    // rotate 180 deg
    // we must take care on the corners, so we always have line after first
    // rotation.
    if ((status.x > 0 && status.y == 0) || (status.y > 0 && status.x == 0)) {
      rotateLeft();
      rotateLeft();
    } else {
      rotateRight();
      rotateRight();
    }
  } else {
    // rotate 90 deg
    if (diff == 1 || diff == -3) {
      rotateLeft();
    } else {
      rotateRight();
    }
  }
  status.orientation = orientation;
}

/* performs special action. currently it faces robot north. */
void doSpecialAction() { rotate(Status::NORTH); }

/* commands robot to go 1 cell NORTH in coordinate system */
void goNorth() {
  rotate(Status::NORTH);
  goForward();
  ++status.y;
}

/* commands robot to go 1 cell SOUTH in coordinate system */
void goSouth() {
  rotate(Status::SOUTH);
  goForward();
  --status.y;
}

/* commands robot to go 1 cell EAST in coordinate system */
void goEast() {
  rotate(Status::EAST);
  goForward();
  ++status.x;
}

/* commands robot to go 1 cell WEST in coordinate system */
void goWest() {
  rotate(Status::WEST);
  goForward();
  --status.x;
}

/* clear column distance between current position and goal by moving robot by
 * the row */
void goRow(const Command &goal) {
  int diff = goal.x - status.x;
  for (int i = 0; i < abs(diff); ++i) {
    if (diff > 0) {
      goEast();
    } else {
      goWest();
    }
  }
}

/* clear row distance between current position and goal by moving robot by the
 * column */
void goCol(const Command &goal) {
  int diff = goal.y - status.y;
  for (int i = 0; i < abs(diff); ++i) {
    if (diff > 0) {
      goNorth();
    } else {
      goSouth();
    }
  }
}

/* commands robot to follow path according to the schedule */
void executeSchedule() {
  starting_position = status; // save for later return
  unsigned long start_time = millis();
  for (Command *cmd = schedule; !cmd->empty(); ++cmd) {
    cmd->print();
    if (!cmd->row_first) {
      goRow(*cmd);
      goCol(*cmd);
    } else {
      goCol(*cmd);
      goRow(*cmd);
    }
    // wait until next command for amount specified in command
    while (cmd->time * 100 > millis() - start_time) {
      if (buttonDown()) {
        delay(500);
        doSpecialAction();
      }
      blinkLed();
    }
  }
}

/* commands robot to get back to the starting position */
void returnHome() {
  Serial.println("returning back to starting position.");
  // create command for starting pos
  Command cmd = {false, starting_position.x, starting_position.y, 0};
  goRow(cmd);
  goCol(cmd);
  rotate(starting_position.orientation);
}

/**
 * arduino entry points
 */

void setup() {
  // initialize serial and wait for port to open:
  Serial.begin(115200);
  left.attach(12);
  right.attach(13);
  // button
  pinMode(button_pin, INPUT_PULLUP);

  // led
  pinMode(led_pin, OUTPUT);

  stopWheels();
  // setup infa array
  for (int i = infra_low; i < infra_high; ++i) {
    pinMode(i, INPUT);
  }

  Serial.println("waiting for commands.");
  // wait 15 seconds to give user a chance to send new commands
  delay(15 * 1000);
  if (Serial.available()) {

    if (readSchedule()) {
      Serial.println("commands received, writing new commands.");
      saveSchedule();
    } else {
      Serial.println("invalid schedule. loading EEPROM contents.");
      loadSchedule();
    }
  } else {
    Serial.println("no commands, loading EEPROM contents.");
    loadSchedule();
  }

  for (int i = 0; schedule[i].time != 0; ++i) {
    schedule[i].print();
  }

  Serial.println("commands loaded, launching robot.");

  // blink 2 times to signal that we are ready
  blinkLed(2);
}

void loop() {
  waitButtonPress();
  executeSchedule();
  blinkLed(4);

  waitButtonPress();
  returnHome();
  blinkLed(4);
}

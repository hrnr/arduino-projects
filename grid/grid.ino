#include <Servo.h>
// use backported EEPROM library from arduino 1.8.1 with new `get` and `put`
// methods
#include "EEPROM.h"

#define button_pin 2
#define infra_low 3
#define infra_high 8

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
};

struct Status {
  // robot's heading
  enum Orientation { NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3 };

  unsigned char x;
  unsigned char y;
  Orientation orientation;
};

/* globals */
Servo left;
Servo right;
// readings from infrared sensors
bool infras[infra_high - infra_low];
Status status;
Status starting_position;
Command schedule[128];

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

bool buttonDown() { return !digitalRead(button_pin); }

/* waits until the button is pressed */
void waitButtonPress() {
  while (!buttonDown()) {
  }
  // wait a bit to allow user put his finger up
  delay(1000);
}

/* read 1 command from serial line */
Command parseCommand() {
  // read first 2 characters - coordinates
  char a1 = Serial.read();
  char a2 = Serial.read();
  // read time
  unsigned int time = Serial.parseInt();
  bool row_first = !isalpha(a1);

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

/* read special status line from serial line */
Status parseStatus() {
  char a1 = Serial.read();
  char a2 = Serial.read();
  char a3 = Serial.read();

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
  }

  return status;
}

void readSchedule() {
  // first line is special
  status = parseStatus();
  // zero out current schedule
  memset(schedule, 0, sizeof(schedule));
  int i = 0;
  while (Serial.available()) {
    // skip whitespace
    while (isspace(Serial.peek())) {
      Serial.read();
    }

    // read command
    if (Serial.available()) {
      schedule[i++] = parseCommand();
    }
  }
}

void saveSchedule() {
  int address = 0;
  EEPROM.put(address, status);
  address += sizeof(status);
  EEPROM.put(address, schedule);
}

void loadSchedule() {
  int address = 0;
  EEPROM.get(address, status);
  address += sizeof(status);
  EEPROM.get(address, schedule);
}

void rotateRight() { Serial.println("R"); }

void rotateLeft() { Serial.println("L"); }

void goForward() { Serial.println("F"); }

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

void goNorth() {
  rotate(Status::NORTH);
  goForward();
  ++status.y;
}

void goSouth() {
  rotate(Status::SOUTH);
  goForward();
  --status.y;
}

void goEast() {
  rotate(Status::EAST);
  goForward();
  ++status.x;
}

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

/* commands robot to follow path according to schedule */
void executeSchedule() {
  starting_position = status; // save for later return
  for (Command *cmd = schedule; !cmd->empty(); ++cmd) {
    cmd->print();
    if (!cmd->row_first) {
      goRow(*cmd);
      goCol(*cmd);
    } else {
      goCol(*cmd);
      goRow(*cmd);
    }
  }
}

/* commands robot to get back to starting position */
void returnHome() {
  Serial.println("returning back to starting position.");
  // create command for starting pos
  Command cmd = {false, starting_position.x, starting_position.y, 0};
  goRow(cmd);
  goCol(cmd);
  rotate(starting_position.orientation);
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

  Serial.println("waiting for commands.");
  // wait 15 seconds to give user a chance to send new commands
  delay(15 * 1000);
  if (Serial.available()) {
    Serial.println("commands received, writing new commands.");
    readSchedule();
    saveSchedule();
  } else {
    Serial.println("no commands, loading EEPROM contents.");
    loadSchedule();
  }

  for (int i = 0; schedule[i].time != 0; ++i) {
    schedule[i].print();
  }

  Serial.println("commands loaded, launching robot.");
}

void loop() {
  waitButtonPress();
  executeSchedule();

  waitButtonPress();
  returnHome();
}

#define avi_left 9
#define avi_right 10
#define fr_left 6
#define fr_right 7

#define MSG_SIZE 6

char msg_buffer[MSG_SIZE];
int speed_left = 0;
int speed_right = 0;

struct Msg {
  char id;
  int16_t left;
  int16_t right;
  uint8_t crc;
} __attribute__((__packed__));

void readMsg() {
  Serial.readBytes(msg_buffer, MSG_SIZE);
  Msg *msg = (Msg*)msg_buffer;
  if (msg->id != 'V') {
    return;
  }
  speed_left = msg->left;
  speed_right = msg->right;
}

void setSpeed() {
  // left wheel
  if (speed_left > 0) {
    // forward
    digitalWrite(fr_left, LOW);
    analogWrite(avi_left, speed_left);
  } else {
    // backward
    digitalWrite(fr_left, HIGH);
    analogWrite(avi_left, -1 * speed_left);
  }

  // right wheel
  if (speed_right > 0) {
    // forward
    digitalWrite(fr_right, HIGH);
    analogWrite(avi_right, speed_right);
  } else {
    // backward
    digitalWrite(fr_right, LOW);
    analogWrite(avi_right, -1 * speed_right);
  }
}

/* arduino  entry functions */
void setup() {
  // setup serial speed
  Serial.begin(57600);
  // left/right
  pinMode(fr_left, OUTPUT);
  pinMode(fr_right, OUTPUT);

  // stop robot
  setSpeed();
}

void loop() {
  if (Serial.available() >= MSG_SIZE) {
    readMsg();
    setSpeed();
  }
}

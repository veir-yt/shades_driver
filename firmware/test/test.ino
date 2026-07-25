
#include "shared.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Serial Test");
}

// define message scheme

// 1 byte, device
// 1 byte, command
// 4 bytes, value

void loop() {
  uint8_t device_id = 69;

  delay(2000);
  send_message(device_id, SET_ENABLE, ON);
  delay(2000);
  send_message(device_id, SET_DIR, CW);
  delay(2000);
  send_message(device_id, SET_STEPS, 1000);
  delay(2000);
  send_message(device_id, SET_ENABLE, OFF);
  delay(2000);

  send_message(device_id, SET_ENABLE, ON);
  delay(2000);
  send_message(device_id, SET_DIR, CCW);
  delay(2000);
  send_message(device_id, SET_STEPS, 2000);
  delay(2000);
  send_message(device_id, SET_ENABLE, OFF);
  delay(2000);
}


void send_message(uint8_t device_id, SHADE_COMMAND_ENUM command, uint32_t value) {
  uint8_t buf[6] = {0};

  buf[0] = device_id; // device id
  buf[1] = command; // command

  buf[2] = (value & 0x000000FF) >> 0;
  buf[3] = (value & 0x0000FF00) >> 8;
  buf[4] = (value & 0x00FF0000) >> 16;
  buf[5] = (value & 0xFF000000) >> 24;

  Serial.write(buf, 6);
}

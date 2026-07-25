#include "shared.h"
#include "TimerOne.h"

#define MOTOR_STEP 2
#define MOTOR_DIR 3
#define MOTOR_BOARD_ENABLE 5

#define MOTOR_CURRENT_SENSE A3

#define ANALOG_REF_V 5.0
#define ANALOG_REF_OHM 1.0
#define ANALOG_REF_PREC 1023

#define AVERAGE_COUNT 10.0

#define DEVICE_ID 69 // TODO, setup with dip switches

volatile float latest_current = 0.0;
volatile float average_current = 0.0;
volatile int latest_reading = 0;

volatile bool motor_enable = false;
volatile bool step = false;
volatile int remaining_steps = 0;

void setup() {
  pinMode(MOTOR_STEP, OUTPUT);
  pinMode(MOTOR_DIR, OUTPUT);

  pinMode(MOTOR_BOARD_ENABLE, OUTPUT);
  digitalWrite(MOTOR_BOARD_ENABLE, HIGH);

  pinMode(MOTOR_CURRENT_SENSE, INPUT);

  Serial.begin(115200);
  Serial.println("Motorrrr");

  Timer1.initialize(500); // run ever x us
  Timer1.attachInterrupt(motorStep);

  delay(2000);

  digitalWrite(MOTOR_BOARD_ENABLE, LOW);
}

// define message scheme

// 1 byte, device
// 1 byte, command
// 4 bytes, value

void loop() {
  readMotorCurrent();

  // comms from master device
  uint8_t device_id = 0;
  uint8_t command = 0;
  uint32_t value = 0;
  
  readMessage(&device_id, &command, &value);

  if (device_id > 0 && device_id == DEVICE_ID) {
    Serial.println(device_id);
    Serial.println(command);
    Serial.println(value);
    Serial.println(average_current);
    Serial.println("-----");

    switch (command) {
      case SET_DIR:
        command_set_dir(value);
        break;
      case SET_STEPS:
        command_set_steps(value);
        break;
      case SET_ENABLE:
        command_set_enable(value);
        break;
      default:
        // bruh
        break;
    }
  }


  delay(200);
}

void command_set_dir(uint32_t value) {
  if (value == CCW) {
    digitalWrite(MOTOR_DIR, HIGH);
  } else if (value == CW) {
    digitalWrite(MOTOR_DIR, LOW);
  }
}

void command_set_steps(uint32_t value) {
  remaining_steps = value;
}

void command_set_enable(uint32_t value) {
  if (value == ON) {
    motor_enable = true;
  } else if (value == OFF) {
    motor_enable = false;
  }
}










void readMessage(uint8_t * device_id, uint8_t * command, uint32_t * value) {
  uint8_t buffer[6] = {0};

  if (Serial.available() >= 6) { // probably null terminator from command prompt
    Serial.readBytes(buffer, 6);

    *device_id = buffer[0];
    *command = buffer[1];

    *value = 0;
    *value |= (buffer[2]) << 0;
    *value |= (buffer[3]) << 8;
    *value |= (buffer[4]) << 16;
    *value |= (buffer[5]) << 24;

    flushSerial();
  } else {
    flushSerial();
  }
}

void flushSerial() {
  while (Serial.available() > 0) {
    int x = Serial.read();
  }
}







void motorStep() {
  bool run_motor = (motor_enable) && (remaining_steps > 0);

  digitalWrite(MOTOR_BOARD_ENABLE, !run_motor); // active low

  if (run_motor) {
    step = !step;
    digitalWrite(MOTOR_STEP, step);
    
    if (!step) {
      remaining_steps--;
    }
  }
}

void readMotorCurrent() {
  int reading = analogRead(MOTOR_CURRENT_SENSE);
  
  // VEIR
  // V = IR
  // ref_voltage * (reading / ref_range) = I * ref_ohms

  float output = (ANALOG_REF_V * (float(reading) / float(ANALOG_REF_PREC))) / ANALOG_REF_OHM;

  latest_current = output;
  average_current = (average_current * ((AVERAGE_COUNT - 1.0) / AVERAGE_COUNT)) + (latest_current / AVERAGE_COUNT);
  latest_reading = reading;
}


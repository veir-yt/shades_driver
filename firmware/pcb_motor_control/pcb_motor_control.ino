#include "shared.h"
#include "TimerOne.h"

#define PIN_MOTOR_STEP 11
#define PIN_MOTOR_DIR 12
#define PIN_MOTOR_BOARD_ENABLE 10

#define PIN_MOTOR_CURRENT_SENSE A0

#define PIN_COMMS_READ_ENABLE 3
#define PIN_COMMS_WRITE_ENABLE 2

#define PIN_DIP_1 4
#define PIN_DIP_2 5
#define PIN_DIP_3 6
#define PIN_DIP_4 7

#define ANALOG_REF_V 2.56
#define ANALOG_REF_OHM 0.47
#define ANALOG_REF_PREC 1023

#define AVERAGE_COUNT 10.0

uint8_t DEVICE_ID = 0;

volatile float latest_current = 0.0;
volatile float average_current = 0.0;

#define INTERRUPT_PERIOD_US 10

volatile bool motor_enable = false;
volatile bool motor_step_toggle = false;
volatile int motor_remaining_steps = 0;
volatile int motor_period = 500; // period between step toggle in us (min 500, max inf)
volatile int motor_elapsed_time = 0;

void setup() {
  motor_setup();

  comms_setup();

  DEVICE_ID = get_device_id();

  Serial.println(DEVICE_ID); // TODO remove

  delay(100);

  Timer1.initialize(INTERRUPT_PERIOD_US); // run every x us
  Timer1.attachInterrupt(fast_interrupt);
}

void motor_setup() {
  pinMode(PIN_MOTOR_STEP, OUTPUT);
  pinMode(PIN_MOTOR_DIR, OUTPUT);

  pinMode(PIN_MOTOR_BOARD_ENABLE, OUTPUT);
  digitalWrite(PIN_MOTOR_BOARD_ENABLE, HIGH); // active low

  pinMode(PIN_MOTOR_CURRENT_SENSE, INPUT);
}

void comms_setup() {
  pinMode(PIN_COMMS_READ_ENABLE, OUTPUT);
  digitalWrite(PIN_COMMS_READ_ENABLE, LOW); // active low

  pinMode(PIN_COMMS_WRITE_ENABLE, OUTPUT);
  digitalWrite(PIN_COMMS_WRITE_ENABLE, LOW); // active high

  delay(100);

  Serial.begin(SERIAL_SPEED);

  delay(100);
}

// using dip switches to set a binary device id (pullup aka active low)
uint8_t get_device_id() {
  pinMode(PIN_DIP_1, INPUT_PULLUP);
  pinMode(PIN_DIP_2, INPUT_PULLUP);
  pinMode(PIN_DIP_3, INPUT_PULLUP);
  pinMode(PIN_DIP_4, INPUT_PULLUP);

  delay(100);

  uint8_t d1 = !digitalRead(PIN_DIP_1);
  uint8_t d2 = !digitalRead(PIN_DIP_2);
  uint8_t d3 = !digitalRead(PIN_DIP_3);
  uint8_t d4 = !digitalRead(PIN_DIP_4);

  uint32_t out = 0;

  out |= (d1 << 0);
  out |= (d2 << 1);
  out |= (d3 << 2);
  out |= (d4 << 3);

  return out;
}

void loop() {
  readMotorCurrent();

  // comms from master device
  uint8_t device_id = 0;
  uint8_t command = 0;
  uint32_t value = 0;
  
  readMessage(&device_id, &command, &value);

  handleMessage(device_id, command, value);

  delay(100);
}

// force each unit to have non 0 device_id
void handleMessage(uint8_t device_id, uint8_t command, uint32_t value) {
  if (DEVICE_ID == 0) return; // invalid hardware

  if (device_id > 0 && device_id != DEVICE_ID) return; // non 0 device id didn't match hardware

  switch (command) {
    case SCE_SET_DIR:
      sce_set_dir(value);
      break;
    case SCE_SET_STEPS:
      sce_set_steps(value);
      break;
    case SCE_SET_ENABLE:
      sce_set_enable(value);
      break;
    case SCE_SET_PERIOD:
      sce_set_period(value);
      break;
    default:
      // bruh
      break;
  }
}

void sce_set_dir(uint32_t value) {
  if (value == MDE_CCW) {
    digitalWrite(PIN_MOTOR_DIR, HIGH);
  } else if (value == MDE_CW) {
    digitalWrite(PIN_MOTOR_DIR, LOW);
  }
}

void sce_set_steps(uint32_t value) {
  motor_remaining_steps = value;
}

void sce_set_enable(uint32_t value) {
  if (value == BSE_ON) {
    motor_enable = true;
  } else if (value == BSE_OFF) {
    motor_enable = false;
    motor_step_toggle = false; // cleanup
    motor_remaining_steps = 0; // cleanup
  }
}

void sce_set_period(uint32_t value) {
  motor_period = MIN(value, 500);
}

void fast_interrupt() {
  motor_elapsed_time += INTERRUPT_PERIOD_US;

  if (motor_elapsed_time > motor_period) {
    motor_step();
    motor_elapsed_time = 0;
  }
}

void motor_step() {
  bool run_motor = (motor_enable) && (motor_remaining_steps > 0);

  // TODO FAST WRITE
  digitalWrite(PIN_MOTOR_BOARD_ENABLE, !run_motor); // active low

  if (!run_motor) return;

  motor_step_toggle = !motor_step_toggle;
  // TODO FAST WRITE
  digitalWrite(PIN_MOTOR_STEP, motor_step_toggle);
  
  if (motor_step_toggle) return; // after 2 toggles, dec steps

  motor_remaining_steps--;
}

// Definition of message scheme:
// 1 byte, device
// 1 byte, command
// 4 bytes, value

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

void readMotorCurrent() {
  int reading = analogRead(PIN_MOTOR_CURRENT_SENSE);
  
  // VEIR
  // V = IR
  // V = I * ref_ohms
  // ref_voltage * (reading / ref_range) = I * ref_ohms
  // I = (ref_voltage * (reading / ref_range)) / ref_ohms

  float output = (ANALOG_REF_V * (float(reading) / float(ANALOG_REF_PREC))) / ANALOG_REF_OHM;

  latest_current = output;
  average_current = (average_current * ((AVERAGE_COUNT - 1.0) / AVERAGE_COUNT)) + (latest_current / AVERAGE_COUNT);
}


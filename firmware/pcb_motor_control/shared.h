
#ifndef SHARED_H
#define SHARED_H

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define SERIAL_SPEED 115200

enum SHADE_COMMAND_ENUM : uint8_t {
  SCE_SET_DIR = 1,
  SCE_SET_STEPS = 2,
  SCE_SET_ENABLE = 3,
  SCE_SET_PERIOD = 4,

  // can get jog function if steps = 50,000 / period

  // add more like
  //   get current,
  //   get motor state (nee to add motor state),

  // home? absolute positions?? (then these are stored on the master in memory???)
  // set top, set botom (after jogging)
  // can we record pos of each on master
};

enum MOTOR_DIR_ENUM : uint8_t {
  MDE_CCW = 1,
  MDE_CW = 2,
};

enum BOOLEAN_STATE_ENUM : uint8_t {
  BSE_ON = 1,
  BSE_OFF = 2,
};

#endif


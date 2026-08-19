#ifndef MOTOR_H
#define MOTOR_H
#include <Arduino.h>
#include <hardware/watchdog.h>
#include <pico/bootrom.h>
#include <pico/time.h>
//#include <pico/stdlib.h>
#include <hardware/pwm.h>
#include "motor_config.h"

#if DEBUG_SERIAL
  #define DBG(x) Serial.print(x)
  #define DBGL(x) Serial.println(x)
#else
  #define DBG(x)
  #define DBGL(x)
#endif


#if USE_CLASSICAL_PID
  #include <PID_v1.h>
  using PID_t = PID;
#else
  #include "PID_vel.h"
  using PID_t = PID_vel;
#endif






enum class MotorStatus {
    STOP_REST, HOLD, RUN, ZERO_INTERPOL, NORM_INTERPOL, TIME_OUT, ACCELERATION, DECELERATION
};

enum class operationMode {
    MANUAL_MODE,
    PARALLEL_MOVEMENT,
    SEQUENTIAL_MOVEMENT
};

class motor {
public:
  motor(int id, int pwm_pin, int forward_pin, int backward_pin,
        int encoderA_pin, int encoderB_pin, int potentiometer_jogging_pin, 
        int zero_switch_pin, double Kp, double Ki, double Kd, int pidSampleTime, volatile unsigned EncoderFilterMicroSecond);
  ~motor();

  void begin();
  void updateEncoder();
  void setMotor();                    // Uses internal output value
  void printDebug();
  void printMode() const;

  void setMode(operationMode newMode) { mode = newMode; };
  void initializeMotorInAuto() ;
  void initializeMotorInManual() ;
  void initializePwm();

  int getId() { return id; };
  operationMode getMode() const { return mode; };
  void setSetpoint(double inputValue) { set_value = inputValue; };
  void setStatus(MotorStatus statusValue) { motorStatus = statusValue; };
  double getSetpoint() { return set_value; };
  MotorStatus getStatus() { return motorStatus; };
  volatile long getEncoderCount() { return EncoderCounter; };
  volatile long setEncoderCount(long EncoderValue) { return EncoderCounter = EncoderValue; };  
  void setOutput(double value) { output = value; };
  double getOutput() { return output; };
  double setMaxSpeed ( double maxSpeed) { return motorMaxSpeed = maxSpeed; };
  double setMinSpeed ( double minSpeed) { return motorMinSpeed = minSpeed; };
  double getMaxSpeed () { return motorMaxSpeed; };
  double getMinSpeed () { return motorMinSpeed; };
  bool getIntorpolationStatus() { return interpolationSuccessfullyDone; };
  void setIntorpolationStatus(bool status) { interpolationSuccessfullyDone = status; };
  bool getZeroInterpolation() { return zeroInterpolationDone; };
  bool getZeroSwitchPinState() { return zeroSwitchTriggered = digitalRead(zero_switch_pin); };
  bool zeroInterpolationFunction();
  int getFilterCount() { return filterCount; };
  int getPwm_pin() { return pwm_pin; };
  int getForward_pin() { return forward_pin; };
  int getBackward_pin() { return backward_pin; };
  int getPidSampleTime() { return pidSampleTime; };
  double getPosition();
  double getError();
  void forceStop();
  void resetControlState();
  PID_t* getPID() { return PID_FUNCTION; }

  // static ISRs (one pair per potential motor)
  static void handleInterruptA0();
  static void handleInterruptB0();
  static void handleInterruptA1();
  static void handleInterruptB1();
  static void handleInterruptA2();
  static void handleInterruptB2();
  static void handleInterruptA3();
  static void handleInterruptB3();
  static void handleInterruptA4();
  static void handleInterruptB4();

private:
  int id;
  int pwm_pin;
  int forward_pin;
  int backward_pin;
  int encoderA_pin;
  int encoderB_pin;
  int potentiometer_jogging_pin;
  int zero_switch_pin;

  MotorStatus motorStatus;
  operationMode mode;
  int EncoderCountsPerRevolution;
  int pidSampleTime;

  bool interpolationSuccessfullyDone;
  bool zeroInterpolationDone;
  bool zeroSwitchTriggered;
  float pwmDutyCycle = 250.0;

  double motorMaxSpeed, motorMinSpeed;
  double Kp, Ki, Kd;    //// Ki = Ti, Kd = Td if PID velocity uses
  double set_value;
  double error;
  double output;

  volatile uint8_t prevState, currState;
  volatile unsigned long lastEncoderMicros;
  volatile long EncoderCounter;
  double current_position;
  volatile unsigned EncoderFilterMicroSecond;

  double potValue, previous_potValue;
  unsigned long last_time;
  int filterCount, numberOfCoefficient;
  int potFilterCounter;

  PID_t* PID_FUNCTION;
  static motor* motorInstances[MAX_NUM_MOTORS];

};

#endif


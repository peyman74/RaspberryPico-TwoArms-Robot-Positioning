#include "motor.h"

// define motorInstances array
motor* motor::motorInstances[MAX_NUM_MOTORS] = { nullptr };

motor::motor(int id, int pwm_pin, int forward_pin, int backward_pin,
             int encoderA_pin, int encoderB_pin, int potentiometer_jogging_pin,
             int zero_switch_pin, double Kp, double Ki, double Kd, int pidSampleTime, volatile unsigned EncoderFilterMicroSecond)
 : id(id),
   pwm_pin(pwm_pin),
   forward_pin(forward_pin),
   backward_pin(backward_pin),
   encoderA_pin(encoderA_pin),
   encoderB_pin(encoderB_pin),
   potentiometer_jogging_pin(potentiometer_jogging_pin),
   zero_switch_pin(zero_switch_pin),
   Kp(Kp), Ki(Ki), Kd(Kd),
   motorMaxSpeed(MAX_SPEED), motorMinSpeed(MOTOR_DEAD_BAND_SPEED),
   EncoderCountsPerRevolution(COUNT_PER_REVOLUTION),
   pidSampleTime(pidSampleTime),
   pwmDutyCycle(250.0),
   interpolationSuccessfullyDone(true),
   zeroInterpolationDone(false),
   zeroSwitchTriggered(false),
   prevState(0),
   currState(0),
   lastEncoderMicros(0),
   EncoderCounter(0),
   set_value(0),
   current_position(0),
   EncoderFilterMicroSecond(EncoderFilterMicroSecond)
{
  motorStatus = MotorStatus::STOP_REST;
  mode = operationMode::SEQUENTIAL_MOVEMENT;

  pinMode(pwm_pin, OUTPUT);
  pinMode(forward_pin, OUTPUT);
  pinMode(backward_pin, OUTPUT);
  pinMode(encoderA_pin, INPUT_PULLUP);
  pinMode(encoderB_pin, INPUT_PULLUP);
  pinMode(potentiometer_jogging_pin, INPUT);
  pinMode(zero_switch_pin, INPUT);  

  initializePwm();

  PID_FUNCTION = new PID_t(&current_position, &output, &set_value, Kp, Ki, Kd, P_ON_E, DIRECT);

  PID_FUNCTION->SetSampleTime(pidSampleTime);
  PID_FUNCTION->SetMode(AUTOMATIC);
  PID_FUNCTION->SetOutputLimits(-motorMaxSpeed, motorMaxSpeed);

  if (id < MAX_NUM_MOTORS) motorInstances[id] = this;
}

motor::~motor() {
  delete PID_FUNCTION;
}

void motor::begin() {
    // Attach interrupts only for this motor id
    switch (id) {
      case 0:
        attachInterrupt(digitalPinToInterrupt(encoderA_pin), motor::handleInterruptA0, CHANGE);
        attachInterrupt(digitalPinToInterrupt(encoderB_pin), motor::handleInterruptB0, CHANGE);
        break;
      case 1:
        attachInterrupt(digitalPinToInterrupt(encoderA_pin), motor::handleInterruptA1, CHANGE);
        attachInterrupt(digitalPinToInterrupt(encoderB_pin), motor::handleInterruptB1, CHANGE);
        break;
      case 2:
        attachInterrupt(digitalPinToInterrupt(encoderA_pin), motor::handleInterruptA2, CHANGE);
        attachInterrupt(digitalPinToInterrupt(encoderB_pin), motor::handleInterruptB2, CHANGE);
        break;
      case 3:
        attachInterrupt(digitalPinToInterrupt(encoderA_pin), motor::handleInterruptA3, CHANGE);
        attachInterrupt(digitalPinToInterrupt(encoderB_pin), motor::handleInterruptB3, CHANGE);
        break;
      case 4:
        attachInterrupt(digitalPinToInterrupt(encoderA_pin), motor::handleInterruptA4, CHANGE);
        attachInterrupt(digitalPinToInterrupt(encoderB_pin), motor::handleInterruptB4, CHANGE);
        break;
    }
}

// create ISR pairs that only call the corresponding motor->updateEncoder()
#define MAKE_ISR_PAIR(N) \
  void motor::handleInterruptA##N() { if (motor::motorInstances[N]) motor::motorInstances[N]->updateEncoder(); } \
  void motor::handleInterruptB##N() { if (motor::motorInstances[N]) motor::motorInstances[N]->updateEncoder(); }

MAKE_ISR_PAIR(0)
MAKE_ISR_PAIR(1)
MAKE_ISR_PAIR(2)
MAKE_ISR_PAIR(3)
MAKE_ISR_PAIR(4)

void motor::updateEncoder() {


#if USE_ENCODER_FILTER   // Filter: ignore too-fast interrupts
  unsigned long now = micros();
  unsigned long delta = now - lastEncoderMicros;

  if (delta < EncoderFilterMicroSecond) {
    return;   // ignore noise
  }

  lastEncoderMicros = now;     // update timestamp for next interrupt 
#endif

  int MSB = digitalRead(encoderA_pin);
  int LSB = digitalRead(encoderB_pin);
  currState = (MSB << 1) | LSB;
  int sum = (prevState << 2) | currState;

  switch (sum) {
    case 0b1101: case 0b0100: case 0b0010: case 0b1011:
      EncoderCounter--;
      break;
    case 0b1110: case 0b0111: case 0b0001: case 0b1000:
      EncoderCounter++;
      break;
  }

  prevState = currState;
}

double motor::getPosition() {
  noInterrupts();
  current_position = (EncoderCounter * 360.0) / (long) EncoderCountsPerRevolution;
  interrupts();
  return current_position;
}

double motor::getError() {
  error = set_value - getPosition();
  return error;
}

void motor::initializePwm() {
  // Keep your original Raspberry Pi Pico PWM initialization logic
  gpio_set_function(pwm_pin, GPIO_FUNC_PWM); 

  int slice_pan = pwm_gpio_to_slice_num(pwm_pin);
  pwm_set_wrap(slice_pan, 1023);

  // set clock divider appropriate (original values preserved)
  pwm_set_clkdiv(slice_pan, 129.8f); // example to ~1kHz as you had

  pwm_set_enabled(slice_pan, true);

  pwm_set_gpio_level(pwm_pin, (int)pwmDutyCycle);
}

void motor::initializeMotorInAuto() {
  interpolationSuccessfullyDone = false;
  filterCount=0;
  potFilterCounter=0;
}

void motor::initializeMotorInManual() {
  previous_potValue = analogRead(potentiometer_jogging_pin);
  interpolationSuccessfullyDone = true;
  filterCount=0;
  potFilterCounter=0;
}

void motor::setMotor() {
   int pwmVal = map(abs(output), 0, motorMaxSpeed, motorMinSpeed, motorMaxSpeed);
 // int pwmVal = constrain(abs(output), motorMinSpeed, motorMaxSpeed);  
  if (output > 0) {
    pwm_set_gpio_level(pwm_pin, pwmVal);
    digitalWrite(forward_pin, HIGH);
    digitalWrite(backward_pin, LOW);
  } else if (output < 0) {
    pwm_set_gpio_level(pwm_pin, pwmVal);
    digitalWrite(forward_pin, LOW);
    digitalWrite(backward_pin, HIGH);
  } else {
    pwm_set_gpio_level(pwm_pin, 0);
    digitalWrite(forward_pin, LOW);
    digitalWrite(backward_pin, LOW);
  }
}


void motor::printDebug() {
    DBGL();
    DBG(" id: ");
    DBG(id);    
    DBG(" | Setpoint: ");
    DBG(getSetpoint());
    DBG(" | Encoder: ");
    DBG(getEncoderCount());

    DBG(" | Position: ");
    DBG(getPosition());
    DBG(" | Error: ");
    DBG(getError());
    DBG(" | Output: ");
    DBG(getOutput());
}

void motor::printMode() const {
  switch (mode) {
    case operationMode::MANUAL_MODE: 
      DBG("Mode: Manual");
      break;
    case operationMode::PARALLEL_MOVEMENT: 
      DBG("Mode: Parallel Movement");
      break;
    case operationMode::SEQUENTIAL_MOVEMENT: 
      DBG("Mode: Sequential Movement");
      break;
  }
}

bool motor::zeroInterpolationFunction() {
  //define later
  return false;
}
void motor::forceStop() {
  set_value = 0;  // may earse afterwards if i used this outside of emergency function
  output = 0;
  motor::setMotor();
  EncoderCounter = 0;
  motorStatus = MotorStatus::STOP_REST;
}

void motor::resetControlState() {
  interpolationSuccessfullyDone = false;
  zeroInterpolationDone = false;
  output = 0;
  set_value = 0;
  EncoderCounter = 0;

  if (PID_FUNCTION) {
    PID_FUNCTION->SetMode(MANUAL);     // stop PID, clear ITerm
    PID_FUNCTION->SetMode(AUTOMATIC); // restart clean
  }
}

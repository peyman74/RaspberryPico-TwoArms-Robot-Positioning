//Strategy 2, use Velocity: if it did not work change Kp to look up table linear changing
// output_k =  output_k-1 + Kp * (error - error_1) + Kp * T / Ti * error + Kp * Td / T * (error - 2 * error_1 - error_2)  // Another writing form of PID formula right point integration
// output_k =  output_k-1 + Kp * (1 +  Td/T) * error + Kp * (T/Ti - 1 - 2*Td/T) * error_1 + Kp * Td/T * error_2 ); right point
// Peyman Edalatjoo – HKA – October 2025
// 07 /11/ 2025  testest with T 10, 50, 100 ms, diffrent K factors. for T 100: Kp 2 or 3 , Ti 6 Td 0 but it still oscilating! applied The Ziegler–Nichols closed-loop tuning algorithm
// but did not worked (t 10ms Kp=3 leads oscilating with Pu=0.777 sec period so Kp=0.45 * 3 = 1.35, Ti= 0.777/2 = 0.4 ! anyway did not work!)
// the best answer (lower Oscilation) in T= 0.01s (10ms) are Kp=6 Ti=0.6 (60*T !!), Td=0
//lets try dynamic Kp according gap! in another ver!
#include <Arduino.h>
#include <pico/multicore.h>
#include "hardware/pwm.h"

#define MAX_SPEED 1023
#define PWM_MIN_SPEED 400
#define PID_SAMPLE_TIME 10
#define BOUNCING 2
#define POT_BOUNC_FILTER_NO 5
#define POT_MAX_DEVIATION 10
#define LED_PIN LED_BUILTIN

const int EncoderCountsPerRevolution = 4000;
//  double pShaftDegree = panEncoderCount * 0.083720930;  // -1 <= 'Error' <= 1  for Prof. Samuel,  it looks 360/0.083720930 = 4301  !!!

// Pin configuration for one motor
const int pwm_pin = 10;
const int forward_pin = 14;
const int backward_pin = 15;
const int encoderA_pin = 18;
const int encoderB_pin = 19;
const int pot_pin = 26;

// PID and motor state variables
double pidSampleTime = 0.01; //10ms
double Kp = 6.0, Ti = 0.6, Td = 0, KP=0, KI=0, KD=0,  constant1, constant2, constant3;
//double Kp = 1.0, Ti = 0.2, Td = 0, KP=0, KI=0, KD=0,  constant1, constant2, constant3;

double setpoint = 0;
double error = 0, error_1 = 0, error_2 = 0, output = 0, motorMaxSpeed = 512, motorMinSpeed = 0;
double current_position = 0;
volatile long EncoderCounter = 0;
volatile uint8_t prevState = 0, currState = 0;

unsigned long last_time = 0;
int filterCount = 0;
bool interpolationDone = true;
int potFilterCounter = 0;
double potValue = 0, previous_potValue = 0;

//--- Use a larger range for dutyCycle
float panDutyCycle = 250.0; // Valor mínimo para mover o motor. Era 0.0. A value between 0.0 and 1023.0 (10-bit range)
int16_t panDutyCyclei;
// Set the 10-bit PWM value directly
//pwm_set_gpio_level(panPwmPin, (int)dutyCycle);
//---
void pwmInit();
// Encoder interrupt service
void updateEncoder() {
  uint8_t a = digitalRead(encoderA_pin);
  uint8_t b = digitalRead(encoderB_pin);
  currState = (a << 1) | b;
  uint8_t sum = (prevState << 2) | currState;

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

double getPosition() {
  noInterrupts();
  long count = EncoderCounter;
  interrupts();
  return (count * 360.0) / EncoderCountsPerRevolution;
}

void setMotor(double out) {
  int pwmVal = map(abs(out), 0, 1023, PWM_MIN_SPEED, MAX_SPEED);
  if (out > 0) {
    pwm_set_gpio_level(pwm_pin, pwmVal);
//    pwm_set_gpio_level(pwm_pin, constrain(out,PWM_MIN_SPEED, MAX_SPEED));
    digitalWrite(forward_pin, HIGH);
    digitalWrite(backward_pin, LOW);
  } else if (out < 0) {
    pwm_set_gpio_level(pwm_pin, pwmVal);
//    pwm_set_gpio_level(pwm_pin, constrain(-out,PWM_MIN_SPEED, MAX_SPEED));
    digitalWrite(forward_pin, LOW);
    digitalWrite(backward_pin, HIGH);
  } else {
    pwm_set_gpio_level(pwm_pin, 0);
    digitalWrite(forward_pin, LOW);
    digitalWrite(backward_pin, LOW);
  }
}

bool PID_Function() {
  unsigned long now = millis();
  if (now - last_time >= PID_SAMPLE_TIME) {

    current_position = getPosition();
    current_position = 0.8 * current_position + 0.2 * getPosition();
    //current_position = (getPosition() + lastPos1 + lastPos2) / 3;
    error = setpoint - current_position;

    // PID midpoint integration
    output += constant1 * error + constant2 * error_1 + constant3 * error_2 ;
    error_2 = error_1;
    error_1 = error;
    output = constrain(output, -motorMaxSpeed, motorMaxSpeed);
    if (fabs(error) < 0.5)
      filterCount++;
    else
      filterCount = 0;

    if (filterCount >= BOUNCING) {
      interpolationDone = true;
      output = 0;
      filterCount = 0;
    }
    Serial.print("Setpoint: ");
    Serial.print(setpoint);
    Serial.print(" | EncCount: ");
    Serial.print(EncoderCounter);
    Serial.print(" | Pos: ");
    Serial.print(current_position);
    Serial.print(" | Err: ");
    Serial.print(error);
    Serial.print(" | Out: ");
    Serial.println(output);
    setMotor((int)output); 
    last_time = now;    
  } 
//  setMotor((int)output);
  return interpolationDone;
}


void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(pwm_pin, OUTPUT);
  pinMode(forward_pin, OUTPUT);
  pinMode(backward_pin, OUTPUT);
  pinMode(encoderA_pin, INPUT_PULLUP);
  pinMode(encoderB_pin, INPUT_PULLUP);
  pinMode(pot_pin, INPUT);
  pinMode(LED_PIN, OUTPUT);
  // Disable internal temperature sensor to avoid noise
  adc_set_temp_sensor_enabled(false);
  pwmInit();
 
  prevState = (digitalRead(encoderA_pin) << 1) | digitalRead(encoderB_pin);
  attachInterrupt(digitalPinToInterrupt(encoderA_pin), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderB_pin), updateEncoder, CHANGE);

  Serial.println("Enter commands: S <setpoint> <Kp> <Ti> <Td>");
}

void loop() {

  // --- Serial Command Input ---
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();
    if (cmd.startsWith("S")) {
      Serial.print("\n Enterred S");
      double newSetpoint, newKp, newTi, newTd;
      sscanf(cmd.c_str(), "S %lf %lf %lf %lf", &newSetpoint, &newKp, &newTi, &newTd);
      setpoint = constrain(newSetpoint, 0, 360);
      Kp = newKp;
      Ti = newTi;
      Td = newTd;
      // output += Kp * (1 +  Td/T) * error + Kp * (T/Ti - 1 - 2*Td/T) * error_1 + Kp * Td/T * error_2 ); or
      // output += Kp * (1 +  Kd) * error + Kp * (Ki - 1 - 2*Kd) * error_1 + Kp * Kd * error_2 ); 
      constant1 = Kp * (1 + Td/pidSampleTime);
      if (Ti != 0) constant2 =  Kp * (pidSampleTime/Ti - 1 - 2*Td/pidSampleTime);
      constant3 = Kp * Td/pidSampleTime;
      //    output += constant1 * error + constant2 * error_1 + constant3 * error_2 ;
      interpolationDone = false;  
      PrintSerial();
      delay(2000);
    }
  }
  // --- Run PID loop ---
//  if (!interpolationDone)
  PID_Function();
  setMotor(output);


}
void PrintSerial(){
    Serial.print("Setpoint=");
    Serial.print(setpoint);
    Serial.print(", Kp=");
    Serial.print(Kp);
    Serial.print(", Ti=");
    Serial.print(Ti);
    Serial.print(", Td=");
    Serial.println(Td);
    Serial.print(", Costants: ");
    Serial.println(constant1);
    Serial.println(constant2);
    Serial.println(constant3);
}

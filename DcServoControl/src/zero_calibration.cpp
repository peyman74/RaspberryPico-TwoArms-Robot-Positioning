/**
 * @file zero_calibration.cpp
 * @brief Implementation of zero position calibration functions
 */

#include "zero_calibration.h"
#include <Arduino.h>

// State machine variables for non-blocking mode
ZeroState zeroState = ZERO_IDLE;
int zeroMotorIndex = 0;
unsigned long zeroTimestamp = 0;

/**
 * @brief Blocking zero search - processes all motors sequentially
 * 
 * For each motor that needs calibration:
 * 1. Move backward until zero switch triggered
 * 2. Stop motor
 * 3. Move forward for 250ms to park position
 * 4. Reset encoder count to 0
 * 
 * Monitors emergency during process and updates watchdog.
 */
bool zeroSearchMotors() {
  DBGL("Starting zero interpolation for all motors...");
  zeroInterpolationDone = false;
  
  // Set all motors to ZERO_INTERPOL mode
  for (int i = 0; i < numMotors; i++) {
    motors[i]->setMinSpeed(0);
    motors[i]->setStatus(MotorStatus::ZERO_INTERPOL);
    motors[i]->getPID()->SetMode(MANUAL);    // stop PID winding up integral part inside-- refer to pid code
  }

  // Process each motor sequentially
  for (int i = 0; i < numMotors; i++) {
    if (!motors[i]->getZeroInterpolation()) { 
      DBG("Motor ");
      DBG(i);
      DBGL(" searching for zero...");
      
      // Move backward until zero switch is triggered
      while(motors[i]->getZeroSwitchPinState()) {
        // Check for emergency during calibration
        extern void checkAndHandleEmergency();
        checkAndHandleEmergency();
        if (emergencyTriggered) 
          return false;
        motors[i]->setOutput(-ZERO_INTERPOLATION_FEED_MIN);
        motors[i]->setMotor();
        watchdog_update();  // Prevent watchdog timeout
      }
      motors[i]->setOutput(0);
      motors[i]->setMotor();
      delay(100); // debounce delay
      
      // Move back forward slowly until zero switch is released
      // Alternative approach: just go back a little bit (current implementation)
      /* while(!motors[i]->getZeroSwitchPinState()) {
        motors[i]->setOutput(ZERO_INTERPOLATION_FEED_MAX);
        motors[i]->setMotor();
        watchdog_update();  // Prevent watchdog timeout
        DBGL(" Motor roll is backing to park state..."); 
      } */
      
      motors[i]->setOutput(ZERO_INTERPOLATION_FEED_MAX);      
      motors[i]->setMotor();
      delay(250); // get a little distance from zero switch
      motors[i]->setOutput(0); 
      motors[i]->setMotor();
      delay(100); // debounce delay
      motors[i]->setIntorpolationStatus(true);
      motors[i]->setEncoderCount(0);
      motors[i]->setStatus(MotorStatus::STOP_REST);
      
      DBG("Motor ");
      DBG(i);
      DBGL(" zero interpolation done. Encoder reset to 0.");
    }
  } 
  
  // Restore normal operating parameters for all motors
  zeroInterpolationDone = true;
  for (int i = 0; i < numMotors; i++){
    motors[i]->setMinSpeed(MOTOR_DEAD_BAND_SPEED);
    motors[i]->getPID()->SetMode(AUTOMATIC); 
  } 
      
  DBGL("Zero interpolation completed for all motors.");
  return true;
}

/**
 * @brief Non-blocking zero calibration - state machine version
 * 
 * Must be called repeatedly from main loop. Processes one state per call.
 * Allows emergency monitoring and prevents watchdog timeouts during
 * long calibration sequences.
 * 
 * Industrial-style implementation: safe, maintainable, non-blocking.
 */
void zeroSearchMotorsNonBlocking() {

  switch (zeroState) {

    case ZERO_IDLE:
      // Initialize calibration sequence
      zeroInterpolationDone = false;
      zeroMotorIndex = 0;
      zeroState = ZERO_PREPARE;
      DBGL("Zero interpolation started (non-blocking)");
      break;

    case ZERO_PREPARE:
      // Setup current motor for calibration
      motors[zeroMotorIndex]->getPID()->SetMode(MANUAL);
      motors[zeroMotorIndex]->setMinSpeed(0);
      motors[zeroMotorIndex]->setStatus(MotorStatus::ZERO_INTERPOL);
      DBGL("Zero preparation (non-blocking)");
      zeroState = ZERO_BACKWARD;
      break;

    case ZERO_BACKWARD:
      // Move backward until zero switch triggered
      if (!motors[zeroMotorIndex]->getZeroSwitchPinState()) {
        // Switch triggered - stop and transition to forward
        motors[zeroMotorIndex]->setOutput(0); 
        motors[zeroMotorIndex]->setMotor();      
        zeroTimestamp = millis();
        zeroState = ZERO_FORWARD;
      } else {
        // Keep moving backward
        motors[zeroMotorIndex]->setOutput(-ZERO_INTERPOLATION_FEED_MIN);   
        motors[zeroMotorIndex]->setMotor();
        DBGL("ZERO_BACKWARD (non-blocking)");  
      }
      break;

    case ZERO_FORWARD:
      // Move forward for 250ms to park position
      if (millis() - zeroTimestamp < 250) {
        motors[zeroMotorIndex]->setOutput(ZERO_INTERPOLATION_FEED_MAX);  
        motors[zeroMotorIndex]->setMotor();
        DBGL("ZERO_FORWARD (non-blocking)");
      } else {
        // Park time elapsed - stop motor
        motors[zeroMotorIndex]->setOutput(0);        
        motors[zeroMotorIndex]->setMotor();          
        zeroState = ZERO_FINALIZE;
      }
      break;

    case ZERO_FINALIZE:
      // Reset encoder and mark motor as calibrated
      motors[zeroMotorIndex]->setEncoderCount(0);
      motors[zeroMotorIndex]->setIntorpolationStatus(true);
      motors[zeroMotorIndex]->setStatus(MotorStatus::STOP_REST);
      DBG("Motor ");
      DBG(zeroMotorIndex);
      DBGL(" zero done");

      zeroMotorIndex++;

      // Check if all motors calibrated
      if (zeroMotorIndex >= numMotors) {
        zeroState = ZERO_DONE;
      } else {
        zeroState = ZERO_PREPARE;  // Move to next motor
      }
      break;

    case ZERO_DONE:
      // Restore normal operating parameters for all motors
      for (int i = 0; i < numMotors; i++){
        motors[i]->setMinSpeed(MOTOR_DEAD_BAND_SPEED);
        motors[i]->getPID()->SetMode(AUTOMATIC);
      }
      zeroInterpolationDone = true;
      zeroState = ZERO_IDLE;
      DBGL("Zero interpolation completed (non-blocking)");
      break;
  }
}

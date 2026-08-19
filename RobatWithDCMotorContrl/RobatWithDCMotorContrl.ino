/**
 * @file main.cpp
 * @brief DC Servo Control System - Main Program
 * 
 * REVISION HISTORY:
 * 13.1.2026 - PID velocity selecting added. Velocity mode not as good as classic!
 *             Integral factor should be almost omitted (Ki~0 / Ti high).
 *             For small distance: reduce K to very low (0.1).
 *             For higher diff: K between 0.1 to 0.9 (use look-up table).
 * 12-3      - Adding other PID choosing PIDs
 * 12/01/2026 - Tested OK!
 * 09/01/2026 - Most stable developed code from Test5.ino
 *              Checked in different circumstances: power unplug/replug, emergency test, etc.
 * 05.01.2025 - Add zero switch
 * 23/12/2025 - Watchdog and emergency switch added, tested successfully
 * 02.02.2026 - Key changes in main loop switch() part - watchdog sequence moved after switch
 *              Emergency pulled up externally with 120k (internal pull-up didn't work!)
 *              Normal condition: LOW (through emergency push button)
 *              If breaks/cuts = emergency: input goes HIGH through pull-up
 *              LOW/HIGH logic inverted from previous program
 *              checkSoftwareWatchdog moved to after switch control
 * 18.12.2025 - With two motors checked - same result as Test4
 *              Key: Filter reduced from 100 to 50 µs for better Motor2 performance
 *              Later: Filter reduced to 30 µs - both motors fine, Motor2 best result!
 *              Without software filter: working perfect! (consistency not fully checked)
 *              Mystery: Motor2 reacted differently. Cable length 1.5m max.
 *              Could it be reflection in encoder pulses?
 * 22/12/2025 - Test on 2 Motors - PERFECT!
 *              Serial syntax: s(S)/p(P) SetPointMotor1 SetPointMotor2 (0-360 degrees)
 *              Tested successfully with: Software filter 20µs, COUNT_PER_REVOLUTION 4350,
 *              MOTOR_DEAD_BAND_SPEED 350
 */

#include "motor.h"
#include "motor_config.h"
#include "zero_calibration.h"  // Zero position calibration functions
#include "system_safety.h"     // Emergency and watchdog safety systems

/* ===== GLOBAL MOTOR CONFIGURATION ===== */
int numMotors = 2;  // Number of motors in system
motor** motors = new motor*[numMotors];

/* ===== SYSTEM STATE FLAGS ===== */
bool zeroInterpolationDone = false;
bool emergencyTriggered = false;
bool emergencyHandlingDone = false;
bool zeroHandlingZoneDone = false;
bool zeroSwitchInstalled = true; // Set to true if any motor has zero switch

/* ===== TIMING ===== */
unsigned long lastLoopKick = 0;  // Software watchdog timestamp

/* ===== HARDWARE PINS ===== */
constexpr uint8_t EMERGENCY_PIN = 4;  // Emergency stop input (external pull-up)
constexpr uint8_t BOOTSEL_PIN = 6;    // Wire to GND to force bootloader

void setup() {

  delay(2000);   // Allow USB enumeration

#ifdef PICO_DEFAULT_LED_PIN
  pinMode(PICO_DEFAULT_LED_PIN, OUTPUT);
  digitalWrite(PICO_DEFAULT_LED_PIN, HIGH);
  delay(300);
  digitalWrite(PICO_DEFAULT_LED_PIN, LOW);
#endif

  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  /* ---------- FORCE BOOTSEL (WIRE TO GND) ---------- */
  pinMode(BOOTSEL_PIN, INPUT_PULLUP);
  if (digitalRead(BOOTSEL_PIN) == LOW) {
    Serial.println("BOOTSEL forced");
    delay(50);
    reset_usb_boot(0, 0);
  }

  /* ---------- EMERGENCY INPUT ---------- */
  pinMode(EMERGENCY_PIN, INPUT_PULLUP);

  DBGL("System initializing...");

  /* ---------- CREATE MOTORS ---------- */
  motors[0] = new motor(0, 10, 14, 15, 18, 19, 26, 27, KP, Ki_Ti, Kd_Td, PID_SAMPLE_TIME, ENCODER_FILTER_M1);
  motors[1] = new motor(1, 11, 16, 17, 20, 21, 26, 28, KP, Ki_Ti, Kd_Td, PID_SAMPLE_TIME, ENCODER_FILTER_M2);

  /* ---------- INITIALIZE MOTORS ---------- */
  for (int i = 0; i < numMotors; i++) {
    motors[i]->begin();
  }
  
  /* ---------- ZERO INTERPOLATION ---------- */
  // Skip zero calibration if no zero switches installed
  zeroInterpolationDone = !zeroSwitchInstalled;
  
  Serial.setTimeout(50);
  watchdog_enable(HW_WATCHDOG_MS, false);  // Keep OFF during development

  DBGL("System ready.");
}


void loop() {

  watchdog_update();
  lastLoopKick = millis();  // Update timestamp for software watchdog

  /* ---------- EMERGENCY MONITORING ---------- */
  checkAndHandleEmergency(); 
  if (emergencyTriggered) {
    softwareWatchdogCheck();
    return;  // Skip normal operation during emergency
  }
  
  /* ---------- ZERO CALIBRATION ---------- */
  // Interpolation needed at startup AND after emergency (industrial requirement)
  if (!zeroInterpolationDone) { 
    #if USE_ZERO_NON_BLOCKING
      zeroSearchMotorsNonBlocking();  // State machine version (recommended)
    #else
      zeroSearchMotors();             // Blocking version
    #endif
    return;  // Skip normal operation until calibration complete
  }
  // During normal operation, zero switch acts as an emergency/limit switch
  if (zeroSwitchInstalled && zeroInterpolationDone) {
    bool anyCollision = false;

    for (int i = 0; i < numMotors; i++) {
      if (!motors[i]->getZeroSwitchPinState()) {  // Switch active = collision/emergency zone
        anyCollision = true;

        // Show message only once per collision event to avoid flooding Serial
        if (!zeroHandlingZoneDone) {
          DBGL("Motor ");
          DBG(i + 1);
          DBG(" collision with Emergency (here zero) switch. please pull the Motor out from emergency zone! ");
        }

        motors[i]->forceStop();
      }
    }

    if (anyCollision) {
      zeroHandlingZoneDone = true;   // Latch until switches are cleared
//      zeroInterpolationDone = false;   // Doing zero is activated. need to go zero after triggering emergency swiches (here zero swithes in second rolle)  
      return;                        // Go back to start of loop, skip Serial/controls
    } else {
      // All switches clear: allow future collisions to print message again
      zeroHandlingZoneDone = false;
    }
  }

  /* ---------- SERIAL COMMAND PARSING ---------- */
  if (Serial.available() > 0) {
    DBGL();
    DBGL("m or M = Manual, e.g: P 30 60, parallel movement, eg: S 30 60");

    String input = Serial.readStringUntil('\n');

    // Parse command: "P 30 60" or "S 30 60" or "M"
    String tokens[numMotors + 1];
    int tokenCount = 0;
    int startIndex = 0;

    for (int i = 0; i < input.length(); i++) {
      if (input.charAt(i) == ' ' || i == input.length() - 1) {
        tokens[tokenCount] = input.substring(startIndex, (i == input.length() - 1) ? i + 1 : i);
        startIndex = i + 1;
        tokenCount++;
        if (tokenCount == numMotors + 1) break;
      }
    }

    /* --- MANUAL MODE COMMAND --- */
    if (tokens[0] == "M" || tokens[0] == "m") {
      DBG("Manual mode active for all motors");
      for (int i = 0; i < numMotors; i++) {
        motors[i]->initializeMotorInManual();
        motors[i]->setMode(operationMode::MANUAL_MODE);
        motors[i]->setStatus(MotorStatus::RUN);      
      }
    }
    /* --- MOVEMENT COMMANDS (P/S + SETPOINTS) --- */
    else if (tokenCount == numMotors+1) {
      // Set movement mode
      if (tokens[0] == "P" || tokens[0] == "p")
        for (int i = 0; i < numMotors; i++) 
          motors[i]->setMode(operationMode::PARALLEL_MOVEMENT);
        
      else if (tokens[0] == "S" || tokens[0] == "s") 
          for (int i = 0; i < numMotors; i++) 
            motors[i]->setMode(operationMode::SEQUENTIAL_MOVEMENT);

      // Process setpoints for each motor
      for (int i = 0; i < numMotors; i++) {
        motors[i]->setSetpoint(constrain(tokens[i+1].toDouble(), 0, 360));
        
        // Check if already at target position
        if (fabs(motors[i]->getError()) < 4*MAX_ACCEPABLE_ERR) {
          motors[i]->initializeMotorInManual();
          motors[i]->setStatus(MotorStatus::STOP_REST);
          DBGL("Already in position. pls enter new position! ");
        }
        else {
          // Start movement to new position
          motors[i]->initializeMotorInAuto();
          motors[i]->setStatus(MotorStatus::RUN); 
          
          // Print motor configuration and status
          DBGL();
          DBG("Motor ");
          DBG(i + 1);
          DBG(" | setpoint: ");
          DBG(motors[i]->getSetpoint());
          DBG(" | Curr Position= ");        
          DBG(motors[i]->getPosition());  
          DBG(" | Kp= ");
          DBG(motors[i]->getPID()->GetKp());
          DBG(" | Ki= ");        
          DBG(motors[i]->getPID()->GetKi());
          DBG(" | Kd= ");        
          DBG(motors[i]->getPID()->GetKd());
          DBG(" | MinFeed= ");        
          DBG(motors[i]->getMinSpeed());
          DBG(" | MaxFeed= ");        
          DBG(motors[i]->getMaxSpeed());  
          DBG(" | SampleTime= ");        
          DBG(motors[i]->getPidSampleTime());                          
        }              
      }
    } else {
      DBGL("Invalid input. Please enter setpoints for all motors.");
    }
  }
  
  /* ---------- CONTROL LOGIC ---------- */
  switch (motors[0]->getMode()) {

    case operationMode::MANUAL_MODE:
    case operationMode::PARALLEL_MOVEMENT:
      // Process all motors simultaneously
      for (int i = 0; i < numMotors; i++) {
        if (motors[i]->getStatus() != MotorStatus::RUN) 
          continue; // Skip motor if not in RUN status

        if (fabs(motors[i]->getError()) > MAX_ACCEPABLE_ERR && !motors[i]->getIntorpolationStatus()) {
          // Still moving to target
          motors[i]->getPID()->Compute();
          if (fabs(motors[i]->getError()) < 3*MAX_ACCEPABLE_ERR)  // Prevent flooding screen
            motors[i]->printDebug();
        } else {
          // Target reached
          motors[i]->setOutput(0);
          motors[i]->setIntorpolationStatus(true);
          motors[i]->setStatus(MotorStatus::STOP_REST);
        }

        motors[i]->setMotor();
      }
      break;

    case operationMode::SEQUENTIAL_MOVEMENT:
      // Process motors one at a time in sequence
      for (int i = 0; i < numMotors;) {
        if (motors[i]->getStatus() != MotorStatus::RUN) {
          i++; // Increment before skipping to avoid infinite loop
          continue;
        }
        
        if (fabs(motors[i]->getError()) > MAX_ACCEPABLE_ERR && !motors[i]->getIntorpolationStatus()) {
          // Motor still moving
          motors[i]->getPID()->Compute();
          motors[i]->setMotor();
          if (fabs(motors[i]->getError()) < 3*MAX_ACCEPABLE_ERR)  // Prevent flooding screen
            motors[i]->printDebug();
          break;   // CRUCIAL: Don't block loop - allow emergency/watchdog monitoring
        } else {
          // Motor reached target - move to next
          motors[i]->setOutput(0);
          motors[i]->setMotor();
          motors[i]->setIntorpolationStatus(true);
          motors[i]->setStatus(MotorStatus::STOP_REST);
          i++;
        }
      }
      break;
  }

  /* ---------- SOFTWARE WATCHDOG (AFTER CONTROL) ---------- */
  softwareWatchdogCheck();
}



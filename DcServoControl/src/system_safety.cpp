/**
 * @file system_safety.cpp
 * @brief Implementation of emergency and watchdog safety systems
 */

#include "system_safety.h"
#include <Arduino.h>

/**
 * @brief Emergency stop handler
 * 
 * Key changes (02.02.2026):
 * - Emergency pulled up externally with 120k (internal pull-up didn't work)
 * - Normal condition: LOW (through emergency push button)
 * - If wire breaks/cuts: goes HIGH through pull-up resistor (emergency state)
 * - Logic inverted from previous program version
 * 
 * Safety feature: Wire disconnect = Emergency (fail-safe design)
 */
void checkAndHandleEmergency() {

  // Check emergency input (HIGH = emergency) - polling method
  if (digitalRead(EMERGENCY_PIN) == HIGH) {
    emergencyTriggered = true;
  }

  // Emergency triggered - stop all motors (one-time action)
  if (emergencyTriggered && !emergencyHandlingDone) {

    DBGL("[EMERG] STOP");

    for (int i = 0; i < numMotors; i++) {
      motors[i]->getPID()->SetMode(MANUAL);  // Disable PID
      motors[i]->forceStop();                // Emergency brake
    }

    emergencyHandlingDone = true;
    return;
  }

  // Emergency cleared - restore normal operation
  if (emergencyTriggered && digitalRead(EMERGENCY_PIN) == LOW) {

    DBGL("[EMERG] CLEARED");
    
    // Re-enable PID for all motors
    for (int i = 0; i < numMotors; i++)
      motors[i]->getPID()->SetMode(AUTOMATIC); 
    
    // Reset emergency flags
    emergencyTriggered = false;
    emergencyHandlingDone = false;
    
    // Force re-zero calibration (essential for industrial safety)
    zeroInterpolationDone = false;
  }
}

/**
 * @brief Software watchdog - prevents infinite loops or stalls
 * 
 * Key changes (02.02.2026):
 * - Moved AFTER control logic in main loop
 * - Ensures entire loop cycle completed before checking
 * 
 * If loop stalls (millis() - lastLoopKick > SW_WATCHDOG_MS):
 * - Logs warning
 * - Enables 200ms hardware watchdog
 * - Enters infinite loop
 * - Hardware watchdog triggers reset
 * 
 * Important: lastLoopKick must be updated at start of each loop()
 */
void softwareWatchdogCheck() {
  if ((millis() - lastLoopKick) > SW_WATCHDOG_MS) {
    DBGL("[SW WD] Loop stalled – reset");
    watchdog_enable(200, false);  // Short timeout, no pause
    delay(500);                   // Ensure message sent
    while (true) {}               // Wait for hardware reset
  }
}

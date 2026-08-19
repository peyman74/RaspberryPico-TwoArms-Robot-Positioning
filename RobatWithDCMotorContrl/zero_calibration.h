/**
 * @file zero_calibration.h
 * @brief Zero position calibration for DC servo motors
 * 
 * Provides both blocking and non-blocking zero calibration methods.
 * Zero calibration locates the mechanical zero position using limit switches
 * and resets encoder counts. Essential for industrial applications,
 * especially after emergency stops.
 */

#ifndef ZERO_CALIBRATION_H
#define ZERO_CALIBRATION_H

#include "motor.h"
#include "motor_config.h"

// External references to shared state (defined in main.cpp)
extern motor** motors;
extern int numMotors;
extern bool zeroInterpolationDone;
extern bool emergencyTriggered;

/**
 * @brief Blocking zero calibration for all motors
 * 
 * Sequentially moves each motor backward to trigger zero switch,
 * then moves forward slightly to park position. Sets encoder to zero.
 * Uses while loops - blocks program execution until complete.
 * 
 * @return true if successful, false if emergency triggered
 */
bool zeroSearchMotors();

/**
 * @brief Non-blocking zero calibration using state machine
 * 
 * Industrial-style state machine approach. Must be called repeatedly
 * from main loop. Allows emergency monitoring and watchdog updates
 * during calibration process.
 * 
 * States: IDLE -> PREPARE -> BACKWARD -> FORWARD -> FINALIZE -> DONE
 */
void zeroSearchMotorsNonBlocking();

// State machine states for non-blocking calibration
enum ZeroState {
  ZERO_IDLE,      // Not running
  ZERO_PREPARE,   // Setup motor for calibration
  ZERO_BACKWARD,  // Moving backward to find zero switch
  ZERO_FORWARD,   // Moving forward to park position
  ZERO_FINALIZE,  // Reset encoder and prepare next motor
  ZERO_DONE       // All motors calibrated
};

// State machine variables (shared across calls)
extern ZeroState zeroState;
extern int zeroMotorIndex;
extern unsigned long zeroTimestamp;

#endif // ZERO_CALIBRATION_H

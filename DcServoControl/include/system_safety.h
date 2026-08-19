/**
 * @file system_safety.h
 * @brief Emergency stop and watchdog safety systems
 * 
 * Implements two critical safety features:
 * 1. Emergency stop monitoring (hardware input)
 * 2. Software watchdog (loop stall detection)
 * 
 * Emergency pin uses external pull-up (120k). Normal = LOW, Emergency = HIGH.
 * If emergency wire breaks or disconnects, system enters safe emergency state.
 */

#ifndef SYSTEM_SAFETY_H
#define SYSTEM_SAFETY_H

#include "motor.h"
#include "motor_config.h"

// External references to shared state (defined in main.cpp)
extern motor** motors;
extern int numMotors;
extern bool emergencyTriggered;
extern bool emergencyHandlingDone;
extern bool zeroInterpolationDone;
extern unsigned long lastLoopKick;
extern const uint8_t EMERGENCY_PIN;

/**
 * @brief Check emergency input and handle state transitions
 * 
 * Three states:
 * 1. Normal operation: EMERGENCY_PIN = LOW
 * 2. Emergency triggered: EMERGENCY_PIN = HIGH -> Stop all motors
 * 3. Emergency cleared: EMERGENCY_PIN goes LOW -> Re-enable system, force re-zero
 * 
 * Must be called frequently from main loop.
 */
void checkAndHandleEmergency();

/**
 * @brief Software watchdog - detect loop stalls
 * 
 * Monitors time since last loop kick (lastLoopKick timestamp).
 * If loop doesn't execute for > SW_WATCHDOG_MS, forces system reset
 * by enabling short hardware watchdog without updates.
 * 
 * Critical: Call AFTER all control logic to ensure loop completed.
 */
void softwareWatchdogCheck();

#endif // SYSTEM_SAFETY_H

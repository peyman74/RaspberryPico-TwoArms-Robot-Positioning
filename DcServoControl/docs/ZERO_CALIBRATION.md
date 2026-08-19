# Zero Calibration / Interpolation Overview

This document explains the role and basic algorithms of the zero calibration ("zero interpolation")
module, based on `include/zero_calibration.h` and `src/zero_calibration.cpp`.

Zero calibration is essential to define a precise mechanical reference (0°) for each motor axis.
All later movements and angles (0°–360°) are measured relative to this reference.

## Role of the zero_calibration module

The module provides two ways to run zero calibration for all motors:

- `bool zeroSearchMotors()`
  - **Blocking** version.
  - Uses `while` loops and `delay()` calls.
  - Easy to understand; the program does not do anything else while calibration runs.

- `void zeroSearchMotorsNonBlocking()`
  - **Non‑blocking** version using a state machine.
  - Must be called repeatedly from the main loop.
  - Allows emergency monitoring and watchdog updates during calibration.

Shared state:
- `extern motor** motors;`
- `extern int numMotors;`
- `extern bool zeroInterpolationDone;`
- `extern bool emergencyTriggered;`
- State machine variables: `zeroState`, `zeroMotorIndex`, `zeroTimestamp`.

The main loop in `src/main.cpp` uses these functions as follows:
- At startup and after certain safety events it sets `zeroInterpolationDone = false`.
- In `loop()`, if `zeroInterpolationDone` is false:
  - It calls either `zeroSearchMotorsNonBlocking()` (recommended) or `zeroSearchMotors()`.
  - It then `return`s, so no normal motion commands are processed until calibration is complete.

## Blocking zero calibration algorithm (zeroSearchMotors)

For each motor that needs calibration, `zeroSearchMotors()` performs these steps:

1. **Prepare all motors for interpolation**
   - Set `zeroInterpolationDone = false`.
   - For each motor:
     - Set `minSpeed` to 0.
     - Set `Status` to `ZERO_INTERPOL`.
     - Set the PID mode to `MANUAL` (prevent integral windup during search).

2. **For each motor (sequentially)**
   - If `getZeroInterpolation()` indicates the motor still needs zeroing:
     1. Print a message that the motor is searching for zero.
     2. **Move backward until the zero switch triggers**:
        - While `getZeroSwitchPinState()` is still **true** (switch not yet activated):
          - Call `checkAndHandleEmergency()`; if `emergencyTriggered` becomes true, abort and
            return `false`.
          - Set a negative output (`-ZERO_INTERPOLATION_FEED_MIN`) and `setMotor()` to move toward
            the zero switch.
          - Call `watchdog_update()` to keep the hardware watchdog happy.
     3. **Stop and debounce**:
        - Set output to 0 and call `setMotor()`.
        - `delay(100)` for debounce.
     4. **Move forward a short distance away from the switch**:
        - Set a small positive output (`ZERO_INTERPOLATION_FEED_MAX`) and call `setMotor()`.
        - `delay(250)` to move a small distance away from the switch.
        - Stop the motor and debounce again (`delay(100)`).
     5. **Finalize for this motor**:
        - Mark interpolation status as done.
        - Reset encoder count to 0.
        - Set status to `STOP_REST`.
        - Print a message that zero interpolation is done for this motor.

3. **After all motors are processed**
   - Set `zeroInterpolationDone = true`.
   - For each motor:
     - Restore `minSpeed` to `MOTOR_DEAD_BAND_SPEED`.
     - Set PID mode back to `AUTOMATIC`.
   - Print that zero interpolation is completed for all motors.

This blocking version is simple but can tie up the CPU until all axes are calibrated.

## Non‑blocking zero calibration algorithm (zeroSearchMotorsNonBlocking)

The non‑blocking version uses a small state machine, defined by the `ZeroState` enum:

- `ZERO_IDLE`
- `ZERO_PREPARE`
- `ZERO_BACKWARD`
- `ZERO_FORWARD`
- `ZERO_FINALIZE`
- `ZERO_DONE`

The variables `zeroState`, `zeroMotorIndex`, and `zeroTimestamp` keep track of progress between
calls. The function `zeroSearchMotorsNonBlocking()` should be called repeatedly from `loop()`
whenever `zeroInterpolationDone` is false.

The algorithm is:

1. **ZERO_IDLE**
   - Initialize calibration:
     - Set `zeroInterpolationDone = false`.
     - Set `zeroMotorIndex = 0`.
     - Transition to `ZERO_PREPARE`.
     - Print a message that zero interpolation started (non‑blocking).

2. **ZERO_PREPARE**
   - Prepare the current motor (`zeroMotorIndex`) for calibration:
     - Set PID mode to `MANUAL`.
     - Set `minSpeed` to 0.
     - Set status to `ZERO_INTERPOL`.
   - Transition to `ZERO_BACKWARD`.

3. **ZERO_BACKWARD**
   - If the zero switch is **already active** (`getZeroSwitchPinState() == false`):
     - Stop the motor (output 0, `setMotor()`).
     - Record current time in `zeroTimestamp = millis()`.
     - Transition to `ZERO_FORWARD`.
   - Otherwise (switch not yet active):
     - Set a small negative output (`-ZERO_INTERPOLATION_FEED_MIN`).
     - Call `setMotor()` to move toward the switch.
     - Print a debug message (optional).
   - Function returns; the loop continues to run and other tasks (including emergency checks) can
     still be performed between calls.

4. **ZERO_FORWARD**
   - While `millis() - zeroTimestamp < 250` ms:
     - Set a small positive output (`ZERO_INTERPOLATION_FEED_MAX`).
     - Call `setMotor()` to move slightly away from the switch and into a park position.
   - After 250 ms has passed:
     - Stop the motor (output 0, `setMotor()`).
     - Transition to `ZERO_FINALIZE`.

5. **ZERO_FINALIZE**
   - For the current motor:
     - Reset encoder count to 0.
     - Mark interpolation as done.
     - Set status to `STOP_REST`.
     - Print a message that zero for this motor is done.
   - Increment `zeroMotorIndex`.
   - If `zeroMotorIndex >= numMotors`:
     - Transition to `ZERO_DONE`.
   - Else:
     - Transition back to `ZERO_PREPARE` to start calibration for the next motor.

6. **ZERO_DONE**
   - For all motors:
     - Restore `minSpeed` to `MOTOR_DEAD_BAND_SPEED`.
     - Set PID mode back to `AUTOMATIC`.
   - Set `zeroInterpolationDone = true`.
   - Transition back to `ZERO_IDLE`.
   - Print that zero interpolation is completed (non‑blocking).

Because this version processes only a small step per call, it lets the main loop remain responsive:
- Emergency monitoring can continue.
- The hardware watchdog can be updated regularly.
- The system behaves more like an industrial motion controller.

## When zero calibration runs

`zeroInterpolationDone` is used as a global flag to decide when calibration is needed:

- On startup: the code sets `zeroInterpolationDone` based on whether zero switches are installed.
- After an emergency: `checkAndHandleEmergency()` sets `zeroInterpolationDone = false` when
  the emergency is cleared.
- After a collision on the zero switch during normal operation: the main loop sets
  `zeroInterpolationDone = false` when a zero‑switch collision is detected and cleared.

Whenever `zeroInterpolationDone` is false, the main loop **only** runs the zero calibration
routine and skips normal motion commands until calibration finishes successfully.

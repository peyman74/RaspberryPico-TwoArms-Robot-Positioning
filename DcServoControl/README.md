# DcServoControl — Project Notes

This repository contains the DC servo control project for two ARM motors.

## Watchdogs

The following text is copied from `docs/WATCHDOGS.md` to provide an at-a-glance explanation
in the project root README.

---

# Watchdog Notes (Hardware + Software)

This file collects explanations and implementation notes for the hardware and software watchdogs used
in this project. See `src/main.cpp` for the actual implementation and the inline comments.

## Goals
- Provide a robust hardware-level reset (RP2040 watchdog) for hard hangs.
- Provide a higher-level software watchdog to detect subsystem failures and attempt soft recovery
  before forcing a reset.

## Hardware watchdog (RP2040)
- API used (in this project): `watchdog_enable(timeout_ms, pause_on_debug)` and `watchdog_update()`.
- Behavior: once enabled with a timeout (e.g. 5000 ms), the hardware watchdog requires periodic calls
  to `watchdog_update()` before the timeout expires. If `watchdog_update()` is not called in time,
  the RP2040 hardware will reset the chip.
- Notes:
  - This is independent of the main CPU thread scheduling and is the most reliable way to recover
    from a hard lock or long blocking operation.
  - We enable the HW watchdog in `setup()` and call `watchdog_update()` at the start of each
    `loop()` iteration in `src/main.cpp`.
  - When forcing a reset intentionally (after failed soft recovery), we shorten the HW watchdog
    timeout and then wait for the reset to occur.

## Software watchdog (project-level)
- Purpose: detect that important activities (like PID compute and main loop progress) are not
  occurring and attempt a graceful recovery before forcing hardware reset.
- Implementation details in this repo:
  - `lastPIDKick`: updated each time we call `PID->Compute()` for any motor.
  - `lastLoopKick`: updated at loop progress points to indicate the main loop is running.
  - `SW_WATCHDOG_MS`: threshold (e.g. 3000 ms) that triggers soft-recovery logic if exceeded.
  - `softRecoveryAttempts` and `MAX_SOFT_RECOVERIES`: allow one (configurable) soft recovery attempt.

- Soft recovery steps performed by the code:
  1. Reinitialize PWM and motor interfaces (`initializePwm()`, `begin()`, `initializeMotorInAuto()` for each motor).
  2. Short `delay()` to let hardware settle.
  3. If soft recovery attempts exceed `MAX_SOFT_RECOVERIES`, force a hardware reset by setting
     a very short HW watchdog timeout and waiting.

## Why both?
- Hardware watchdog: guaranteed reset when the system hangs — MUST HAVE for production/field devices.
- Software watchdog: lets the program try corrective measures first and can provide logging/diagnostics
  prior to reset. Also useful for detecting degraded but not completely dead conditions.

## Tuning recommendations
- HW_WATCHDOG_MS: choose a value comfortably larger than the maximum expected single blocking
  operation (e.g. communications timeout + margin). 3000–10000 ms is common; 5000 ms is a good start.
- SW_WATCHDOG_MS: choose lower than HW timeout to give software a chance to recover (e.g. 2000–4000 ms).
- Soft recovery attempts: keep low (0–2). Repeated soft recovery attempts can mask persistent faults.

## Where to look in code
- Hardware enable: `src/main.cpp` — `watchdog_enable(HW_WATCHDOG_MS, false)` inside `setup()`.
- Feeding HW watchdog: `src/main.cpp` — `watchdog_update()` at top of `loop()`.
- SW watchdog variables and logic: `src/main.cpp` around the command handling block — variables
  `lastPIDKick`, `lastLoopKick`, `SW_WATCHDOG_MS`, and the soft-recovery block.

## Safety notes
- Before forcing a hardware reset, the code attempts a soft recovery. If your hardware requires
  a controlled shutdown (e.g., to park motors or save state), add those steps to the soft recovery
  path before forcing the reset. For critical actuators consider transitioning to a safe state
  (stop motors) first.

## Debugging and diagnostics
- Add serial prints to the soft recovery path to log the reason for recovery/reset.
- Consider toggling a debug GPIO or logging to persistent storage (if available) before reset
  to help diagnose persistent failures in the field.

## Example values used in this project
- `HW_WATCHDOG_MS = 5000` — hardware watchdog timeout in milliseconds.
- `SW_WATCHDOG_MS = 3000` — software watchdog threshold in milliseconds.

---

If you prefer a shorter README, I can extract just the key points and keep the full document in `docs/WATCHDOGS.md`.

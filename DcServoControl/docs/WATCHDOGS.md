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
- Purpose: detect when the main loop stops running for too long and trigger a controlled reset.
- Implementation details in this repo (current version):
  - `lastLoopKick`: updated at the start of each `loop()` iteration.
  - `SW_WATCHDOG_MS`: threshold (e.g. 3000 ms) that defines the maximum allowed time between
    two successful loop iterations.

- Behavior of `softwareWatchdogCheck()` (see `src/system_safety.cpp`):
  1. Compute `millis() - lastLoopKick`.
  2. If this value is greater than `SW_WATCHDOG_MS`, assume the main loop has stalled.
  3. Print a diagnostic message (`[SW WD] Loop stalled – reset`).
  4. Enable the hardware watchdog with a short timeout (e.g. 200 ms).
  5. Wait in an infinite loop until the hardware watchdog resets the MCU.

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
- SW watchdog timestamp: `src/main.cpp` — `lastLoopKick` is updated at the start of `loop()`.
- SW watchdog check and reset logic: `src/system_safety.cpp` — `softwareWatchdogCheck()`.

## Safety notes
- Before enabling the short hardware watchdog in `softwareWatchdogCheck()`, the code already
  stops motors in the normal control path (for example via emergency handling or status logic).
  If your hardware requires additional controlled shutdown steps (e.g., move to a park position
  or save state), those should be added before calling `softwareWatchdogCheck()` or inside it
  just before enabling the short timeout.

## Debugging and diagnostics
- Add serial prints to the soft recovery path to log the reason for recovery/reset.
- Consider toggling a debug GPIO or logging to persistent storage (if available) before reset
  to help diagnose persistent failures in the field.

## Example values used in this project
- `HW_WATCHDOG_MS = 5000` — hardware watchdog timeout in milliseconds.
- `SW_WATCHDOG_MS = 3000` — software watchdog threshold in milliseconds.

---

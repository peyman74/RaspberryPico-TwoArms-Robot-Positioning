# System Safety Overview (Emergency + Watchdog)

This document explains the role and simple algorithm of the system safety module, based on the
current implementation in `include/system_safety.h` and `src/system_safety.cpp`.

## Role of the system_safety module

The safety module provides two main functions that are called from the main loop:

- `checkAndHandleEmergency()`
  - Monitors the hardware emergency input pin.
  - Stops all motors immediately when an emergency is detected.
  - Restores normal operation when the emergency is cleared.
  - Forces a new zero interpolation after an emergency, for industrial‑style safety.

- `softwareWatchdogCheck()`
  - Checks that the main loop is still running regularly.
  - If the loop stalls for too long, it triggers a controlled reset via the hardware watchdog.

Shared state variables (declared `extern` in `system_safety.h`, defined in `src/main.cpp`) allow the
safety code to interact with the motors and high‑level flags:

- `motor** motors`, `int numMotors`
- `bool emergencyTriggered`, `bool emergencyHandlingDone`
- `bool zeroInterpolationDone`
- `unsigned long lastLoopKick`
- `const uint8_t EMERGENCY_PIN`

## Emergency handling algorithm

The emergency input is wired **fail‑safe**:
- Normal condition: `EMERGENCY_PIN = LOW` (through the emergency push button to GND).
- Emergency or wire break: `EMERGENCY_PIN = HIGH` (through an external 120 kΩ pull‑up).

`checkAndHandleEmergency()` is called on every loop iteration and follows this simple logic:

1. **Read emergency pin**
   - If `digitalRead(EMERGENCY_PIN) == HIGH`, set `emergencyTriggered = true`.

2. **First action when emergency is triggered**
   - If `emergencyTriggered == true` and `emergencyHandlingDone == false`:
     - Print `[EMERG] STOP` on the serial terminal.
     - For each motor:
       - Set the PID controller mode to `MANUAL` so it stops driving the output.
       - Call `forceStop()` to immediately stop the motor.
     - Set `emergencyHandlingDone = true` to avoid repeating the same action.
     - `return;` to exit the function.

3. **When emergency is cleared**
   - If `emergencyTriggered == true` and the pin has gone back LOW:
     - Print `[EMERG] CLEARED`.
     - For each motor:
       - Set the PID controller mode back to `AUTOMATIC`.
     - Clear `emergencyTriggered` and `emergencyHandlingDone`.
     - Set `zeroInterpolationDone = false` so that the main loop will run a new zero
       interpolation sequence before allowing normal movements.

This design ensures that:
- Any emergency or wiring fault stops all motion quickly.
- After the emergency is removed, the system does **not** immediately resume from an unknown
  position; instead, it forces the axes to be re‑zeroed.

## Software watchdog algorithm

The software watchdog is implemented in `softwareWatchdogCheck()` and uses the timestamp variable
`lastLoopKick` and the constant `SW_WATCHDOG_MS` (defined in configuration).

The main loop in `src/main.cpp` does:
- At the top of `loop()`:
  - Call `watchdog_update()` to feed the **hardware** watchdog.
  - Set `lastLoopKick = millis();` to record that a new loop iteration has started.
- At the very end of `loop()`:
  - Call `softwareWatchdogCheck()`.

`softwareWatchdogCheck()` performs:

1. Compute the elapsed time since the last loop kick:
   - `elapsed = millis() - lastLoopKick`.

2. If `elapsed` is greater than `SW_WATCHDOG_MS`:
   - Print `[SW WD] Loop stalled – reset` on the serial terminal.
   - Call `watchdog_enable(200, false);` to configure the hardware watchdog with a very short
     timeout (200 ms).
   - `delay(500);` to give time for the message to be sent.
   - Enter an infinite loop `while (true) {}`.

3. The hardware watchdog then times out and **resets the microcontroller**.

Because `softwareWatchdogCheck()` is called **after** the control logic in `loop()`, it only
considers the loop healthy if a full iteration has completed. If the loop gets stuck in the
middle (for example, in a blocking function), `lastLoopKick` will stop updating, and the
software watchdog will eventually detect that and cause a reset.

## Interaction with main.cpp

In `src/main.cpp` the sequence is:

1. `watchdog_update();`
2. `lastLoopKick = millis();`
3. `checkAndHandleEmergency();`
4. Zero interpolation handling.
5. Zero‑switch collision handling.
6. Serial command parsing.
7. Control logic (manual / parallel / sequential).
8. `softwareWatchdogCheck();`

This ordering ensures:
- Emergency conditions are checked early in the loop.
- Control logic runs only when the system is in a safe state.
- The watchdog verifies that the entire loop continues to execute within the expected time.

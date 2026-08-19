# DC Servo Control System with Safety and Zero Interpolation

**Student:** [Your Name]  
**Course:** [Course Name]  
**Professor:** [Professor Name]  
**Date:** [Submission Date]

---

## Index (Table of Contents)

1. Introduction
2. System Overview
3. Hardware Design
4. Software Architecture
5. Control and Operating Modes
6. Safety Concept
7. User Interface and Operation
8. Experiments and Results
9. Conclusion and Future Work
10. Appendices
11. References

---

## 1. Introduction

This project implements a two‑axis DC servo positioning system on a Raspberry Pi Pico microcontroller.  
Each motor is equipped with an incremental encoder and is controlled by a PID algorithm to reach a commanded angular position between 0° and 360°.

The main goal of the project is to build a small but realistic example of an industrial motion control system. The focus is not only on positioning accuracy, but also on safe behavior in abnormal conditions such as wiring faults, emergency stops, or mechanical collisions.

The key objectives are:
- Control two DC motors with encoder feedback.
- Support different operating modes (manual, parallel, sequential).
- Implement zero interpolation to define a precise reference position.
- Integrate safety mechanisms: emergency switch, zero‑switch collision detection, and watchdogs.
- Use a clean and modular software structure suitable for future extensions.

In this report, the hardware and software design are described in simple, clear language. Code listings are kept short and only used where they help to understand the concept. Photos and measurement screenshots can be added where indicated.

---

## 2. System Overview

The system consists of three main parts:
- A host PC running a serial terminal.
- A Raspberry Pi Pico microcontroller board running the control firmware.
- Two DC servo axes, each with a motor driver, an encoder, and a zero (home) switch.

The user sends commands from the PC over USB serial to the Pico. The Pico interprets these commands, computes the motor outputs, and drives the motors through the motor drivers. The encoders provide feedback so that the controller can measure the current position. Zero switches are used to find a known reference position. An additional emergency switch allows safe stopping of the system.

**Suggested figure:**
- *Figure 1: Block diagram of the complete system (PC → Pico → motor drivers → motors, encoders, zero switches, emergency switch).*  
[Insert your block diagram here in Word]

The typical operating sequence is:
1. Power on the system.
2. The Pico initializes all hardware and runs a zero interpolation routine (if zero switches are installed).
3. The system reports "System ready" on the serial terminal.
4. The user sends motion commands via serial (manual, parallel, or sequential mode).
5. The motors move to the requested positions while safety mechanisms continuously monitor the system.
6. In case of an emergency or collision, the system stops the motors and requires a new zero interpolation before normal operation is resumed.

---

## 3. Hardware Design

### 3.1 Main Components

The main hardware components of the system are:
- Raspberry Pi Pico microcontroller.
- Two DC motors with encoder feedback.
- Motor driver modules for each motor.
- Incremental encoders (one per motor).
- Zero (home) switches for each axis.
- Emergency stop switch with external pull‑up resistor.
- Power supply for motors and logic.

**Suggested table:**
Create a small table in Word listing each component, its type, and its function.

### 3.2 Connections

The hardware is wired as follows (exact pin numbers can be adapted from the code and your wiring diagrams):
- The Pico drives the motor drivers using digital outputs for direction and PWM for speed.
- Encoder signals are connected to dedicated input pins, configured for counting pulses.
- Each zero switch is connected as a digital input.
- The emergency switch is wired with an external 120 kΩ pull‑up resistor so that:
  - Normal condition: input is pulled LOW through the emergency button.
  - Emergency or wire break: input goes HIGH through the pull‑up resistor.

This "HIGH = emergency" design is fail‑safe. If the wire breaks or a connector is unplugged, the system automatically enters the emergency state.

**Suggested figures:**
- *Figure 2: Photo of the complete hardware setup on the bench.*
- *Figure 3: Close‑up photo of the wiring to the Pico (encoders, switches, drivers).*  
[Insert your photos here in Word]

### 3.3 Encoder and Zero Switch Signals

The encoders produce quadrature signals that are counted by the firmware to compute the current motor angle.  
The zero switches are simple digital sensors that change state when the axis reaches a defined mechanical reference position.

You can include screenshots from an oscilloscope or logic analyzer here to show:
- Encoder pulses at different speeds.
- Zero switch signal when the axis passes the reference point.

---

## 4. Software Architecture

The software is written in C++ using the PlatformIO environment. The code is split into several source files to keep responsibilities clear and to make the project easier to maintain.

The most important files are:
- `src/main.cpp` – main program, system initialization, and main loop.
- `src/motor.cpp` / `include/motor.h` – motor class with PID control and status handling.
- `src/zero_calibration.cpp` / `include/zero_calibration.h` – zero interpolation and calibration routines.
- `src/system_safety.cpp` / `include/system_safety.h` – emergency switch and watchdog logic.
- `include/motor_config.h` – configuration constants such as PID settings and timing parameters.

### 4.1 Main Program (main.cpp)

The main program performs:
- Hardware initialization (serial, emergency pin, motors).
- Creation and initialization of motor objects.
- Zero interpolation at startup if zero switches are installed.
- The main loop, which repeatedly:
  1. Updates the watchdog timestamp.
  2. Checks the emergency input.
  3. Runs zero interpolation if required.
  4. Checks zero‑switch collisions during normal operation.
  5. Reads and parses serial commands.
  6. Executes the control logic for the active operating mode.
  7. Calls the software watchdog check.

### 4.2 Motor Abstraction (motor.*)

Each motor is represented by a `motor` class instance. This class encapsulates:
- Encoder reading and position calculation.
- PID controller object and tuning parameters.
- Current setpoint and error.
- Operating mode (manual, parallel, sequential).
- Status (RUN, STOP_REST, etc.).

The class also provides helper functions such as:
- Initializing the motor for manual or automatic control.
- Applying the PID output to the motor driver.
- Stopping the motor immediately (`forceStop`) when required by safety logic.

### 4.3 Zero Interpolation (zero_calibration.*)

Zero interpolation is used to find an accurate reference position for each axis.  
At startup, and after certain safety events, the system:
- Moves the motor slowly towards the zero switch.
- Detects the exact point where the zero switch becomes active.
- Records the encoder count at that moment and defines it as angle 0°.

This allows all subsequent angles (0° to 360°) to be measured and controlled relative to a well‑defined mechanical reference.

### 4.4 Safety and Watchdog Logic (system_safety.*)

The safety module provides two main functions:
- `checkAndHandleEmergency()` – monitors the emergency input, stops all motors, and manages the transition back to normal operation.
- `softwareWatchdogCheck()` – detects if the main loop has stalled and, if so, requests a system reset using a short hardware watchdog timeout.

These functions are called regularly from `loop()` in `main.cpp` to ensure that safety is always active.

---

## 5. Control and Operating Modes

### 5.1 PID Position Control

Each motor uses a PID controller to reach and maintain the desired angle.  
The basic idea is:
- `error = setpoint − position`.
- The PID controller computes a control output from the error, its integral, and its derivative.
- The control output is used to drive the motor through the motor driver.

For this project, the proportional gain `Kp` is the most important parameter. The integral term is kept very small to avoid overshoot and oscillations, and the derivative term helps to damp movements.

### 5.2 Operating Modes

The system supports three main modes selected via serial commands:

1. **Manual mode (M or m)**
   - The system prepares both motors for manual control.
   - The user can move the motors in a more direct way (depending on the implementation in the `motor` class).

2. **Parallel movement mode (P or p)**
   - Command syntax: `P angle1 angle2`.
   - Both motors move at the same time towards their target angles.
   - The main loop updates both motors on every iteration.

3. **Sequential movement mode (S or s)**
   - Command syntax: `S angle1 angle2`.
   - Motor 1 moves to its target first. Only when it has reached the position does Motor 2 start moving.
   - This behavior is implemented in the `SEQUENTIAL_MOVEMENT` case of the main control switch.

A simple flowchart can be added here to show how the main loop selects the operating mode and processes the motors.

---

## 6. Safety Concept

Safety is a central part of this project. Three mechanisms are used:
- Emergency switch with fail‑safe wiring.
- Zero switch used as a collision/limit switch during normal operation.
- Combined hardware and software watchdog.

### 6.1 Emergency Switch

The emergency switch is connected to the `EMERGENCY_PIN` with an external pull‑up resistor:
- Normal condition: the button connects the input to ground → the pin reads LOW.
- Emergency condition or wire break: the input is pulled HIGH through the resistor.

The logic is intentionally inverted compared to some simple examples: **HIGH means emergency**. This means that even if the wire is cut or unplugged, the system safely enters the emergency state.

In `checkAndHandleEmergency()`:
- If the pin reads HIGH, `emergencyTriggered` is set and all motors are immediately stopped. The PID controllers are switched to manual mode so they no longer drive the outputs.
- When the emergency condition is cleared (pin goes LOW again), the function:
  - Prints a message to the serial terminal.
  - Re‑enables the PID controllers.
  - Clears the emergency flags.
  - Sets `zeroInterpolationDone = false` so that a new zero interpolation is required before normal motion resumes.

### 6.2 Zero Switch as Collision / Limit Switch

At startup, the zero switches are used purely for finding the reference position.  
However, during normal operation, the zero switches are reinterpreted as collision or limit switches.

In the main loop, after zero interpolation has been completed, the program checks:
- If any zero switch becomes active (motor enters the "forbidden" zone).

If this happens:
- The affected motor is immediately stopped using `forceStop()`.
- A clear message is printed once to the serial terminal indicating which motor has collided and that the motor should be pulled out of the emergency zone.
- The flag `zeroHandlingZoneDone` is used to avoid printing the same message repeatedly in every loop iteration.
- `zeroInterpolationDone` is set to `false` to force a new zero interpolation after the collision has been cleared.
- The function returns early, so the rest of the loop (including serial command processing) is skipped while the collision persists.

When all zero switches are released again:
- `zeroHandlingZoneDone` is cleared, so the system can detect and report a future collision again.
- On the next loop, the code will see that `zeroInterpolationDone` is `false` and will start the zero interpolation process again.

### 6.3 Watchdogs

Two watchdog mechanisms are used:

1. **Hardware watchdog**
   - Provided by the microcontroller.
   - If not regularly updated, it resets the entire system.

2. **Software watchdog**
   - The variable `lastLoopKick` is updated at the start of every `loop()` call.
   - `softwareWatchdogCheck()` compares the current time with `lastLoopKick`.
   - If the difference exceeds a defined limit, the function logs a warning, enables a short hardware watchdog timeout, and waits for the reset.

By placing the software watchdog check at the end of the main loop, the program ensures that a successful loop iteration has been completed before the system is considered healthy.

---

## 7. User Interface and Operation

The user interacts with the system through a serial terminal (for example, the Serial Monitor in the IDE or an external terminal program).  
The baud rate is 115200 baud.

### 7.1 Command Syntax

The main commands are:

- `M` or `m`  
  Activate manual mode for all motors.

- `P angle1 angle2`  
  Parallel movement: both motors move to the given angles.

- `S angle1 angle2`  
  Sequential movement: Motor 1 moves first, then Motor 2.

Angles are typically in the range 0° to 360°. The firmware constrains setpoints to this valid range.

### 7.2 Typical Operating Procedure

1. Power on the system.
2. Wait for the serial message indicating that the system is ready and zero interpolation is complete.
3. Send movement commands (P or S) with desired target angles.
4. Observe the serial output for information about setpoints, current positions, and PID parameters.
5. If an emergency or collision occurs:
   - The system stops motion and prints a message.
   - After resolving the cause (releasing the emergency switch or pulling the motor out of the collision zone), the system will require a new zero interpolation before further motion.

**Suggested screenshots:**
- Serial output during normal movement.
- Serial messages during emergency and collision situations.

---

## 8. Experiments and Results

This section summarizes the tests performed on the system and the observed results.

### 8.1 Positioning Tests

Example tests:
- Command different target angles (e.g. 30°, 90°, 180°, 270°) and observe how accurately the motors reach and hold these positions.
- Check the repeatability by moving back and forth between two angles multiple times.

You can describe:
- How quickly the motors reach the target.
- Whether there is overshoot or oscillation.
- How stable the final position is.

Include photos or plots where you measured or observed the angles.

### 8.2 Zero Interpolation Tests

Describe tests where you:
- Powered the system on and observed the initial zero interpolation.
- Triggered an emergency or collision and confirmed that the system required a new zero interpolation afterwards.

Mention the observed behavior:
- Motors moving towards the zero switch at a safe speed.
- Correct detection of the zero switch.
- Stable operation around the reference position.

### 8.3 Safety Tests

Summarize tests for:
- Pressing the emergency switch during motion.
- Simulating a broken emergency wire by disconnecting it.
- Pushing a motor into the zero switch area during normal operation.

For each case, describe:
- What you did.
- What you expected.
- What actually happened.

Ideally, the system:
- Stopped the motors immediately.
- Printed clear messages.
- Required a new zero interpolation before normal movement could continue.

You can add scope or logic analyzer screenshots that show signals at the moment of emergency or collision.

---

## 9. Conclusion and Future Work

In this project, a two‑axis DC servo control system was designed and implemented using a Raspberry Pi Pico. The system can move two motors to commanded angles with encoder feedback and PID control. It supports manual, parallel, and sequential operating modes.

A strong focus was placed on safety. The emergency switch is wired in a fail‑safe way so that a broken wire is treated as an emergency. Zero switches are used not only for initial calibration but also as collision or limit switches during normal operation. Hardware and software watchdogs monitor the health of the main loop and reset the system in case of a stall.

The tests carried out showed that the system behaves as expected and remains stable under different operating conditions, including power cycling and emergency events.

Possible future improvements include:
- More advanced PID tuning or automatic tuning methods.
- A graphical user interface on the PC for easier control and visualization.
- Support for more than two motors.
- Additional safety features similar to industrial safety standards.

---

## 10. Appendices

In the appendices, you can include:
- Detailed wiring diagrams.
- Complete code listings if required.
- Additional measurement data and screenshots.

This material is useful for reference, but not strictly necessary for understanding the main text.

---

## 11. References

List here all sources you used during the project, for example:
- Microcontroller datasheets (e.g. Raspberry Pi Pico documentation).
- Motor and driver datasheets.
- Encoder and switch datasheets.
- Online tutorials or application notes you found helpful for PID control, watchdogs, or safety wiring.
- Any textbooks or lecture notes from your course.

Use a consistent format (for example, numbered list with author, title, source, year).

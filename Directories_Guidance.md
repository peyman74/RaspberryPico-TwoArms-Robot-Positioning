### Naming and Application of the Directories

The following directories contain different versions and implementations of the two-arm robotic project. Each directory corresponds to a specific development stage, control method, or development environment.

1. **`MyArduiniProject`**
   This directory contains my first version of the project, developed as initial attempts to implement and test the two-arm robotic project using the Arduino IDE.

2. **`SpeedLoopKpTiTd`**
   This directory implements a simple **Velocity PID control loop** using the Arduino IDE.

   For an explanation of the Velocity PID method, please refer to the relevant pages from the **Doğan Ibrahim** book provided in this project.

3. **`DcServoControl`**
   This directory includes both control-loop methods explained in `Project_Report.pdf`, particularly **Chapter 5, Part 2 (Section 5.2 – PID Position Control)**. Please also refer to **Figure 27**.

   The two implemented methods are:

   * **Velocity PID loop**
   * **Classical PID loop**

   The selected control method is determined by a parameter in `motor_config.h`:

   * If `USE_CLASSICAL_PID == 1`, the **Classical PID controller** is used.
   * Otherwise, the **Velocity PID controller** is used.

4. **`RobatWithDCMotorContrl`**
   This directory contains a similar implementation to `DcServoControl`, but it has been adapted and refined specifically for the **Arduino IDE**, rather than **VS Code with the PlatformIO extension**.

   Therefore:

   * If you are more comfortable with the **Arduino IDE**, use `RobatWithDCMotorContrl`.
   * If you are more comfortable with **VS Code + PlatformIO**, use `DcServoControl`.

   The two directories essentially are identical; the main difference is just the development environment and project structure.

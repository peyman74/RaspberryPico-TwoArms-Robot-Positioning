### Naming and Application of the Directories

1. **SpeedLoopKpTiTd**  
   Uses a simple **Velocity PID control loop** implemented in the Arduino IDE.
2. **DcServoControl**  
   Includes both control-loop types explained in `Project_Report.pdf (pay attention to chapter 5 part2; 5.2-`PID Position Control:
   - Velocity PID loop
   - Classical PID loop

     pay attention to figure 27

   The selected control method depends on a parameter in `motor_config.h`.
   - If `USE_CLASSICAL_PID == 1`, the Classical PID controller is used.
   - Otherwise, the Velocity PID controller is applied.
3. **RobatWithDCMotorContrl**  
   Similar to the `DcServoControl` directory, but adapted and refined for the Arduino IDE instead of VS Code with the PlatformIO extension
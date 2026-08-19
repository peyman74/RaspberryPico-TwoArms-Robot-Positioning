/*
Root Causes of Oscillation 
1. Velocity form is extremely sensitive to noise and encoder jitter
 output += constant1 * error + constant2 * error_1 + constant3 * error_2;  
 is correct for velocity form, but it amplifies noise on the error difference (error - error_1).
Even a ±1 encoder count fluctuation causes output jumps → oscillations.

2. Your sample time (PID_SAMPLE_TIME = 100 ms) is too long

100 ms = 10 Hz update rate. For most small DC motors, this is too slow — the system responds faster than your control loop, so it overshoots and oscillates.

✅ Fix:
Try PID_SAMPLE_TIME = 10 ms (0.01 s) or even 5 ms, and reduce Kp accordingly (e.g., 1/10).

✅ Fix options:
Apply a small low-pass filter to the measured position:
current_position = 0.8 * current_position + 0.2 * getPosition();
Or average multiple readings:
current_position = (getPosition() + lastPos1 + lastPos2) / 3;

3. Output integration accumulation without limit

In your code:
output += ...

output grows indefinitely if you don’t constrain it.

✅ Add immediately:

output = constrain(output, -motorMaxSpeed, motorMaxSpeed);

just after the PID calculation line.
4. PWM dead zone
onstrain(out, PWM_MIN_SPEED, MAX_SPEED)

but that causes an abrupt jump at ±300 → nonlinear response near zero → overshoot.

✅ Fix:
Use a smooth mapping:

int pwmVal = map(abs(out), 0, 1023, PWM_MIN_SPEED, MAX_SPEED);
pwm_set_gpio_level(pwm_pin, pwmVal);

PID_SAMPLE_TIME = 10; // ms
Kp = 1.0;
Ti = 0.2;
Td = 0.0;

*/
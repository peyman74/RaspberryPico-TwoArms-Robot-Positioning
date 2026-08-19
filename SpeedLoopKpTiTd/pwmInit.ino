  // PWM
  // Initialize the pin for PWM. Set the pin as a PWM output
  // Set PWM function for both GPIO pins

void pwmInit() {
  gpio_set_function(pwm_pin, GPIO_FUNC_PWM); 

  // Get slice numbers for both pins
  int slice_pan = pwm_gpio_to_slice_num(pwm_pin);

  // Set wrap for both slices (10-bit resolution)
  // Sets the PWM top value to 1023, allowing for a 10-bit resolution.
  pwm_set_wrap(slice_pan, 1023);

  // Set duty cycle level for both pins (50% duty cycle)
  // Set the duty cycle (scaled up to match the wrap value)
  // Scales the 8-bit duty cycle value to match the 10-bit wrap.
  ///pwm_set_gpio_level(pwm_pin, dutyCycle * 1023 / 255);
  ///pwm_set_gpio_level(pwm_pin, (int)panDutyCycle);
  ///pwm_set_gpio_level(tiltPwmPin, dutyCycle * 1023 / 255);  DEFINIR MAIS TARDE

  // Set clock divider for both slices (adjust for desired frequency)
  // Configure frequency:
  // Set clock divider to adjust frequency (choose an appropriate divider for your frequency)
  // Adjusts the PWM frequency by dividing the clock speed.
  // To achieve a specific frequency like 2 kHz, PWM_freq = (125 MHz) / [(Clock_Divider)(Top + 1)]
  // For 2 kHz and Top = 1023, Clock_Divider = 61.0  
  ///pwm_set_clkdiv(slice_pan, 61.0f); // For 2 kHz frequency
  ///pwm_set_clkdiv(slice_tilt, 61.0f); // Same frequency for both

  pwm_set_clkdiv(slice_pan, 129.8f); // For 1 kHz frequency.

  ///pwm_set_clkdiv(slice_pan, 150.0f); // For ~800 Hz frequency
  ///pwm_set_clkdiv(slice_tilt, 150.0f); // Same frequency for both

  // Enable both slices
  pwm_set_enabled(slice_pan, true);

  pwm_set_gpio_level(pwm_pin, (int)panDutyCycle);

}
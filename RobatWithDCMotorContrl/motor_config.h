#pragma once

/* ===== PID SELECTION ===== */
#define USE_CLASSICAL_PID   1   // 1 = PID_v1, 0 = PID_vel

/* ===== FEATURE SWITCHES ===== */
#define USE_ZERO_NON_BLOCKING   1
#define USE_ENCODER_FILTER     0
#define DEBUG_SERIAL           1

/* ===== WATCHDOG ===== */
constexpr uint32_t HW_WATCHDOG_MS = 5000; // hardware watchdog timeout (ms)
constexpr uint32_t SW_WATCHDOG_MS = 3000;  // software watchdog timeout (ms)

/* ===== PID TUNING (GLOBAL DEFAULTS) ===== */
#if USE_CLASSICAL_PID
  constexpr double KP        = 5.0;
  constexpr double Ki_Ti     = 0.0;    // Ki
  constexpr double Kd_Td = 0.25;   // Kd
  constexpr int PID_SAMPLE_TIME = 10;
#else
  constexpr double KP        = 0.9;
  constexpr double Ki_Ti     = 9000.0; // Ti
  constexpr double Kd_Td = 0.0;    // Td
  constexpr int PID_SAMPLE_TIME = 100;
#endif

/* ===== SYSTEM CONSTANTS ===== */
constexpr int MAX_NUM_MOTORS = 5;
constexpr int COUNT_PER_REVOLUTION = 4350;        // ms
constexpr int MOTOR_DEAD_BAND_SPEED = 350;
constexpr int MAX_SPEED = 1023;
constexpr double MAX_ACCEPABLE_ERR = 0.1;
constexpr int MANUAL_FEED = 50; // percentage 1 - 100% of maximum feed

/* ===== ZERO INTERPOLATION ===== */
constexpr int ZERO_INTERPOLATION_FEED_MIN = 278;
constexpr int ZERO_INTERPOLATION_FEED_MAX = 280;

/* ===== ENCODER FILTER ===== */
constexpr unsigned ENCODER_FILTER_M1 = 50;  // µs
constexpr unsigned ENCODER_FILTER_M2 = 20;

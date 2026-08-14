#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// Power & Sleep Configuration
// =============================================================================

// Reflash safety window duration on initial cold boot (2 minutes). Used
// to ensure ESP goes not in deep sleep modus before you have a chance to
// reflash it.
#define REFLASH_SAFETY_WINDOW_MS   (120 * 1000UL)

// Total window duration to remain in SHORT_DEEP_SLEEP mode after reboot (40 minutes)
// It allows to have more measurement after the initial startup or after a reset. May help
// to troubleshoot receiver, dashboards or re-calibrate sensor.
#define SHORT_SLEEP_WINDOW_SEC     (40 * 60)

// Duration of short deep sleep per cycle (2 minutes)
#define SHORT_SLEEP_DURATION_SEC   (2 * 60)

// Duration of long deep sleep per cycle (12 hours). Normal 'production' sleep duration
// optimized to get enough data to track plants but save battery.
#define LONG_SLEEP_DURATION_SEC    (12 * 60 * 60)

#endif // CONFIG_H

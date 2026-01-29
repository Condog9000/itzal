#pragma once

/**
 * @brief Trigger the battery gauge display
 *
 * Shows the current battery level on the WS2812 LEDs
 * using a color gradient (red=low, green=full).
 * The display automatically turns off after the configured
 * duration (CONFIG_BATTERY_GAUGE_DISPLAY_MS).
 *
 * @return 0 on success, negative error code on failure
 */
int battery_gauge_show(void);

/**
 * @brief Turn off the battery gauge display
 *
 * Immediately turns off the battery gauge LEDs.
 *
 * @return 0 on success, negative error code on failure
 */
int battery_gauge_off(void);

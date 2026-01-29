#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/init.h>

#include <zmk_battery_gauge/battery_gauge.h>

LOG_MODULE_REGISTER(battery_gauge, CONFIG_LOG_DEFAULT_LEVEL);

#define LED_COUNT CONFIG_BATTERY_GAUGE_LED_COUNT
#define BRIGHTNESS_SCALE CONFIG_BATTERY_GAUGE_BRIGHTNESS

/* LED strip device */
#define LED_STRIP_NODE DT_CHOSEN(zmk_underglow)

#if !DT_NODE_EXISTS(LED_STRIP_NODE)
#error "No underglow LED strip chosen in devicetree"
#endif

static const struct device *led_strip = DEVICE_DT_GET(LED_STRIP_NODE);

/* Battery device */
#if DT_HAS_CHOSEN(zmk_battery)
#define BATTERY_NODE DT_CHOSEN(zmk_battery)
static const struct device *battery = DEVICE_DT_GET(BATTERY_NODE);
#else
static const struct device *battery = NULL;
#endif

/* Timer for auto-off */
static void battery_gauge_timer_handler(struct k_timer *timer);
K_TIMER_DEFINE(battery_gauge_timer, battery_gauge_timer_handler, NULL);

/* Work item for LED updates (required from timer context) */
static void battery_gauge_off_work_handler(struct k_work *work);
K_WORK_DEFINE(battery_gauge_off_work, battery_gauge_off_work_handler);

/*
 * Color scheme for battery gauge (stacked gradient)
 * LED 0 (top)    = Green  (full/100%)
 * LED 1          = Green  (80%+)
 * LED 2          = Yellow (60%+)
 * LED 3          = Orange (40%+)
 * LED 4 (bottom) = Red    (20%+)
 *
 * Colors are scaled by brightness config
 */
static const struct led_rgb gauge_colors[LED_COUNT] = {
    { .r = 0,   .g = 255, .b = 0   },  /* LED 0: Green (top - full) */
    { .r = 0,   .g = 255, .b = 0   },  /* LED 1: Green */
    { .r = 255, .g = 255, .b = 0   },  /* LED 2: Yellow */
    { .r = 255, .g = 128, .b = 0   },  /* LED 3: Orange */
    { .r = 255, .g = 0,   .b = 0   },  /* LED 4: Red (bottom - low) */
};

/**
 * Scale a color by the configured brightness
 */
static struct led_rgb scale_color(const struct led_rgb *color) {
    struct led_rgb scaled = {
        .r = (color->r * BRIGHTNESS_SCALE) / 255,
        .g = (color->g * BRIGHTNESS_SCALE) / 255,
        .b = (color->b * BRIGHTNESS_SCALE) / 255,
    };
    return scaled;
}

/**
 * Get current battery percentage
 */
static int get_battery_percent(void) {
    if (battery == NULL || !device_is_ready(battery)) {
        LOG_WRN("Battery device not available, returning 100%%");
        return 100;
    }

    struct sensor_value soc;
    int ret = sensor_sample_fetch(battery);
    if (ret < 0) {
        LOG_ERR("Failed to fetch battery sample: %d", ret);
        return 100;
    }

    ret = sensor_channel_get(battery, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE, &soc);
    if (ret < 0) {
        LOG_ERR("Failed to get battery SOC: %d", ret);
        return 100;
    }

    LOG_DBG("Battery SOC: %d%%", soc.val1);
    return soc.val1;
}

/**
 * Calculate how many LEDs to light based on battery percentage
 * Returns 1-5 (always show at least 1 LED if battery > 0)
 */
static int percent_to_led_count(int percent) {
    if (percent <= 0) {
        return 0;
    }
    /* Map 1-20% = 1 LED, 21-40% = 2 LEDs, etc. */
    return (percent + 19) / 20;
}

/**
 * Work handler for turning off LEDs (called from work queue)
 */
static void battery_gauge_off_work_handler(struct k_work *work) {
    ARG_UNUSED(work);
    battery_gauge_off();
}

/**
 * Timer handler for auto-off
 */
static void battery_gauge_timer_handler(struct k_timer *timer) {
    ARG_UNUSED(timer);
    /* Schedule work to turn off LEDs (can't do LED ops in timer context) */
    k_work_submit(&battery_gauge_off_work);
}

int battery_gauge_show(void) {
    if (!device_is_ready(led_strip)) {
        LOG_ERR("LED strip device not ready");
        return -ENODEV;
    }

    /* Cancel any pending timer */
    k_timer_stop(&battery_gauge_timer);

    int percent = get_battery_percent();
    int lit_count = percent_to_led_count(percent);

    LOG_INF("Battery: %d%%, lighting %d LEDs", percent, lit_count);

    struct led_rgb pixels[LED_COUNT] = {0};

    /*
     * Fill LEDs from bottom (LED 4) to top (LED 0)
     * LED 4 is the first to light (at 20%+)
     * LED 0 is the last to light (at 100%)
     */
    for (int i = 0; i < lit_count && i < LED_COUNT; i++) {
        int led_index = (LED_COUNT - 1) - i;  /* Start from bottom */
        pixels[led_index] = scale_color(&gauge_colors[led_index]);
    }

    int ret = led_strip_update_rgb(led_strip, pixels, LED_COUNT);
    if (ret < 0) {
        LOG_ERR("Failed to update LED strip: %d", ret);
        return ret;
    }

    /* Start auto-off timer */
    k_timer_start(&battery_gauge_timer,
                  K_MSEC(CONFIG_BATTERY_GAUGE_DISPLAY_MS),
                  K_NO_WAIT);

    return 0;
}

int battery_gauge_off(void) {
    if (!device_is_ready(led_strip)) {
        return -ENODEV;
    }

    k_timer_stop(&battery_gauge_timer);

    struct led_rgb pixels[LED_COUNT] = {0};
    int ret = led_strip_update_rgb(led_strip, pixels, LED_COUNT);
    if (ret < 0) {
        LOG_ERR("Failed to clear LED strip: %d", ret);
        return ret;
    }

    LOG_DBG("Battery gauge display off");

    return 0;
}

#if CONFIG_BATTERY_GAUGE_SHOW_ON_BOOT

static int battery_gauge_init(void) {
    LOG_INF("Battery gauge module initialized");

    if (!device_is_ready(led_strip)) {
        LOG_ERR("LED strip not ready at init");
        return -ENODEV;
    }

    /* Show battery level on boot */
    return battery_gauge_show();
}

/* Initialize after LED strip is ready (APPLICATION level, priority 90) */
SYS_INIT(battery_gauge_init, APPLICATION, 90);

#endif /* CONFIG_BATTERY_GAUGE_SHOW_ON_BOOT */

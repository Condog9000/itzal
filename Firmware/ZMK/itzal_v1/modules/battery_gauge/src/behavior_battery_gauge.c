#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>

#include <zmk_battery_gauge/battery_gauge.h>

LOG_MODULE_DECLARE(battery_gauge, CONFIG_LOG_DEFAULT_LEVEL);

#define DT_DRV_COMPAT zmk_behavior_battery_gauge

static int behavior_battery_gauge_init(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);

    LOG_DBG("Battery gauge behavior triggered");
    return battery_gauge_show();
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    /* Nothing to do on release */
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_battery_gauge_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

#define BAT_GAUGE_INST(n)                                                      \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_battery_gauge_init, NULL, NULL, NULL,  \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,  \
                            &behavior_battery_gauge_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BAT_GAUGE_INST)

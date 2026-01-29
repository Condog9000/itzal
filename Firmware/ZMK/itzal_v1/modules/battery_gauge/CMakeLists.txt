if(CONFIG_BATTERY_GAUGE)
    zephyr_include_directories(include)
    zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)

    zephyr_library_named(battery_gauge)
    zephyr_library_sources(
        src/battery_gauge.c
        src/behavior_battery_gauge.c
    )
endif()

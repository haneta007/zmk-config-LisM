/*
 * Copyright (c) 2026 haneta007
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT lism_input_processor_axis_lock

#include <limits.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <drivers/input_processor.h>

enum axis_lock_axis {
    AXIS_LOCK_NONE,
    AXIS_LOCK_X,
    AXIS_LOCK_Y,
};

struct axis_lock_config {
    uint16_t decision_threshold;
    uint16_t unlock_timeout_ms;
};

struct axis_lock_data {
    enum axis_lock_axis locked_axis;
    uint64_t x_magnitude;
    uint64_t y_magnitude;
    int64_t x_value;
    int64_t y_value;
    int64_t last_movement_timestamp;
};

static uint32_t magnitude(int32_t value) {
    return value < 0 ? (uint32_t)(-(int64_t)value) : (uint32_t)value;
}

static int32_t clamp_to_int32(int64_t value) {
    if (value > INT32_MAX) {
        return INT32_MAX;
    }

    if (value < INT32_MIN) {
        return INT32_MIN;
    }

    return (int32_t)value;
}

static void reset_axis_lock(struct axis_lock_data *data) {
    data->locked_axis = AXIS_LOCK_NONE;
    data->x_magnitude = 0;
    data->y_magnitude = 0;
    data->x_value = 0;
    data->y_value = 0;
}

static int axis_lock_handle_event(const struct device *dev, struct input_event *event,
                                  uint32_t param1, uint32_t param2,
                                  struct zmk_input_processor_state *state) {
    const struct axis_lock_config *cfg = dev->config;
    struct axis_lock_data *data = dev->data;

    ARG_UNUSED(param1);
    ARG_UNUSED(param2);
    ARG_UNUSED(state);

    if (event->type != INPUT_EV_REL ||
        (event->code != INPUT_REL_X && event->code != INPUT_REL_Y)) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    if (event->value != 0) {
        int64_t now = k_uptime_get();

        if (data->last_movement_timestamp > 0 &&
            now - data->last_movement_timestamp >= cfg->unlock_timeout_ms) {
            reset_axis_lock(data);
        }

        data->last_movement_timestamp = now;
    }

    bool is_x = event->code == INPUT_REL_X;

    if (data->locked_axis != AXIS_LOCK_NONE) {
        bool is_locked_axis = (data->locked_axis == AXIS_LOCK_X && is_x) ||
                              (data->locked_axis == AXIS_LOCK_Y && !is_x);

        if (!is_locked_axis) {
            event->value = 0;
        }

        return ZMK_INPUT_PROC_CONTINUE;
    }

    if (event->value != 0) {
        if (is_x) {
            data->x_magnitude += magnitude(event->value);
            data->x_value += event->value;
        } else {
            data->y_magnitude += magnitude(event->value);
            data->y_value += event->value;
        }
    }

    if (!event->sync ||
        data->x_magnitude + data->y_magnitude < cfg->decision_threshold) {
        event->value = 0;
        return ZMK_INPUT_PROC_CONTINUE;
    }

    bool lock_x = data->x_magnitude >= data->y_magnitude;
    data->locked_axis = lock_x ? AXIS_LOCK_X : AXIS_LOCK_Y;
    event->code = lock_x ? INPUT_REL_X : INPUT_REL_Y;
    event->value = clamp_to_int32(lock_x ? data->x_value : data->y_value);
    data->x_magnitude = 0;
    data->y_magnitude = 0;
    data->x_value = 0;
    data->y_value = 0;

    return ZMK_INPUT_PROC_CONTINUE;
}

static const struct zmk_input_processor_driver_api axis_lock_driver_api = {
    .handle_event = axis_lock_handle_event,
};

#define AXIS_LOCK_INST(n)                                                                          \
    BUILD_ASSERT(DT_INST_PROP(n, decision_threshold) > 0,                                          \
                 "decision-threshold must be positive");                                          \
    BUILD_ASSERT(DT_INST_PROP(n, unlock_timeout_ms) > 0,                                           \
                 "unlock-timeout-ms must be positive");                                           \
    static const struct axis_lock_config axis_lock_config_##n = {                                  \
        .decision_threshold = DT_INST_PROP(n, decision_threshold),                                 \
        .unlock_timeout_ms = DT_INST_PROP(n, unlock_timeout_ms),                                   \
    };                                                                                             \
    static struct axis_lock_data axis_lock_data_##n;                                               \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, &axis_lock_data_##n, &axis_lock_config_##n, POST_KERNEL,  \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &axis_lock_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AXIS_LOCK_INST)

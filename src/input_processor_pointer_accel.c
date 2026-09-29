/*
 * Copyright (c) 2026 haneta007
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT lism_input_processor_pointer_accel

#include <limits.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <drivers/input_processor.h>

struct pointer_accel_config {
    uint16_t base_cpi;
    uint16_t max_cpi;
    uint16_t accel_start;
    uint16_t accel_full;
};

static uint32_t effective_cpi(const struct pointer_accel_config *cfg, uint32_t magnitude) {
    if (magnitude <= cfg->accel_start) {
        return cfg->base_cpi;
    }

    if (magnitude >= cfg->accel_full) {
        return cfg->max_cpi;
    }

    uint32_t progress = magnitude - cfg->accel_start;
    uint32_t range = cfg->accel_full - cfg->accel_start;
    uint32_t cpi_range = cfg->max_cpi - cfg->base_cpi;

    return cfg->base_cpi + (cpi_range * progress) / range;
}

static int pointer_accel_handle_event(const struct device *dev, struct input_event *event,
                                      uint32_t param1, uint32_t param2,
                                      struct zmk_input_processor_state *state) {
    const struct pointer_accel_config *cfg = dev->config;

    ARG_UNUSED(param1);
    ARG_UNUSED(param2);

    if (event->type != INPUT_EV_REL ||
        (event->code != INPUT_REL_X && event->code != INPUT_REL_Y)) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    int64_t raw_value = event->value;
    uint32_t magnitude = raw_value < 0 ? (uint32_t)-raw_value : (uint32_t)raw_value;
    uint32_t cpi = effective_cpi(cfg, magnitude);
    int64_t numerator = raw_value * cpi;

    if (state != NULL && state->remainder != NULL) {
        numerator += *state->remainder;
    }

    int64_t scaled = numerator / cfg->base_cpi;
    bool clamped = false;

    if (scaled > INT16_MAX) {
        scaled = INT16_MAX;
        clamped = true;
    } else if (scaled < INT16_MIN) {
        scaled = INT16_MIN;
        clamped = true;
    }

    if (state != NULL && state->remainder != NULL) {
        *state->remainder =
            clamped ? 0 : (int16_t)(numerator - (scaled * cfg->base_cpi));
    }

    event->value = (int32_t)scaled;
    return ZMK_INPUT_PROC_CONTINUE;
}

static const struct zmk_input_processor_driver_api pointer_accel_driver_api = {
    .handle_event = pointer_accel_handle_event,
};

#define POINTER_ACCEL_INST(n)                                                                      \
    BUILD_ASSERT(DT_INST_PROP(n, base_cpi) > 0, "base-cpi must be positive");                    \
    BUILD_ASSERT(DT_INST_PROP(n, max_cpi) >= DT_INST_PROP(n, base_cpi),                           \
                 "max-cpi must be at least base-cpi");                                           \
    BUILD_ASSERT(DT_INST_PROP(n, accel_full) > DT_INST_PROP(n, accel_start),                      \
                 "accel-full must be greater than accel-start");                                 \
    static const struct pointer_accel_config pointer_accel_config_##n = {                         \
        .base_cpi = DT_INST_PROP(n, base_cpi),                                                     \
        .max_cpi = DT_INST_PROP(n, max_cpi),                                                       \
        .accel_start = DT_INST_PROP(n, accel_start),                                               \
        .accel_full = DT_INST_PROP(n, accel_full),                                                 \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &pointer_accel_config_##n, POST_KERNEL,             \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &pointer_accel_driver_api);

DT_INST_FOREACH_STATUS_OKAY(POINTER_ACCEL_INST)

#ifndef LV_CONF_H
#define LV_CONF_H

// Selective overrides only - every LV_* setting not mentioned here falls
// back to LVGL's own default (see lv_conf_internal.h), which is the
// documented, supported way to use a minimal lv_conf.h with
// -DLV_CONF_INCLUDE_SIMPLE.

#define LV_COLOR_DEPTH 16

// LVGL's builtin allocator, backed by a fixed-size static arena
// (LV_MEM_SIZE) kept in internal RAM - LVGL's own object/style metadata is
// made of many small, latency-sensitive allocations, so it deliberately
// does NOT share the PSRAM-backed frame buffers allocated in
// LvglDisplay.cpp.
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (48 * 1024)

// No RTOS task pump here - main.cpp calls lv_timer_handler() once per
// Arduino loop() iteration instead.
#define LV_USE_OS LV_OS_NONE

// Arduino's millis() as LVGL's tick source, instead of a periodic ISR.
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#endif

// Widgets used by the navigation shell (Phase 4) and already-planned views
// (see plan.md Section 5/7) - listed explicitly so later phases don't need
// to revisit this file.
#define LV_USE_TABVIEW 1
#define LV_USE_LABEL 1
#define LV_USE_BUTTON 1
#define LV_USE_BAR 1
#define LV_USE_SWITCH 1
#define LV_USE_SLIDER 1
#define LV_USE_ARC 1
#define LV_USE_ROLLER 1
#define LV_USE_COLORWHEEL 1
#define LV_USE_LIST 1
#define LV_USE_SCALE 1
#define LV_USE_QRCODE 1

// Larger readable size for ValueView's numeric labels (Phase 5) - the
// default LV_FONT_MONTSERRAT_14 stays enabled by LVGL's own default and
// remains every other widget's font.
#define LV_FONT_MONTSERRAT_24 1

#endif  // LV_CONF_H

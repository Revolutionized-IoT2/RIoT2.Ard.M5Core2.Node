#ifndef LV_CONF_H
#define LV_CONF_H

// Selective overrides only - every LV_* setting not mentioned here falls
// back to LVGL's own default (see lv_conf_internal.h), which is the
// documented, supported way to use a minimal lv_conf.h with
// -DLV_CONF_INCLUDE_SIMPLE.

// 16bpp (RGB565), matching the panel's native format - reverted back from a
// 32bpp (XRGB8888) + downconvert experiment (theory: LVGL's RGB565 blend
// math quantizes to 5/6/5 bits *before* alpha-mixing AA glyph edges, see
// git history) that built and ran but did not measurably reduce the
// reported "grainy" text on real hardware, so it wasn't worth its ~2x RAM/
// CPU cost. See LvglDisplay.cpp for the current buffering approach (small
// internal-RAM partial buffers, matching 0xxon/LVGL-PlatformIO-Example).
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

// Arduino's millis() as LVGL's tick source. NOTE: LVGL 9.x no longer reads
// LV_TICK_CUSTOM/LV_TICK_CUSTOM_INCLUDE/LV_TICK_CUSTOM_SYS_TIME_EXPR at all
// (that was the v8 mechanism) - lv_tick.c ignores them completely. The
// actual tick source must be registered at runtime via lv_tick_set_cb(),
// which is done in LvglDisplay::begin(). These defines are kept only so a
// future downgrade to LVGL v8 (or anyone skimming this file expecting the
// old mechanism) isn't misled into thinking no tick source is configured;
// they have no effect in LVGL 9.x.
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

// NOTE: LV_FONT_UNSCII_16 was tried here as an anti-aliasing A/B test and
// reverted - it rendered corrupted/overlapping glyphs on this display
// (not just a different style), so bpp=1 bitmap fonts are not a viable
// option here without further investigation.
//
// QrSettingsView's header/SSID labels use a custom-generated bpp=8 build of
// Montserrat 18px instead (src/lv_font_montserrat_18_bpp8.c) - no lv_conf.h
// flag needed for it, it's a standalone font source file.

#endif  // LV_CONF_H

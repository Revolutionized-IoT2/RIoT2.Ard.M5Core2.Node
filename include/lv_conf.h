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

// NOTE: this arena used to be a static internal-DRAM array sized 48KB
// (even below LVGL's own 64KB default) - too small once ColorSchemeView's
// gradient-heavy hue-bar/SV-pad rebuild (many more lv_obj_t + per-object
// styles/gradients than any other view) is added on top of every other
// eagerly-built tab. LV_USE_ASSERT_MALLOC defaults to 1 and
// LV_ASSERT_HANDLER defaults to a silent `while(1);` halt (LV_USE_LOG is 0
// by default too - no message printed first), so exhausting this arena
// doesn't crash/reboot, it completely and silently freezes the device.
// Internal DRAM turned out to have only ~12KB of dram0_0_seg headroom left
// on this build (confirmed by bisecting a plain size increase - even 64KB
// already overflowed dram0_0_seg by 4064 bytes) once WiFi/BLE/etc.'s own
// static allocations are accounted for - not enough margin for future
// views. So instead of fighting internal DRAM for a few more KB, point
// LVGL's allocator pool at PSRAM (8MB, effectively unlimited here) via
// LV_MEM_POOL_INCLUDE/LV_MEM_POOL_ALLOC (see lv_mem_core_builtin.c - when
// these are defined it calls LV_MEM_POOL_ALLOC(LV_MEM_SIZE) for the pool
// instead of a static internal-RAM array). If ColorSchemeView (or any
// future view) still somehow exhausts this much larger pool, see
// LvglDisplay.cpp::lvglLogPrintCb() - an LV_ASSERT_MALLOC failure now
// prints "[LVGL] ... Failed to allocate ..." over Serial right before
// halting, instead of freezing silently.
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN
#define LV_MEM_POOL_INCLUDE "esp_heap_caps.h"
#define LV_MEM_POOL_ALLOC(size) heap_caps_malloc((size), MALLOC_CAP_SPIRAM)
#define LV_MEM_SIZE (512 * 1024)

// Routes LV_LOG_WARN/ERROR (incl. LV_ASSERT_MALLOC failure messages, e.g.
// "Failed to allocate item for the gradient") to a callback registered in
// LvglDisplay.cpp via lv_log_register_print_cb() - without this, an
// LV_ASSERT_MALLOC failure hits LV_ASSERT_HANDLER's default `while(1);`
// halt completely silently (no message at all), which is indistinguishable
// from any other freeze. LV_LOG_PRINTF stays 0 since vendored/library code
// calling raw libc printf() has been observed to stop reaching the serial
// terminal shortly after boot on this target - our callback uses
// Serial.printf() instead (see LvglDisplay.cpp::lvglLogPrintCb()).
#define LV_USE_LOG 1

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
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_BAR 1
#define LV_USE_SWITCH 1
#define LV_USE_SLIDER 1
#define LV_USE_ARC 1
#define LV_USE_ROLLER 1
#define LV_USE_COLORWHEEL 1
#define LV_USE_LIST 1
#define LV_USE_SCALE 1
#define LV_USE_TEXTAREA 1
#define LV_USE_SPINBOX 1
#define LV_USE_QRCODE 1

// Larger readable size for ValueView's numeric labels (Phase 5) - the
// default LV_FONT_MONTSERRAT_14 stays enabled by LVGL's own default and
// remains every other widget's font.
#define LV_FONT_MONTSERRAT_24 1

// Extra-large size for ClockView's time label.
#define LV_FONT_MONTSERRAT_48 1

// NOTE: LV_FONT_UNSCII_16 was tried here as an anti-aliasing A/B test and
// reverted - it rendered corrupted/overlapping glyphs on this display
// (not just a different style), so bpp=1 bitmap fonts are not a viable
// option here without further investigation.
//
// QrSettingsView's header/SSID labels use a custom-generated bpp=8 build of
// Montserrat 18px instead (src/lv_font_montserrat_18_bpp8.c) - no lv_conf.h
// flag needed for it, it's a standalone font source file.

#endif  // LV_CONF_H

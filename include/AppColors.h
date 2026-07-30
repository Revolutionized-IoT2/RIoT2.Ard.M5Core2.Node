#pragma once

#include <lvgl.h>

// Shared "Material Design Indigo" palette - keeps every view's chrome/accent
// colors (tab bar, menu tiles, buttons, gauge needle, popups, ...) drawing
// from the same set of hex values instead of each picking its own ad-hoc
// one, and matches the indigo-tinted icons under assets/icons (their baked
// -in circle background is exactly indigo() below, 0x3F51B5).
namespace AppColors {
inline lv_color_t indigo() { return lv_color_hex(0x3F51B5); }
inline lv_color_t indigoLighten5() { return lv_color_hex(0xE8EAF6); }
inline lv_color_t indigoLighten4() { return lv_color_hex(0xC5CAE9); }
inline lv_color_t indigoLighten3() { return lv_color_hex(0x9FA8DA); }
inline lv_color_t indigoLighten2() { return lv_color_hex(0x7986CB); }
inline lv_color_t indigoLighten1() { return lv_color_hex(0x5C6BC0); }
inline lv_color_t indigoDarken1() { return lv_color_hex(0x3949AB); }
inline lv_color_t indigoDarken2() { return lv_color_hex(0x303F9F); }
inline lv_color_t indigoDarken3() { return lv_color_hex(0x283593); }
inline lv_color_t indigoDarken4() { return lv_color_hex(0x1A237E); }
inline lv_color_t indigoAccent1() { return lv_color_hex(0x8C9EFF); }
inline lv_color_t indigoAccent2() { return lv_color_hex(0x536DFE); }
inline lv_color_t indigoAccent3() { return lv_color_hex(0x3D5AFE); }
inline lv_color_t indigoAccent4() { return lv_color_hex(0x304FFE); }
}  // namespace AppColors

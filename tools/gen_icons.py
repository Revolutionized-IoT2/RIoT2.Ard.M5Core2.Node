from PIL import Image
import os

src_dir = r"C:\Src\RIoT2\RIoT2.Ard.M5Core2.Node\assets\icons"
out_c = r"C:\Src\RIoT2\RIoT2.Ard.M5Core2.Node\src\Icons.c"
out_h = r"C:\Src\RIoT2\RIoT2.Ard.M5Core2.Node\include\Icons.h"

# Only the icons actually wired up in NavigationController's
# iconForClassFullName() are embedded here - assets/icons has several extra
# icons (CamIndoor, CamOut, Hub, Motion, RFID, Shield, ShieldLock, Sliders,
# Thermostat, Touch, ...) with no matching View yet; they're kept as source
# PNGs for future use but not compiled in, to avoid spending flash on
# images nothing currently references.
names = [
    "Alert", "BLE", "Button", "Clock", "Gauge", "Light",
    "Notification", "Percent", "Scene", "Slider", "Switch", "Timer", "Values",
]
files = [os.path.join(src_dir, name + ".png") for name in names]

def ident(stem):
    return "icon_" + stem.lower()

c_lines = []
c_lines.append('#include "Icons.h"')
c_lines.append('')

h_lines = []
h_lines.append('#pragma once')
h_lines.append('')
h_lines.append('#include <lvgl.h>')
h_lines.append('')
h_lines.append('#ifdef __cplusplus')
h_lines.append('extern "C" {')
h_lines.append('#endif')
h_lines.append('')

for f in files:
    stem = os.path.splitext(os.path.basename(f))[0]
    cid = ident(stem)
    im = Image.open(f).convert("RGBA")
    w, h = im.size
    px = im.load()

    rgb_bytes = bytearray()
    alpha_bytes = bytearray()
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            r5 = r >> 3
            g6 = g >> 2
            b5 = b >> 3
            val = (r5 << 11) | (g6 << 5) | b5
            rgb_bytes.append(val & 0xFF)
            rgb_bytes.append((val >> 8) & 0xFF)
            alpha_bytes.append(a)

    data = bytes(rgb_bytes) + bytes(alpha_bytes)
    stride = w * 2

    c_lines.append(f'// Converted from {os.path.basename(f)} ({w}x{h}, RGB565A8: RGB565 plane + A8 alpha plane)')
    c_lines.append(f'static const uint8_t {cid}_map[] = {{')
    row = []
    for byte in data:
        row.append(f'0x{byte:02x}')
        if len(row) == 16:
            c_lines.append('    ' + ','.join(row) + ',')
            row = []
    if row:
        c_lines.append('    ' + ','.join(row) + ',')
    c_lines.append('};')
    c_lines.append('')
    c_lines.append(f'const lv_image_dsc_t {cid} = {{')
    c_lines.append(f'    .header = {{')
    c_lines.append(f'        .magic = LV_IMAGE_HEADER_MAGIC,')
    c_lines.append(f'        .cf = LV_COLOR_FORMAT_RGB565A8,')
    c_lines.append(f'        .flags = 0,')
    c_lines.append(f'        .w = {w},')
    c_lines.append(f'        .h = {h},')
    c_lines.append(f'        .stride = {stride},')
    c_lines.append(f'        .reserved_2 = 0,')
    c_lines.append(f'    }},')
    c_lines.append(f'    .data_size = sizeof({cid}_map),')
    c_lines.append(f'    .data = {cid}_map,')
    c_lines.append(f'    .reserved = NULL,')
    c_lines.append(f'    .reserved_2 = NULL,')
    c_lines.append('};')
    c_lines.append('')

    h_lines.append(f'extern const lv_image_dsc_t {cid};')

h_lines.append('')
h_lines.append('#ifdef __cplusplus')
h_lines.append('}')
h_lines.append('#endif')
h_lines.append('')

with open(out_c, "w", newline='\n') as fh:
    fh.write('\n'.join(c_lines))

with open(out_h, "w", newline='\n') as fh:
    fh.write('\n'.join(h_lines))

print("wrote", out_c, os.path.getsize(out_c), "bytes")
print("wrote", out_h)
print("identifiers:", [ident(os.path.splitext(os.path.basename(f))[0]) for f in files])

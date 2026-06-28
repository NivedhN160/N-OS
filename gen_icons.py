import math

def to_c_array(name, w, h, pixel_func):
    lines = [f"const unsigned int icon_{name}[{w*h}] = {{"]
    for y in range(h):
        row = []
        for x in range(w):
            r, g, b, a = pixel_func(x, y, w, h)
            val = (int(a) << 24) | (int(r) << 16) | (int(g) << 8) | int(b)
            row.append(f"0x{val:08X}")
        lines.append("    " + ", ".join(row) + ",")
    lines.append("};")
    return "\n".join(lines)

def firefox_icon(x, y, w, h):
    cx, cy = w/2, h/2
    dx, dy = x - cx, y - cy
    dist = math.sqrt(dx*dx + dy*dy)
    if dist > w/2 - 1: return 0,0,0,0
    r, g, b = 0, 100, 200
    if (y < cy and x < cx + 4) or (x > cx and y > cy - 2) or (x < cx and y > cy + 2):
        r, g, b = 255, 120, 0
        if y < cy/2: r,g,b = 255,180,0
        if dx*dx + dy*dy < (w/4)*(w/4) and x > cx: r,g,b=0,100,200 
    return r, g, b, 255

def folder_icon(x, y, w, h):
    if y < 2 or y > h-2 or x < 1 or x > w-2: return 0,0,0,0
    if y < 4 and x > w/2: return 0,0,0,0
    r, g, b = 255, 210, 80
    if y == 2 or x == 1: r,g,b = 255,230,120
    if y > h-4: r,g,b = 220,180,50
    return r, g, b, 255

def notepad_icon(x, y, w, h):
    if y < 1 or y > h-2 or x < 2 or x > w-3: return 0,0,0,0
    r,g,b = 250,250,250
    if y % 3 == 0 and x > 4 and x < w-4: r,g,b = 150,150,250 
    if x < 4: r,g,b = 200,0,0 
    return r, g, b, 255

def terminal_icon(x, y, w, h):
    if y < 1 or y > h-2 or x < 1 or x > w-2: return 0,0,0,0
    if y < 3: return 100,100,100,255 
    r,g,b = 20,20,20
    if x > 2 and x < 5 and y > 4 and y < 8: r,g,b = 0,255,0
    return r, g, b, 255

def settings_icon(x, y, w, h):
    cx, cy = w/2, h/2
    dx, dy = x - cx, y - cy
    dist = math.sqrt(dx*dx + dy*dy)
    if dist > w/2 - 1: return 0,0,0,0
    if dist < w/4: return 0,0,0,0 
    angle = math.atan2(dy, dx)
    if dist > w/2 - 3 and math.sin(angle * 6) < 0: return 0,0,0,0
    return 150, 150, 150, 255

def calc_icon(x, y, w, h):
    if y < 1 or y > h-2 or x < 2 or x > w-3: return 0,0,0,0
    if y < 5: return 150, 200, 150, 255 
    r,g,b = 80,80,80
    if (x-2)%4 < 3 and (y-5)%3 < 2: r,g,b = 200,200,200 
    return r,g,b,255

def start_sphere(x, y, w, h):
    cx, cy = w/2, h/2
    dx, dy = x - cx, y - cy
    dist = math.sqrt(dx*dx + dy*dy)
    if dist > w/2 - 1: return 0,0,0,0
    r, g, b = 20, 50, 150
    if dx < 0 and dy < 0: r,g,b = min(255, r+80), min(255, g+80), min(255, b+80)
    if (x > cx-4 and x < cx-2) or (x > cx+2 and x < cx+4) or abs(dx-dy) < 1.5:
        if abs(dx) < 5 and abs(dy) < 6: return 255,255,255,255
    return r,g,b,255

with open("kernel/icons.h", "w") as f:
    f.write("#ifndef ICONS_H\n#define ICONS_H\n\n")
    f.write(to_c_array("firefox", 16, 16, firefox_icon) + "\n\n")
    f.write(to_c_array("folder", 16, 16, folder_icon) + "\n\n")
    f.write(to_c_array("notepad", 16, 16, notepad_icon) + "\n\n")
    f.write(to_c_array("terminal", 16, 16, terminal_icon) + "\n\n")
    f.write(to_c_array("settings", 16, 16, settings_icon) + "\n\n")
    f.write(to_c_array("calc", 16, 16, calc_icon) + "\n\n")
    f.write(to_c_array("start", 24, 24, start_sphere) + "\n\n")
    f.write("#endif\n")

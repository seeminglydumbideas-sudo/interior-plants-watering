import os
from PIL import Image

def color_to_565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

# TFT_MAGENTA is 0xF81F
TRANSPARENT_COLOR_565 = 0xF81F

def process_image(path, name):
    # Load RGBA image
    img = Image.open(path).convert('RGBA')
    
    pixels = list(img.getdata())
    out = []
    for p in pixels:
        # p is (r, g, b, a)
        if p[3] < 128:  # Transparent or semi-transparent pixel
            out.append(hex(TRANSPARENT_COLOR_565))
        else:
            out.append(hex(color_to_565(p[0], p[1], p[2])))
    
    return f"const uint16_t {name}[1024] PROGMEM = {{\n  " + ", ".join(out) + "\n};\n"

header = "#ifndef SPRITES_H\n#define SPRITES_H\n\n#include <Arduino.h>\n\n"

state_map = {'h': '2h', 'u': '1u', 'd': '0d'}
for i in range(4):
    for state in ['h', 'u', 'd']:
        var_name = f"tree_{i}_{state}"
        path = f"sprites/tree_{i}_{state_map[state]}.png"
        if os.path.exists(path):
            print(f"Applying chroma key for {path}...")
            header += process_image(path, var_name)

# Process shed
if os.path.exists("sprites/shed.png"):
    print("Applying chroma key for sprites/shed.png...")
    header += process_image("sprites/shed.png", "shed_sprite")

# Process wet floor sign
if os.path.exists("sprites/wet_floor.png"):
    print("Applying chroma key for sprites/wet_floor.png...")
    # It's 16x16, let's adjust process_image to handle size dynamically
    img = Image.open("sprites/wet_floor.png").convert('RGBA')
    w, h = img.size
    pixels = list(img.getdata())
    out = []
    for p in pixels:
        if p[3] < 128: out.append(hex(0xF81F))
        else: out.append(hex(color_to_565(p[0], p[1], p[2])))
    header += f"const uint16_t wet_floor_sprite[{w*h}] PROGMEM = {{\n  " + ", ".join(out) + "\n};\n"

# Process scene
if os.path.exists("sprites/scene2.png"):
    print("Processing sprites/scene2.png...")
    img = Image.open("sprites/scene2.png").convert('RGBA')
    w, h = img.size
    pixels = list(img.getdata())
    out = []
    for p in pixels:
        if p[3] < 128: out.append(hex(0x0000)) # Black for transparent pixels
        else: out.append(hex(color_to_565(p[0], p[1], p[2])))
    header += f"const uint16_t scene_sprite[{w*h}] PROGMEM = {{\n  " + ", ".join(out) + "\n};\n\n"

# Process rain animation
if os.path.exists("sprites/rain.png"):
    print("Processing sprites/rain.png...")
    rain_img = Image.open("sprites/rain.png").convert('RGBA')
    for frame in range(4):
        row = frame // 2
        col = frame % 2
        left = col * 32
        top = row * 32
        right = left + 32
        bottom = top + 32
        
        sprite_crop = rain_img.crop((left, top, right, bottom))
        pixels = list(sprite_crop.getdata())
        out = []
        for p in pixels:
            if p[3] < 128: out.append(hex(0xF81F)) # TFT_MAGENTA transparency masking
            else: out.append(hex(color_to_565(p[0], p[1], p[2])))
        
        var_name = f"rain_sprite_{frame}"
        header += f"const uint16_t {var_name}[1024] PROGMEM = {{\n  " + ", ".join(out) + "\n};\n\n"

# Process gardener sprites
gardener_path = "sprites/gardener.png"
if not os.path.exists(gardener_path) and os.path.exists("sprites/gsrdener.png"):
    import shutil
    shutil.copy("sprites/gsrdener.png", gardener_path)
    print("Copied sprites/gsrdener.png to sprites/gardener.png for consistency.")

if os.path.exists(gardener_path):
    print(f"Processing {gardener_path}...")
    gardener_img = Image.open(gardener_path).convert('RGBA')
    names = [
        ["gardener_f", "gardener_f_l", "gardener_f_r"],
        ["gardener_b", "gardener_b_l", "gardener_b_r"],
        ["gardener_r", "gardener_r_l", "gardener_r_r"]
    ]
    for row in range(3):
        for col in range(3):
            left = col * 16
            top = row * 16
            right = left + 16
            bottom = top + 16
            sprite_crop = gardener_img.crop((left, top, right, bottom))
            
            pixels = list(sprite_crop.getdata())
            out = []
            for p in pixels:
                if p[3] < 128: out.append(hex(0xF81F)) # TFT_MAGENTA transparency masking
                else: out.append(hex(color_to_565(p[0], p[1], p[2])))
            
            var_name = names[row][col]
            header += f"const uint16_t {var_name}[256] PROGMEM = {{\n  " + ", ".join(out) + "\n};\n\n"

header += "\n#endif"

with open("Sprites.h", "w") as f:
    f.write(header)

print("Created Sprites.h with TFT_MAGENTA (0xF81F) transparency masking.")

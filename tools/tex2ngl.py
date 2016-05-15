#!/usr/bin/python
import os
import sys
from PIL import Image

def ngl_name(string):
    """Makes string an almost directly usable C identifier.
    May start with a digit, so only use as suffix."""
    return "".join([c if c.isalnum() else "_" for c in string])

def color2ngl(r, g, b, _=0):
    return (r & 0b11111000) << 8 | (g & 0b11111100) << 3 | (b & 0b11111000) >> 3

def tex2ngl(src):
    """Converts src to C header containing nGL TEXTURE. Returns tuple(code, object name, tuple(w, h))."""
    img = Image.open(src)
    width, height = img.size
    name = ngl_name(os.path.splitext(os.path.basename(src))[0])

    out = "COLOR texdata_{name}[] = {{\n".format(name=name)
    i = 0
    for rgb in img.getdata():
        out += "0x" + format(color2ngl(*rgb), "04x") + ", "
        i += 1
        if i % width == 0:
            out += "\n    "
    out += "};\n\n"

    out += """constexpr const TEXTURE tex_{name} = {{
    .width = {width},
    .height = {height},
    .has_transparency = false,
    .transparent_color = 0x0000,
    .bitmap = texdata_{name}
}};

""".format(name=name, width=width, height=height)

    return out, "tex_" + name, (width, height)

def main(argv):
    if len(argv) != 2 and len(argv) != 3:
        print("Usage: tex2ngl.py <input.png> (<output.h>)\nDoes not yet support transparency")
        return 2

    outputpath = argv[2] if len(argv) > 2 else os.path.splitext(argv[1])[0] + ".h"
    return 0 if open(outputpath, "w").write(tex2ngl(argv[1])[0]) else 1

if __name__ == "__main__":
    sys.exit(main(sys.argv))
    

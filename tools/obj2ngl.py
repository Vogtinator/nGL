#!/usr/bin/python
from __future__ import print_function

import os
import sys

from tex2ngl import color2ngl, ngl_name, tex2ngl

# Globals:
# For a description of materials, see handle_newmtl.
# For a description of objects, see handle_o
materials = {} # Dict of all materials
current_material = "" # Index into materials
objects = {} # Dict of all objects
current_object = "" # Index into objects
usemtl = "" # Current selected material
ngl_textures = {} # Dict of filename -> (objname, (w, h))

# Dict of all vectors (list of floats)
vectors = {'position': [], 'texture': []}

def syntax_error(error, file, line):
    print("{1}:{2}: {0}".format(error, file, line))
    return False

def args_to_vector(args):
    """Parse vector: "1.3 0.0005 0.1" => [1.3, 0.0005, 0.1]"""
    return [float(f) for f in args]

def maybe_int(string):
    return -1 if len(string) == 0 else int(string)

def args_to_face(args):
    """Parse face: "1/2 3/5 6/7" => [[1, 2], [3, 5], [6, 7]]"""
    return [[maybe_int(i) for i in c.split("/")] for c in args]

def handle_mtllib(args, file, line):
    return parse_file(" ".join(args[1:]))

def handle_o(args, file, line):
    if len(args) != 2:
        return syntax_error("Expected exactly one argument", file, line)

    global objects
    global current_object

    current_object = args[1]
    objects[current_object] = {'faces': [], 'material': None, 'face_type': None}
    return True

def handle_newmtl(args, file, line):
    if len(args) != 2:
        return syntax_error("Expected exactly one argument", file, line)

    global materials
    global current_material

    current_material = args[1]
    materials[current_material] = {'colors': {'ambient': None, 'diffuse': None, 'specular': None, 'e': None},
                                   'maps': {'ambient': None, 'diffuse': None, 'specular': None, 'e': None}}
    return True

def handle_v(args, file, line):
    if len(args) != 4:
        return syntax_error("Expected exactly three arguments", file, line)

    global vectors

    vectors["position"] += [args_to_vector(args[1:4])]
    return True


def handle_vt(args, file, line):
    if len(args) != 3 and len(args) != 4:
        return syntax_error("Expected two or three arguments", file, line)

    global vectors

    vectors["texture"] += [args_to_vector(args[1:3])]
    return True

def handle_f(args, file, line):
    if len(args) < 3:
        return syntax_error("Expected at least three arguments", file, line)

    global objects
    global current_object
    global current_usemtl
    
    # Emulate object with multiple materials as multiple objects
    material = objects[current_object]['material']
    face_type = objects[current_object]['face_type']
    if (material is not None and material != current_usemtl) or (face_type is not None and face_type != len(args) - 1):
        handle_o(["o", current_object + "_" + str(line)], file, line)

    objects[current_object]['face_type'] = len(args) - 1
    if objects[current_object]['material'] is None:
        objects[current_object]['material'] = current_usemtl

    objects[current_object]['faces'] += [args_to_face(args[1:])]
    return True

def handle_usemtl(args, file, line):
    if len(args) != 2:
        return syntax_error("Expected exactly one argument", file, line)

    global current_usemtl

    current_usemtl = args[1]
    return True

map_K_str = {'Ka': "ambient", 'Kd': "diffuse", 'Ks': "specular", 'Ke': "e"}

def handle_K(args, file, line):
    if len(args) != 4:
        return syntax_error("Expected exactly four arguments", file, line)

    global materials
    global current_material

    materials[current_material]['colors'][map_K_str[args[0]]] = args_to_vector(args[1:4])

    return True

def handle_map_K(args, file, line):
    if len(args) != 2:
        return syntax_error("Expected exactly one argument", file, line)

    global materials
    global current_material

    materials[current_material]['maps'][map_K_str[args[0][4:]]] = args[1]

    return True

def handler_404(args, file, line):
    return syntax_error("Command {0!r} unimplemented or invalid".format(args[0]), file, line)

def ignore(args, file, line):
    return True

handlers = {
    'mtllib':   handle_mtllib,
    'newmtl':   handle_newmtl,
    'o':        handle_o,
    'g':        handle_o,
    'Ns':       ignore,
    'Ni':       ignore,
    'illum':    ignore,
    'd':        ignore,
    'Ka':       handle_K,
    'Kd':       handle_K,
    'Ks':       handle_K,
    'Ke':       handle_K,
    'map_Ka':   handle_map_K,
    'map_Kd':   handle_map_K,
    'map_d':    ignore,
    'v':        handle_v,
    'vt':       handle_vt,
    'vn':       ignore,
    'usemtl':   handle_usemtl,
    'Tr':       ignore,
    'Tf':       ignore,
    's':        ignore,
    'f':        handle_f,
    'l':        handle_f # Treat line as face with two points
    }

def load_obj(path):
    global objects
    objects = {}
    global current_object
    current_object = ""
    global materials
    materials = {}
    global current_material
    current_material = ""
    # Add a stub object, in case the file itself doesn't contain 'o'
    handle_o(["o", os.path.basename(path)], "<internal>", -1)
    return parse_file(path)

def parse_file(path):
    with open(path) as objfile:
        os.chdir(os.path.dirname(os.path.abspath(path)))
        for lineidx, line in enumerate(objfile.readlines()):
            args = line.split();
            if len(args) == 0 or args[0].startswith("#"):
                continue # Comment or empty

            handler = handlers.get(args[0], handler_404)
            if not handler(args, os.path.basename(path), lineidx + 1):
                return False
    return True

def ngl_vertex(obj, vertex, color, position_min):
    global objects
    global materials
    global ngl_textures
    global vectors

    u = 0.0
    v = 0.0

    material = objects[obj]['material']
    if material:
        texture = materials[material]['maps']['diffuse']
        if texture and vertex[1] != -1:
            _, (width, height) = ngl_textures[texture] # Already loaded by ngl_obj
            u, v = vectors['texture'][vertex[1] - 1]
            if 0.0 > u or u > 1.0 or 0.0 > v or v > 1.0:
                u = min(1.0, max(u, 0.0))
                v = min(1.0, max(v, 0.0))
                print("Warning: Texture coords out of range not supported", file=sys.stderr)

            u *= width
            v = height - v*height
            color = "0" # When a texture is available, the color is used for flags. Keep them unset.

    return "{" + ", ".join([str(vertex[0] - position_min), format(u, "1.3f")+"f", format(v, "1.3f")+"f", color]) + "}"

def ngl_face(obj, face, position_min):
    global objects
    global materials

    color = "0"
    material = objects[obj]['material']
    if material:
        color = color2ngl(*[int(255*i) for i in materials[material]['colors']['diffuse']])
        color = "0x" + format(color, "04x")

    return ",\n    ".join([ngl_vertex(obj, vertex, color, position_min) for vertex in face])

def ngl_obj(obj, position_name):
    global objects
    global materials
    global vectors
    global ngl_textures

    ret = ""

    if len(objects[obj]['faces']) == 0:
        return ret # Don't output empty objects

    name = ngl_name(obj)

    if False in [(len(v) == 3) for v in objects[obj]['faces']]:
        if False in [(len(v) == 4) for v in objects[obj]['faces']]:
            if False in [(len(v) == 2) for v in objects[obj]['faces']]:
                raise Exception("Object is not completely out of quads or triangles!")
            else:
                draw_mode = "GL_LINES"
        else:
            draw_mode = "GL_QUADS"
    else:
        draw_mode = "GL_TRIANGLES"

    texture = None
    material = objects[obj]['material']
    texfile = materials[material]['maps']['diffuse'] if material is not None else None
    if texfile is not None:
        if texfile not in ngl_textures:
            code, texture, size = tex2ngl(texfile)
            ret += code
            ngl_textures[texfile] = (texture, size)

        texture, _ = ngl_textures[texfile]

    #If you draw all objects separately, uncomment this.
    #With this commented, all objects share the entire positions_ array.
    #position_min = min([index[0] for face in objects[obj]['faces'] for index in face])
    #position_max = max([index[0] for face in objects[obj]['faces'] for index in face])
    position_min = 1
    position_max = len(vectors['position'])

    vertex_list = [ngl_face(obj, face, position_min) for face in objects[obj]['faces']]
    count_vertices = sum([len(v) for v in objects[obj]['faces']])
    vertices = ",\n    ".join(vertex_list)

    texture = "nullptr" if texture is None else "&" + texture

    ret += """const IndexedVertex vertices_{name}[{count_vertices}] = {{
    {vertices}
}};

const ngl_object obj_{name} = {{
    {position_max} - {position_min},
    {position_name} + {position_min},
    {draw_mode},
    {count_vertices},
    vertices_{name},
    {texture}
}};

""".format(count_vertices=count_vertices, name=name,
           vertices=vertices, position_name=position_name,
           position_min=position_min-1, position_max=position_max,
           draw_mode=draw_mode, texture=texture)

    return ret

def dump_ngl(path, source):
    global objects
    global materials
    global vectors

    name = ngl_name(os.path.basename(path))

    position_list = [("{"+str(v[0])+"f, "+str(v[1])+"f, "+str(v[2])+"f}") for v in vectors['position']]
    positions = ",\n    ".join(position_list)

    out = """//Generated from {source} by obj2ngl.py
#include "gldrawarray.h"

struct ngl_object {{
    unsigned int count_positions;
    const VECTOR3 *positions;
    GLDrawMode draw_mode;
    unsigned int count_vertices;
    const IndexedVertex *vertices;
    const TEXTURE *texture;
}};

static const VECTOR3 positions_{name}[] = {{
    {positions}
}};

""".format(source=source, name=name, positions=positions);

    out += "".join([ngl_obj(obj, "positions_" + name) for obj in objects.keys()])

    objs = []
    for obj in objects.keys():
        if len(objects[obj]['faces']) != 0:
            objs += ["&obj_" + ngl_name(obj)]

    out += """const ngl_object *objs_{name}[] = {{
    {objs}
}};
""".format(name=name, objs=",\n    ".join(objs))

    output = open(path, "w")
    output.write(out)
    output.close()

    return True

def main(argv):
    if len(argv) != 2 and len(argv) != 3:
        print("Usage: obj2ngl.py <input.obj> (<output.h>)")
        return 2

    if not load_obj(argv[1]):
        return 1

    outputpath = argv[2] if len(argv) > 2 else os.path.splitext(argv[1])[0] + ".h"
    return 0 if dump_ngl(outputpath, os.path.basename(argv[1])) else 1

if __name__ == "__main__":
    sys.exit(main(sys.argv))

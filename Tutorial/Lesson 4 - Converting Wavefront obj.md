Lesson 4 - Converting Wavefront obj
-----------------------------------
Typing each ```VERTEX``` and ```VECTOR3``` in manually is tedious and error prone.

```tools/obj2ngl.py``` allows you to convert .obj files to nGL-compatible C++ header files automatically, together with textures.

Let's use at a simple .obj file, an untextured cube, the default model in blender:

```
# Blender v2.77 (sub 0) OBJ File: ''
# www.blender.org
mtllib cube.mtl
o Cube
v 1.000000 -1.000000 -1.000000
v 1.000000 -1.000000 1.000000
v -1.000000 -1.000000 1.000000
v -1.000000 -1.000000 -1.000000
v 1.000000 1.000000 -0.999999
v 0.999999 1.000000 1.000001
v -1.000000 1.000000 1.000000
v -1.000000 1.000000 -1.000000
vn 0.0000 -1.0000 0.0000
vn 0.0000 1.0000 0.0000
vn 1.0000 0.0000 0.0000
vn -0.0000 -0.0000 1.0000
vn -1.0000 -0.0000 -0.0000
vn 0.0000 0.0000 -1.0000
usemtl Material
s off
f 1//1 2//1 3//1 4//1
f 5//2 8//2 7//2 6//2
f 1//3 5//3 6//3 2//3
f 2//4 6//4 7//4 3//4
f 3//5 7//5 8//5 4//5
f 5//6 1//6 4//6 8//6
```

and the referenced cube.mtl:
```
# Blender MTL File: 'None'
# Material Count: 1

newmtl Material
Ns 96.078431
Ka 1.000000 1.000000 1.000000
Kd 0.640000 0.640000 0.640000
Ks 0.500000 0.500000 0.500000
Ke 0.000000 0.000000 0.000000
Ni 1.000000
d 1.000000
illum 2
```

Invoke ```obj2ngl.py <obj file> <output>``` in the directory where the .obj's resources are placed, like this:

```
nGL/tools/obj2ngl.py cube.obj cube.h
```

Resulting cube.h:

```c
//Generated from cube.obj by obj2ngl.py
#include "gldrawarray.h"

struct ngl_object {
    unsigned int count_positions;
    const VECTOR3 *positions;
    GLDrawMode draw_mode;
    unsigned int count_vertices;
    const IndexedVertex *vertices;
    const TEXTURE *texture;
};

static const VECTOR3 positions_cube_h[] = {
    {1.0f, -1.0f, -1.0f},
    {1.0f, -1.0f, 1.0f},
    {-1.0f, -1.0f, 1.0f},
    {-1.0f, -1.0f, -1.0f},
    {1.0f, 1.0f, -0.999999f},
    {0.999999f, 1.0f, 1.000001f},
    {-1.0f, 1.0f, 1.0f},
    {-1.0f, 1.0f, -1.0f}
};

const IndexedVertex vertices_Cube[24] = {
    {0, 0.000f, 0.000f, 0xa514},
    {1, 0.000f, 0.000f, 0xa514},
    {2, 0.000f, 0.000f, 0xa514},
    {3, 0.000f, 0.000f, 0xa514},
    {4, 0.000f, 0.000f, 0xa514},
    {7, 0.000f, 0.000f, 0xa514},
    {6, 0.000f, 0.000f, 0xa514},
    {5, 0.000f, 0.000f, 0xa514},
    {0, 0.000f, 0.000f, 0xa514},
    {4, 0.000f, 0.000f, 0xa514},
    {5, 0.000f, 0.000f, 0xa514},
    {1, 0.000f, 0.000f, 0xa514},
    {1, 0.000f, 0.000f, 0xa514},
    {5, 0.000f, 0.000f, 0xa514},
    {6, 0.000f, 0.000f, 0xa514},
    {2, 0.000f, 0.000f, 0xa514},
    {2, 0.000f, 0.000f, 0xa514},
    {6, 0.000f, 0.000f, 0xa514},
    {7, 0.000f, 0.000f, 0xa514},
    {3, 0.000f, 0.000f, 0xa514},
    {4, 0.000f, 0.000f, 0xa514},
    {0, 0.000f, 0.000f, 0xa514},
    {3, 0.000f, 0.000f, 0xa514},
    {7, 0.000f, 0.000f, 0xa514}
};

const ngl_object obj_Cube = {
    9 - 0,
    positions_cube_h + 0,
    GL_QUADS,
    24,
    vertices_Cube,
    nullptr
};

const ngl_object *objs_cube_h[] = {
    &obj_Cube
};
```

to draw an ```ngl_object```, we just need to invoke:

<nobr>```nglDrawArray(obj->vertices, obj->count_vertices, obj->positions, obj->count_positions, processed, obj->draw_mode);```</nobr>

To allocate ```processed``` we take the maximum of ```count_positions``` in the header file:


```c
size_t max_pos = 0;
for(auto &&obj : objs_cube_h)
    if(obj->count_positions > max_pos)
        max_pos = obj->count_positions;

ProcessedPosition *processed = new ProcessedPosition[max_pos];
```

for drawing, we use a similiar loop, calling ```nGLDrawArray`` and also handling the case with textures:

```c
for(auto &&obj : objs_cube_h)
{
    glBindTexture(obj->texture);
    nglDrawArray(obj->vertices, obj->count_vertices, obj->positions, obj->count_positions, processed, obj->draw_mode);
}
```

We also add a call to ```glScale3f(100, 100, 100);``` before the loop to make the cube appear bigger.

If there are multiple objects in a file that are all part of the same mesh and thus drawn with the same transformation matrix, the most common case, it makes sense to apply the transformation only once:

```c
bool transformed = false;
for(auto &&obj : objs_cube_h)
{
    glBindTexture(obj->texture);
    nglDrawArray(obj->vertices, obj->count_vertices, obj->positions, obj->count_positions, processed, obj->draw_mode, !transformed);
    transformed = true;
}
```

In case there are many different meshes in the file, you should consider tuning ```obj2ngl.py``` and remove the transformed bool, see the comment in the ```ngl_obj``` function.

The boilerplate code got updated a bit, with cross-compatibility for building on non-Nspire platforms as well, on which nGL uses SDL for graphics. You don't need to change your code for that, only make sure that you don't use any nspire-specific functions when ```_TINSPIRE``` is not defined.

Also, it uses the ```newTexture``` function from ```texturetools.h``` for framebuffer allocation now instead of a plain ```new[]```/```delete[]```.

The result
----------

Due to not using textures, it looks quite weird and ugly.
The code is reusable though, with most of the .obj files you can get.

![result](http://i.imgur.com/klIS9H1.gif)

With the same code (just adjusted position and scale), but a different .obj I got from www.models-resource.com I got the following result:

![result](http://i.imgur.com/Db2REly.gif)

Have fun modeling and coding!

Full code that runs on Nspire and PC:

```c
#ifdef _TINSPIRE
#include <libndls.h>
#endif

#include "gl.h"
#include "gldrawarray.h"
#include "texturetools.h"

#include "cube.h"

int main()
{
    // Initialize nGL first
    nglInit();
    // Allocate the framebuffer
    TEXTURE *screen = newTexture(SCREEN_WIDTH, SCREEN_HEIGHT, 0, false);
    nglSetBuffer(screen->bitmap);

    GLFix angle = 0;

    size_t max_pos = 0;
    for(auto &&obj : objs_cube_h)
        if(obj->count_positions > max_pos)
            max_pos = obj->count_positions;

    ProcessedPosition *processed = new ProcessedPosition[max_pos];

    #ifdef _TINSPIRE
    while(!isKeyPressed(KEY_NSPIRE_ESC))
    #else
    for(unsigned int i = 1300;--i;)
    #endif
    {
        glPushMatrix();
		glColor3f(0.4f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glTranslatef(0, 0, 400);
		angle += 1;
		nglRotateY(angle.normaliseAngle());

        glScale3f(100, 100, 100);

        bool transformed = false;
        for(auto &&obj : objs_cube_h)
        {
            glBindTexture(obj->texture);
            nglDrawArray(obj->vertices, obj->count_vertices, obj->positions, obj->count_positions, processed, obj->draw_mode, !transformed);
            transformed = true;
        }

        glPopMatrix();
        nglDisplay();
    }

    delete[] processed;
    // Deinitialize nGL
    nglUninit();
    deleteTexture(screen);
}
```

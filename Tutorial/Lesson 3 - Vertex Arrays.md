Lesson 3 - Vertex Arrays
------------------------
Now that you know the basics of nGL and can display pretty much anything,
optimizing your code and usage of the API is the next step.
The code in the previous lessons used tha part of the API known as "immediate mode" in OpenGL. It got deprecated in OpenGL 2.x and removed in 3.2.
The flaws of that API become visible directly by looking at the last example:

```c
const VERTEX triangle[] =
{
	{0, 0, 0, 0, 16, 0xFFFF}, // 1
	{0, 100, 0, 0, 0, 0xFFFF}, // 2
	{100, 0, 0, 16, 16, 0xFFFF}, // 3

	{0, 100, 0, 0, 0, 0xFFFF}, // 2
	{0, 0, 0, 0, 16, 0xFFFF}, // 1
	{100, 0, 0, 16, 16, 0xFFFF}, // 3
};
```

We can see here that every VERTEX is listed twice here.
In more complex models you'd have thousands of duplicate vertices, which not only requires more space, it's also more expensive to draw.
The solution for this is the ```nglDrawArray``` function in ```gldrawarray.h```.
You give it a list of vertex positions (<nobr>```const VECTOR3 *positions```</nobr>) and a list of indexed vertices, each referring to a position.
The advantage is that now it calculates each position only once.
This isn't noticable for our lonely triangle and in fact, we already avoid this by looking at the current angle:

```c
angle < GLFix(90) || angle > GLFix(270) ? triangle : (triangle + 3)
```

This time we'll also utilize the new ```TEXTURE_DRAW_BACKFACE``` flag for vertices
to avoid having both sides of the triangle separately.
Simply set the color of the vertices to ```TEXTURE_DRAW_BACKFACE``` and backface culling will be skipped.

The data used for our triangle is really simple:

```c
const VECTOR3 triangle_pos[] =
{
	{0,   0, 0}, // 1
	{0, 100, 0}, // 2
	{100, 0, 0}, // 3
};

const IndexedVertex triangle_ver[] =
{
    {0, 0,  16, TEXTURE_DRAW_BACKFACE},
    {1, 0,   0, TEXTURE_DRAW_BACKFACE},
    {2, 16, 16, TEXTURE_DRAW_BACKFACE},
};
```

```nglDrawArray``` needs a temporary storage for the computed position values.
As this array is fairly small and constant sized, we'll allocate that on the stack.

```c
ProcessedPosition processed[3];
nglDrawArray(triangle_ver, 3, triangle_pos, 3, processed, GL_TRIANGLES);
```
The result
----------

The outcome is absolutely identical to lesson 2:

![result](http://i.imgur.com/wf68Z2h.gif)

```c
#include <libndls.h>
#include "gl.h"
#include "gldrawarray.h"

#include "wall.h"

const VECTOR3 triangle_pos[] =
{
	{0,   0, 0}, // 1
	{0, 100, 0}, // 2
	{100, 0, 0}, // 3
};

const IndexedVertex triangle_ver[] =
{
    {0, 0,  16, TEXTURE_DRAW_BACKFACE},
    {1, 0,   0, TEXTURE_DRAW_BACKFACE},
    {2, 16, 16, TEXTURE_DRAW_BACKFACE},
};

int main()
{
    // Initialize nGL first
    nglInit();
    // Allocate the framebuffer
    COLOR *framebuffer = new COLOR[SCREEN_WIDTH * SCREEN_HEIGHT];
    nglSetBuffer(framebuffer);

	// Load the wall texture
	glBindTexture(&wall);

	GLFix angle = 0;
    ProcessedPosition processed[3];

    while(!isKeyPressed(KEY_NSPIRE_ESC))
    {
		glPushMatrix();
		glColor3f(0.4f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glTranslatef(0, 0, 400);
		angle += 1;
		nglRotateY(angle.normaliseAngle());
        nglDrawArray(triangle_ver, 3, triangle_pos, 3, processed, GL_TRIANGLES);
		glPopMatrix();
        nglDisplay();
    }

    // Deinitialize nGL
    nglUninit();
    delete[] framebuffer;
}
```

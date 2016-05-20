Lesson 2 - Textures
-------------------
You think that rotating white triangle looks boring? I agree!
Like all modern 3D graphics, nGL supports texture mapping.
Like OpenGL, you have two attributes per vertex used for texture mapping: U and V.
This is a texture:

```
 +--------> X
 |  \O/
 |   |
 |  / \
 |
 V
 Y
```

A texture is a picture that is mapped onto a 3D object at specific points, the vertices.
The X-Coordinate of a texture is the U-Attribute of the vertex, the Y-Coordinate is the V-Attribute.
In OpenGL, a texture has its top left corner at (0/0) and it's bottom right corner at (1/1).
This makes U and V independant of texture resolutions, but nGL uses a different format.
In OpenGL, integer U and V point between pixels, in nGL they point in the pixel center.
The top left is always (0/0), but for a 64x128 texture the bottom right pixel is at (63/127).
Confused? Don't worry, it's very easy in practice.

Creating a texture
------------------
To create a texture, you may use any image editor you like, GIMP, Krita, Paint, they all work.
For this example, we're going to use this PNG:

![texture](http://i.imgur.com/vUUKAXx.png)

To use it from within nGL, we have to convert it into a different format, using [ConvertImg](http://github.com/Vogtinator/ConvertImg).
You can also use the experimental python (2 and 3) utility in tools/tex2ngl.py, which requires pillow.
Invoke "ConvertImg --format=ngl wall.png wall.h" to create "wall.h".
For reference, it's also included below:

```c
//Generated from wall.png (output format: ngl)
static uint16_t wall_data[] = {
0xa1c1, 0xa961, 0xa921, 0xa921, 0xa961, 0xa1a1, 0x9a22, 0x821, 0xa1c1, 0xa961, 0xa921, 0xa921, 0xa961, 0xa1a1, 0x9a22, 0x821, 
0xa1a1, 0xa921, 0xb0c0, 0xb0a0, 0xa901, 0xa181, 0x9a02, 0x821, 0xa1a1, 0xa921, 0xb0c0, 0xb0a0, 0xa901, 0xa181, 0x9a02, 0x821, 
0xa1a1, 0xa921, 0xb0c0, 0xb0a0, 0xa901, 0xa181, 0xa202, 0x821, 0xa1a1, 0xa921, 0xb0c0, 0xb0a0, 0xa901, 0xa181, 0xa202, 0x821, 
0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 
0xa1a1, 0x9a22, 0x821, 0xa1c1, 0xa961, 0xa921, 0xa921, 0xa961, 0xa1a1, 0x9a22, 0x821, 0xa1c1, 0xa961, 0xa921, 0xa921, 0xa961, 
0xa181, 0x9a02, 0x821, 0xa1a1, 0xa921, 0xb0c0, 0xb0a0, 0xa901, 0xa181, 0x9a02, 0x821, 0xa1a1, 0xa921, 0xb0c0, 0xb0a0, 0xa901, 
0xa181, 0xa202, 0x821, 0xa1a1, 0xa921, 0xb0c0, 0xb0a0, 0xa901, 0xa181, 0xa202, 0x821, 0xa1a1, 0xa921, 0xb0c0, 0xb0a0, 0xa901, 
0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 
0xa921, 0xa961, 0xa1a1, 0x9a22, 0x821, 0xa1c1, 0xa961, 0xa921, 0xa921, 0xa961, 0xa1a1, 0x9a22, 0x821, 0xa1c1, 0xa961, 0xa921, 
0xb0a0, 0xa901, 0xa181, 0x9a02, 0x821, 0xa1a1, 0xa921, 0xb0c0, 0xb0a0, 0xa901, 0xa181, 0x9a02, 0x821, 0xa1a1, 0xa921, 0xb0c0, 
0xb0a0, 0xa901, 0xa181, 0xa202, 0x821, 0xa1a1, 0xa921, 0xb0c0, 0xb0a0, 0xa901, 0xa181, 0xa202, 0x821, 0xa1a1, 0xa921, 0xb0c0, 
0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 
0x9a22, 0x821, 0xa1c1, 0xa961, 0xa921, 0xa921, 0xa961, 0xa1a1, 0x9a22, 0x821, 0xa1c1, 0xa961, 0xa921, 0xa921, 0xa961, 0xa1a1, 
0x9a02, 0x821, 0xa1a1, 0xa921, 0xb0c0, 0xb0a0, 0xa901, 0xa181, 0x9a02, 0x821, 0xa1a1, 0xa921, 0xb0c0, 0xb0a0, 0xa901, 0xa181, 
0xa202, 0x821, 0xa1a1, 0xa921, 0xb0c0, 0xb0a0, 0xa901, 0xa181, 0xa202, 0x821, 0xa1a1, 0xa921, 0xb0c0, 0xb0a0, 0xa901, 0xa181, 
0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 0x821, 
};
static TEXTURE wall{
.width = 16,
.height = 16,
.has_transparency = true,
.transparent_color = 0,
.bitmap = wall_data };
```

Mapping the texture
-------------------
The triangle we used in Lesson 1 is depicted below, with vertices:

```
Y
^
| 2
| |\
| |_\
| 1  3
------->X
```

```c
const VERTEX triangle[] =
{
	{0, 0, 0, 0, 0, 0xFFFF}, // 1
	{0, 100, 0, 0, 0, 0xFFFF}, // 2
	{100, 0, 0, 0, 0, 0xFFFF}, // 3

	{0, 100, 0, 0, 0, 0xFFFF}, // 2
	{0, 0, 0, 0, 0, 0xFFFF}, // 1
	{100, 0, 0, 0, 0, 0xFFFF}, // 3
};
```
We have to assign the right coordinates now:
Vertex 1 is bottom left: 0 16
Vertex 2 is top left: 0 0
Vertex 3 is bottom right: 16 16
As the color has a special meaning if textures are used, we set it to 0.

That gives us the following array:

```c
const VERTEX triangle[] =
{
	{0, 0, 0, 0, 16, 0}, // 1
	{0, 100, 0, 0, 0, 0}, // 2
	{100, 0, 0, 16, 16, 0}, // 3

	{0, 100, 0, 0, 0, 0}, // 2
	{0, 0, 0, 0, 16, 0}, // 1
	{100, 0, 0, 16, 16, 0}, // 3
};
```

The code
--------
First we need to uncomment the line in glconfig.h to enable texture mapping:
```c
#define TEXTURE_SUPPORT
```
and run "make clean".
At the top of main.cpp we have to include the texture:
```c
#include "wall.h"
```
To actually use the texture, we have bind it. 
The best place for this is before the while loop, as this has to be done only once:
```c
// Load the wall texture
glBindTexture(&wall);
```

Finally, we move the triangle a bit closer to the camera. Change
```c
glTranslatef(0, 0, 1000);
```
to
```c
glTranslatef(0, 0, 400);
```

That's it!

The result
----------
If you compile an run it now, you'll see this:
![result](http://i.imgur.com/wf68Z2h.gif)
Quite impressive already, for a single triangle.

```c
#include <libndls.h>
#include "gl.h"

#include "wall.h"

const VERTEX triangle[] =
{
	{0, 0, 0, 0, 16, 0}, // 1
	{0, 100, 0, 0, 0, 0}, // 2
	{100, 0, 0, 16, 16, 0}, // 3

	{0, 100, 0, 0, 0, 0}, // 2
	{0, 0, 0, 0, 16, 0}, // 1
	{100, 0, 0, 16, 16, 0}, // 3
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
    while(!isKeyPressed(KEY_NSPIRE_ESC))
    {
		glPushMatrix();
		glColor3f(0.4f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glTranslatef(0, 0, 400);
		angle += 1;
		nglRotateY(angle.normaliseAngle());
		glBegin(GL_TRIANGLES);
			nglAddVertices(angle < GLFix(90) || angle > GLFix(270) ? triangle : (triangle + 3), 3);
		glEnd();
		glPopMatrix();
        nglDisplay();
    }
    
    // Deinitialize nGL
    nglUninit();
    delete[] framebuffer;
}
```

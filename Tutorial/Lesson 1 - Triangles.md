Lesson 1 - Triangles
--------------------
In this short guide you'll learn what nGL is, what it can do and how you can use it.
Let's start with a quick overview:
- Basic 3D graphics functionality
- No GPU: No shaders, no buffers on the GPU-side
- Single core, single threaded

It's basically what 3D programming looked like in the early 90s, when there was no graphics accelleration
and everything has to be optimized for the target system.
Contrary to most DOS games, nGL doesn't use a single line of assembly! (Except for the FPS counter...)

First, you need to know the basics of 3D graphics and how they work.
I recommend reading a book about OpenGL, you need to undestand what view and projection matrices are and what they do.

A first program
---------------
You already know that all? Great, let's start programming!
A simple program using nGL has this basic structure:
```c
#include <libndls.h>
#include "gl.h"

int main()
{
    // Initialize nGL first
    nglInit();
    // Allocate the framebuffer
    COLOR *framebuffer = new COLOR[SCREEN_WIDTH * SCREEN_HEIGHT];
    nglSetBuffer(framebuffer);
    
    while(!isKeyPressed(KEY_NSPIRE_ESC))
    {
        // Clear the screen and also the depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nglDisplay();
    }
    
    // Deinitialize nGL
    nglUninit();
    delete[] framebuffer;
}
```

As an example, place this as main.cpp in the nGL directory, copy glconfig_example.h to glconfig.h
and run make. In a real project, you'll want to seperate nGL from your code, but for now that'll work.

if you run this snippet, you'll see a black screen until you quit by pressing ESC.
To change the background color, add
```c
glColor3f(0.4f, 0.7f, 1.0f);
```
before glClear to use a nice sky blue.

GLFix
-----
One of the reasons nGL is so fast on a calculator without FPU is that it is based on fixed-point math.
GLFix is an 32-bit integer with 8-bit fractional part. Basically, it counts the 1/256th of values.
The integer 2 would be 512 as 8-bit fixed-point value, but you don't have to worry about that,
GLFix does those conversations for you automatically.

The obvious advantages are speed and also, well, more "reliable" behaviour, but it also
has the disadvantage that any value smaller than 1/256th is rounded down to 0, the accuracy is fixed.
If your values are scaled properly across your entire application, this won't be an issue,
but keep it in mind.


First triangle
--------------
The triangle is the basic structure everything in the GL-World is built on. Yes, there are Quads,
but they consist of two triangles:
```
    | /| or |\ |
    |/ |    | \|
```

In nGL, backface culling is mandatory if you're not using nglDrawTriangle directly, so only
triangles with vertices in clockwise-order are visible:

```
Y
^
| 2
| |\
| |_\
| 1  3
------->X
```

The "camera" is at (0,0,0) looking in (0,0,1). In OpenGL, the camera looks in (0,0,-1).
Our first triangle is going to be 100 wide, 100 high and have a distance of 1000 to the camera.
So we construct our first triangle in an array of vertices outside of main():
```c
const VERTEX triangle[] =
{
//	 X, Y, Z,    U, V, Color
	{0, 0, 1000, 0, 0, 0xFFFF},   // 1
	{0, 100, 1000, 0, 0, 0xFFFF}, // 2
	{100, 0, 1000, 0, 0, 0xFFFF}, // 3
};
```

and this code after glClear:
```c
glBegin(GL_TRIANGLES);
	nglAddVertices(triangle, 3);
glEnd();
```

The type COLOR in nGL is a RGB565-encoded unsigned 16-bit integer. 0xFFFF is white, 0x0000 black.

Result: ![Result](http://i.imgur.com/w1ZydLD.png)

Some movement
-------------
So, now we want something less boring than a static screen with a white triangle.
What about a spinning white triangle?
You might think inserting nglRotateY is enough here - but it's not!
For one, we need to work against backface culling, and also set the rotation axis correctly.
The triangle isn't at (0,0,0), it's at (0,0,1000) right now.
For a rotation around the Y-axis of the triangle it is needed to center the triangle around (0,0,0):
```c
const VERTEX triangle[] =
{
	{0, 0, 0, 0, 0, 0xFFFF},
	{0, 100, 0, 0, 0, 0xFFFF},
	{100, 0, 0, 0, 0, 0xFFFF},
};
```

and apply the correct transformations:
```c
glTranslatef(0, 0, 1000);
nglRotateY(10);
glBegin(GL_TRIANGLES);
	nglAddVertices(triangle, 3);
glEnd();
```

When you run this, you'll see a flickering triangle.
This is because nGL doesn't reset the state after nglDisplay(), just like OpenGL.
Add glPushMatrix() before glClear and glPopMatrix() before nglDisplay() to make it work as expected.
To finally get some movement, declare a new variable
```c
GLFix angle = 0;
```

before the while loop and also change the nglRotateY call to
```c
angle += 1;
nglRotateY(angle.normaliseAngle());
```

To wrap back to 0 once angle reaches 360, angle.normaliseAngle() is used.
This will rotate the triangle around the Y-axis, as expected. However, one half is still missing.
To fix that, add another triangle, this time the backface, to the array:
```c
const VERTEX triangle[] =
{
	{0, 0, 0, 0, 0, 0xFFFF}, // 1
	{0, 100, 0, 0, 0, 0xFFFF}, // 2
	{100, 0, 0, 0, 0, 0xFFFF}, // 3

	{0, 100, 0, 0, 0, 0xFFFF}, // 2
	{0, 0, 0, 0, 0, 0xFFFF}, // 1
	{100, 0, 0, 0, 0, 0xFFFF}, //3
};
```

and change the call to nglAddVertices to also include the second triangle:
```c
nglAddVertices(triangle, 6);
```

Alternatively, you could also switch between them as needed, which is also faster:
```c
nglAddVertices(angle < GLFix(90) || angle > GLFix(270) ? triangle : (triangle + 3), 3);
```

Result: ![Animated result](http://i.imgur.com/zfvqqlr.gif)

Complete code:
```c
#include <libndls.h>
#include "gl.h"

const VERTEX triangle[] =
{
	{0, 0, 0, 0, 0, 0xFFFF}, // 1
	{0, 100, 0, 0, 0, 0xFFFF}, // 2
	{100, 0, 0, 0, 0, 0xFFFF}, // 3

	{0, 100, 0, 0, 0, 0xFFFF}, // 2
	{0, 0, 0, 0, 0, 0xFFFF}, // 1
	{100, 0, 0, 0, 0, 0xFFFF}, //3
};

int main()
{
    // Initialize nGL first
    nglInit();
    // Allocate the framebuffer
    COLOR *framebuffer = new COLOR[SCREEN_WIDTH * SCREEN_HEIGHT];
    nglSetBuffer(framebuffer);
    
	GLFix angle = 0;
    while(!isKeyPressed(KEY_NSPIRE_ESC))
    {
		glPushMatrix();
		glColor3f(0.4f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glTranslatef(0, 0, 1000);
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

That's it for the first part. If requested, I'll do more!

#ifndef GL_H
#define GL_H

#ifndef __cplusplus
#error You need to use a C++ compiler to use nGL!
#endif

//nGL version 0.8
#include "fix.h"

#include "glconfig.h"

//These values are used to calculate offsets into the buffer.
//If you want something like FBOs, make them variables and set them accordingly.
//Watch out for different buffer sizes!
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

//GLFix is an integral part of all calculations.
//Changing resolution and width may be an improvement or even break everything.
typedef Fix<8, int32_t> GLFix;

/* Column vectors and matrices:
 * [ [0][0] [0][1] [0][2] [0][3] ]   [x]
 * [ [1][0] [1][1] [1][2] [1][3] ] * [y]
 * [ [2][0] [2][1] [2][2] [2][3] ]   [z]
 * [ [3][0] [3][1] [3][2] [3][3] ]   [1] (not used anywhere)
 */

/* If TEXTURE_SUPPORT is enabled and a VERTEX has the highest nibble set, black pixels of the texture won't be drawn */
#define TEXTURE_TRANSPARENT 0xF000

typedef uint16_t COLOR;

struct VECTOR3
{
    VECTOR3() : VECTOR3(0, 0, 0) {}
    VECTOR3(const GLFix x, const GLFix y, const GLFix z)
        : x(x), y(y), z(z) {}

    void print() const { printf("(%d %d %d)", x.toInteger<int>(), y.toInteger<int>(), z.toInteger<int>()); }

    GLFix x, y, z;
};

struct VERTEX
{
    VERTEX() : VERTEX(0, 0, 0, 0, 0, 0) {}
    VERTEX(const GLFix x, const GLFix y, const GLFix z, const GLFix u, const GLFix v, const COLOR c)
        : x(x), y(y), z(z), u(u), v(v), c(c) {}

    void print() const { printf("(%d %d %d) (0x%x) (%d %d)\n", x.toInteger<int>(), y.toInteger<int>(), z.toInteger<int>(), c, u.toInteger<int>(), v.toInteger<int>()); }

    GLFix x, y, z;
    GLFix u, v;
    COLOR c;
};

struct TEXTURE
{
    uint16_t width; uint16_t height;
    bool has_transparency; COLOR transparent_color;
    COLOR *bitmap;
};

class MATRIX {
public:
    MATRIX() {}
    GLFix data[4][4] = {};
};

#define GL_COLOR_BUFFER_BIT 1<<0
#define GL_DEPTH_BUFFER_BIT 1<<1

enum GLDrawMode
{
    GL_TRIANGLES,
    GL_QUADS,
    GL_QUAD_STRIP, //Not really tested
    GL_LINE_STRIP
};

//Range [0-1]
struct RGB
{
    RGB() : RGB(0,0,0) {}
    RGB(const GLFix r, const GLFix g, const GLFix b) : r(r), g(g), b(b) {}
    GLFix r, g, b;
};

#ifdef FPS_COUNTER
    extern volatile unsigned int fps;
#endif
extern MATRIX *transformation;

RGB rgbColor(const COLOR c);
COLOR colorRGB(const RGB rgb);
COLOR colorRGB(const GLFix r, const GLFix g, const GLFix b);

//Invoke once before using any other functions
void nglInit();
void nglUninit();
//The buffer to render to
void nglSetBuffer(COLOR *screenBuf);
void nglSetNearPlane(const GLFix near_plane);
GLFix nglGetNearPlane();
GLFix nglZBufferAt(const unsigned int x, const unsigned int y);
//Display the buffer
void nglDisplay();
void nglSetColor(const COLOR c);
void nglRotateX(const GLFix a);
void nglRotateY(const GLFix a);
void nglRotateZ(const GLFix a);
//To add nGL VERTEX instances directly without using old gl*3f calls
void nglAddVertices(const VERTEX *buffer, unsigned int length);
void nglAddVertex(const VERTEX &vertex);
void nglAddVertex(const VERTEX *vertex);
#ifdef TEXTURE_SUPPORT
    //Basically glDisable(GL_TEXTURE_2D)
    void nglForceColor(const bool force);
#endif
//Warning: The nglDraw*-Functions apply perspective projection!
//Returns whether the triangle is front-facing
bool nglDrawTriangle(const VERTEX *low, const VERTEX *middle, const VERTEX *high, bool backface_culling = true);
bool nglIsBackface(const VERTEX *v1, const VERTEX *v2, const VERTEX *v3);
void nglDrawTriangleZClipped(const VERTEX *low, const VERTEX *middle, const VERTEX *high);
void nglInterpolateVertexZ(const VERTEX *from, const VERTEX *to, VERTEX *res);
void nglDrawLine3D(const VERTEX *v1, const VERTEX *v2);

void nglPerspective(VERTEX *v);
void nglMultMatVectRes(const MATRIX *mat1, const VERTEX *vect, VERTEX *res);
void nglMultMatVectRes(const MATRIX *mat1, const VECTOR3 *vect, VECTOR3 *res);
void nglMultMatMat(MATRIX *mat1, const MATRIX *mat2);
const TEXTURE *nglGetTexture();

void glLoadIdentity();
void glBegin(const GLDrawMode mode);
inline void glEnd() { }
void glClear(const int buffers);
void glTranslatef(const GLFix x, const GLFix y, const GLFix z);

void glBindTexture(const TEXTURE *tex);
void glTexCoord2f(const GLFix nu, const GLFix nv);
void glColor3f(const GLFix r, const GLFix g, const GLFix b);
void glVertex3f(const GLFix x, const GLFix y, const GLFix z);
void glScale3f(const GLFix x, const GLFix y, const GLFix z);
void glPushMatrix();
void glPopMatrix();

#endif

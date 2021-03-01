#include <ctime>
#include <utility>
#include <algorithm>

#ifdef _TINSPIRE
#include <libndls.h>
#else
#include <assert.h>
#ifdef _WIN32
    #include <SDL.h>
    #include <signal.h>
#else
    #include <SDL/SDL.h>
#endif
static SDL_Window* sdl_window;
static SDL_Renderer* sdl_renderer;

static SDL_Texture* sdl_texture; // send to gpu
#endif

#include "gl.h"
#include "fastmath.h"

#define M(m, y, x) (m.data[y][x])
#define P(m, y, x) (m->data[y][x])

MATRIX *transformation;
static COLOR color;
static GLFix u, v;
static COLOR *screen;
static GLFix *z_buffer;
static GLFix near_plane = 256;
static const TEXTURE *texture;
static unsigned int vertices_count = 0; // For glAddVertex calls
static VERTEX vertices[4]; // // For glAddVertex calls
static GLDrawMode draw_mode = GL_TRIANGLES;
static bool force_color = false, is_monochrome;
#ifdef _TINSPIRE
    static COLOR *screen_inverted; //For monochrome calcs
#endif
#ifdef FPS_COUNTER
    volatile unsigned int fps;
#endif
#ifdef SAFE_MODE
    static int matrix_stack_left = MATRIX_STACK_SIZE;
#endif

void nglInit()
{
    init_fastmath();
    transformation = new MATRIX[MATRIX_STACK_SIZE];

    //C++ <3
    z_buffer = new std::remove_reference<decltype(*z_buffer)>::type[SCREEN_WIDTH*SCREEN_HEIGHT];
    glLoadIdentity();
    color = colorRGB(0, 0, 0); //Black
    u = v = 0;

    texture = nullptr;
    vertices_count = 0;
    draw_mode = GL_TRIANGLES;

    #ifdef _TINSPIRE
        is_monochrome = lcd_type() == SCR_320x240_4;

        if(is_monochrome)
        {
            screen_inverted = new COLOR[SCREEN_WIDTH*SCREEN_HEIGHT];
            lcd_init(SCR_320x240_16);
        }
        else
            lcd_init(SCR_320x240_565);
    #else
        SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN, &sdl_window, &sdl_renderer);

        sdl_texture = SDL_CreateTexture(sdl_renderer,
            SDL_PIXELFORMAT_RGB565,
            SDL_TEXTUREACCESS_STREAMING,
            SCREEN_WIDTH, SCREEN_HEIGHT);

        SDL_SetWindowTitle(sdl_window, "nGL");

    #ifndef _WIN32
            signal(SIGINT, SIG_DFL);
    #endif
    assert(sdl_renderer);
    assert(sdl_window);
    assert(sdl_texture);
    #endif

    #ifdef SAFE_MODE
        matrix_stack_left = MATRIX_STACK_SIZE;
    #endif
}

void nglUninit()
{
    uninit_fastmath();
    delete[] transformation;
    delete[] z_buffer;

    #ifdef _TINSPIRE
        delete[] screen_inverted;
    #endif

    #ifdef _TINSPIRE
        lcd_init(SCR_TYPE_INVALID);
    #else
        //SDL_DestroyRenderer(sdl_renderer);
        SDL_Quit();
    #endif
}

void nglMultMatMat(MATRIX *mat1, MATRIX *mat2)
{
    GLFix a00 = P(mat1, 0, 0), a01 = P(mat1, 0, 1), a02 = P(mat1, 0, 2), a03 = P(mat1, 0, 3);
    GLFix a10 = P(mat1, 1, 0), a11 = P(mat1, 1, 1), a12 = P(mat1, 1, 2), a13 = P(mat1, 1, 3);
    GLFix a20 = P(mat1, 2, 0), a21 = P(mat1, 2, 1), a22 = P(mat1, 2, 2), a23 = P(mat1, 2, 3);
    GLFix a30 = P(mat1, 3, 0), a31 = P(mat1, 3, 1), a32 = P(mat1, 3, 2), a33 = P(mat1, 3, 3);

    GLFix b00 = P(mat2, 0, 0), b01 = P(mat2, 0, 1), b02 = P(mat2, 0, 2), b03 = P(mat2, 0, 3);
    GLFix b10 = P(mat2, 1, 0), b11 = P(mat2, 1, 1), b12 = P(mat2, 1, 2), b13 = P(mat2, 1, 3);
    GLFix b20 = P(mat2, 2, 0), b21 = P(mat2, 2, 1), b22 = P(mat2, 2, 2), b23 = P(mat2, 2, 3);
    GLFix b30 = P(mat2, 3, 0), b31 = P(mat2, 3, 1), b32 = P(mat2, 3, 2), b33 = P(mat2, 3, 3);

    P(mat1, 0, 0) = a00*b00 + a01*b10 + a02*b20 + a03*b30;
    P(mat1, 0, 1) = a00*b01 + a01*b11 + a02*b21 + a03*b31;
    P(mat1, 0, 2) = a00*b02 + a01*b12 + a02*b22 + a03*b32;
    P(mat1, 0, 3) = a00*b03 + a01*b13 + a02*b23 + a03*b33;
    P(mat1, 1, 0) = a10*b00 + a11*b10 + a12*b20 + a13*b30;
    P(mat1, 1, 1) = a10*b01 + a11*b11 + a12*b21 + a13*b31;
    P(mat1, 1, 2) = a10*b02 + a11*b12 + a12*b22 + a13*b32;
    P(mat1, 1, 3) = a10*b03 + a11*b13 + a12*b23 + a13*b33;
    P(mat1, 2, 0) = a20*b00 + a21*b10 + a22*b20 + a23*b30;
    P(mat1, 2, 1) = a20*b01 + a21*b11 + a22*b21 + a23*b31;
    P(mat1, 2, 2) = a20*b02 + a21*b12 + a22*b22 + a23*b32;
    P(mat1, 2, 3) = a20*b03 + a21*b13 + a22*b23 + a23*b33;
    P(mat1, 3, 0) = a30*b00 + a31*b10 + a32*b20 + a33*b30;
    P(mat1, 3, 1) = a30*b01 + a31*b11 + a32*b21 + a33*b31;
    P(mat1, 3, 2) = a30*b02 + a31*b12 + a32*b22 + a33*b32;
    P(mat1, 3, 3) = a30*b03 + a31*b13 + a32*b23 + a33*b33;
}

void nglMultMatVectRes(const MATRIX *mat1, const VERTEX *vect, VERTEX *res)
{
    GLFix x = vect->x, y = vect->y, z = vect->z;

    res->x = P(mat1, 0, 0)*x + P(mat1, 0, 1)*y + P(mat1, 0, 2)*z + P(mat1, 0, 3);
    res->y = P(mat1, 1, 0)*x + P(mat1, 1, 1)*y + P(mat1, 1, 2)*z + P(mat1, 1, 3);
    res->z = P(mat1, 2, 0)*x + P(mat1, 2, 1)*y + P(mat1, 2, 2)*z + P(mat1, 2, 3);
}

void nglMultMatVectRes(const MATRIX *mat1, const VECTOR3 *vect, VECTOR3 *res)
{
    GLFix x = vect->x, y = vect->y, z = vect->z;

    res->x = P(mat1, 0, 0)*x + P(mat1, 0, 1)*y + P(mat1, 0, 2)*z + P(mat1, 0, 3);
    res->y = P(mat1, 1, 0)*x + P(mat1, 1, 1)*y + P(mat1, 1, 2)*z + P(mat1, 1, 3);
    res->z = P(mat1, 2, 0)*x + P(mat1, 2, 1)*y + P(mat1, 2, 2)*z + P(mat1, 2, 3);
}

void nglPerspective(VERTEX *v)
{
#ifdef BETTER_PERSPECTIVE
    float new_z = v->z;
    decltype(new_z) new_x = v->x, new_y = v->y;
    decltype(new_z) div = decltype(new_z)(near_plane)/new_z;

    new_x *= div;
    new_y *= div;

    v->x = new_x;
    v->y = new_y;
#else
    GLFix div = near_plane/v->z;

    //Round to integers, as we don't lose the topmost bits with integer multiplication
    v->x = div * v->x.toInteger<int>();
    v->y = div * v->y.toInteger<int>();
#endif

    // (0/0) is in the center of the screen
    v->x += SCREEN_WIDTH/2;
    v->y += SCREEN_HEIGHT/2;

    v->y = GLFix(SCREEN_HEIGHT - 1) - v->y;

#if defined(SAFE_MODE) && defined(TEXTURE_SUPPORT)
    //TODO: Move this somewhere else
    if(force_color)
        return;

    if(v->u > GLFix(texture->width))
    {
        printf("Warning: Texture coord out of bounds!\n");
        v->u = texture->height;
    }
    else if(v->u < GLFix(0))
    {
        printf("Warning: Texture coord out of bounds!\n");
        v->u = 0;
    }

    if(v->v > GLFix(texture->height))
    {
        printf("Warning: Texture coord out of bounds!\n");
        v->v = texture->height;
    }
    else if(v->v < GLFix(0))
    {
        printf("Warning: Texture coord out of bounds!\n");
        v->v = 0;
    }
#endif
}

void nglPerspective(VECTOR3 *v)
{
#ifdef BETTER_PERSPECTIVE
    float new_z = v->z;
    decltype(new_z) new_x = v->x, new_y = v->y;
    decltype(new_z) div = decltype(new_z)(near_plane)/new_z;

    new_x *= div;
    new_y *= div;

    v->x = new_x;
    v->y = new_y;
#else
    GLFix div = near_plane/v->z;

    //Round to integers, as we don't lose the topmost bits with integer multiplication
    v->x = div * v->x.toInteger<int>();
    v->y = div * v->y.toInteger<int>();
#endif

    // (0/0) is in the center of the screen
    v->x += SCREEN_WIDTH/2;
    v->y += SCREEN_HEIGHT/2;

    v->y = GLFix(SCREEN_HEIGHT - 1) - v->y;
}

void nglSetBuffer(COLOR *screenBuf)
{
    screen = screenBuf;
}

void nglDisplay()
{
    #ifdef _TINSPIRE
        if(is_monochrome)
        {
            //Flip everything, as 0xFFFF is white on CX, but black on classic
            COLOR *ptr = screen + SCREEN_HEIGHT*SCREEN_WIDTH, *ptr_inv = screen_inverted + SCREEN_HEIGHT*SCREEN_WIDTH;
            while(--ptr >= screen)
                *--ptr_inv = ~*ptr;

            lcd_blit(screen_inverted, SCR_320x240_16);
        }
        else
            lcd_blit(screen, SCR_320x240_565);
    #else
        SDL_UpdateTexture(sdl_texture, NULL, screen, SCREEN_WIDTH*sizeof(COLOR));
        SDL_RenderClear(sdl_renderer);
        SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
        SDL_RenderPresent(sdl_renderer);
    #endif

    #ifdef FPS_COUNTER
        static unsigned int frames = 0;
        ++frames;

        static time_t last = 0;
        time_t now = time(nullptr);
        if(now != last)
        {
            fps = frames;
            printf("FPS: %u\n", frames);
            last = now;
            frames = 0;
        }
    #endif
}

void nglRotateX(const GLFix a)
{
    MATRIX rot;

    const GLFix sina = fast_sin(a), cosa = fast_cos(a);

    M(rot, 0, 0) = 1;
    M(rot, 1, 1) = cosa;
    M(rot, 1, 2) = -sina;
    M(rot, 2, 1) = sina;
    M(rot, 2, 2) = cosa;
    M(rot, 3, 3) = 1;

    nglMultMatMat(transformation, &rot);
}

void nglRotateY(const GLFix a)
{
    MATRIX rot;

    const GLFix sina = fast_sin(a), cosa = fast_cos(a);

    M(rot, 0, 0) = cosa;
    M(rot, 0, 2) = sina;
    M(rot, 1, 1) = 1;
    M(rot, 2, 0) = -sina;
    M(rot, 2, 2) = cosa;
    M(rot, 3, 3) = 1;

    nglMultMatMat(transformation, &rot);
}

void nglRotateZ(const GLFix a)
{
    MATRIX rot;

    const GLFix sina = fast_sin(a), cosa = fast_cos(a);

    M(rot, 0, 0) = cosa;
    M(rot, 0, 1) = -sina;
    M(rot, 1, 0) = sina;
    M(rot, 1, 1) = cosa;
    M(rot, 2, 2) = 1;
    M(rot, 3, 3) = 1;

    nglMultMatMat(transformation, &rot);
}

inline void pixel(const int x, const int y, const GLFix z, const COLOR c)
{
    if(x < 0 || y < 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT)
        return;

    const int pitch = x + y*SCREEN_WIDTH;

    if(z <= GLFix(CLIP_PLANE) || z_buffer[pitch] <= z)
        return;

    z_buffer[pitch] = z;

    screen[pitch] = c;
}

RGB rgbColor(const COLOR c)
{
    const GLFix r = (c >> 11) & 0b11111;
    const GLFix g = (c >> 5) & 0b111111;
    const GLFix b = (c >> 0) & 0b11111;

    return {r / GLFix(0b11111), g / GLFix(0b111111), b / GLFix(0b11111)};
}

COLOR colorRGB(const RGB rgb)
{
    return colorRGB(rgb.r, rgb.g, rgb.b);
}

COLOR colorRGB(const GLFix r, const GLFix g, const GLFix b)
{
    const int r1 = (r * GLFix(0b11111)).round();
    const int g1 = (g * GLFix(0b111111)).round();
    const int b1 = (b * GLFix(0b11111)).round();

    return ((r1 & 0b11111) << 11) | ((g1 & 0b111111) << 5) | (b1 & 0b11111);
}

GLFix nglZBufferAt(const unsigned int x, const unsigned int y)
{
    if(x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT)
        return 0;

    return z_buffer[x + y * SCREEN_WIDTH];
}

constexpr GLFix ABS(GLFix a) {
    return a >= GLFix(0) ? a : -a;
}

//Doesn't interpolate colors even if enabled
void nglDrawLine3D(const VERTEX *v1, const VERTEX *v2)
{    
    // Z-Clipping!
    if(v1->z < near_plane || v2->z < near_plane)
        return;

    VERTEX v1_p = *v1, v2_p = *v2;
    nglPerspective(&v1_p);
    nglPerspective(&v2_p);

    const GLFix diff_x = v2_p.x - v1_p.x;
    const GLFix diff_y = v2_p.y - v1_p.y;

    const COLOR c = v1_p.c;

    //Height > width? -> Interpolate X
    //if(dy > GLFix(1) || dy < GLFix(-1))

    // if check passes diff_y should always be non zero
    if (ABS(diff_x) < ABS(diff_y))
    {
        if(v2_p.y < v1_p.y)
            std::swap(v1_p, v2_p);

        const GLFix dx = diff_x / diff_y;
        const GLFix dz = (v2_p.z - v1_p.z) / diff_y;

        int end_y = v2_p.y;

        if(end_y >= SCREEN_HEIGHT)
            end_y = SCREEN_HEIGHT - 1;

        for(; v1_p.y <= GLFix(end_y); ++v1_p.y)
        {
            pixel(v1_p.x, v1_p.y, v1_p.z, c);

            v1_p.x += dx;
            v1_p.z += dz;
        }
    }
    else
    {
        const GLFix dy = diff_x != GLFix(0) ? (diff_y / diff_x) : diff_y;

        if(v2_p.x < v1_p.x)
            std::swap(v1_p, v2_p);

        // still need ternary check (for const) since diff_x could be 0
        const GLFix dz = diff_x != GLFix(0) ? (v2_p.z - v1_p.z) / diff_x : (v2_p.z - v1_p.z);

        int end_x = v2_p.x;

        if(end_x >= SCREEN_WIDTH)
            end_x = SCREEN_WIDTH - 1;

        for(; v1_p.x <= GLFix(end_x); ++v1_p.x)
        {
            pixel(v1_p.x, v1_p.y, v1_p.z, c);

            v1_p.y += dy;
            v1_p.z += dz;
        }
    }
}

//I hate code duplication more than macros and includes
#ifdef TEXTURE_SUPPORT
    #define TRANSPARENCY
    #include "triangle.inc.h"
    #undef TRANSPARENCY
    #undef TEXTURE_SUPPORT
    #define FORCE_COLOR
    #include "triangle.inc.h"
    #define TEXTURE_SUPPORT
    #undef FORCE_COLOR
#endif
#include "triangle.inc.h"

static void interpolateVertexXLeft(const VERTEX *from, const VERTEX *to, VERTEX *res)
{
    GLFix diff = to->x - from->x;
    if(diff < GLFix(1) && diff > GLFix(-1))
        diff = 1;

    GLFix end = 0;
    GLFix t = (end - from->x) / diff;

    res->x = end;
    res->y = from->y + (to->y - from->y) * t;
    res->z = from->z + (to->z - from->z) * t;

#ifdef TEXTURE_SUPPORT
    res->u = from->u + (to->u - from->u) * t;
    res->u = res->u.wholes();
    res->v = from->v + (to->v - from->v) * t;
    res->v = res->v.wholes();
#endif

#ifdef INTERPOLATE_COLORS
    RGB c_from = rgbColor(from->c);
    RGB c_to = rgbColor(to->c);

    res->c = colorRGB(c_from.r + (c_to.r - c_from.r) * t, c_from.r + (c_to.r - c_from.r) * t, c_from.r + (c_to.r - c_from.r) * t);
#else
    res->c = from->c;
#endif
}

//Left X clipping
void nglDrawTriangleXRightZClipped(const VERTEX *low, const VERTEX *middle, const VERTEX *high)
{
    const VERTEX* invisible[3];
    const VERTEX* visible[3];
    int count_invisible = -1, count_visible = -1;

    if(low->x < GLFix(0))
        invisible[++count_invisible] = low;
    else
        visible[++count_visible] = low;

    if(middle->x < GLFix(0))
        invisible[++count_invisible] = middle;
    else
        visible[++count_visible] = middle;

    if(high->x < GLFix(0))
        invisible[++count_invisible] = high;
    else
        visible[++count_visible] = high;

    //Interpolated vertices
    VERTEX v1, v2;

    switch(count_visible)
    {
    case -1:
        break;
    case 0:
        interpolateVertexXLeft(invisible[0], visible[0], &v1);
        interpolateVertexXLeft(invisible[1], visible[0], &v2);
        nglDrawTriangleXZClipped(visible[0], &v1, &v2);
        break;
    case 1:
        interpolateVertexXLeft(invisible[0], visible[0], &v1);
        interpolateVertexXLeft(invisible[0], visible[1], &v2);
        nglDrawTriangleXZClipped(visible[0], visible[1], &v1);
        nglDrawTriangleXZClipped(visible[1], &v1, &v2);
        break;
    case 2:
        nglDrawTriangleXZClipped(visible[0], visible[1], visible[2]);
        break;
    }
}

static void interpolateVertexXRight(const VERTEX *from, const VERTEX *to, VERTEX *res)
{
    GLFix diff = to->x - from->x;
    if(diff < GLFix(1) && diff > GLFix(-1))
        diff = 1;

    GLFix end = (SCREEN_WIDTH - 1);
    GLFix t = (end - from->x) / diff;

    res->x = end;
    res->y = from->y + (to->y - from->y) * t;
    res->z = from->z + (to->z - from->z) * t;

#ifdef TEXTURE_SUPPORT
    res->u = from->u + (to->u - from->u) * t;
    res->u = res->u.wholes();
    res->v = from->v + (to->v - from->v) * t;
    res->v = res->v.wholes();
#endif

#ifdef INTERPOLATE_COLORS
    RGB c_from = rgbColor(from->c);
    RGB c_to = rgbColor(to->c);

    res->c = colorRGB(c_from.r + (c_to.r - c_from.r) * t, c_from.r + (c_to.r - c_from.r) * t, c_from.r + (c_to.r - c_from.r) * t);
#else
    res->c = from->c;
#endif
}

//Right X clipping
void nglDrawTriangleZClipped(const VERTEX *low, const VERTEX *middle, const VERTEX *high)
{
    //If not on screen, skip
    if((low->y < GLFix(0) && middle->y < GLFix(0) && high->y < GLFix(0)) || (low->y >= GLFix(SCREEN_HEIGHT) && middle->y >= GLFix(SCREEN_HEIGHT) && high->y >= GLFix(SCREEN_HEIGHT)))
        return;

    const VERTEX* invisible[3];
    const VERTEX* visible[3];
    int count_invisible = -1, count_visible = -1;

    if(low->x >= GLFix(SCREEN_WIDTH))
        invisible[++count_invisible] = low;
    else
        visible[++count_visible] = low;

    if(middle->x >= GLFix(SCREEN_WIDTH))
        invisible[++count_invisible] = middle;
    else
        visible[++count_visible] = middle;

    if(high->x >= GLFix(SCREEN_WIDTH))
        invisible[++count_invisible] = high;
    else
        visible[++count_visible] = high;

    //Interpolated vertices
    VERTEX v1, v2;

    switch(count_visible)
    {
    case -1:
        break;
    case 0:
        interpolateVertexXRight(invisible[0], visible[0], &v1);
        interpolateVertexXRight(invisible[1], visible[0], &v2);
        nglDrawTriangleXRightZClipped(visible[0], &v1, &v2);
        break;
    case 1:
        interpolateVertexXRight(invisible[0], visible[0], &v1);
        interpolateVertexXRight(invisible[0], visible[1], &v2);
        nglDrawTriangleXRightZClipped(visible[0], visible[1], &v1);
        nglDrawTriangleXRightZClipped(visible[1], &v1, &v2);
        break;
    case 2:
        nglDrawTriangleXRightZClipped(visible[0], visible[1], visible[2]);
        break;
    }
}

#ifdef Z_CLIPPING
    void nglInterpolateVertexZ(const VERTEX *from, const VERTEX *to, VERTEX *res)
    {
        GLFix diff = to->z - from->z;
        if(diff < GLFix(1) && diff > GLFix(-1))
            diff = 1;

        GLFix t = (GLFix(CLIP_PLANE) - from->z) / diff;

        res->x = from->x + (to->x - from->x) * t;
        res->y = from->y + (to->y - from->y) * t;
        res->z = CLIP_PLANE;

        #ifdef TEXTURE_SUPPORT
            res->u = from->u + (to->u - from->u) * t;
            res->u = res->u.wholes();
            res->v = from->v + (to->v - from->v) * t;
            res->v = res->v.wholes();
        #endif

        #ifdef INTERPOLATE_COLORS
            RGB c_from = rgbColor(from->c);
            RGB c_to = rgbColor(to->c);

            res->c = colorRGB(c_from.r + (c_to.r - c_from.r) * t, c_from.r + (c_to.r - c_from.r) * t, c_from.r + (c_to.r - c_from.r) * t);
        #else
            res->c = from->c;
        #endif
    }
#endif

bool nglIsBackface(const VERTEX *v1, const VERTEX *v2, const VERTEX *v3)
{
    int x1 = v2->x - v1->x, x2 = v3->x - v1->x;
    int y1 = v2->y - v1->y, y2 = v3->y - v1->y;

    return x1 * y2 < x2 * y1;
}

bool nglIsBackface(const VECTOR3 *v1, const VECTOR3 *v2, const VECTOR3 *v3)
{
    int x1 = v2->x - v1->x, x2 = v3->x - v1->x;
    int y1 = v2->y - v1->y, y2 = v3->y - v1->y;

    return x1 * y2 < x2 * y1;
}

bool nglDrawTriangle(const VERTEX *low, const VERTEX *middle, const VERTEX *high, bool backface_culling)
{
#ifndef Z_CLIPPING
    if(low->z < GLFix(CLIP_PLANE) || middle->z < GLFix(CLIP_PLANE) || high->z < GLFix(CLIP_PLANE))
        return true;

    VERTEX low_p = *low, middle_p = *middle, high_p = *high;

    nglPerspective(&low_p);
    nglPerspective(&middle_p);
    nglPerspective(&high_p);

    if(backface_culling && nglIsBackface(&low_p, &middle_p, &high_p))
        return false;

    nglDrawTriangleZClipped(&low_p, &middle_p, &high_p);

    return true;
#else

    VERTEX invisible[3];
    VERTEX visible[3];
    int count_invisible = -1, count_visible = -1;

    if(low->z < GLFix(CLIP_PLANE))
        invisible[++count_invisible] = *low;
    else
        visible[++count_visible] = *low;

    if(middle->z < GLFix(CLIP_PLANE))
        invisible[++count_invisible] = *middle;
    else
        visible[++count_visible] = *middle;

    if(high->z < GLFix(CLIP_PLANE))
        invisible[++count_invisible] = *high;
    else
        visible[++count_visible] = *high;

    //Interpolated vertices
    VERTEX v1, v2;

    switch(count_visible)
    {
    case -1:
        return true;

    case 0:
        nglInterpolateVertexZ(&invisible[0], &visible[0], &v1);
        nglInterpolateVertexZ(&invisible[1], &visible[0], &v2);

        nglPerspective(&visible[0]);
        nglPerspective(&v1);
        nglPerspective(&v2);

        if(backface_culling && nglIsBackface(&visible[0], &v1, &v2))
            return false;

        nglDrawTriangleZClipped(&visible[0], &v1, &v2);
        return true;

    case 1:
        nglInterpolateVertexZ(&visible[0], &invisible[0], &v1);
        nglInterpolateVertexZ(&visible[1], &invisible[0], &v2);

        nglPerspective(&visible[0]);
        nglPerspective(&visible[1]);
        nglPerspective(&v1);

        if(backface_culling && nglIsBackface(&visible[0], &visible[1], &v1))
            return false;

        nglPerspective(&v2);
        nglDrawTriangleZClipped(&visible[0], &visible[1], &v1);
        nglDrawTriangleZClipped(&visible[1], &v1, &v2);
        return true;

    case 2:
        nglPerspective(&visible[0]);
        nglPerspective(&visible[1]);
        nglPerspective(&visible[2]);

        if(backface_culling && nglIsBackface(&visible[0], &visible[1], &visible[2]))
            return false;

        nglDrawTriangleZClipped(&visible[0], &visible[1], &visible[2]);
        return true;

    default:
        return true;
    }
#endif
}

void nglSetColor(const COLOR c)
{
    color = c;
}

void nglAddVertices(const VERTEX *buffer, unsigned int length)
{
    while(length--)
        nglAddVertex(buffer++);
}

void nglAddVertex(const VERTEX &vertex)
{
    nglAddVertex(&vertex);
}

void nglAddVertex(const VERTEX* vertex)
{
    #if defined(SAFE_MODE) && defined(TEXTURE_SUPPORT)
        if(texture == nullptr && !force_color)
        {
            printf("ngl.lang.NoTextureException: Please, don't make me dereference the nullptr!\n");
            return;
        }
    #endif

    VERTEX *current_vertex = &vertices[vertices_count];

    current_vertex->c = vertex->c;
    #ifdef TEXTURE_SUPPORT
        current_vertex->u = vertex->u;
        current_vertex->v = vertex->v;
    #endif

    nglMultMatVectRes(transformation, vertex, current_vertex);

    ++vertices_count;

    switch(draw_mode)
    {
    case GL_TRIANGLES:
        if(vertices_count != 3)
            break;

        vertices_count = 0;

#ifdef WIREFRAME_MODE
        nglDrawLine3D(&vertices[0], &vertices[1]);
        nglDrawLine3D(&vertices[0], &vertices[2]);
        nglDrawLine3D(&vertices[2], &vertices[1]);
#else
        nglDrawTriangle(&vertices[0], &vertices[1], &vertices[2], NGL_DRAW_COLOR || (vertices[0].c & TEXTURE_DRAW_BACKFACE) != TEXTURE_DRAW_BACKFACE);
#endif
        break;

    case GL_QUADS:
        if(vertices_count != 4)
            break;

        vertices_count = 0;

#ifdef WIREFRAME_MODE
        nglDrawLine3D(&vertices[0], &vertices[1]);
        nglDrawLine3D(&vertices[1], &vertices[2]);
        nglDrawLine3D(&vertices[2], &vertices[3]);
        nglDrawLine3D(&vertices[3], &vertices[0]);
#else
        if(nglDrawTriangle(&vertices[0], &vertices[1], &vertices[2]), NGL_DRAW_COLOR || (vertices[0].c & TEXTURE_DRAW_BACKFACE) != TEXTURE_DRAW_BACKFACE)
            nglDrawTriangle(&vertices[2], &vertices[3], &vertices[0], false);
#endif
        break;

    case GL_QUAD_STRIP:
        if(vertices_count != 4)
            break;

        vertices_count = 2;

#ifdef WIREFRAME_MODE
        nglDrawLine3D(&vertices[0], &vertices[1]);
        nglDrawLine3D(&vertices[1], &vertices[2]);
        nglDrawLine3D(&vertices[2], &vertices[3]);
        nglDrawLine3D(&vertices[3], &vertices[0]);
#else
        if(nglDrawTriangle(&vertices[0], &vertices[1], &vertices[2]), NGL_DRAW_COLOR || (vertices[0].c & TEXTURE_DRAW_BACKFACE) != TEXTURE_DRAW_BACKFACE)
            nglDrawTriangle(&vertices[2], &vertices[3], &vertices[0], false);
#endif

        vertices[0] = vertices[2];
        vertices[1] = vertices[3];
        break;
    case GL_LINE_STRIP:
        if(vertices_count != 2)
            break;

        vertices_count = 1;

        nglDrawLine3D(&vertices[0], &vertices[1]);

        vertices[0] = vertices[1];
        break;
    }
}

const TEXTURE *nglGetTexture()
{
    return texture;
}

void glBindTexture(const TEXTURE *tex)
{
    texture = tex;

#ifdef SAFE_MODE
    if(tex->has_transparency && tex->transparent_color != 0)
        printf("Bound texture doesn't have black as transparent color!\n");
#endif
}

void nglForceColor(const bool force)
{
    force_color = force;
}


bool nglIsForceColor()
{
    return force_color;
}

void nglSetNearPlane(const GLFix new_near_plane)
{
    near_plane = new_near_plane;
}

GLFix nglGetNearPlane()
{
    return near_plane;
}

void glColor3f(const GLFix r, const GLFix g, const GLFix b)
{
    color = colorRGB(r, g, b);
}

void glTexCoord2f(const GLFix nu, const GLFix nv)
{
    u = nu;
    v = nv;
}

void glVertex3f(const GLFix x, const GLFix y, const GLFix z)
{
    const VERTEX ver{x, y, z, u, v, color};
    nglAddVertex(&ver);
}

void glBegin(GLDrawMode mode)
{
    vertices_count = 0;
    draw_mode = mode;
}

void glClear(const int buffers)
{
    if(buffers & GL_COLOR_BUFFER_BIT)
        std::fill(screen, screen + SCREEN_WIDTH*SCREEN_HEIGHT, color);

    if(buffers & GL_DEPTH_BUFFER_BIT)
        std::fill(z_buffer, z_buffer + SCREEN_WIDTH*SCREEN_HEIGHT, z_buffer->maxValue());
}

void glLoadIdentity()
{
    *transformation = {}; // Copy empty matrix into transformation
    P(transformation, 0, 0) = P(transformation, 1, 1) = P(transformation, 2, 2) = P(transformation, 3, 3) = 1;
}

void glTranslatef(const GLFix x, const GLFix y, const GLFix z)
{
    MATRIX trans;

    M(trans, 0, 0) = M(trans, 1, 1) = M(trans, 2, 2) = M(trans, 3, 3) = 1;
    M(trans, 0, 3) = x;
    M(trans, 1, 3) = y;
    M(trans, 2, 3) = z;

    nglMultMatMat(transformation, &trans);
}

void glScale3f(const GLFix x, const GLFix y, const GLFix z)
{
    MATRIX scale;

    M(scale, 0, 0) = x;
    M(scale, 1, 1) = y;
    M(scale, 2, 2) = z;
    M(scale, 3, 3) = 1;

    nglMultMatMat(transformation, &scale);
}

void glPopMatrix()
{
    #ifdef SAFE_MODE
        if(matrix_stack_left == MATRIX_STACK_SIZE)
        {
            printf("Error: No matrix left on the stack anymore!\n");
            return;
        }
        ++matrix_stack_left;
    #endif

    --transformation;
}

void glPushMatrix()
{
    #ifdef SAFE_MODE
        if(matrix_stack_left == 0)
        {
            printf("Error: Matrix stack limit reached!\n");
            return;
        }
        matrix_stack_left--;
    #endif

    ++transformation;
    *transformation = *(transformation - 1);
}

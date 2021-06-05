#include <cassert>

#include "gldrawarray.h"

/* Create a vertex out of a VECTOR3 and IndexedVertex */
#define MAKE_VERTEX(vec, iver) { (vec).x, (vec).y, (vec).z, (iver).u, (iver).v, (iver).c }

/* Apply perspective to a ProcessedVertex
 * and return the resulting VERTEX.
 * Caches the result in the ProcessedVertex. */
static VERTEX perspective(const IndexedVertex &v, ProcessedPosition &p)
{
    if(!p.perspective_available)
    {
        p.perspective = p.transformed;
        nglPerspective(&p.perspective);
        p.perspective_available = true;
    }

    return MAKE_VERTEX(p.perspective, v);
}

static bool drawTriangle(ProcessedPosition *processed, const IndexedVertex &low, const IndexedVertex &middle, const IndexedVertex &high, bool backface_culling)
{
    ProcessedPosition &p_low = processed[low.index], &p_middle = processed[middle.index], &p_high = processed[high.index];

    if(p_low.transformed.z < GLFix(CLIP_PLANE) && p_middle.transformed.z < GLFix(CLIP_PLANE) && p_high.transformed.z < GLFix(CLIP_PLANE))
        return true;

    VERTEX invisible[3];
    const IndexedVertex *visible[3];
    ProcessedPosition *p_visible[3];
    unsigned int count_invisible = 0, count_visible = 0;

    if(p_low.transformed.z < GLFix(CLIP_PLANE))
        invisible[count_invisible++] = MAKE_VERTEX(p_low.transformed, low);
    else
    {
        visible[count_visible] = &low;
        p_visible[count_visible++] = &p_low;
    }

    if(p_middle.transformed.z < GLFix(CLIP_PLANE))
        invisible[count_invisible++] = MAKE_VERTEX(p_middle.transformed, middle);
    else
    {
        visible[count_visible] = &middle;
        p_visible[count_visible++] = &p_middle;
    }

    if(p_high.transformed.z < GLFix(CLIP_PLANE))
        invisible[count_invisible++] = MAKE_VERTEX(p_high.transformed, high);
    else
    {
        visible[count_visible] = &high;
        p_visible[count_visible++] = &p_high;
    }

    //Interpolated vertices
    VERTEX v1, v2;

    //Temporary vertices
    VERTEX t0, t1;

    switch(count_visible)
    {
    case 0:
        return true;
#ifdef Z_CLIPPING
    case 1:
        t0 = MAKE_VERTEX(p_visible[0]->transformed, *visible[0]);

        nglInterpolateVertexZ(&invisible[0], &t0, &v1);
        nglInterpolateVertexZ(&invisible[1], &t0, &v2);

        t0 = perspective(*visible[0], *p_visible[0]);
        nglPerspective(&v1);
        nglPerspective(&v2);

        if(backface_culling && nglIsBackface(&t0, &v1, &v2))
            return false;

        nglDrawTriangleZClipped(&t0, &v1, &v2);
        return true;

    case 2:
        t0 = MAKE_VERTEX(p_visible[0]->transformed, *visible[0]);
        t1 = MAKE_VERTEX(p_visible[1]->transformed, *visible[1]);

        nglInterpolateVertexZ(&t0, &invisible[0], &v1);
        nglInterpolateVertexZ(&t1, &invisible[0], &v2);

        t0 = perspective(*visible[0], *p_visible[0]);
        t1 = perspective(*visible[1], *p_visible[1]);
        nglPerspective(&v1);

        //TODO: Hack: This doesn't work as expected
        /*if(backface_culling && nglIsBackface(&t0, &t1, &v1))
            return false;*/

        nglPerspective(&v2);
        nglDrawTriangleZClipped(&t0, &t1, &v1);
        nglDrawTriangleZClipped(&t1, &v1, &v2);
        return true;
#endif
    case 3:
        invisible[0] = perspective(low, p_low);
        invisible[1] = perspective(middle, p_middle);
        invisible[2] = perspective(high, p_high);

        if(backface_culling && nglIsBackface(&invisible[0], &invisible[1], &invisible[2]))
            return false;

        nglDrawTriangleZClipped(&invisible[0], &invisible[1], &invisible[2]);
        return true;

    default:
        return true;
    }
}

void nglDrawArray(const IndexedVertex *vertices, const unsigned int count_vertices, const VECTOR3 *positions, const unsigned int count_positions, ProcessedPosition *processed, const GLDrawMode draw_mode, const bool reset_processed)
{
    if(reset_processed)
    {
        // Reset processed vertices and apply transformation
        for(unsigned int i = 0; i < count_positions; ++i)
        {
            processed[i].perspective_available = false;
            nglMultMatVectRes(transformation, &positions[i], &processed[i].transformed);
        }
    }

    // Draw
    if(draw_mode == GL_TRIANGLES)
    {
        for(unsigned int i = 0; i < count_vertices; i += 3)
            drawTriangle(processed, vertices[i], vertices[i + 1], vertices[i + 2], !nglGetTexture() || (vertices[i].c & TEXTURE_DRAW_BACKFACE) != TEXTURE_DRAW_BACKFACE);
    }
    else if(draw_mode == GL_QUADS)
    {
        for(unsigned int i = 0; i < count_vertices; i += 4)
        {
            // Either none or both parts of a quad face the camera
            if(drawTriangle(processed, vertices[i], vertices[i + 1], vertices[i + 2], !nglGetTexture() || (vertices[i].c & TEXTURE_DRAW_BACKFACE) != TEXTURE_DRAW_BACKFACE))
                drawTriangle(processed, vertices[i + 2], vertices[i + 3], vertices[i], false);
        }
    }
    else
        assert(!"Not implemented");
}


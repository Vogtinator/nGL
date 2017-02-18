#include "gl.h"

/* Just like the functions for triangle drawing, this works in three steps:
 *
 * First, find the vertex with the lowest Y (low), then the vertex with the highest (high).
 * Then, select middle_l that it is left of middle_r.
 *
 * +-------------------------->
 * |                          X
 * |            * low
 * |           / \
 * |  middle_l *  * middle_r
 * |            \ /
 * |             * high
 * |
 * V Y
 *
 * This is a bit more complex for the following case:
 *
 * +-------------------------->
 * |                          X
 * | middle_l *---* low
 * |          |   |
 * |          |   |
 * |    high  *---* middle_r
 * |
 * V Y
 *
 * If you swap middle_l and low here, the constraints are still true, but now we have an edge
 * between low and high... So swap middle_l/low and middle_r/high in that case.
 *
 * Then, calculate the slopes of each of the edges. If middle_l->y or middle_r->y <= 0,
 * some slopes can be ignores as they are not visible anyway.
 * The last step is iterating from max(0, low->y) to min(SCREEN_HEIGHT, high->y),
 * going from the left edge to the right edge on the current scanline. The start and end
 * values for X, Z, R/G/B and U/V are calculated using the slopes.
 */

#ifdef INTERPOLATE_COLORS
    #error "Not implemented"
#endif

void nglDrawQuadXZClipped(const VERTEX *low, const VERTEX *middle_l, const VERTEX *middle_r, const VERTEX *high)
{
    /* Sort all vertices */

    if(middle_l->y < low->y)
        std::swap(middle_l, low);

    if(middle_r->y < low->y)
        std::swap(middle_r, low);

    if(high->y < low->y)
        std::swap(high, low);

    if(middle_l->y > high->y)
        std::swap(middle_l, high);

    if(middle_r->y > high->y)
        std::swap(middle_r, high);

    if(middle_l->x > middle_r->x)
        std::swap(middle_l, middle_r);

    if(high->y < GLFix(0) || low->y >= SCREEN_HEIGHT)
        return; // Not on screen. Should be caught earlier!

    // Special case
    if(middle_l->x == middle_r->x)
    {
        if(low->y == middle_l->y && low->x < middle_l->x)
            std::swap(low, middle_l);
        else if(high->y == middle_r->y && high->x > middle_r->x)
            std::swap(high, middle_r);
        else
            return; // 0 pixels wide. TODO: Draw as line or skip?
    }

    /* Calculate all slopes
       TODO: Only the necessary ones! */

    struct Slope {
        GLFix dx, dz;
        #ifdef TEXTURE_SUPPORT
            GLFix du, dv;
        #endif
    } slope_lml, slope_lmr, slope_mlh, slope_mrh;

    unsigned int height_lml = middle_l->y - low->y,
                 height_lmr = middle_r->y - low->y,
                 height_mlh = high->y - middle_l->y,
                 height_mrh = high->y - middle_r->y;

    auto calcSlope = [] (const VERTEX &v1, const VERTEX &v2, unsigned int dy, Slope &slope)
    {
        if(dy <= 0)
            return;

        slope.dx = (v2.x - v1.x) / dy;
        slope.dz = (v2.z - v1.z) / dy;

        #ifdef TEXTURE_SUPPORT
            slope.du = (v2.u - v1.u) / dy;
            slope.dv = (v2.v - v1.v) / dy;
        #endif
    };

    calcSlope(*low, *middle_l, height_lml, slope_lml);
    calcSlope(*low, *middle_r, height_lmr, slope_lmr);
    calcSlope(*middle_l, *high, height_mlh, slope_mlh);
    calcSlope(*middle_r, *high, height_mrh, slope_mrh);

    /* TODO: Pointer? */
    Slope slope_left = slope_lml, slope_right = slope_lmr;

    GLFix y = low->y, yend = std::min(SCREEN_HEIGHT, high->y.floor());
    GLFix xstart = low->x, xend = xstart;
    GLFix zstart = low->z, zend = zstart;

    #ifdef TEXTURE_SUPPORT
        GLFix ustart = low->u, uend = ustart;
        GLFix vstart = low->v, vend = vstart;
    #endif

    /* Interpolate to visible area */
    // TODO: Skip to middle_* directly
    if(y < GLFix(0))
    {
        unsigned int dy;

        if(middle_l->y < middle_r->y)
            dy = middle_l->y - y;
        else
            dy = middle_r->y - y;

        xstart += slope_left.dx * dy;
        zstart += slope_left.dz * dy;
        xend += slope_right.dx * dy;
        zend += slope_right.dz * dy;

        if(middle_l->y < middle_r->y)
            slope_left = slope_mlh;
        else
            slope_right = slope_mrh;

        y += dy;

        if(y < GLFix(0))
        {
            if(middle_l->y >= middle_r->y)
                dy = middle_l->y - y;
            else
                dy = middle_r->y - y;

            xstart += slope_left.dx * dy;
            zstart += slope_left.dz * dy;
            xend += slope_right.dx * dy;
            zend += slope_right.dz * dy;

            if(middle_l->y >= middle_r->y)
                slope_left = slope_mlh;
            else
                slope_right = slope_mrh;

            y += dy;

            if(y < GLFix(0))
            {
                dy = -y.floor();

                xstart += slope_left.dx * dy;
                zstart += slope_left.dz * dy;
                xend += slope_right.dx * dy;
                zend += slope_right.dz * dy;

                y = 0;
            }
        }

        y = 0;
    }

    COLOR *cur_line = screen + (SCREEN_WIDTH * y.floor());
    GLFix *z_line = z_buffer + (SCREEN_WIDTH * y.floor());

    for(; y < yend; ++y)
    {
        // Which slopes are we on?
        if(y == middle_l->y)
        {
            slope_left = slope_mlh;
            xstart = middle_l->x;
            zstart = middle_l->z;

            #ifdef TEXTURE_SUPPORT
                ustart = middle_l->u;
                vstart = middle_l->v;
            #endif
        }

        if(y == middle_r->y)
        {
            slope_right = slope_mrh;
            xend = middle_r->x;
            zend = middle_r->z;

            #ifdef TEXTURE_SUPPORT
                uend = middle_r->u;
                vend = middle_r->v;
            #endif
        }

        unsigned int line_width = xend - xstart + 1;

        GLFix dz_line = (zend - zstart) / line_width;

        GLFix z = zstart;

        #ifdef TEXTURE_SUPPORT
            GLFix u = ustart, v = vstart;

            GLFix du_line = (uend - ustart) / line_width,
                  dv_line = (vend - vstart) / line_width;
        #endif

        COLOR *px = cur_line + xstart.floor();
        GLFix *zp = z_line + xstart.floor();
        for(int x = xstart.floor(); x < xend.floor(); ++x)
        {
            // Test Z
            if(*zp > z)
            {
                COLOR c;
                #ifdef TEXTURE_SUPPORT
                    c = texture->bitmap[u.floor() + v.floor()*texture->width];
                #else
                    c = low->c;
                #endif

                *px = c;
                *zp = z;
            }

            // Next pixel
            ++px;
            ++zp;

            z += dz_line;

            #ifdef TEXTURE_SUPPORT
                u += du_line;
                v += dv_line;
            #endif
        }

        xstart += slope_left.dx;
        zstart += slope_left.dz;

        xend += slope_right.dx;
        zend += slope_right.dz;

        #ifdef TEXTURE_SUPPORT
            ustart += slope_left.du;
            vstart += slope_left.dv;

            uend += slope_right.du;
            vend += slope_right.dv;
        #endif

        z_line += SCREEN_WIDTH;
        cur_line += SCREEN_WIDTH;
    }
}

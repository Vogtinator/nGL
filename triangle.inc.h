//This file will be included in gl.cpp for various different versions
#ifdef TRANSPARENCY
    static void nglDrawTransparentTriangleXZClipped(const VERTEX *low, const VERTEX *middle, const VERTEX *high)
    {
#else
    #ifdef FORCE_COLOR
        static void nglDrawTriangleXZClippedForceColor(const VERTEX *low, const VERTEX *middle, const VERTEX *high)
        {
    #else
        void nglDrawTriangleXZClipped(const VERTEX *low, const VERTEX *middle, const VERTEX *high)
        {
            #ifdef TEXTURE_SUPPORT
                if(!texture)
                    return nglDrawTriangleXZClippedForceColor(low, middle, high);

                if(__builtin_expect((low->c & TEXTURE_TRANSPARENT) == TEXTURE_TRANSPARENT, 0))
                    return nglDrawTransparentTriangleXZClipped(low, middle, high);
            #endif
    #endif
#endif
    if((low->y < GLFix(0) && middle->y < GLFix(0) && high->y < GLFix(0))
        || (low->y >= GLFix(SCREEN_HEIGHT) && middle->y >= GLFix(SCREEN_HEIGHT) && high->y >= GLFix(SCREEN_HEIGHT)))
        return;

    if(middle->y > high->y)
        std::swap(middle, high);

    if(low->y > middle->y)
        std::swap(low, middle);

    if(middle->y > high->y)
        std::swap(middle, high);

    if(high->y < GLFix(0) || low->y >= GLFix(SCREEN_HEIGHT))
        return;

    // The ranges of values from here on allows using some more bits for precision:
    // X is clipped to screen coords, Z is >= CLIP_PLANE and the application won't
    // draw primitives that far away.
    // U and V are bounded to the texture size and R, G and B are between 0 - 1.
    // Only issue is Y, but exceeding the range there is not that likely in practice.
    using TriFix = Fix<10, int32_t>;

    int low_y = low->y, middle_y = middle->y, high_y = high->y + 1;

    const int height_upper = high_y - middle_y;

    const TriFix dx_upper = TriFix(high->x - middle->x) / height_upper;
    const TriFix dz_upper = TriFix(high->z - middle->z) / height_upper;

    const int height_lower = middle_y - low_y + 1;

    const TriFix dx_lower = TriFix(middle->x - low->x) / height_lower;
    const TriFix dz_lower = TriFix(middle->z - low->z) / height_lower;

    const int height_far = high_y - low_y;

    const TriFix dx_far = TriFix(high->x - low->x) / height_far;
    const TriFix dz_far = TriFix(high->z - low->z) / height_far;

    #ifdef TEXTURE_SUPPORT
        const TriFix du_upper = TriFix(high->u - middle->u) / height_upper;
        const TriFix dv_upper = TriFix(high->v - middle->v) / height_upper;

        const TriFix du_lower = TriFix(middle->u - low->u) / height_lower;
        const TriFix dv_lower = TriFix(middle->v - low->v) / height_lower;

        const TriFix du_far = TriFix(high->u - low->u) / height_far;
        const TriFix dv_far = TriFix(high->v - low->v) / height_far;

        TriFix ustart = low->u, uend = low->u;
        TriFix vstart = low->v, vend = low->v;
    #elif defined(INTERPOLATE_COLORS)
        const RGB high_rgb = rgbColor(high->c);
        const RGB middle_rgb = rgbColor(middle->c);
        const RGB low_rgb = rgbColor(low->c);

        const TriFix dr_upper = TriFix(high_rgb.r - middle_rgb.r) / height_upper;
        const TriFix dg_upper = TriFix(high_rgb.g - middle_rgb.g) / height_upper;
        const TriFix db_upper = TriFix(high_rgb.b - middle_rgb.b) / height_upper;

        const TriFix dr_lower = TriFix(middle_rgb.r - low_rgb.r) / height_lower;
        const TriFix dg_lower = TriFix(middle_rgb.g - low_rgb.g) / height_lower;
        const TriFix db_lower = TriFix(middle_rgb.b - low_rgb.b) / height_lower;

        const TriFix dr_far = TriFix(high_rgb.r - low_rgb.r) / height_far;
        const TriFix dg_far = TriFix(high_rgb.g - low_rgb.g) / height_far;
        const TriFix db_far = TriFix(high_rgb.b - low_rgb.b) / height_far;

        TriFix rstart = low_rgb.r, rend = low_rgb.r;
        TriFix gstart = low_rgb.g, gend = low_rgb.g;
        TriFix bstart = low_rgb.b, bend = low_rgb.b;
    #endif

    int y = low_y;
    TriFix xstart = low->x, zstart = low->z, xend = low->x, zend = low->z;

    //Vertical clipping
    if(y < 0)
    {
        const int diff = -y;
        int diff_lower = diff;
        int diff_upper = 0;
        if(diff_lower > height_lower)
        {
            diff_lower = height_lower;
            diff_upper = diff - diff_lower;
        }

        y = 0;

        xstart += dx_far * diff;
        zstart += dz_far * diff;
        xend += dx_lower * diff_lower;
        zend += dz_lower * diff_lower;
        xend += dx_upper * diff_upper;
        zend += dz_upper * diff_upper;

        #ifdef TEXTURE_SUPPORT
            ustart += du_far * diff;
            vstart += dv_far * diff;
            uend += du_lower * diff_lower;
            vend += dv_lower * diff_lower;
            uend += du_upper * diff_upper;
            vend += dv_upper * diff_upper;
        #elif defined(INTERPOLATE_COLORS)
            rstart += dr_far * diff;
            gstart += dg_far * diff;
            bstart += db_far * diff;
            rend += dr_lower * diff_lower;
            gend += dg_lower * diff_lower;
            bend += db_lower * diff_lower;
            rend += dr_upper * diff_upper;
            gend += dg_upper * diff_upper;
            bend += db_upper * diff_upper;
        #endif
    }

    if(high_y >= SCREEN_HEIGHT)
        high_y = SCREEN_HEIGHT - 1;

    int pitch = y * SCREEN_WIDTH;
    decltype(z_buffer) z_buf_line = z_buffer + pitch;
    decltype(screen) screen_buf_line = screen + pitch;

    TriFix dx_current = dx_lower, dz_current = dz_lower;
#ifdef TEXTURE_SUPPORT
    TriFix du_current = du_lower, dv_current = dv_lower;
#elif defined(INTERPOLATE_COLORS)
    TriFix dr_current = dr_lower, dg_current = dg_lower, db_current = db_lower;
#endif

    if(__builtin_expect(y > middle_y, false))
    {
        dx_current = dx_upper;
        dz_current = dz_upper;

        #ifdef TEXTURE_SUPPORT
            du_current = du_upper;
            dv_current = dv_upper;
        #elif defined(INTERPOLATE_COLORS)
            dr_current = dr_upper;
            dg_current = dg_upper;
            db_current = db_upper;
        #endif
    }

    #ifdef TEXTURE_SUPPORT
        //Stack access is faster
        TEXTURE loc_texture = *texture;
    #endif

    //If xstart will get smaller than xend
    if(dx_lower < dx_far)
        goto otherway;

    for(; y <= high_y; y += 1, z_buf_line += SCREEN_WIDTH, screen_buf_line += SCREEN_WIDTH)
    {
        const int x1 = xstart, x2 = xend;
        const int line_width = x2 - x1;
        if(__builtin_expect(line_width >= 1, true))
        {
            const auto inv_l = Fix<16, int32_t>(1) / line_width;
            const TriFix dz = (zend - zstart) * inv_l;
            TriFix z = zstart;

            #ifdef TEXTURE_SUPPORT
                const TriFix du = (uend - ustart) * inv_l;
                const TriFix dv = (vend - vstart) * inv_l;
                TriFix u = ustart, v = vstart;
            #elif defined(INTERPOLATE_COLORS)
                const TriFix dr = (rend - rstart) * inv_l;
                const TriFix dg = (gend - gstart) * inv_l;
                const TriFix db = (bend - bstart) * inv_l;

                TriFix r = rstart, g = gstart, b = bstart;
            #endif

            decltype(z_buffer) z_buf = z_buf_line + x1;
            decltype(screen) screen_buf = screen_buf_line + x1;
            for(int x = x1; x <= x2; x += 1, ++z_buf, ++screen_buf)
            {
                if(__builtin_expect(TriFix(*z_buf) > z, true))
                {
                    #ifdef TEXTURE_SUPPORT
                        COLOR c = loc_texture.bitmap[u.floor() + v.floor()*loc_texture.width];
                        #ifdef TRANSPARENCY
                            if(__builtin_expect(c != 0x0000, 1))
                            {
                                *screen_buf = c;
                                *z_buf = z;
                            }
                        #else
                            *screen_buf = c;
                            *z_buf = z;
                        #endif
                    #elif defined(INTERPOLATE_COLORS)
                        *screen_buf = colorRGB(r, g, b);
                        *z_buf = z;
                    #else
                        *screen_buf = low->c;
                        *z_buf = z;
                    #endif
                }

                #ifdef TEXTURE_SUPPORT
                    u += du;
                    v += dv;
                #elif defined(INTERPOLATE_COLORS)
                    r += dr;
                    g += dg;
                    b += db;
                #endif

                z += dz;
            }
        }

        xstart += dx_far;
        zstart += dz_far;

        xend += dx_current;
        zend += dz_current;

        #ifdef TEXTURE_SUPPORT
                ustart += du_far;
                vstart += dv_far;
                uend += du_current;
                vend += dv_current;
        #elif defined(INTERPOLATE_COLORS)
                rstart += dr_far;
                gstart += dg_far;
                bstart += db_far;

                rend += dr_current;
                gend += dg_current;
                bend += db_current;
        #endif

        if(__builtin_expect(y == middle_y, false))
        {
            dx_current = dx_upper;
            dz_current = dz_upper;

            #ifdef TEXTURE_SUPPORT
                du_current = du_upper;
                dv_current = dv_upper;
            #elif defined(INTERPOLATE_COLORS)
                dr_current = dr_upper;
                dg_current = dg_upper;
                db_current = db_upper;
            #endif
        }
    }

    return;

    otherway:
    for(; y <= high_y; y += 1, screen_buf_line += SCREEN_WIDTH, z_buf_line += SCREEN_WIDTH)
    {
        const int x1 = xend, x2 = xstart;
        const int line_width = x1 - x2;
        if(__builtin_expect(line_width <= -1, true))
        {
            const auto inv_l = Fix<16, int32_t>(1) / line_width;
            //Here are the differences
            const TriFix dz = (zend - zstart) * inv_l;
            TriFix z = zend;

            #ifdef TEXTURE_SUPPORT
                const TriFix du = (uend - ustart) * inv_l;
                const TriFix dv = (vend - vstart) * inv_l;
                TriFix u = uend, v = vend;
            #elif defined(INTERPOLATE_COLORS)
                const TriFix dg = (gend - gstart) * inv_l;
                const TriFix db = (bend - bstart) * inv_l;
                const TriFix dr = (rend - rstart) * inv_l;

                TriFix r = rend, g = gend, b = bend;
            #endif

            decltype(z_buffer) z_buf = z_buf_line + x1;
            decltype(screen) screen_buf = screen_buf_line + x1;
            for(int x = x1; x <= x2; x += 1, ++z_buf, ++screen_buf)
            {
                if(__builtin_expect(TriFix(*z_buf) > z, true))
                {
                    #ifdef TEXTURE_SUPPORT
                        COLOR c = loc_texture.bitmap[u.floor() + v.floor()*loc_texture.width];
                        #ifdef TRANSPARENCY
                            if(__builtin_expect(c != 0x0000, 1))
                            {
                                *screen_buf = c;
                                *z_buf = z;
                            }
                        #else
                            *screen_buf = c;
                            *z_buf = z;
                        #endif
                    #elif defined(INTERPOLATE_COLORS)
                        *screen_buf = colorRGB(r, g, b);
                        *z_buf = z;
                    #else
                        *screen_buf = low->c;
                        *z_buf = z;
                    #endif
                }

                #ifdef TEXTURE_SUPPORT
                    u += du;
                    v += dv;
                #elif defined(INTERPOLATE_COLORS)
                    r += dr;
                    g += dg;
                    b += db;
                #endif

                z += dz;
            }
        }

        xstart += dx_far;
        zstart += dz_far;

        xend += dx_current;
        zend += dz_current;

        #ifdef TEXTURE_SUPPORT
                ustart += du_far;
                vstart += dv_far;

                uend += du_current;
                vend += dv_current;
        #elif defined(INTERPOLATE_COLORS)
                rstart += dr_far;
                gstart += dg_far;
                bstart += db_far;

                rend += dr_current;
                gend += dg_current;
                bend += db_current;
        #endif

        if(__builtin_expect(y == middle_y, false))
        {
            dx_current = dx_upper;
            dz_current = dz_upper;

            #ifdef TEXTURE_SUPPORT
                du_current = du_upper;
                dv_current = dv_upper;
            #elif defined(INTERPOLATE_COLORS)
                dr_current = dr_upper;
                dg_current = dg_upper;
                db_current = db_upper;
            #endif
        }
    }
}

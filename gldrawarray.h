#ifndef GLDRAWARRAY_H
#define GLDRAWARRAY_H

#include "gl.h"

struct IndexedVertex {
    unsigned int index;
    GLFix u, v;
    COLOR c;
};

struct ProcessedPosition {
    VECTOR3 transformed;
    VECTOR3 perspective;
    bool perspective_available;
};

/* Faster way to draw a mesh.
 * vertices: Array of IndexedVertex with size count_vertices
 * positions: Array of VECTOR3 with size count_positions the IndexedVertex's refer to
 * processed: Array of ProcessedVertex with size count_positions. Allocate and free it yourself.
 * reset_processed: Set to false if you want to use the same positions with the same transformation. Default is true.
 * draw_mode: GL_TRIANGLES or GL_QUADS */
void nglDrawArray(const IndexedVertex *vertices, const unsigned int count_vertices, const VECTOR3 *positions, const unsigned int count_positions, ProcessedPosition *processed, const GLDrawMode draw_mode = GL_TRIANGLES, const bool reset_processed = true);

#endif // GLDRAWARRAY_H

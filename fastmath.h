#ifndef FASTMATH_H
#define FASTMATH_H

#include "gl.h"

typedef Fix<8, int32_t> FFix;

void init_fastmath();
void uninit_fastmath();
FFix fast_sin(FFix deg);
FFix fast_cos(FFix deg);

#endif

#ifndef GRAPHICS_UTILS_H
#define GRAPHICS_UTILS_H

#include "core/types.h"
#include "core/tealeaf_context.h"
#include "core/texture_2d.h"

void apply_composite_operation(int composite_op);
void set_up_full_compositing(context_2d *ctx, int x, int y, int width, int height, int composite_op);
bool is_full_canvas_composite_operation(int composite_op);

#endif

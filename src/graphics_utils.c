#include "core/graphics_utils.h"
#include "core/geometry.h"
#include "log.h"
#include <stdlib.h>

void apply_composite_operation(int composite_op) {
    int sfactor, dfactor;

    switch (composite_op) {
    case source_atop:
        sfactor = GL_DST_ALPHA;
        dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;

    case source_in:
        sfactor = GL_DST_ALPHA;
        dfactor = GL_ZERO;
        break;

    case source_out:
        sfactor = GL_ONE_MINUS_DST_ALPHA;
        dfactor = GL_ZERO;
        break;

    case source_over:
        sfactor = GL_ONE;
        dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;

    case destination_atop:
        sfactor = GL_DST_ALPHA;
        dfactor = GL_SRC_ALPHA;
        break;

    case destination_in:
        sfactor = GL_ZERO;
        dfactor = GL_SRC_ALPHA;
        break;

    case destination_out:
        sfactor = GL_ONE_MINUS_SRC_ALPHA;
        dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;

    case destination_over:
        sfactor = GL_DST_ALPHA;
        dfactor = GL_SRC_ALPHA;
        break;

    case lighter:
        sfactor = GL_ONE;
        dfactor = GL_ONE;
        break;

    case x_or:
    case copy:
    default:
        sfactor = GL_ONE;
        dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    }

    glEnable(GL_BLEND);
    glBlendFunc(sfactor, dfactor);
}

//redraw read pixels
//apply the blendfunc
void set_up_full_compositing(context_2d *ctx, int x, int y, int width, int height, int composite_op) {
    //clear the 4 rect's around the area
    LOG("SETUP %i %i %i %i", x, y, ctx->width, height);
    rect_2d top_rect = {0, 0, ctx->width, y};
    rect_2d left_rect = {0, 0, x, ctx->height};
    rect_2d bot_rect = {0, y + height, ctx->width, ctx->height - (y + height)};
    rect_2d right_rect = {x + width, 0, ctx->width - (x + width), ctx->height};
    context_2d_clearRect(ctx, &top_rect);
    context_2d_clearRect(ctx, &left_rect);
    context_2d_clearRect(ctx, &bot_rect);
    context_2d_clearRect(ctx, &right_rect);
}

bool is_full_canvas_composite_operation(int composite_op) {
    return (composite_op == source_in || composite_op == source_out ||
            composite_op == destination_in || composite_op == destination_atop);
}


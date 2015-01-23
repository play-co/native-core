
/* @license
 * This file is part of the Game Closure SDK.
 *
 * The Game Closure SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * The Game Closure SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with the Game Closure SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "core/timestep/timestep_view.h"
#include "core/timestep/timestep_image_map.h"
#include "core/list.h"
#include "js/js_timestep_view.h"
#include "js/js.h"
#include "core/log.h"
#include "core/tealeaf_context.h"

static unsigned int UID = 0;
static int add_order = 0;

static void default_view_render(timestep_view *v, context_2d *ctx) {
    return;
}

static void image_view_render(timestep_view *v, context_2d *ctx) {
    LOGFN("image_view_render");


    timestep_image_map *map = (timestep_image_map *) v->view_data;
    if (map && map->url) {
        float scale_x = (float)v->width / (map->margin_left + map->width + map->margin_right);
        float scale_y = (float)v->height / (map->margin_top + map->height + map->margin_bottom);
        rect_2d src_rect = {
          static_cast<float>(map->x),
          static_cast<float>(map->y),
          static_cast<float>(map->width),
          static_cast<float>(map->height)
        };
        rect_2d dest_rect = {
            scale_x * map->margin_left,
            scale_y * map->margin_top,
            scale_x * map->width,
            scale_y * map->height
        };
#if defined(DEBUG)
        if (map->canary != CANARY_GOOD) {
            LOG("ERROR: !! The map canary is dead !! %x", map->canary);
        } else {
#endif
            context_2d_drawImage(ctx, 0, map->url, &src_rect, &dest_rect);
#if defined(DEBUG)
        }
#endif
    }


    LOGFN("end image_view_render");
}

static void default_view_tick(timestep_view *v, double dt) {
}

timestep_view *timestep_view_init() {
    LOGFN("timestep_view_init");
    timestep_view *v = (timestep_view*)malloc(sizeof(timestep_view));
    v->uid = ++UID;
    v->has_jsrender = false;
    v->has_jstick = false;

    v->superview = NULL;
    v->subview_array_size = 4;
    v->subviews = (timestep_view**)malloc(sizeof(timestep_view*) * v->subview_array_size);

    v->subview_count = 0;
    v->subview_index = 0;
    v->x = 0;
    v->y = 0;
    v->width = UNDEFINED_DIMENSION;
    v->height = UNDEFINED_DIMENSION;
    v->r = 0;
    v->anchor_x = 0;
    v->anchor_y = 0;
    v->offset_x = 0;
    v->offset_y = 0;
    v->flip_x = false;
    v->flip_y = false;
    v->scale = 1;
    v->scale_x = 1;
    v->scale_y = 1;
    v->abs_scale = 1;
    v->clip = false;
    v->visible = true;
    v->z_index = 0;
    v->dirty_z_index = false;
    v->opacity = 1;
    v->timestep_view_render = default_view_render;
    v->timestep_view_tick = default_view_tick;

    v->composite_operation = 0;

    v->needs_reflow = true;

    v->background_color.r = 0;
    v->background_color.g = 0;
    v->background_color.b = 0;
    v->background_color.a = 0;

    v->anim_count = 0;
    v->max_anims = 0;
    v->anims = NULL;
    v->view_data = NULL;
    js_object_wrapper_init(&v->map_ref);

    v->filter_color.r = 0;
    v->filter_color.g = 0;
    v->filter_color.b = 0;
    v->filter_color.a = 0;
    v->filter_type = 0;

    LOGFN("end timestep_view_init");

    return v;
}

void timestep_view_set_type(timestep_view *v, unsigned int type) {
    LOGFN("timestep_view_set_type");
    switch (type) {
    default:
    case DEFAULT_RENDER:
        break;
    case IMAGE_VIEW:
        v->timestep_view_render = image_view_render;
        break;
    }
    LOGFN("end timestep_view_set_type");
}

static double abs_scale = 1;

void timestep_view_start_render() {
    abs_scale = 1;
}

void timestep_view_wrap_render(timestep_view *v, context_2d *ctx, JS_OBJECT_WRAPPER js_ctx, JS_OBJECT_WRAPPER js_opts) {
    LOGFN("timestep_view_wrap_render");
    if (!v->visible || !v->opacity) {
        return;
    }

    if (v->dirty_z_index) {
        v->dirty_z_index = false;
        timestep_view_sort_subviews(v);
    }
    if (v->width < 0 || v->height < 0) {
        return;
    }

    context_2d_save(ctx);
    context_2d_translate(ctx, v->x + v->anchor_x + v->offset_x, v->y + v->anchor_y + v->offset_y);

    if (v->r) {
        context_2d_rotate(ctx, v->r);
    }
    if (v->scale != 1 || v->scale_x != 1 || v->scale_y != 1) {
        context_2d_scale(ctx, v->scale * v->scale_x, v->scale * v->scale_y);
        abs_scale *= v->scale;
    }

    v->abs_scale = abs_scale;

    if (v->opacity != 1) {
        double alpha = context_2d_getGlobalAlpha(ctx);
        context_2d_setGlobalAlpha(ctx, alpha * v->opacity);
    }

    context_2d_translate(ctx, -v->anchor_x, -v->anchor_y);

    if (v->clip) {
        rect_2d r = {0, 0, static_cast<float>(v->width), static_cast<float>(v->height)};
        context_2d_setClip(ctx, r);
    }

    //apply filters
    rgba f = v->filter_color;
    ctx->filter_color.r = f.r;
    ctx->filter_color.g = f.g;
    ctx->filter_color.b = f.b;
    ctx->filter_color.a = f.a;
    if (v->filter_type != FILTER_NONE) {
        context_2d_set_filter_type(ctx, v->filter_type);
    }

    if (v->background_color.a > 0) {
        //LOG("render %i with background %f %f %f %f", v->uid, v->background_color.r, v->background_color.g, v->background_color.b, v->background_color.a);

        rect_2d r = {0, 0, static_cast<float>(v->width), static_cast<float>(v->height)};
        context_2d_fillRect(ctx, &r, &v->background_color);
    }


    if (v->flip_x || v->flip_y) {
        context_2d_translate(ctx,
                             v->flip_x ? v->width / 2 : 0,
                             v->flip_y ? v->height / 2 : 0);

        context_2d_scale(ctx,
                         v->flip_x ? -1 : 1,
                         v->flip_y ? -1 : 1);

        context_2d_translate(ctx,
                             v->flip_x ? -v->width / 2 : 0,
                             v->flip_y ? -v->height / 2 : 0);
    }

    // Set Global Composite Operation
    if (0 != v->composite_operation) {
        context_2d_setGlobalCompositeOperation(ctx, v->composite_operation);
    }

  JS_OBJECT_WRAPPER js_viewport;
  bool should_restore_viewport = false;
    if (v->has_jsrender) {
        should_restore_viewport = true;
        js_viewport = def_get_viewport(js_opts);
        def_timestep_view_render(v->js_view, js_ctx, js_opts);
    } else {
        v->timestep_view_render(v, ctx);
    }

    //restore filters
    if (v->filter_type != FILTER_NONE) {
        context_2d_set_filter_type(ctx, FILTER_NONE);
    }
    ctx->filter_color.r = 0;
    ctx->filter_color.g = 0;
    ctx->filter_color.b = 0;
    ctx->filter_color.a = 0;

    for (unsigned int i = 0; i < v->subview_count; i++) {
        timestep_view *subview = v->subviews[i];
        timestep_view_wrap_render(subview, ctx, js_ctx, js_opts);
    }

    if (should_restore_viewport) {
        def_restore_viewport(js_opts, js_viewport);
    }

    context_2d_restore(ctx);

    LOGFN("end timestep_view_wrap_render");
}

void timestep_view_render(timestep_view *v) {
    // do background color???
}



void timestep_view_wrap_tick(timestep_view *v, double dt) {
    LOGFN("timestep_view_wrap_tick");
    if (v->has_jstick) {
        def_timestep_view_tick(v->js_view, dt);
    } else {
        v->timestep_view_tick(v, dt);
    }

    for (unsigned int i = 0; i < v->subview_count; i++) {
        timestep_view *subview = v->subviews[i];
        if (subview) {
            timestep_view_wrap_tick(subview, dt);
        }
    }

    // TODO: needs repaint?
    LOGFN("end timestep_view_wrap_tick");
}

void timestep_view_tick(timestep_view *v, double dt) {

}

void timestep_view_add_animation(timestep_view *view, view_animation *anim) {
    LOGFN("timestep_view_add_animation");

    if (view->anim_count == view->max_anims) {
        view->max_anims = view->max_anims ? view->max_anims * 2 : 1;
        view->anims = (view_animation**)realloc(view->anims, sizeof(view_animation *) * view->max_anims);
    }
    view->anims[view->anim_count++] = anim;
    LOGFN("end timestep_view_add_animation");
}

void timestep_view_remove_animation(timestep_view *view, view_animation *anim) {
    LOGFN("timestep_view_remove_animation");

    //TODO this shouldn't be linear
    unsigned int i;
    view->anim_count--; //decrement now because if it's the last one we don't have to do anything
    for (i = 0; i < view->anim_count; i++) {
        if (view->anims[i] == anim) {
            memcpy(view->anims[i+1], view->anims[i], sizeof(view_animation *) * view->anim_count - i );
        }
    }
    LOGFN("end timestep_view_remove_animation");
}

static int timestep_view_comparator(const void *a, const void *b) {
    int diff = (*(timestep_view**)a)->z_index - (*(timestep_view**)b)->z_index;
    if (diff == 0) {
        return (*(timestep_view**)a)->added_at - (*(timestep_view**)b)->added_at;
    } else {
        return diff;
    }
}

void timestep_view_sort_subviews(timestep_view *v) {
    LOGFN("timestep_view_sort_subviews");
    qsort(v->subviews, v->subview_count, sizeof(timestep_view*), timestep_view_comparator);
    for (unsigned int i = 0; i < v->subview_count; i++) {
        v->subviews[i]->subview_index = i;
    }
    LOGFN("end timestep_view_sort_subviews");
}

bool timestep_view_add_subview(timestep_view *v, timestep_view *subview) {
    LOGFN("timestep_view_add_subview");
    if (subview->superview == v) {
        return false;
    }
    if (subview->superview) {
        timestep_view_remove_subview(subview->superview, subview);
    }
    if (v->subview_array_size <= v->subview_count) {
        v->subview_array_size *= 2;
        v->subviews = (timestep_view**)realloc(v->subviews, sizeof(timestep_view*) * v->subview_array_size);
    }

    //LOG(">>> adding to view %i subview %i; current size %i", v->uid, subview->uid, v->subview_count);
    subview->subview_index = v->subview_count;
    v->subviews[v->subview_count++] = subview;
    subview->superview = v;
    subview->added_at = ++add_order;
    subview->needs_reflow = true;
    v->dirty_z_index = true;

    LOGFN("end timestep_view_add_subview");
    return true;
}

bool timestep_view_remove_subview(timestep_view *v, timestep_view *subview) {
    LOGFN("timestep_view_remove_subview");
    unsigned int index = subview->subview_index;
    unsigned int count = v->subview_count;

    timestep_view **pos = &v->subviews[index];
    //LOG("removing %i from %i", subview->uid, v->uid);
    //LOG("count %i index %i", count, index);
    if (index < count && *pos == subview) {
        timestep_view **to = pos;
        timestep_view **from = to + 1;
        int bytes = sizeof(timestep_view*) * (count - index - 1);
        memmove(to, from, bytes);
        v->subview_count--;
        for (unsigned int i = index; i < count - 1; ++i) {
            v->subviews[i]->subview_index = i;
        }
        subview->superview = NULL;
        LOGFN("end timestep_view_remove_subview");
        return true;
    } else {
//		LOG("ERRROR!!!!!!!!!!!!!!!!!!!!!!!! tried to remove a subview that wasn't a subview!!!!!!!!!!!!!!!");
        LOGFN("end timestep_view_remove_subview");
        return false;
    }
}
void timestep_view_add_filter(timestep_view *v, rgba *color) {
    v->filter_color.r = color->r;
    v->filter_color.g = color->g;
    v->filter_color.b = color->b;
    v->filter_color.a = color->a;
}

void timestep_view_clear_filters(timestep_view *v) {
    v->filter_color.r = 0;
    v->filter_color.g = 0;
    v->filter_color.b = 0;
    v->filter_color.a = 0;
}

timestep_view *timestep_view_get_superview(timestep_view *v) {
    LOGFN("timestep_view_get_superview");
    return v->superview;
}

void timestep_view_delete(timestep_view *v) {
    LOGFN("timestep_view_delete");

    for (unsigned int i = 0, count = v->anim_count; i < count; ++i) {
        // Unlink from the animation
        v->anims[i]->view = NULL;
    }

    // Free memory for animation array
    free(v->anims);

    // If view is still connected,
    if (v->superview) {
        // Disconnect it before deleting
        timestep_view_remove_subview(v->superview, v);
    }

    // Disconnect all subviews
    for (unsigned int i = 0, count = v->subview_count; i < count; ++i) {
        timestep_view *subview = v->subviews[i];

        // Stomp.
        subview->superview = NULL;
    }

    free(v->subviews);
    js_object_wrapper_delete(&v->map_ref);
    free(v);
}

CEXPORT void timestep_view_shutdown() {
    UID = 0;
    add_order = 0;
}

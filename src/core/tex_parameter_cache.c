#include "core/tex_parameter_cache.h"

static tex_parameter_cache_t parameter_cache;

tex_parameter_cache_t *get_tex_parameter_cache() {
  return &parameter_cache;
}

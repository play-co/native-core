#ifndef TEX_PARAMETER_CACHE_H
#define TEX_PARAMETER_CACHE_H

typedef struct tex_parameter_cache {
  int min_filter;
  int mag_filter;
  int wrap_s;
  int wrap_t;
} tex_parameter_cache_t;

tex_parameter_cache_t *get_tex_parameter_cache();

#endif //TEX_PARAMETER_CACHE_H

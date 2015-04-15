#ifndef APPLICATION_BUNDLE_H
#define APPLICATION_BUNDLE_H

#include "core/core_js.h"
#include "uthash/uthash.h"

struct application_bundle {
    char *name;
    char *path;
    char *bundle_path;
    char *remote_url;
    bool local;
    bool remote;
    js_bundle_t *js;
    UT_hash_handle hh;
};

// TODO is this used?
typedef enum {
    NEW,
    EVALUATED,
    LAUNCHING,
    READY
} js_state_t;

#ifdef __cplusplus
#define CEXPORT extern "C"
#else
#define CEXPORT
#endif

CEXPORT application_bundle_t* get_application_bundle(const char *name);

CEXPORT application_bundle_t* init_application_bundle(const char *name,
                                                      const char *url);

CEXPORT application_bundle_t* get_active_application();

CEXPORT void enter_application_bundle(application_bundle_t *bundle);
CEXPORT void exit_application_bundle(application_bundle_t *bundle);

CEXPORT bool ready_for_tick(application_bundle_t*);

#endif

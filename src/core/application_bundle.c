#include <stdlib.h>

#include "core/core_js.h"
#include "core/log.h"

#include "core/application_bundle.h"
#include "uthash/uthash.h"

static application_bundle_t *bundles = NULL;
application_bundle_t *current_bundle = NULL;

void resource_loader_init_bundle(application_bundle_t*);


application_bundle_t* init_application_bundle(const char *name,
                                              const char *url) {
    LOG("init_application_bundle %s", name);

    application_bundle_t *b = NULL;

    b = (application_bundle_t*)malloc(sizeof(application_bundle_t));
    b->name = strdup(name);
    b->path = NULL;
    b->remote_url = NULL;
    b->bundle_path = NULL;
    b->show_preload = true;

    if (url == NULL || strlen(url) == 0) {
        b->local = true;
    } else {
        b->local = false;
        b->remote_url = strdup(url);
    }

    b->remote = !b->local;

    resource_loader_init_bundle(b);
    js_init_bundle(b);

    HASH_ADD_KEYPTR(hh, bundles, b->name, strlen(b->name), b);

    return b;
}

application_bundle_t* get_application_bundle(const char *name) {
    LOG("get_application_bundle %s", name);

    application_bundle_t *app = NULL;
    HASH_FIND_STR(bundles, name, app);
    return app;
}

void resource_loader_set_bundle(const application_bundle_t*);

void enter_application_bundle(application_bundle_t *bundle) {
    LOG("enter_application_bundle %s", bundle->name);
    if (current_bundle != NULL) {
        exit_application_bundle(current_bundle);
    }

    current_bundle = bundle;

    resource_loader_set_bundle(bundle);
    js_enter_bundle(bundle);
}

void exit_application_bundle(application_bundle_t *bundle) {
    if (current_bundle != NULL) {
        js_exit_bundle(bundle);
        current_bundle = NULL;
    }
}

application_bundle_t* get_active_application() {
    return current_bundle;
}

bool ready_for_tick(application_bundle_t *bundle) {
    return js_ready_for_tick(bundle);
}

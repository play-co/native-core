#include "core/core_js.h"
#include "core/log.h"

#include "core/application_bundle.h"
#include "uthash/uthash.h"

static application_bundle_t *bundles = 0;

application_bundle_t *current_bundle = 0;

static application_bundle_t* init_application_bundle(const char *name) {
    LOG("init_application_bundle %s", name);

    application_bundle_t *b = 0;

    b = (application_bundle_t*)malloc(sizeof(application_bundle_t));
    b->name = strdup(name);

    js_init_bundle(b);

    HASH_ADD_KEYPTR(hh, bundles, b->name, strlen(b->name), b);

    return b;
}

application_bundle_t* get_application_bundle(const char *name) {
    LOG("get_application_bundle %s", name);
    application_bundle_t *b = 0;
    HASH_FIND_STR(bundles, name, b);

    if (b == 0) {
        return init_application_bundle(name);
    }

    return b;
}


void enter_application_bundle(application_bundle_t *bundle) {
    LOG("enter_application_bundle %s", bundle->name);
    if (current_bundle != 0) {
        exit_application_bundle(current_bundle);
    }

    current_bundle = bundle;
    js_enter_bundle(bundle);
}

void exit_application_bundle(application_bundle_t *bundle) {
    if (current_bundle != 0) {
        js_exit_bundle(bundle);
        current_bundle = 0;
    }
}

application_bundle_t* get_active_application() {
    return current_bundle;
}

bool ready_for_tick(application_bundle_t *bundle) {
    return js_ready_for_tick(bundle);
}

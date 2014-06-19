/* @license
 * This file is part of the Game Closure SDK.
 *
 * The Game Closure SDK is free software: you can redistribute it and/or modify
 * it under the terms of the Mozilla Public License v. 2.0 as published by Mozilla.

 * The Game Closure SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Mozilla Public License v. 2.0 for more details.

 * You should have received a copy of the Mozilla Public License v. 2.0
 * along with the Game Closure SDK.  If not, see <http://mozilla.org/MPL/2.0/>.
 */

/**
 * @file	 config.c
 * @brief
 */
#include "core/config.h"
#include <string.h>
#include <stdlib.h>

#define DEF_SIMULATE_ID "tealeaf"
#define DEF_CODE_PATH "native.js"
#define DEF_CODE_HOST "localhost"
#define DEF_TCP_HOST "localhost"
#define DEF_TCP_PORT 4747
#define DEF_CODE_PORT 9200
#define DEF_ENTRY_POINT "gc.native.launchClient"
#define DEF_WIDTH 480
#define DEF_HEIGHT 800
#define DEF_REMOTE_LOADING false
#define DEF_SPLASH "loading.png"

typedef struct config_t {
    const char *code_path;      // Code path
    const char *code_host;      // Code remote hostname
    int code_port;              // Code port
    const char *tcp_host;       // TCP remote hostname
    int tcp_port;               // TCP port
    const char *entry_point;    // Code entry point
    int width;                  // Screen width
    int height;                 // Screen height
    bool remote_loading;        // Textures and other resources loaded from Java?
    const char *simulate_id;    // Simulate id
    const char *splash;			// Splash path
} config;

static config cfg = {
    DEF_CODE_PATH,
    DEF_CODE_HOST,
    DEF_CODE_PORT,
    DEF_TCP_HOST,
    DEF_TCP_PORT,
    DEF_ENTRY_POINT,
    DEF_WIDTH,
    DEF_HEIGHT,
    DEF_REMOTE_LOADING,
    DEF_SIMULATE_ID,
    DEF_SPLASH
};

#define RESET_STR(X, DEF) { \
    if ((X) && (X) != (DEF)) { \
        free((char *)(X)); \
    } \
    (X) = (DEF); \
}

#define SET_STR(X, VAL, DEF) { \
    if (VAL) { \
        if ((X) && (X) != (DEF)) { \
            free((char*)(X)); \
        } \
        (X) = strdup(VAL); \
    } \
}

// Setters
/**
 * @name	config_set_simulate_id
 * @brief	sets the simulate id on the config object
 * @param	simulate_id - the id identifying which game is being simulated {EXAMPLE}
 * @retval	NONE
 */
void config_set_simulate_id(const char *simulate_id) {
    SET_STR(cfg.simulate_id, simulate_id, DEF_SIMULATE_ID);
}

/**
 * @name	config_set_remote_loading
 * @brief	turns remote loading on / off on the config object
 * @param	remote_loading - whether remote loading should be on / off
 * @retval	NONE
 */
void config_set_remote_loading(bool remote_loading) {
    cfg.remote_loading = remote_loading;
}

/**
 * @name	config_set_code_host
 * @brief	sets the code host on the config object
 * @param	code_host - the code host {EXAMPLE}
 * @retval	NONE
 */
void config_set_code_host(const char *code_host) {
    SET_STR(cfg.code_host, code_host, DEF_CODE_HOST);
}

/**
 * @name	config_set_entry_point
 * @brief	sets the entry point on the config object
 * @param	entry_point - the entry point {EXAMPLE}
 * @retval	NONE
 */
void config_set_entry_point(const char *entry_point) {
    SET_STR(cfg.entry_point, entry_point, DEF_ENTRY_POINT);
}

/**
 * @name	config_set_tcp_host
 * @brief	sets the tcp host on the config object
 * @param	tcp_host - the tcp host {EXAMPLE}
 * @retval	NONE
 */
void config_set_tcp_host(const char *tcp_host) {
    SET_STR(cfg.tcp_host, tcp_host, DEF_TCP_HOST);
}

/**
 * @name	config_set_code_path
 * @brief	sets the code path on the config object
 * @param	code_path - the code path {EXAMPLE}
 * @retval	NONE
 */
void config_set_code_path(const char *code_path) {
    SET_STR(cfg.code_path, code_path, DEF_CODE_PATH);
}

/**
 * @name	config_set_tcp_port
 * @brief	sets the tcp port on the config object
 * @param	port - the tcp port {EXAMPLE}
 * @retval	NONE
 */
void config_set_tcp_port(int port) {
    cfg.tcp_port = port;
}

/**
 * @name	config_set_code_port
 * @brief	sets the code port on the config object
 * @param	port - the code port {EXAMPLE}
 * @retval	NONE
 */
void config_set_code_port(int port) {
    cfg.code_port = port;
}

/**
 * @name	config_set_screen_width
 * @brief	sets the screen width on the config object
 * @param	width - width of the screen
 * @retval	NONE
 */
void config_set_screen_width(int width) {
    cfg.width = width;
}

/**
 * @name	config_set_screen_height
 * @brief	sets the screen height on the config object
 * @param	height - height of the screen
 * @retval	NONE
 */
void config_set_screen_height(int height) {
    cfg.height = height;
}

/**
 * @name	config_set_splash
 * @brief	sets the splash screen path on the config object
 * @param	splash - path to splash screen
 * @retval	NONE
 */
void config_set_splash(const char *splash) {
    SET_STR(cfg.splash, splash, DEF_SPLASH);
}


// Getters
/**
 * @name	config_get_simulate_id
 * @brief	gets the simulate id from the config
 * @retval	char* - the simulate id
 */
const char *config_get_simulate_id() {
    return cfg.simulate_id;
}

/**
 * @name	config_get_remote_loading
 * @brief	gets whether remote loading is on or off from the config
 * @retval	bool - true or false depending on if remote loading is on
 */
bool config_get_remote_loading() {
    return cfg.remote_loading;
}

/**
 * @name	config_get_code_host
 * @brief	gets the code host from the config
 * @retval	char* - the code host
 */
const char *config_get_code_host() {
    return cfg.code_host;
}

/**
 * @name	config_get_code_path
 * @brief	gets the code path from the config
 * @retval	char* - the code path
 */
const char *config_get_code_path() {
    return cfg.code_path;
}

/**
 * @name	config_get_tcp_host
 * @brief	gets the tcp host from the config
 * @retval	char* - tcp host
 */
const char *config_get_tcp_host() {
    return cfg.tcp_host;
}

/**
 * @name	config_get_entry_point
 * @brief	gets the entry point from the config
 * @retval	char* - entry point
 */
const char *config_get_entry_point() {
    return cfg.entry_point;
}

/**
 * @name	config_get_code_port
 * @brief	gets the code port from the config
 * @retval	int - code port
 */
int config_get_code_port() {
    return cfg.code_port;
}

/**
 * @name	config_get_tcp_port
 * @brief	gets the tcp port from the config
 * @retval	int - tcp port
 */
int config_get_tcp_port() {
    return cfg.tcp_port;
}

/**
 * @name	config_get_screen_width
 * @brief	gets the screen width from the config
 * @retval	int - screen width
 */
int config_get_screen_width() {
    return cfg.width;
}

/**
 * @name	config_get_screen_height
 * @brief	gets the screen height from the config
 * @retval	int - screen height
 */
int config_get_screen_height() {
    return cfg.height;
}

/**
 * @name	config_get_splash
 * @brief	gets the splash screen path from the config
 * @retval	const char * - splash path
 */
const char *config_get_splash() {
    return cfg.splash;
}


/**
 * @name	config_clear
 * @brief	resets all config options to defaults
 * @retval	NONE
 */
void config_clear() {
    RESET_STR(cfg.code_host, DEF_CODE_HOST);
    RESET_STR(cfg.tcp_host, DEF_TCP_HOST);
    RESET_STR(cfg.entry_point, DEF_ENTRY_POINT);
    RESET_STR(cfg.simulate_id, DEF_SIMULATE_ID);
    RESET_STR(cfg.code_path, DEF_CODE_PATH);
    cfg.code_port = DEF_CODE_PORT;
    cfg.tcp_port = DEF_TCP_PORT;
    cfg.width = DEF_WIDTH;
    cfg.height = DEF_HEIGHT;
    cfg.remote_loading = DEF_REMOTE_LOADING;
    RESET_STR(cfg.splash, DEF_SPLASH);
}

#undef SET_STR
#undef RESET_STR

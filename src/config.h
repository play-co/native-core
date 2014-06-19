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

#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

#if defined(__cplusplus)
extern "C" {
#endif

	const char *config_get_simulate_id();
	void config_set_simulate_id(const char *simulate_id);

	bool config_get_remote_loading();
	void config_set_remote_loading(bool remote_loading);

	const char *config_get_code_host();
	void config_set_code_host(const char *url);

	void config_set_entry_point(const char *entry_point);
	const char *config_get_entry_point();

	void config_set_app_id(const char *app_id);
	const char *config_get_app_id();

	void config_set_tcp_host(const char *host);
	const char *config_get_tcp_host();

	void config_set_code_path(const char *code_path);
	const char *config_get_code_path();

	void config_set_tcp_port(int port);
	int config_get_tcp_port();

	void config_set_code_port(int port);
	int config_get_code_port();

	void config_set_screen_width(int width);
	int config_get_screen_width();

	void config_set_screen_height(int height);
	int config_get_screen_height();
	
	void config_set_splash(const char *splash);
	const char *config_get_splash();

	void config_clear();

#if defined(__cplusplus)
} // extern "C"
#endif

#endif

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

#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

void sound_manager_load_sound(const char *url);
void sound_manager_play_sound(const char *url, float volume, bool loop);
void sound_manager_stop_sound(const char *url);
void sound_manager_pause_sound(const char *url);
void sound_manager_set_volume(const char *url, float volume);
void sound_manager_seek_to(const char *url, float position);
void sound_manager_load_background_music(const char *url);
void sound_manager_play_background_music(const char *url, float volume, bool loop);
void sound_manager_stop_loading_sound();
	
#ifdef __cplusplus
}
#endif

#endif

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

#ifndef GL_H
#define GL_H

#define GL_GLEXT_PROTOTYPES

#ifdef ANDROID
#define GL_ES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#elif __APPLE__
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE
#define GL_ES
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#elif TARGET_IPHONE_SIMULATOR
#define GL_ES
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#elif TARGET_OS_MAC
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#endif
#elif __linux__
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif


#ifndef GL_ES
#define GL_BINDFRAMEBUFFER glBindFramebuffer
#define GL_FRAMEBUFFERTEXTURE2D glFramebufferTexture2D
#define GL_CHECKFRAMEBUFFERSTATUS glCheckFramebufferStatus
#define GL_GENFRAMEBUFFERS glGenFramebuffers
#define GL_ORTHO glOrtho
#endif

#endif //GL_H

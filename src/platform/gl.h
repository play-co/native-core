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

//#define ENABLE_GLTRACE 1
// Define ENABLE_GLTRACE to report errors in GL commands at the console (slow)
#ifndef ENABLE_GLTRACE
#define GLTRACE(cmd) cmd
#else

#define GLTRACE_STRINGIZE(x) GLTRACE_STRINGIZE1(x)
#define GLTRACE_STRINGIZE1(x) #x
#define GLTRACE_FILELINE __FILE__ " at line " GLTRACE_STRINGIZE(__LINE__)

#define GLTRACE(cmd) cmd; \
	{	int gltrace_err = glGetError(); \
		if (gltrace_err != 0) { \
			LOG("{gl} TRACE: Error %d at " #cmd " in " GLTRACE_FILELINE, gltrace_err); \
		} \
	}
#endif

#endif //GL_H

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
 * @file	 tealeaf_shaders.c
 * @brief
 */
#include "tealeaf_shaders.h"
#include "tealeaf_context.h"
#include "platform/gl.h"
#include "core/log.h"
#include <stdlib.h>

static char *linear_add_vertex_shader_code = "														\
																						\
  attribute vec2 attr_vertex_coord;														\
  attribute vec2 attr_tex_coord;														\
  																						\
  uniform mat4 proj_matrix;																\
																						\
  varying vec2 v_tex_coord;																\
																						\
  void main(void) {																		\
    gl_Position = proj_matrix * vec4(attr_vertex_coord, 0.0, 1.0);						\
    v_tex_coord = attr_tex_coord;														\
  }																						\
";

/* 'float a = base.a' was added because of what seems
 * to be a shader compilation bug on the LG Nexus 4
 * where if add_color was a vec4(0,0,0,0) and base.a
 * was anything, base + add_color * base.a would produce
 * a full white fragment, but maintained the proper
 * alpha value.
 */
static char *linear_add_fragment_shader_code = "													\
	precision mediump float;															\
																						\
	varying vec2 v_tex_coord;															\
	uniform lowp vec4 draw_color;														\
	uniform lowp vec4 add_color;														\
																						\
	uniform sampler2D tex_sampler;														\
																						\
	void main(void) {	\
		vec4 base = draw_color*texture2D(tex_sampler, v_tex_coord.st) ;	\
		float a = base.a;\
		gl_FragColor = base + add_color * a;\
	}";

static char *vertex_shader_code = "														\
																						\
  attribute vec2 attr_vertex_coord;														\
  attribute vec2 attr_tex_coord;														\
  																						\
  uniform mat4 proj_matrix;																\
																						\
  varying vec2 v_tex_coord;																\
																						\
  void main(void) {																		\
    gl_Position = proj_matrix * vec4(attr_vertex_coord, 0.0, 1.0);						\
    v_tex_coord = attr_tex_coord;														\
  }																						\
";

static char *fragment_shader_code = "													\
	precision mediump float;															\
																						\
	varying vec2 v_tex_coord;															\
	uniform lowp vec4 draw_color;														\
	uniform lowp vec4 add_color;														\
																						\
	uniform sampler2D tex_sampler;														\
																						\
	void main(void) {	\
		gl_FragColor= draw_color*texture2D(tex_sampler, v_tex_coord.st) ;                 \
	}";

static char *fill_rect_fragment_shader_code = "											\
	precision mediump float;															\
																						\
	uniform lowp vec4 draw_color;														\
																						\
	void main(void) {																	\
		gl_FragColor = draw_color;														\
	}";


static char *drawing_vertex_shader_code = "												\
																						\
	attribute vec2 attr_vertex_coord;													\
																						\
	uniform mat4 proj_matrix;															\
	uniform float point_size;															\
																						\
	void main(void) {																	\
		gl_Position = proj_matrix * vec4(attr_vertex_coord, 0.0, 1.0);					\
		gl_PointSize = point_size;														\
	}																					\
";

static char *drawing_fragment_shader_code = "											\
	precision mediump float;															\
																						\
	uniform lowp vec4 draw_color;														\
																						\
	uniform sampler2D tex_sampler;														\
																						\
	void main(void) {																	\
		float alpha = texture2D(tex_sampler, gl_PointCoord).a;							\
		gl_FragColor = draw_color * alpha;												\
	}";


tealeaf_shader global_shaders[NUM_SHADERS];

unsigned int current_shader;

/**
 * @name	load_shader
 * @brief	creates and compiles a shader
 * @param	type - (int) type type of shader to compile (GL_VERTEX_SHADER | GL_FRAGMENT_SHADER)
 * @param	code - (char *) code to be loaded and compiled for the shader
 * @param   description - (const char *) debug description for the logs in case it fails
 * @retval	int -
 */
int load_shader(int type, const char *code, const char *description) {
    // create a vertex shader type (GLES20.GL_VERTEX_SHADER)
    // or a fragment shader type (GLES20.GL_FRAGMENT_SHADER)
    int shader = glCreateShader(type);
    // add the source code to the shader and compile it
    GLTRACE(glShaderSource(shader, 1, &code, NULL));
    GLTRACE(glCompileShader(shader));
    int bDidCompile;
    GLTRACE(glGetShaderiv(shader, GL_COMPILE_STATUS, &bDidCompile));

    if (!bDidCompile) {
        // Display shader program compile failure
        const char *stype = "unknown";
        switch (type) {
        case GL_VERTEX_SHADER:
            stype = "vertex";
            break;
        case GL_FRAGMENT_SHADER:
            stype = "fragment";
            break;
        }
        LOG("{shaders} WARNING: COMPILE FAILED for %s shader program '%s'", stype, description);
    }

    return shader;
}

/**
 * @name	tealeaf_shaders_load
 * @brief	creates the fragment / vertex shader with the given shader code, returning a shader program
 * @param	vertex_shader_code - (char *) vertex shader code
 * @param	fragment_shader_code - (char *) fragment shader code
 * @param   description - (const char *) debug description for the logs in case it fails
 * @retval	int - gl int of the shader program
 */
int tealeaf_shaders_load(char *vertex_shader_code, char *fragment_shader_code, const char *description) {
    int vertex_shader = load_shader(GL_VERTEX_SHADER, vertex_shader_code, description);
    int fragment_shader = load_shader(GL_FRAGMENT_SHADER, fragment_shader_code, description);
    int program = glCreateProgram();             // create empty OpenGL Program
    GLTRACE(glAttachShader(program, vertex_shader));   // add the vertex shader to program
    GLTRACE(glAttachShader(program, fragment_shader)); // add the fragment shader to program
    GLTRACE(glLinkProgram(program));                  // creates OpenGL program executables
    int linked;
    GLTRACE(glGetProgramiv(program, GL_LINK_STATUS, &linked));

    if (!linked) {
        LOG("{shaders} ERROR: LINK FAILED for shader program '%s'", description);

        int maxLength;
        GLTRACE(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength));
        maxLength = maxLength + 1;
        // TODO: this might be unicode?
        GLchar info_log[maxLength];
        GLTRACE(glGetProgramInfoLog(program, maxLength, &maxLength, info_log));
        LOG("%s", info_log);
        exit(1);
    } else {
        LOG("{shaders} Compiled and linked shader program '%s'", description);
    }

    return program;
}

/**
 * @name	tealeaf_shaders_primary_init
 * @brief	initilizes the primary texture drawing shader code and variables
 * @retval	NONE
 */
void tealeaf_shaders_primary_init() {
    tealeaf_shader *shader = &global_shaders[PRIMARY_SHADER];
    shader->program = tealeaf_shaders_load(vertex_shader_code, fragment_shader_code, "primary");
    GLTRACE(glUseProgram(shader->program));
    // texture binding -- always use texture 0
    shader->tex_sampler = glGetUniformLocation(shader->program, "tex_sampler");
    GLTRACE(glUniform1i(shader->tex_sampler, 0));
    // shader binding for projection matrix
    shader->proj_matrix = glGetUniformLocation(shader->program, "proj_matrix");
    // shader binding for vertex/texture coordinates
    shader->tex_coords = glGetAttribLocation(shader->program, "attr_tex_coord");
    shader->vertex_coords = glGetAttribLocation(shader->program, "attr_vertex_coord");
    shader->draw_color = glGetUniformLocation(shader->program, "draw_color");
}

/**
 * @name	tealeaf_shaders_linear_add_init
 * @brief	initilizes the linear add shader code and variables
 * @retval	NONE
 */
void tealeaf_shaders_linear_add_init() {
    tealeaf_shader *shader = &global_shaders[LINEAR_ADD_SHADER];
    shader->program = tealeaf_shaders_load(linear_add_vertex_shader_code, linear_add_fragment_shader_code, "linear add");
    GLTRACE(glUseProgram(shader->program));
    // texture binding -- always use texture 0
    shader->tex_sampler = glGetUniformLocation(shader->program, "tex_sampler");
    GLTRACE(glUniform1i(shader->tex_sampler, 0));
    // shader binding for projection matrix
    shader->proj_matrix = glGetUniformLocation(shader->program, "proj_matrix");
    // shader binding for vertex/texture coordinates
    shader->tex_coords = glGetAttribLocation(shader->program, "attr_tex_coord");
    shader->vertex_coords = glGetAttribLocation(shader->program, "attr_vertex_coord");
    shader->draw_color = glGetUniformLocation(shader->program, "draw_color");
    shader->add_color = glGetUniformLocation(shader->program, "add_color");
}

/**
 * @name	tealeaf_shaders_drawing_init
 * @brief	initilizes the point sprite shader code and variables
 * @retval	NONE
 */
void tealeaf_shaders_drawing_init() {
    tealeaf_shader *shader = &global_shaders[DRAWING_SHADER];
    shader->program = tealeaf_shaders_load(drawing_vertex_shader_code, drawing_fragment_shader_code, "drawing");
    GLTRACE(glUseProgram(shader->program));
    shader->tex_sampler = glGetUniformLocation(shader->program, "tex_sampler");
    GLTRACE(glUniform1i(shader->tex_sampler, 0));
    // shader binding for projection matrix
    shader->proj_matrix = glGetUniformLocation(shader->program, "proj_matrix");
    // shader binding for vertex/texture coordinates
    shader->vertex_coords = glGetAttribLocation(shader->program, "attr_vertex_coord");
    shader->draw_color = glGetUniformLocation(shader->program, "draw_color");
    shader->point_size = glGetUniformLocation(shader->program, "point_size");
}

/**
 * @name	tealeaf_shaders_fill_rect_init
 * @brief	initilizes the fill rect shader code and variables
 * @retval	NONE
 */
void tealeaf_shaders_fill_rect_init() {
    tealeaf_shader *shader = &global_shaders[FILL_RECT_SHADER];
    shader->program = tealeaf_shaders_load(vertex_shader_code, fill_rect_fragment_shader_code, "fill rect");
    GLTRACE(glUseProgram(shader->program));
    // shader binding for projection matrix
    shader->proj_matrix = glGetUniformLocation(shader->program, "proj_matrix");
    // shader binding for vertex/texture coordinates
    shader->vertex_coords = glGetAttribLocation(shader->program, "attr_vertex_coord");
    shader->draw_color = glGetUniformLocation(shader->program, "draw_color");
}

/**
 * @name	tealeaf_shaders_primary_bind
 * @brief	binds the primary shader's program / attributes
 * @retval	NONE
 */
static void inline tealeaf_shaders_primary_bind() {
    tealeaf_shader *shader = &global_shaders[PRIMARY_SHADER];
    GLTRACE(glUseProgram(shader->program));
    GLTRACE(glEnableVertexAttribArray(shader->vertex_coords));
    GLTRACE(glEnableVertexAttribArray(shader->tex_coords));
}

/**
 * @name	tealeaf_shaders_primary_unbind
 * @brief	unbinds the primary shader's program / attributes
 * @retval	NONE
 */
static void inline tealeaf_shaders_primary_unbind() {
    tealeaf_shader *shader = &global_shaders[PRIMARY_SHADER];
    GLTRACE(glDisableVertexAttribArray(shader->vertex_coords));
    GLTRACE(glDisableVertexAttribArray(shader->tex_coords));
}

/**
 * @name	tealeaf_shaders_fill_rect_bind
 * @brief	binds the filil rect shader's program / attributes
 * @retval	NONE
 */
static void inline tealeaf_shaders_fill_rect_bind() {
    tealeaf_shader *shader = &global_shaders[FILL_RECT_SHADER];
    GLTRACE(glUseProgram(shader->program));
    GLTRACE(glEnableVertexAttribArray(shader->vertex_coords));
}

/**
 * @name	tealeaf_shaders_fill_rect_unbind
 * @brief	unbinds the fill rect shader's program / attributes
 * @retval	NONE
 */
static void inline tealeaf_shaders_fill_rect_unbind() {
    tealeaf_shader *shader = &global_shaders[FILL_RECT_SHADER];
    GLTRACE(glDisableVertexAttribArray(shader->vertex_coords));
}

/**
 * @name	tealeaf_shaders_drawing_bind
 * @brief	binds the point sprite shader's program / attributes
 * @retval	NONE
 */
static void inline tealeaf_shaders_drawing_bind() {
    tealeaf_shader *shader = &global_shaders[DRAWING_SHADER];
    GLTRACE(glUseProgram(shader->program));
    GLTRACE(glEnableVertexAttribArray(shader->vertex_coords));
}

/**
 * @name	tealeaf_shaders_drawing_unbind
 * @brief	unbinds the point sprite shader's program / attributes
 * @retval	NONE
 */
static void inline tealeaf_shaders_drawing_unbind() {
    tealeaf_shader *shader = &global_shaders[DRAWING_SHADER];
    GLTRACE(glDisableVertexAttribArray(shader->vertex_coords));
}

/**
 * @name	tealeaf_shaders_linear_add_bind
 * @brief	binds the linear add shader's program / attributes
 * @retval	NONE
 */
static void inline tealeaf_shaders_linear_add_bind() {
    tealeaf_shader *shader = &global_shaders[LINEAR_ADD_SHADER];
    GLTRACE(glUseProgram(shader->program));
    GLTRACE(glEnableVertexAttribArray(shader->tex_coords));
    GLTRACE(glEnableVertexAttribArray(shader->vertex_coords));
}

/**
 * @name	tealeaf_shaders_linear_add_unbind
 * @brief	unbinds the linear add shader's program / attributes
 * @retval	NONE
 */
static void inline tealeaf_shaders_linear_add_unbind() {
    tealeaf_shader *shader = &global_shaders[LINEAR_ADD_SHADER];
    GLTRACE(glDisableVertexAttribArray(shader->vertex_coords));
    GLTRACE(glDisableVertexAttribArray(shader->tex_coords));
}

/**
 * @name	tealeaf_shaders_bind
 * @brief	unbinds the current shader and binds the given shader
 * @param	shader_type - (unsigned int) shader program to bind to gl
 * @retval	NONE
 */
void tealeaf_shaders_bind(unsigned int shader_type) {
    if (current_shader == shader_type) {
        return;
    }

    // unbind old shader
    if (current_shader == PRIMARY_SHADER) {
        tealeaf_shaders_primary_unbind();
    } else if (current_shader == DRAWING_SHADER) {
        tealeaf_shaders_drawing_unbind();
    } else if (current_shader == FILL_RECT_SHADER) {
        tealeaf_shaders_fill_rect_unbind();
    } else if (current_shader == LINEAR_ADD_SHADER) {
        tealeaf_shaders_linear_add_unbind();
    }

    // bind new shader
    if (shader_type == PRIMARY_SHADER) {
        tealeaf_shaders_primary_bind();
    } else if (shader_type == DRAWING_SHADER) {
        tealeaf_shaders_drawing_bind();
    } else if (shader_type == FILL_RECT_SHADER) {
        tealeaf_shaders_fill_rect_bind();
    } else if (shader_type == LINEAR_ADD_SHADER) {
        tealeaf_shaders_linear_add_bind();
    }

    current_shader = shader_type;

    tealeaf_context_update_shader(tealeaf_canvas_get()->active_ctx, shader_type, false);
}

/**
 * @name	tealeaf_shaders_init
 * @brief	initializes all shaders, binds the primary shader
 * @retval	NONE
 */
void tealeaf_shaders_init() {
    use_single_shader = false;
    tealeaf_shaders_primary_init();
    tealeaf_shaders_drawing_init();
    tealeaf_shaders_fill_rect_init();
    tealeaf_shaders_linear_add_init();
    tealeaf_shaders_primary_bind();
}

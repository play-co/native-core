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

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "core/types.h"

typedef struct matrix_t {
	float m00, m01, m02, m03,
	      m10, m11, m12, m13,
	      m20, m21, m22, m23,
	      m30, m31, m32, m33;
} matrix_4x4;


typedef struct rect_t {
	float x, y, width, height;
} rect_2d;

typedef struct rect_vertices_t {
	float x1, y1;
	float x2, y2;
	float x3, y3;
	float x4, y4;
} rect_2d_vertices;

__attribute__((unused)) static inline void rect_2d_to_rect_2d_vertices(const rect_2d *in, rect_2d_vertices *out) {
	out->x1 = in->x;
	out->y1 = in->y;
	out->x2 = in->x + in->width;
	out->y2 = in->y;
	out->x3 = in->x + in->width;
	out->y3 = in->y + in->height;
	out->x4 = in->x;
	out->y4 = in->y + in->height;
}

void matrix_4x4_add(matrix_4x4 *a, matrix_4x4 *b);
__attribute__((unused)) static void inline matrix_4x4_identity(matrix_4x4 *a) {
	float *A = (float *) a;
	A[0] = 1;
	A[1] = 0;
	A[2] = 0;
	A[3] = 0;
	A[4] = 0;
	A[5] = 1;
	A[6] = 0;
	A[7] = 0;
	A[8] = 0;
	A[9] = 0;
	A[10] = 1;
	A[11] = 0;
	A[12] = 0;
	A[13] = 0;
	A[14] = 0;
	A[15] = 1;
}
void matrix_4x4_ortho(matrix_4x4 *a, float left, float right, float top, float bottom, float zNear, float zFar);
void matrix_4x4_transpose(matrix_4x4 *a);
void matrix_4x4_copy(matrix_4x4 *src, matrix_4x4 *dest);
void matrix_4x4_rotate(matrix_4x4 *a, float angle, float x, float y, float z);
void matrix_4x4_translate(matrix_4x4 *a, float x, float y, float z);
void matrix_4x4_scale(matrix_4x4 *a, float x, float y, float z);

#define PRINT_MATRIX(A) LOG("%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n", (A)->m00, (A)->m01, (A)->m02, (A)->m03, (A)->m10, (A)->m11, (A)->m12, (A)->m13, (A)->m20, (A)->m21, (A)->m22, (A)->m23, (A)->m30, (A)->m31, (A)->m32, (A)->m33);

#define PP_NARG(...) \
	PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
	PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
                  _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
                  _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
                  _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
                  _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
                  _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
                  _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
                  _61,_62,_63,N,...) N
#define PP_RSEQ_N() \
	63,62,61,60,                   \
	59,58,57,56,55,54,53,52,51,50, \
	49,48,47,46,45,44,43,42,41,40, \
	39,38,37,36,35,34,33,32,31,30, \
	29,28,27,26,25,24,23,22,21,20, \
	19,18,17,16,15,14,13,12,11,10, \
	9,8,7,6,5,4,3,2,1,0


#define matrix_4x4_multiply(...) \
	matrix_4x4_multiply_(PP_NARG(__VA_ARGS__))(__VA_ARGS__)
#define matrix_4x4_multiply_(nargs) \
	matrix_4x4_multiply__(nargs)
#define matrix_4x4_multiply__(nargs) \
	matrix_4x4_multiply ## nargs

#define matrix_4x4_multiply5(m, x, y, x2, y2) \
	matrix_4x4_multiply_m_f_f_f_f(m, x, y, x2, y2)

#define matrix_4x4_multiply3(a, b, dest) \
	maxtrix_4x4_multiply_m_m_m(a, b, dest)

#define matrix_4x4_multiply10(a, r, rx1, ry1, rx2, ry2, rx3, ry3, rx4, ry5) \
	matrix_4x4_multiply_m_r_f_f_f_f_f_f_f_f(a, r, rx1, ry1, rx2, ry2, rx3, ry3, rx4, ry5)

//
void matrix_4x4_multiply_m_m_m(matrix_4x4 *a, matrix_4x4 *b, matrix_4x4 *dest);
void matrix_4x4_multiply_m_f_f_f_f(const matrix_4x4 *a, float x, float y, float *x2, float *y2);

__attribute__((unused))static inline void matrix_4x4_multiply_m_r_f_f_f_f_f_f_f_f(const matrix_4x4 *a, const rect_2d *rect,
        float *rx1, float *ry1,
        float *rx2, float *ry2,
        float *rx3, float *ry3,
        float *rx4, float *ry4) {
	matrix_4x4_multiply(a, rect->x, rect->y, rx1, ry1);
	matrix_4x4_multiply(a, rect->x + rect->width, rect->y, rx2, ry2);
	matrix_4x4_multiply(a, rect->x + rect->width, rect->y + rect->height, rx3, ry3);
	matrix_4x4_multiply(a, rect->x, rect->y + rect->height, rx4, ry4);
}

__attribute__((unused)) static inline void matrix_4x4_multiply_m_r_r(const matrix_4x4 *matrix, const rect_2d_vertices *in, rect_2d_vertices *out) {
	matrix_4x4_multiply(matrix, in->x1, in->y1, &out->x1, &out->y1);
	matrix_4x4_multiply(matrix, in->x2, in->y2, &out->x2, &out->y2);
	matrix_4x4_multiply(matrix, in->x3, in->y3, &out->x4, &out->y4);
	matrix_4x4_multiply(matrix, in->x4, in->y4, &out->x3, &out->y3);
}

__attribute__((unused)) static inline bool rect_2d_equals(const rect_2d *a, const rect_2d *b) {
	return a->x == b->x && a->y == b->y && a->width == b->width && a->height == b->height;
}

//3x3 matrix functions

//#define MATRIX_3x3_ALLOW_SKEW

#define matrix_3x3_multiply(...) \
    matrix_3x3_multiply_(PP_NARG(__VA_ARGS__))(__VA_ARGS__)
#define matrix_3x3_multiply_(nargs) \
    matrix_3x3_multiply__(nargs)
#define matrix_3x3_multiply__(nargs) \
    matrix_3x3_multiply ## nargs

#define matrix_3x3_multiply5(m, x, y, x2, y2) \
    matrix_3x3_multiply_m_f_f_f_f(m, x, y, x2, y2)

#define matrix_3x3_multiply3(a, b, dest) \
    maxtrix_3x3_multiply_m_m_m(a, b, dest)

#define matrix_3x3_multiply10(a, r, rx1, ry1, rx2, ry2, rx3, ry3, rx4, ry5) \
    matrix_3x3_multiply_m_r_f_f_f_f_f_f_f_f(a, r, rx1, ry1, rx2, ry2, rx3, ry3, rx4, ry5)

typedef struct matrix_3x3_t {
	float m00, m01, m02,
    m10, m11, m12,
    m20, m21, m22;
} matrix_3x3;
void matrix_3x3_ortho(matrix_3x3 *a, float left, float right, float top, float bottom);
void matrix_3x3_transpose(matrix_3x3 *a);
void matrix_3x3_rotate(matrix_3x3 *a, float angle);
void matrix_3x3_translate(matrix_3x3 *a, float x, float y);
void matrix_3x3_scale(matrix_3x3 *a, float x, float y);

__attribute__((unused)) static void inline matrix_3x3_identity(matrix_3x3 *a) {
	a->m00 = 1;
	a->m01 = 0;
	a->m02 = 0;
	a->m10 = 0;
	a->m11 = 1;
	a->m12 = 0;
	a->m20 = 0;
	a->m21 = 0;
	a->m22 = 1;
}

void matrix_3x3_multiply_m_f_f_f_f(const matrix_3x3 *a, float x, float y, float *x2, float *y2);

__attribute__((unused))static inline void matrix_3x3_multiply_m_r_f_f_f_f_f_f_f_f(const matrix_3x3 *a, const rect_2d *rect, float *rx1, float *ry1,float *rx2, float *ry2,float *rx3, float *ry3,float *rx4, float *ry4) {
#ifdef MATRIX_3x3_ALLOW_SKEW
	matrix_3x3_multiply(a, rect->x, rect->y, rx1, ry1);
	matrix_3x3_multiply(a, rect->x + rect->width, rect->y, rx2, ry2);
	matrix_3x3_multiply(a, rect->x + rect->width, rect->y + rect->height, rx3, ry3);
	matrix_3x3_multiply(a, rect->x, rect->y + rect->height, rx4, ry4);
#else
	//compute the location of the top left point of the rectangle
	matrix_3x3_multiply(a, rect->x, rect->y, rx1, ry1);

	//rotate the height vector to be added back later
	*rx4 = a->m01 * rect->height;
	*ry4 = a->m11 * rect->height;

	//compute the location of the top right point of the rectangle
	*rx2 = *rx1 + a->m00 * rect->width;
	*ry2 = *ry1 + a->m10 * rect->width;
	
	//compute the location of the bottom right point of the rectangle
	*rx3 = *rx2 + *rx4;
	*ry3 = *ry2 + *ry4;
	
	//compute the location of the bottom left point of the rectangle
	*rx4 += *rx1;
	*ry4 += *ry1;
#endif
}

__attribute__((unused)) static inline void matrix_3x3_multiply_m_r_r(const matrix_3x3 *matrix, const rect_2d_vertices *in, rect_2d_vertices *out) {
	matrix_3x3_multiply(matrix, in->x1, in->y1, &out->x1, &out->y1);
	matrix_3x3_multiply(matrix, in->x2, in->y2, &out->x2, &out->y2);
	matrix_3x3_multiply(matrix, in->x3, in->y3, &out->x4, &out->y4);
	matrix_3x3_multiply(matrix, in->x4, in->y4, &out->x3, &out->y3);
}


#endif // MATRIX_H

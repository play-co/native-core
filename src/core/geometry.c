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
 * @file	 geometry.c
 * @brief
 */
#include "geometry.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>

#define FSWAP(temp, a, b) temp = a; a = b; b = temp;
static float epsilon = 1e-6;
#define FLOAT_EQUAL(x, y) (x > y - epsilon && x < y + epsilon)

//3x3 Matrix Functions

void inline matrix_3x3_transpose(matrix_3x3 *a) {
    float t;
    FSWAP(t, a->m10, a->m01);
    FSWAP(t, a->m20, a->m02);
    FSWAP(t, a->m12, a->m21);
}

//Create a 3x3 orthogonal projection matrix
void matrix_3x3_ortho(matrix_3x3 *a, float left, float right, float top, float bottom) {
    float *A = (float *) a;
    float rl = (right - left);
    float tb = (top - bottom);
    A[0] = 2 / rl;
    A[1] = 0;
    A[2] = -(right + left) / rl;
    A[3] = 0;
    A[4] = 2 / tb;
    A[5] = -(top + bottom) / tb;
    A[6] = 0;
    A[7] = 0;
    A[8] = 1;

}

//Rotate matrix a by the given angle
void matrix_3x3_rotate(matrix_3x3 *a, float angle) {
    //2x2 rotation matrix formed by angle
    float R_00 = cos(angle);
    float R_10 = sin(angle);
    float R_01 = -R_10;//-sin(angle);
    float R_11 = R_00;//cos(angle);

    //temporary variables to hold values in the matrix when doing
    //the multiplications
    float t0,t1;

    //These multiplications apply R to the rotation matrix inside a
    t0 = a->m00;
    t1 = a->m01;
    a->m00 = R_00 * t0 + R_10 * t1;
    a->m01 = R_01 * t0 + R_11 * t1;

    t0 = a->m10;
    t1 = a->m11;
    a->m10 = R_00 * t0 + R_10 * t1;
    a->m11 = R_01 * t0 + R_11 * t1;

#ifdef MATRIX_3x3_ALLOW_SKEW
    //These multiplications apply R to the skew components at the bottom
    //off the matrix
    t0 = a->m20;
    t1 = a->m21;
    a->m20 = R_00 * t0 + R_10 * t1;
    a->m21 = R_01 * t0 + R_11 * t1;
#endif
}

//Translate the matrix a by x, y
void matrix_3x3_translate(matrix_3x3 *a, float x, float y) {
    //Add x,y rotated to the current translation components
    a->m02 += x * a->m00 + y * a->m01;
    a->m12 += x * a->m10 + y * a->m11;

#ifdef MATRIX_3x3_ALLOW_SKEW
    //If skew is allowed add to those components as well
    a->m22 += x * a->m20 + y * a->m21;
#endif
}

//Scale the matrix a by x and y
void matrix_3x3_scale(matrix_3x3 *a, float x, float y) {
    //multiply the first column by x
    a->m00 *= x;
    a->m10 *= x;

    //multiply the second column by y
    a->m01 *= y;
    a->m11 *= y;

#ifdef MATRIX_3x3_ALLOW_SKEW
    //multiply the skew components if skewing is allowed
    a->m20 *= x;
    a->m21 *= y;
#endif
}

//Multiply the points x,y by the matrix a and output their values in x2 and y2
void matrix_3x3_multiply_m_f_f_f_f(const matrix_3x3 *a, float x, float y, float *x2, float *y2) {

#ifdef MATRIX_3x3_ALLOW_SKEW
    //if skewing is allowed make sure to devide by the w component
    *x2 = x * a->m00 + y * a->m01 + a->m02;
    *y2 = x * a->m10 + y * a->m11 + a->m12;
    float w = x * a->m20 + y * a->m21 + a->m22;

    if (!FLOAT_EQUAL(w, 1)) {
        *x2 /= w;
        *y2 /= w;
    }

#else
    //if there is no skewing just multiply x, y by the rotation matrix in a
    *x2 = x * a->m00 + y * a->m01 + a->m02;
    *y2 = x * a->m10 + y * a->m11 + a->m12;
#endif
}

//4x4 Matrix Functions

#define O(y,x) (y + (x<<2))

/**
 * @name	matrix_4x4_multiply_f_f_f
 * @brief	multiplies two matrices together (represented as arrays of floats), storing the answer in dest
 * @param	src1 - (const float *__restrict__) first matrix to multiply
 * @param	src2 - (const float *__restrict__) second matrix to multiply
 * @param	dest - (float *__restrict__) destination of the mulitplied matrcies
 * @retval	NONE
 */
static void matrix_4x4_multiply_f_f_f(const float *__restrict__ src1, const float *__restrict__ src2, float *__restrict__ dest) {
    *(dest + O(0, 0)) = (*(src1 + O(0, 0)) **(src2 + O(0, 0))) + (*(src1 + O(0, 1)) **(src2 + O(1, 0))) + (*(src1 + O(0, 2)) **(src2 + O(2, 0))) + (*(src1 + O(0, 3)) **(src2 + O(3, 0)));
    *(dest + O(0, 1)) = (*(src1 + O(0, 0)) **(src2 + O(0, 1))) + (*(src1 + O(0, 1)) **(src2 + O(1, 1))) + (*(src1 + O(0, 2)) **(src2 + O(2, 1))) + (*(src1 + O(0, 3)) **(src2 + O(3, 1)));
    *(dest + O(0, 2)) = (*(src1 + O(0, 0)) **(src2 + O(0, 2))) + (*(src1 + O(0, 1)) **(src2 + O(1, 2))) + (*(src1 + O(0, 2)) **(src2 + O(2, 2))) + (*(src1 + O(0, 3)) **(src2 + O(3, 2)));
    *(dest + O(0, 3)) = (*(src1 + O(0, 0)) **(src2 + O(0, 3))) + (*(src1 + O(0, 1)) **(src2 + O(1, 3))) + (*(src1 + O(0, 2)) **(src2 + O(2, 3))) + (*(src1 + O(0, 3)) **(src2 + O(3, 3)));
    *(dest + O(1, 0)) = (*(src1 + O(1, 0)) **(src2 + O(0, 0))) + (*(src1 + O(1, 1)) **(src2 + O(1, 0))) + (*(src1 + O(1, 2)) **(src2 + O(2, 0))) + (*(src1 + O(1, 3)) **(src2 + O(3, 0)));
    *(dest + O(1, 1)) = (*(src1 + O(1, 0)) **(src2 + O(0, 1))) + (*(src1 + O(1, 1)) **(src2 + O(1, 1))) + (*(src1 + O(1, 2)) **(src2 + O(2, 1))) + (*(src1 + O(1, 3)) **(src2 + O(3, 1)));
    *(dest + O(1, 2)) = (*(src1 + O(1, 0)) **(src2 + O(0, 2))) + (*(src1 + O(1, 1)) **(src2 + O(1, 2))) + (*(src1 + O(1, 2)) **(src2 + O(2, 2))) + (*(src1 + O(1, 3)) **(src2 + O(3, 2)));
    *(dest + O(1, 3)) = (*(src1 + O(1, 0)) **(src2 + O(0, 3))) + (*(src1 + O(1, 1)) **(src2 + O(1, 3))) + (*(src1 + O(1, 2)) **(src2 + O(2, 3))) + (*(src1 + O(1, 3)) **(src2 + O(3, 3)));
    *(dest + O(2, 0)) = (*(src1 + O(2, 0)) **(src2 + O(0, 0))) + (*(src1 + O(2, 1)) **(src2 + O(1, 0))) + (*(src1 + O(2, 2)) **(src2 + O(2, 0))) + (*(src1 + O(2, 3)) **(src2 + O(3, 0)));
    *(dest + O(2, 1)) = (*(src1 + O(2, 0)) **(src2 + O(0, 1))) + (*(src1 + O(2, 1)) **(src2 + O(1, 1))) + (*(src1 + O(2, 2)) **(src2 + O(2, 1))) + (*(src1 + O(2, 3)) **(src2 + O(3, 1)));
    *(dest + O(2, 2)) = (*(src1 + O(2, 0)) **(src2 + O(0, 2))) + (*(src1 + O(2, 1)) **(src2 + O(1, 2))) + (*(src1 + O(2, 2)) **(src2 + O(2, 2))) + (*(src1 + O(2, 3)) **(src2 + O(3, 2)));
    *(dest + O(2, 3)) = (*(src1 + O(2, 0)) **(src2 + O(0, 3))) + (*(src1 + O(2, 1)) **(src2 + O(1, 3))) + (*(src1 + O(2, 2)) **(src2 + O(2, 3))) + (*(src1 + O(2, 3)) **(src2 + O(3, 3)));
    *(dest + O(3, 0)) = (*(src1 + O(3, 0)) **(src2 + O(0, 0))) + (*(src1 + O(3, 1)) **(src2 + O(1, 0))) + (*(src1 + O(3, 2)) **(src2 + O(2, 0))) + (*(src1 + O(3, 3)) **(src2 + O(3, 0)));
    *(dest + O(3, 1)) = (*(src1 + O(3, 0)) **(src2 + O(0, 1))) + (*(src1 + O(3, 1)) **(src2 + O(1, 1))) + (*(src1 + O(3, 2)) **(src2 + O(2, 1))) + (*(src1 + O(3, 3)) **(src2 + O(3, 1)));
    *(dest + O(3, 2)) = (*(src1 + O(3, 0)) **(src2 + O(0, 2))) + (*(src1 + O(3, 1)) **(src2 + O(1, 2))) + (*(src1 + O(3, 2)) **(src2 + O(2, 2))) + (*(src1 + O(3, 3)) **(src2 + O(3, 2)));
    *(dest + O(3, 3)) = (*(src1 + O(3, 0)) **(src2 + O(0, 3))) + (*(src1 + O(3, 1)) **(src2 + O(1, 3))) + (*(src1 + O(3, 2)) **(src2 + O(2, 3))) + (*(src1 + O(3, 3)) **(src2 + O(3, 3)));
};

/**
 * @name	matrix_4x4_multiply_m_m_m
 * @brief	multiples two matrices together, storing the answer in dest
 * @param	a - (matrix_4x4 *) first matrix to multiply
 * @param	b - (matrix_4x4 *) second matrix to multiply
 * @param	dest - (matrix_4x4 *) destination of the multiplied matrices
 * @retval	NONE
 */
void matrix_4x4_multiply_m_m_m(matrix_4x4 *a, matrix_4x4 *b, matrix_4x4 *dest) {
    matrix_4x4_multiply_f_f_f((float *) a, (float *) b, (float *) dest);
}

/**
 * @name	matrix_4x4_add
 * @brief	adds matrix b to matrix a
 * @param	a - (matrix_4x4 *) matrix being added to
 * @param	b - (matrix_4x4 *) matrix being added in
 * @retval	NONE
 */
void inline matrix_4x4_add(matrix_4x4 *a, matrix_4x4 *b) {
    float *A = (float *) a;
    float *B = (float *) b;
    A[0] += B[0];
    A[1] += B[1];
    A[2] += B[2];
    A[3] += B[3];
    A[4] += B[4];
    A[5] += B[5];
    A[6] += B[6];
    A[7] += B[7];
    A[8] += B[8];
    A[9] += B[9];
    A[10] += B[10];
    A[11] += B[11];
    A[12] += B[12];
    A[13] += B[13];
    A[14] += B[14];
    A[15] += B[15];
}

/**
 * @name	matrix_4x4_transpose
 * @brief	transposes the given matrix
 * @param	a - (matrix_4x4 *) matrix to be transposed
 * @retval	NONE
 */
void inline matrix_4x4_transpose(matrix_4x4 *a) {
    float t;
    FSWAP(t, a->m10, a->m01);
    FSWAP(t, a->m20, a->m02);
    FSWAP(t, a->m30, a->m03);
    FSWAP(t, a->m12, a->m21);
    FSWAP(t, a->m13, a->m31);
    FSWAP(t, a->m23, a->m32);
}

/**
 * @name	matrix_4x4_ortho
 * @brief	creates a parallel projection of the given matrix
 * @param	a - (matrix_4x4 *) matrix to be projected
 * @param	left - (float) left coordinate of the vertical clipping plane
 * @param	right - (float) right coordinate of the vertical clipping plane
 * @param	top - (float) top coordinate of the horizontal clipping plane
 * @param	bottom - (float) bottom coordinate of the horizontal clipping plane
 * @param	zNear - (float) distance to the near clipping plane
 * @param	zFar - (float) distance to the far clipping plane
 * @retval	NONE
 */
void inline matrix_4x4_ortho(matrix_4x4 *a, float left, float right, float top, float bottom, float zNear, float zFar) {
    /*
    *       2
    * ------------       0              0              tx
    * right - left
    *
    *                    2
    *     0         ------------        0              ty
    *               top - bottom
    *
    *                                   -2
    *     0              0         ------------        tz
    *                               zFar-zNear
    *
    *     0              0              0              1
    *
    * where
    *
    *   tx = - (right + left) / (right - left)
    *
    *   ty = - (top + bottom) / (top - bottom)
    *
    *   tz = - (zFar + zNear) / (zFar - zNear)
    */
    float *A = (float *) a;
    float rl = (right - left);
    float tb = (top - bottom);
    float fn = (zFar - zNear);
    A[0] = 2 / rl;
    A[1] = 0;
    A[2]  = 0;
    A[3] = -(right + left) / rl;
    A[4] = 0;
    A[5] = 2 / tb;
    A[6]  = 0;
    A[7] = -(top + bottom) / tb;
    A[8] = 0;
    A[9] = 0;
    A[10] = -2 / fn;
    A[11] = -(zFar + zNear) / fn;
    A[12] = 0;
    A[13] = 0;
    A[14] = 0;
    A[15] = 1;
}


/**
 * @name	matrix_4x4_copy
 * @brief	copies the source matrix into the destination matrix
 * @param	src - (matrix_4x4 *) matrix to be copied
 * @param	dest - (matrix_4x4 *) matrix to be copied into
 * @retval	NONE
 */
inline void matrix_4x4_copy(matrix_4x4 *src, matrix_4x4 *dest) {
    memcpy(dest, src, sizeof(matrix_4x4));
}

/**
 * @name	matrix_4x4_rotate
 * @brief	rotates the given matrix about the given angle and x, y, z
 * @param	a - (matrix_4x4 *) matrix to rotate
 * @param	angle - (float) angle to rotate by
 * @param	x - (float) x position of the point to rotate about
 * @param	y - (float) y position of the point to rotate about
 * @param	z - (float) z position of the point to rotate about
 * @retval	NONE
 */
void matrix_4x4_rotate(matrix_4x4 *a, float angle, float x, float y, float z) {
    float mag = x * x + y * y + z * z;

    if (!FLOAT_EQUAL(mag, 1.0f)) {
        mag = sqrt(mag);
        x /= mag;
        y /= mag;
        z /= mag;
    }

    float c = cos(angle);
    float c2 = 1 - c;
    float s = sin(angle);
    matrix_4x4 result, rotation;
    float *R = (float *) &rotation;
    R[0] = x * x * c2 + c;
    R[1] = x * y * c2 - z * s;
    R[2] = x * z * c2 + y * s;
    R[3] = 0;
    R[4] = y * x * c2 + z * s;
    R[5] = y * y * c2 + c;
    R[6] = y * z * c2 - x * s;
    R[7] = 0;
    R[8] = x * z * c2 - y * s;
    R[9] = y * z * c2 + x * s;
    R[10] = z * z * c2 + c;
    R[11] = 0;
    R[12] = 0;
    R[13] = 0;
    R[14] = 0;
    R[15] = 1;
    matrix_4x4_multiply_f_f_f(R, (float *)a, (float *)&result);
    matrix_4x4_copy(&result, a);
    // c = cos (angle), s = sin (angle), and ||(x, y, z)|| = 1
    /* x^2(1-c)+c     xy(1-c)-zs     xz(1-c)+ys     0
    * yx(1-c)+zs     y^2(1-c)+c     yz(1-c)-xs     0
    * xz(1-c)-ys     yz(1-c)+xs     z^2(1-c)+c     0
    *      0              0               0        1
    */
}



/**
 * @name	matrix_4x4_translate
 * @brief	translates the given matrix by x, y, z
 * @param	a - (matrix_4x4 *) matrix to translate
 * @param	x - (float) amount to translate by in the x direction
 * @param	y - (float) amount to translate by in the y direction
 * @param	z - (float) amount to translate by in the z direction
 * @retval	NONE
 */
void matrix_4x4_translate(matrix_4x4 *a, float x, float y, float z) {
    matrix_4x4 translation, result;
    float *T = (float *) &translation;
    matrix_4x4_identity(&translation);
    T[3] = x;
    T[7] = y;
    T[11] = z;
    matrix_4x4_multiply_f_f_f(T, (float *)a, (float *)&result);
    matrix_4x4_copy(&result, a);
}


/**
 * @name	matrix_4x4_scale
 * @brief	scales the given matrix by x, y, z
 * @param	a - (matrix_4x4 *) matrix to scale
 * @param	x - (float) amount to scale by in the x direction
 * @param	y - (float) amount to scale by in the y direction
 * @param	z - (float) amount to scale by in the z direction
 * @retval	NONE
 */
void matrix_4x4_scale(matrix_4x4 *a, float x, float y, float z) {
    matrix_4x4 scale, result;
    matrix_4x4_identity(&scale);
    scale.m00 = x;
    scale.m11 = y;
    scale.m22 = z;
    scale.m33 = 1;
    matrix_4x4_multiply_f_f_f((float *)&scale, (float *)a, (float *)&result);
    matrix_4x4_copy(&result, a);
}


/**
 * @name	matrix_4x4_multiply_m_f_f_f_f
 * @brief	multiplies coordinates x and y by the given matrix
 * @param	a - (const matrix_4x4 *) matrix to multiply x and y by
 * @param	x - (float) x coordinate to be multiplied by matrix a
 * @param	y - (float) y coordinate to be multiplied by matrix a
 * @param	x2 - (float *) pointer where the post multiplied x coordinate will be stored
 * @param	y2 - (float *) pointer where the post multiplied y coordinate will be stored
 * @retval	NONE
 */
void matrix_4x4_multiply_m_f_f_f_f(const matrix_4x4 *a, float x, float y, float *x2, float *y2) {
    *x2 = x * a->m00 + y * a->m01 + 0 * a->m02 + 1 * a->m03;
    *y2 = x * a->m10 + y * a->m11 + 0 * a->m12 + 1 * a->m13;
    float w = x * a->m30 + y * a->m31 + 0 * a->m32 + 1 * a->m33;

    if (!FLOAT_EQUAL(w, 1)) {
        *x2 /= w;
        *y2 /= w;
    }
}


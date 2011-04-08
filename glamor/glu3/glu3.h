/*
 * Copyright © 2009 Ian D. Romanick
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef __glu3_h__
#define __glu3_h__

#include <GL/gl.h>

#define GLU_VERSION_3_0

struct GLUmat4;

struct GLUvec4 {
	GLfloat values[4];

#ifdef __cplusplus
	inline GLUvec4(void)
	{
	}

	inline GLUvec4(GLfloat x , GLfloat y, GLfloat z, GLfloat w)
	{
		values[0] = x;
		values[1] = y;
		values[2] = z;
		values[3] = w;
	}

	inline GLUvec4(const GLUvec4 &v)
	{
		values[0] = v.values[0];
		values[1] = v.values[1];
		values[2] = v.values[2];
		values[3] = v.values[3];
	}

	GLUvec4 operator *(const GLUmat4 &) const;
	GLUvec4 operator *(const GLUvec4 &) const;
	GLUvec4 operator *(GLfloat) const;

	GLUvec4 operator +(const GLUvec4 &) const;
	GLUvec4 operator -(const GLUvec4 &) const;
#endif /* __cplusplus */
};


struct GLUmat4 {
	struct GLUvec4 col[4];

#ifdef __cplusplus
	inline GLUmat4(void)
	{
	}

	inline GLUmat4(const GLUvec4 & c0, const GLUvec4 & c1,
		       const GLUvec4 & c2, const GLUvec4 & c3)
	{
		col[0] = c0;
		col[1] = c1;
		col[2] = c2;
		col[3] = c3;
	}

	inline GLUmat4(const GLUmat4 &m)
	{
		col[0] = m.col[0];
		col[1] = m.col[1];
		col[2] = m.col[2];
		col[3] = m.col[3];
	}


	GLUvec4 operator *(const GLUvec4 &) const;
	GLUmat4 operator *(const GLUmat4 &) const;
	GLUmat4 operator *(GLfloat) const;

	GLUmat4 operator +(const GLUmat4 &) const;
	GLUmat4 operator -(const GLUmat4 &) const;
#endif	/* __cplusplus */
};

#define GLU_MAX_STACK_DEPTH 32

struct GLUmat4Stack {
	struct GLUmat4 stack[GLU_MAX_STACK_DEPTH];
	unsigned top;

#ifdef __cplusplus
	GLUmat4Stack() : top(0)
	{
		/* empty */
	}
#endif	/* __cplusplus */
};

#ifndef __cplusplus
typedef struct GLUvec4 GLUvec4;
typedef struct GLUmat4 GLUmat4;
typedef struct GLUmat4Stack GLUmat4Stack;
#endif /*  __cplusplus */


#ifdef __cplusplus
extern "C" {
#endif
#if 0
GLfloat gluDot4_4v(const GLUvec4 *, const GLUvec4 *);
GLfloat gluDot3_4v(const GLUvec4 *, const GLUvec4 *);
GLfloat gluDot2_4v(const GLUvec4 *, const GLUvec4 *);

void gluCross4v(GLUvec4 *result, const GLUvec4 *, const GLUvec4 *);
void gluNormalize4v(GLUvec4 *result, const GLUvec4 *);
GLfloat gluLength4v(const GLUvec4 *);
GLfloat gluLengthSqr4v(const GLUvec4 *);
void gluOuter4v(GLUmat4 *result, const GLUvec4 *, const GLUvec4 *);


void gluMult4v_4v(GLUvec4 *result, const GLUvec4 *, const GLUvec4 *);
void gluDiv4v_4v(GLUvec4 *result, const GLUvec4 *, const GLUvec4 *);
void gluAdd4v_4v(GLUvec4 *result, const GLUvec4 *, const GLUvec4 *);
void gluSub4v_4v(GLUvec4 *result, const GLUvec4 *, const GLUvec4 *);

void gluMult4v_f(GLUvec4 *result, const GLUvec4 *, GLfloat);
void gluDiv4v_f(GLUvec4 *result, const GLUvec4 *, GLfloat);
void gluAdd4v_f(GLUvec4 *result, const GLUvec4 *, GLfloat);
void gluSub4v_f(GLUvec4 *result, const GLUvec4 *, GLfloat);

void gluMult4m_4m(GLUmat4 *result, const GLUmat4 *, const GLUmat4 *);
void gluAdd4m_4m(GLUmat4 *result, const GLUmat4 *, const GLUmat4 *);
void gluSub4m_4m(GLUmat4 *result, const GLUmat4 *, const GLUmat4 *);
void gluMult4m_4v(GLUvec4 *result, const GLUmat4 *m, const GLUvec4 *v);

void gluMult4m_f(GLUmat4 *result, const GLUmat4 *, GLfloat);

void gluScale4v(GLUmat4 *result, const GLUvec4 *);
void gluTranslate3f(GLUmat4 *result, GLfloat x, GLfloat y, GLfloat z);
void gluTranslate4v(GLUmat4 *result, const GLUvec4 *);
void gluRotate4v(GLUmat4 *result, const GLUvec4 *axis, GLfloat angle);
void gluLookAt4v(GLUmat4 *result, const GLUvec4 *eye, const GLUvec4 *center,
		 const GLUvec4 *up);
void gluPerspective4f(GLUmat4 *result, GLfloat fovy, GLfloat aspect,
		      GLfloat near, GLfloat far);
void gluTranspose4m(GLUmat4 *result, const GLUmat4 *m);
void gluFrustum6f(GLUmat4 *result,
		  GLfloat left, GLfloat right,
		  GLfloat bottom, GLfloat top,
		  GLfloat near, GLfloat far);
void gluOrtho6f(GLUmat4 *result,
		GLfloat left, GLfloat right,
		GLfloat bottom, GLfloat top,
		GLfloat near, GLfloat far);
#endif
extern const GLUmat4 gluIdentityMatrix;

#ifdef __cplusplus
};
#endif

#ifdef __cplusplus
GLfloat gluDot4(const GLUvec4 &, const GLUvec4 &);
GLfloat gluDot3(const GLUvec4 &, const GLUvec4 &);
GLfloat gluDot2(const GLUvec4 &, const GLUvec4 &);

GLUvec4 gluCross(const GLUvec4 &, const GLUvec4 &);
GLUvec4 gluNormalize(const GLUvec4 &);
GLfloat gluLength(const GLUvec4 &);
GLfloat gluLengthSqr(const GLUvec4 &);
#endif /* __cplusplus */

#include "glu3_scalar.h"

#endif /* __glu3_h__ */

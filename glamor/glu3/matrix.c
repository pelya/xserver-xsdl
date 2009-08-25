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

#include <string.h>
#include "glu3.h"

#define DEG2RAD(d) ((d) * M_PI / 180.0)

const GLUmat4 gluIdentityMatrix = {
	{
		{ { 1.0f, 0.0f,  0.0f,  0.0f } },
		{ { 0.0f, 1.0f,  0.0f,  0.0f } },
		{ { 0.0f, 0.0f,  1.0f,  0.0f } },
		{ { 0.0f, 0.0f,  0.0f,  1.0f } }
	}
};


void gluTranslate4v(GLUmat4 *result, const GLUvec4 *t)
{
	memcpy(result, & gluIdentityMatrix, sizeof(gluIdentityMatrix));
	result->col[3] = *t;
	result->col[3].values[3] = 1.0f;
}


void gluScale4v(GLUmat4 *result, const GLUvec4 *t)
{
	memcpy(result, & gluIdentityMatrix, sizeof(gluIdentityMatrix));
	result->col[0].values[0] = t->values[0];
	result->col[1].values[1] = t->values[1];
	result->col[2].values[2] = t->values[2];
}


void gluLookAt4v(GLUmat4 *result,
		 const GLUvec4 *_eye,
		 const GLUvec4 *_center,
		 const GLUvec4 *_up)
{
	static const GLUvec4 col3 = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	const GLUvec4 e = { 
		{ -_eye->values[0], -_eye->values[1], -_eye->values[2], 0.0f }
	};
	GLUmat4  translate;
	GLUmat4  rotate;
	GLUmat4  rotateT;
	GLUvec4  f;
	GLUvec4  s;
	GLUvec4  u;
	GLUvec4  center, up;

	center = *_center;
	center.values[3] = 0;
	up = *_up;
	up.values[3] = 0;

	gluAdd4v_4v(& f, &center, &e);
	gluNormalize4v(& f, & f);

	gluNormalize4v(& u, &up);

	gluCross4v(& s, & f, & u);
	gluCross4v(& u, & s, & f);

	rotate.col[0] = s;
	rotate.col[1] = u;
	rotate.col[2].values[0] = -f.values[0];
	rotate.col[2].values[1] = -f.values[1];
	rotate.col[2].values[2] = -f.values[2];
	rotate.col[2].values[3] = 0.0f;
	rotate.col[3] = col3;
	gluTranspose4m(& rotateT, & rotate);

	gluTranslate4v(& translate, & e);
	gluMult4m_4m(result, & rotateT, & translate);
}


void gluRotate4v(GLUmat4 *result, const GLUvec4 *_axis, GLfloat angle)
{
	GLUvec4 axis;
	const float c = cos(angle);
	const float s = sin(angle);
	const float one_c = 1.0 - c;

	float xx;
	float yy;
	float zz;

	float xs;
	float ys;
	float zs;

	float xy;
	float xz;
	float yz;

	/* Only normalize the 3-component axis.  A gluNormalize3v might be
	 * appropriate to save us some computation.
	 */
	axis = *_axis;
	axis.values[3] = 0;
	gluNormalize4v(&axis, &axis);

	xx = axis.values[0] * axis.values[0];
	yy = axis.values[1] * axis.values[1];
	zz = axis.values[2] * axis.values[2];

	xs = axis.values[0] * s;
	ys = axis.values[1] * s;
	zs = axis.values[2] * s;

	xy = axis.values[0] * axis.values[1];
	xz = axis.values[0] * axis.values[2];
	yz = axis.values[1] * axis.values[2];


	result->col[0].values[0] = (one_c * xx) + c;
	result->col[0].values[1] = (one_c * xy) + zs;
	result->col[0].values[2] = (one_c * xz) - ys;
	result->col[0].values[3] = 0.0;

	result->col[1].values[0] = (one_c * xy) - zs;
	result->col[1].values[1] = (one_c * yy) + c;
	result->col[1].values[2] = (one_c * yz) + xs;
	result->col[1].values[3] = 0.0;


	result->col[2].values[0] = (one_c * xz) + ys;
	result->col[2].values[1] = (one_c * yz) - xs;
	result->col[2].values[2] = (one_c * zz) + c;
	result->col[2].values[3] = 0.0;

	result->col[3].values[0] = 0.0;
	result->col[3].values[1] = 0.0;
	result->col[3].values[2] = 0.0;
	result->col[3].values[3] = 1.0;
}


void
gluPerspective4f(GLUmat4 *result,
		 GLfloat fovy, GLfloat aspect, GLfloat near, GLfloat far)
{
	const double sine = sin(DEG2RAD(fovy / 2.0));
	const double cosine = cos(DEG2RAD(fovy / 2.0));
	const double sine_aspect = sine * aspect;
	const double dz = far - near;


	memcpy(result, &gluIdentityMatrix, sizeof(gluIdentityMatrix));
	if ((sine == 0.0) || (dz == 0.0) || (sine_aspect == 0.0)) {
		return;
	}

	result->col[0].values[0] = cosine / sine_aspect;
	result->col[1].values[1] = cosine / sine;
	result->col[2].values[2] = -(far + near) / dz;
	result->col[2].values[3] = -1.0;
	result->col[3].values[2] = -2.0 * near * far / dz;
	result->col[3].values[3] =  0.0;
}

void gluFrustum6f(GLUmat4 *result,
		  GLfloat left, GLfloat right,
		  GLfloat bottom, GLfloat top,
		  GLfloat near, GLfloat far)
{
	memcpy(result, &gluIdentityMatrix, sizeof(gluIdentityMatrix));

	result->col[0].values[0] = (2.0 * near) / (right - left);
	result->col[1].values[1] = (2.0 * near) / (top - bottom);
	result->col[2].values[0] = (right + left) / (right - left);
	result->col[2].values[1] = (top + bottom) / (top - bottom);
	result->col[2].values[2] = -(far + near) / (far - near);
	result->col[2].values[3] = -1.0;
	result->col[3].values[2] = -2.0 * near * far / (far - near);
	result->col[3].values[3] =  0.0;
}

void gluOrtho6f(GLUmat4 *result,
		GLfloat left, GLfloat right,
		GLfloat bottom, GLfloat top,
		GLfloat near, GLfloat far)
{
	memcpy(result, &gluIdentityMatrix, sizeof(gluIdentityMatrix));

	result->col[0].values[0] = 2.0f / (right - left);
	result->col[3].values[0] = -(right + left) / (right - left);

	result->col[1].values[1] = 2.0f / (top - bottom);
	result->col[3].values[1] = -(top + bottom) / (top - bottom);

	result->col[2].values[2] = -2.0f / (far - near);
	result->col[3].values[2] = -(far + near) / (far - near);
}

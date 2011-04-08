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

#include <math.h>
#include <string.h>

static inline void gluMult4v_4v(GLUvec4 *result,
				const GLUvec4 *v1, const GLUvec4 *v2)
{
	result->values[0] = v1->values[0] * v2->values[0];
	result->values[1] = v1->values[1] * v2->values[1];
	result->values[2] = v1->values[2] * v2->values[2];
	result->values[3] = v1->values[3] * v2->values[3];
}


static inline void gluDiv4v_4v(GLUvec4 *result,
			       const GLUvec4 *v1, const GLUvec4 *v2)
{
	result->values[0] = v1->values[0] / v2->values[0];
	result->values[1] = v1->values[1] / v2->values[1];
	result->values[2] = v1->values[2] / v2->values[2];
	result->values[3] = v1->values[3] / v2->values[3];
}


static inline void gluAdd4v_4v(GLUvec4 *result,
			       const GLUvec4 *v1, const GLUvec4 *v2)
{
	result->values[0] = v1->values[0] + v2->values[0];
	result->values[1] = v1->values[1] + v2->values[1];
	result->values[2] = v1->values[2] + v2->values[2];
	result->values[3] = v1->values[3] + v2->values[3];
}


static inline void gluSub4v_4v(GLUvec4 *result,
			       const GLUvec4 *v1, const GLUvec4 *v2)
{
	result->values[0] = v1->values[0] - v2->values[0];
	result->values[1] = v1->values[1] - v2->values[1];
	result->values[2] = v1->values[2] - v2->values[2];
	result->values[3] = v1->values[3] - v2->values[3];
}


static inline void gluMult4v_f(GLUvec4 *result,
			       const GLUvec4 *v1, GLfloat f)
{
	result->values[0] = v1->values[0] * f;
	result->values[1] = v1->values[1] * f;
	result->values[2] = v1->values[2] * f;
	result->values[3] = v1->values[3] * f;
}


static inline void gluDiv4v_f(GLUvec4 *result,
			      const GLUvec4 *v1, GLfloat f)
{
	result->values[0] = v1->values[0] / f;
	result->values[1] = v1->values[1] / f;
	result->values[2] = v1->values[2] / f;
	result->values[3] = v1->values[3] / f;
}


static inline void gluAdd4v_f(GLUvec4 *result,
			      const GLUvec4 *v1, GLfloat f)
{
	result->values[0] = v1->values[0] + f;
	result->values[1] = v1->values[1] + f;
	result->values[2] = v1->values[2] + f;
	result->values[3] = v1->values[3] + f;
}


static inline void gluSub4v_f(GLUvec4 *result,
			      const GLUvec4 *v1, GLfloat f)
{
	result->values[0] = v1->values[0] - f;
	result->values[1] = v1->values[1] - f;
	result->values[2] = v1->values[2] - f;
	result->values[3] = v1->values[3] - f;
}


static inline void gluMult4m_f(GLUmat4 *result,
			       const GLUmat4 *m, GLfloat f)
{
	GLUmat4 temp;

	gluMult4v_f(& temp.col[0], & m->col[0], f);
	gluMult4v_f(& temp.col[1], & m->col[1], f);
	gluMult4v_f(& temp.col[2], & m->col[2], f);
	gluMult4v_f(& temp.col[3], & m->col[3], f);
	*result = temp;
}


static inline void gluMult4m_4v(GLUvec4 *result,
				const GLUmat4 *m, const GLUvec4 *v)
{
	GLUvec4 temp[6];
	unsigned i;

	for (i = 0; i < 4; i++) {
		gluMult4v_f(& temp[i], & m->col[i], v->values[i]);
	}

	gluAdd4v_4v(& temp[4], & temp[0], & temp[1]);
	gluAdd4v_4v(& temp[5], & temp[2], & temp[3]);
	gluAdd4v_4v(result,    & temp[4], & temp[5]);
}


static inline void gluAdd4m_4m(GLUmat4 *result,
			       const GLUmat4 *m1, const GLUmat4 *m2)
{
	GLUmat4 temp;

	gluAdd4v_4v(& temp.col[0], & m1->col[0], & m2->col[0]);
	gluAdd4v_4v(& temp.col[1], & m1->col[1], & m2->col[1]);
	gluAdd4v_4v(& temp.col[2], & m1->col[2], & m2->col[2]);
	gluAdd4v_4v(& temp.col[3], & m1->col[3], & m2->col[3]);
	*result = temp;
}

static inline void gluSub4m_4m(GLUmat4 *result,
			       const GLUmat4 *m1, const GLUmat4 *m2)
{
	GLUmat4 temp;

	gluSub4v_4v(& temp.col[0], & m1->col[0], & m2->col[0]);
	gluSub4v_4v(& temp.col[1], & m1->col[1], & m2->col[1]);
	gluSub4v_4v(& temp.col[2], & m1->col[2], & m2->col[2]);
	gluSub4v_4v(& temp.col[3], & m1->col[3], & m2->col[3]);
	*result = temp;
}

static inline GLfloat gluDot4_4v(const GLUvec4 *v1, const GLUvec4 *v2)
{
	return v1->values[0] * v2->values[0]
		+ v1->values[1] * v2->values[1]
		+ v1->values[2] * v2->values[2]
		+ v1->values[3] * v2->values[3];
}


static inline GLfloat gluDot3_4v(const GLUvec4 *v1, const GLUvec4 *v2)
{
	return v1->values[0] * v2->values[0]
		+ v1->values[1] * v2->values[1]
		+ v1->values[2] * v2->values[2];
}


static inline GLfloat gluDot2_4v(const GLUvec4 *v1, const GLUvec4 *v2)
{
	return v1->values[0] * v2->values[0]
		+ v1->values[1] * v2->values[1];
}


static inline void gluCross4v(GLUvec4 *result,
			      const GLUvec4 *v1, const GLUvec4 *v2)
{
	GLUvec4 temp;

	temp.values[0] = (v1->values[1] * v2->values[2]) 
		- (v1->values[2] * v2->values[1]);
	temp.values[1] = (v1->values[2] * v2->values[0]) 
		- (v1->values[0] * v2->values[2]);
	temp.values[2] = (v1->values[0] * v2->values[1]) 
		- (v1->values[1] * v2->values[0]);
	temp.values[3] = 0.0;
	*result = temp;
}


static inline void gluOuter4v(GLUmat4 *result,
			      const GLUvec4 *v1, const GLUvec4 *v2)
{
	GLUmat4 temp;

	gluMult4v_f(& temp.col[0], v1, v2->values[0]);
	gluMult4v_f(& temp.col[1], v1, v2->values[1]);
	gluMult4v_f(& temp.col[2], v1, v2->values[2]);
	gluMult4v_f(& temp.col[3], v1, v2->values[3]);
	*result = temp;
}


static inline GLfloat gluLengthSqr4v(const GLUvec4 *v)
{
	return gluDot4_4v(v, v);
}


static inline GLfloat gluLength4v(const GLUvec4 *v)
{
	return sqrt(gluLengthSqr4v(v));
}


static inline void gluNormalize4v(GLUvec4 *result, const GLUvec4 *v)
{
	gluDiv4v_f(result, v, gluLength4v(v));
}



static inline void gluTranspose4m(GLUmat4 *result, const GLUmat4 *m)
{
	unsigned i;
	unsigned j;
	GLUmat4 temp;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			temp.col[i].values[j] = m->col[j].values[i];
		}
	}

	*result = temp;
}


static inline void gluMult4m_4m(GLUmat4 *result,
				const GLUmat4 *m1, const GLUmat4 *m2)
{
	GLUmat4 temp;
	unsigned i;

	for (i = 0; i < 4; i++) {
		gluMult4m_4v(& temp.col[i], m1, & m2->col[i]);
	}

	*result = temp;
}



static inline void gluTranslate3f(GLUmat4 *result,
				  GLfloat x, GLfloat y, GLfloat z)
{
	memcpy(result, & gluIdentityMatrix, sizeof(gluIdentityMatrix));
	result->col[3].values[0] = x;
	result->col[3].values[1] = y;
	result->col[3].values[2] = z;
}


#ifdef __cplusplus
static inline GLfloat gluDot4(const GLUvec4 &v1, const GLUvec4 &v2)
{
	return v1.values[0] * v2.values[0]
		+ v1.values[1] * v2.values[1]
		+ v1.values[2] * v2.values[2]
		+ v1.values[3] * v2.values[3];
}


static inline GLfloat gluDot3(const GLUvec4 &v1, const GLUvec4 &v2)
{
	return v1.values[0] * v2.values[0]
		+ v1.values[1] * v2.values[1]
		+ v1.values[2] * v2.values[2];
}


static inline GLfloat gluDot2(const GLUvec4 &v1, const GLUvec4 &v2)
{
	return v1.values[0] * v2.values[0]
		+ v1.values[1] * v2.values[1];
}


inline GLUvec4 GLUvec4::operator+(const GLUvec4 &v) const
{
	return GLUvec4(values[0] + v.values[0],
		       values[1] + v.values[1],
		       values[2] + v.values[2],
		       values[3] + v.values[3]);
}


inline GLUvec4 GLUvec4::operator-(const GLUvec4 &v) const
{
	return GLUvec4(values[0] - v.values[0],
		       values[1] - v.values[1],
		       values[2] - v.values[2],
		       values[3] - v.values[3]);
}


inline GLUvec4 GLUvec4::operator*(const GLUvec4 &v) const
{
	return GLUvec4(values[0] * v.values[0],
		       values[1] * v.values[1],
		       values[2] * v.values[2],
		       values[3] * v.values[3]);
}


inline GLUvec4 GLUvec4::operator*(GLfloat f) const
{
	return GLUvec4(values[0] * f,
		       values[1] * f,
		       values[2] * f,
		       values[3] * f);
}


inline GLUvec4 GLUvec4::operator*(const GLUmat4 &m) const
{
	return GLUvec4(gluDot4(*this, m.col[0]),
		       gluDot4(*this, m.col[1]),
		       gluDot4(*this, m.col[2]),
		       gluDot4(*this, m.col[3]));
}


inline GLUmat4 GLUmat4::operator+(const GLUmat4 &m) const
{
	GLUmat4 temp;

	gluAdd4m_4m(& temp, this, &m);
	return temp;
}


inline GLUmat4 GLUmat4::operator-(const GLUmat4 &m) const
{
	return GLUmat4(col[0] - m.col[0],
		       col[1] - m.col[1],
		       col[2] - m.col[2],
		       col[3] - m.col[3]);
}


inline GLUmat4 GLUmat4::operator*(GLfloat f) const
{
	GLUmat4 temp;

	gluMult4m_f(& temp, this, f);
	return temp;
}


inline GLUvec4 GLUmat4::operator*(const GLUvec4 &v) const
{
	return (col[0] * v.values[0])
		+ (col[1] * v.values[1])
		+ (col[2] * v.values[2])
		+ (col[3] * v.values[3]);
}


inline GLUmat4 GLUmat4::operator*(const GLUmat4 &m) const
{
	GLUmat4 temp;

	gluMult4m_4m(& temp, this, &m);
	return temp;
}


#endif /* __cplusplus */

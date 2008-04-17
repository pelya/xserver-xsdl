/*
 * Copyright (C) 2008 Apple, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */

#ifndef _XQ_THREAD_SAFETY_H_
#define _XQ_THREAD_SAFETY_H_

#include <pthread.h>

extern pthread_t SERVER_THREAD;
extern pthread_t APPKIT_THREAD;

#define threadSafetyID(tid) (pthread_equal((tid), SERVER_THREAD) ? "X Server Thread" : "Appkit Thread")

/* Print message to ErrorF if we're in the wrong thread */
void threadAssert(pthread_t tid);

#define TA_SERVER() threadAssert(SERVER_THREAD)
#define TA_APPKIT() threadAssert(APPKIT_THREAD)

#endif _XQ_THREAD_SAFETY_H_

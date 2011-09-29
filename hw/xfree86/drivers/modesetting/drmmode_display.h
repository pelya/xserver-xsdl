/*
 * Copyright © 2007 Red Hat, Inc.
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *     Dave Airlie <airlied@redhat.com>
 *
 */
#ifndef DRMMODE_DISPLAY_H
#define DRMMODE_DISPLAY_H

#include "xf86drmMode.h"
#ifdef HAVE_UDEV
#include "libudev.h"
#endif

struct dumb_bo {
  uint32_t handle;
  uint32_t size;
  void *ptr;
  int map_count;
  uint32_t pitch;
};

typedef struct {
  int fd;
  unsigned fb_id;
  drmModeResPtr mode_res;
  drmModeFBPtr mode_fb;
  int cpp;
  ScrnInfoPtr scrn;
#ifdef HAVE_UDEV
  struct udev_monitor *uevent_monitor;
  InputHandlerProc uevent_handler;
#endif
  drmEventContext event_context;
  struct dumb_bo *front_bo;
} drmmode_rec, *drmmode_ptr;

typedef struct {
  drmmode_ptr drmmode;
  unsigned old_fb_id;
  int flip_count;
  void *event_data;
  unsigned int fe_frame;
  unsigned int fe_tv_sec;
  unsigned int fe_tv_usec;
} drmmode_flipdata_rec, *drmmode_flipdata_ptr;

typedef struct {
  drmmode_flipdata_ptr flipdata;
  Bool dispatch_me;
} drmmode_flipevtcarrier_rec, *drmmode_flipevtcarrier_ptr;

typedef struct {
    drmmode_ptr drmmode;
    drmModeCrtcPtr mode_crtc;
    int hw_id;
    struct dumb_bo *cursor_bo;
  //    struct radeon_bo *rotate_bo;
    unsigned rotate_fb_id;
    uint16_t lut_r[256], lut_g[256], lut_b[256];
} drmmode_crtc_private_rec, *drmmode_crtc_private_ptr;

typedef struct {
    drmModePropertyPtr mode_prop;
    uint64_t value;
    int num_atoms; /* if range prop, num_atoms == 1; if enum prop, num_atoms == num_enums + 1 */
    Atom *atoms;
} drmmode_prop_rec, *drmmode_prop_ptr;


typedef struct {
    drmmode_ptr drmmode;
    int output_id;
    drmModeConnectorPtr mode_output;
    drmModeEncoderPtr *mode_encoders;
    drmModePropertyBlobPtr edid_blob;
    int dpms_enum_id;
    int num_props;
    drmmode_prop_ptr props;
    int enc_mask;
    int enc_clone_mask;
} drmmode_output_private_rec, *drmmode_output_private_ptr;


extern Bool drmmode_pre_init(ScrnInfoPtr pScrn, drmmode_ptr drmmode, int cpp);
//extern Bool drmmode_set_bufmgr(ScrnInfoPtr pScrn, drmmode_ptr drmmode, struct radeon_bo_manager *bufmgr);
//extern void drmmode_set_cursor(ScrnInfoPtr scrn, drmmode_ptr drmmode, int id, struct radeon_bo *bo);
void drmmode_adjust_frame(ScrnInfoPtr pScrn, drmmode_ptr drmmode, int x, int y, int flags);
extern Bool drmmode_set_desired_modes(ScrnInfoPtr pScrn, drmmode_ptr drmmode);
extern void drmmode_copy_fb(ScrnInfoPtr pScrn, drmmode_ptr drmmode);
extern Bool drmmode_setup_colormap(ScreenPtr pScreen, ScrnInfoPtr pScrn);

extern void drmmode_uevent_init(ScrnInfoPtr scrn, drmmode_ptr drmmode);
extern void drmmode_uevent_fini(ScrnInfoPtr scrn, drmmode_ptr drmmode);

extern int drmmode_get_height_align(ScrnInfoPtr scrn, uint32_t tiling);
extern int drmmode_get_pitch_align(ScrnInfoPtr scrn, int bpe, uint32_t tiling);
extern int drmmode_get_base_align(ScrnInfoPtr scrn, int bpe, uint32_t tiling);

//Bool radeon_do_pageflip(ScrnInfoPtr scrn, struct radeon_bo *new_front, void *data, int ref_crtc_hw_id);
Bool drmmode_create_initial_bos(ScrnInfoPtr pScrn, drmmode_ptr drmmode);
void *drmmode_map_front_bo(drmmode_ptr drmmode);
Bool drmmode_map_cursor_bos(ScrnInfoPtr pScrn, drmmode_ptr drmmode);
void drmmode_free_bos(ScrnInfoPtr pScrn, drmmode_ptr drmmode);
#endif

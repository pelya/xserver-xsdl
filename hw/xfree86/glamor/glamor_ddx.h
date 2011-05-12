#ifndef GLAMOR_DDX_H
#define GLAMOR_DDX_H

Bool glamor_resize(ScrnInfoPtr scrn, int width, int height);
void glamor_frontbuffer_handle(ScrnInfoPtr scrn,
			       uint32_t *handle, uint32_t *pitch);
Bool glamor_load_cursor(ScrnInfoPtr scrn,
			CARD32 *image, int width, int height);

void glamor_cursor_handle(ScrnInfoPtr scrn, EGLImageKHR image, uint32_t *handle, uint32_t *pitch);
EGLImageKHR glamor_create_cursor_argb(ScrnInfoPtr scrn, int width, int height);

Bool drmmode_pre_init(ScrnInfoPtr scrn, int fd, int cpp);
void drmmode_closefb(ScrnInfoPtr scrn);

#endif

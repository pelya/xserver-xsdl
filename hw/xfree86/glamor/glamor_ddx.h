#ifndef GLAMOR_DDX_H
#define GLAMOR_DDX_H

#include <gbm.h>
#define GLAMOR_FOR_XORG 1
#include <glamor.h>
struct glamor_ddx_screen_private {
        struct gbm_bo *root_bo;
        struct gbm_bo *cursor_bo;
        struct gbm_device *gbm;

	CreateScreenResourcesProcPtr CreateScreenResources;
	CloseScreenProcPtr CloseScreen;
	int fd;
	int cpp;
};

struct glamor_gbm_cursor {
       struct gbm_bo *cursor_bo;
       PixmapPtr cursor_pixmap;
};

inline static struct glamor_ddx_screen_private *
glamor_ddx_get_screen_private(ScrnInfoPtr scrn)
{
	return (struct glamor_ddx_screen_private *) (scrn->driverPrivate);
}

Bool glamor_front_buffer_resize(ScrnInfoPtr scrn, int width, int height);
void glamor_frontbuffer_handle(ScrnInfoPtr scrn,
			       uint32_t *handle, uint32_t *pitch);
Bool glamor_load_cursor(ScrnInfoPtr scrn,
			int width, int height);

void glamor_cursor_handle(struct glamor_gbm_cursor *cursor, uint32_t *handle, uint32_t *pitch);
Bool glamor_create_cursor(ScrnInfoPtr scrn, struct glamor_gbm_cursor *cursor, int width, int height);
void glamor_destroy_cursor(ScrnInfoPtr scrn, struct glamor_gbm_cursor *cursor) ;

Bool drmmode_pre_init(ScrnInfoPtr scrn, int fd, int cpp);
void drmmode_closefb(ScrnInfoPtr scrn);

#endif

#ifdef GLAMOR_GLES2

#define GL_BGRA                                 GL_BGRA_EXT
#define GL_COLOR_INDEX                          0x1900
#define GL_BITMAP                               0x1A00
#define GL_UNSIGNED_INT_8_8_8_8                 0x8035
#define GL_UNSIGNED_INT_8_8_8_8_REV             0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV          0x8368
#define GL_UNSIGNED_INT_10_10_10_2              0x8036
#define GL_UNSIGNED_SHORT_5_6_5_REV             0x8364
#define GL_UNSIGNED_SHORT_1_5_5_5_REV           0x8366
#define GL_UNSIGNED_SHORT_4_4_4_4_REV           0x8365

#define GL_PIXEL_PACK_BUFFER              0x88EB
#define GL_PIXEL_UNPACK_BUFFER            0x88EC
#define GL_CLAMP_TO_BORDER                0x812D

#define GL_READ_WRITE                     0x88BA
#define GL_READ_ONLY                      0x88B8
#define GL_WRITE_ONLY                     0x88B9
#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_PACK_ROW_LENGTH                      0x0D02
#define GL_UNPACK_ROW_LENGTH                    0x0CF2

#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 0x8CDC
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE 0x8D56

#define GL_PACK_INVERT_MESA               0x8758

#endif

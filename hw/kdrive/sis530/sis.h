/*
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _SIS_H_
#define _SIS_H_
#include "kdrive.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <asm/io.h>
#include <stdio.h>

/*
 *  Linear Addressing		000 0000 - 0ff ffff (16m)
 *  Image data transfer		100 0000 - 100 7fff (32k)
 *  Empty			100 8000 - 100 81ff
 *  MMIO registers		100 8200 - 100 8480
 *
 * We don't care about the image transfer or PCI regs, so
 * this structure starts at the MMIO regs
 */
    
typedef volatile CARD32 VOL32;
typedef volatile CARD16 VOL16;
typedef volatile CARD8 VOL8;

#define SIS_MMIO_OFFSET	0x8200

typedef struct _sis530General {
    VOL32	src_base;		/* 8200 */
    VOL16	src_pitch;		/* 8204 */
    VOL16	_pad0;			/* 8206 */
    VOL16	src_y;			/* 8208 */
    VOL16	src_x;			/* 820a */
    VOL16	dst_y;			/* 820c */
    VOL16	dst_x;			/* 820e */
    VOL32	dst_base;		/* 8210 */
    VOL16	dst_pitch;		/* 8214 */
    VOL16	dst_height;		/* 8216 */
    VOL16	rect_width;		/* 8218 */
    VOL16	rect_height;		/* 821a */
    VOL32	pattern_fg;		/* 821c */
    VOL32	pattern_bg;		/* 8220 */
    VOL32	src_fg;			/* 8224 */
    VOL32	src_bg;			/* 8228 */
    VOL8	mask[8];		/* 822c */
    VOL16	clip_left;		/* 8234 */
    VOL16	clip_top;		/* 8236 */
    VOL16	clip_right;		/* 8238 */
    VOL16	clip_bottom;		/* 823a */
    VOL32	command;		/* 823c */
    VOL32	status;			/* 8240 */
    VOL8	_pad1[0xbc];		/* 8244 */
    VOL8	pattern[256];		/* 8300 */
					/* 8400 */
} SisGeneral;

typedef struct _sis530Line {
    VOL8	_pad0[8];		/* 8200 */
    VOL16	x0;			/* 8208 */
    VOL16	y0;			/* 820a */
    VOL16	x1;			/* 820c */
    VOL16	y1;			/* 820e */
    VOL32	dst_base;		/* 8210 */
    VOL16	dst_pitch;		/* 8214 */
    VOL16	dst_height;		/* 8216 */
    VOL16	count;			/* 8218 */
    VOL16	style_period;		/* 821a */
    VOL32	fg;			/* 821c */
    VOL32	bg;			/* 8220 */
    VOL8	_pad1[8];		/* 8224 */
    VOL32	style0;			/* 822c */
    VOL32	style1;			/* 8228 */
    VOL16	clip_left;		/* 8234 */
    VOL16	clip_top;		/* 8236 */
    VOL16	clip_right;		/* 8238 */
    VOL16	clip_bottom;		/* 823a */
    VOL32	command;		/* 823c */
    VOL32	status;			/* 8240 */
    VOL8	_pad2[0xbc];		/* 8244 */
    struct {
	VOL16	x;
	VOL16	y;
    }		data[96];		/* 8300 */
					/* 8480 */
} SisLine;

typedef struct _sis530Transparent {
    VOL32	src_base;		/* 8200 */
    VOL16	src_pitch;		/* 8204 */
    VOL16	_pad0;			/* 8206 */
    VOL16	src_y;			/* 8208 */
    VOL16	src_x;			/* 820a */
    VOL16	dst_y;			/* 820c */
    VOL16	dst_x;			/* 820e */
    VOL32	dst_base;		/* 8210 */
    VOL16	dst_pitch;		/* 8214 */
    VOL16	dst_height;		/* 8216 */
    VOL16	rect_width;		/* 8218 */
    VOL16	rect_height;		/* 821a */
    VOL32	dst_key_high;		/* 821c */
    VOL32	dst_key_low;		/* 8220 */
    VOL32	src_key_high;		/* 8224 */
    VOL32	src_key_low;		/* 8228 */
    VOL8	_pad1[8];		/* 822c */
    VOL16	clip_left;		/* 8234 */
    VOL16	clip_top;		/* 8236 */
    VOL16	clip_right;		/* 8238 */
    VOL16	clip_bottom;		/* 823a */
    VOL32	command;		/* 823c */
    VOL32	status;			/* 8240 */
					/* 8244 */
} SisTransparent;

typedef struct _sis530Multiple {
    VOL8	_pad0[8];		/* 8200 */
    VOL16	count;			/* 8208 */
    VOL16	y;			/* 820a */
    VOL16	x0_start;		/* 820c */
    VOL16	x0_end;			/* 820e */
    VOL32	dst_base;		/* 8210 */
    VOL16	dst_pitch;		/* 8214 */
    VOL16	dst_height;		/* 8216 */
    VOL8	_pad1[4];		/* 8218 */
    VOL32	fg;			/* 821c */
    VOL32	bg;			/* 8220 */
    VOL8	_pad2[8];		/* 8224 */
    VOL8	mask[8];		/* 822c */
    VOL16	clip_left;		/* 8234 */
    VOL16	clip_top;		/* 8236 */
    VOL16	clip_right;		/* 8238 */
    VOL16	clip_bottom;		/* 823a */
    VOL32	command;		/* 823c */
    VOL32	status;			/* 8240 */
    VOL16	x1_start;		/* 8244 */
    VOL16	x1_end;			/* 8246 */
    VOL8	_pad3[0xb8];		/* 8248 */
    VOL8	pattern[64];		/* 8300 */
    struct {
	VOL16	x_start;
	VOL16	y_end;
    }		data[80];		/* 8340 */
					/* 8480 */
} SisMultiple;

typedef struct _sis530Trapezoid {
    VOL8	_pad0[8];		/* 8200 */
    VOL16	height;			/* 8208 */
    VOL16	y;			/* 820a */
    VOL16	left_x;			/* 820c */
    VOL16	right_x;	    	/* 820e */
    VOL32	dst_base;		/* 8210 */
    VOL16	dst_pitch;		/* 8214 */
    VOL16	dst_height;		/* 8216 */
    VOL8	_pad1[4];    		/* 8218 */
    VOL32	fg;			/* 821c */
    VOL32	bg;			/* 8220 */
    VOL8	_pad2[8];		/* 8224 */
    VOL8	mask[8];		/* 822c */
    VOL16	clip_left;		/* 8234 */
    VOL16	clip_top;		/* 8236 */
    VOL16	clip_right;		/* 8238 */
    VOL16	clip_bottom;		/* 823a */
    VOL32	command;		/* 823c */
    VOL32	status;			/* 8240 */
    VOL16	left_dx;		/* 8244 */
    VOL16	left_dy;		/* 8246 */
    VOL16	right_dx;		/* 8248 */
    VOL16	right_dy;		/* 824a */
    VOL32	left_error;		/* 824c */
    VOL32	right_error;		/* 8250 */
					/* 8254 */
} SisTrapezoid;

typedef struct _sisAccel {
    VOL8	pad[0x80];		/* 8200 */
    VOL32	src_addr;	    	/* 8280 */
    VOL32	dst_addr;		/* 8284 */
    VOL32	pitch;			/* 8288 */
    VOL32	dimension;		/* 828c */
    VOL32	fg;			/* 8290 */
    VOL32	bg;			/* 8294 */
    

    VOL32	clip_ul;		/* 82a0 */
    VOL32	clip_br;		/* 82a4 */

    VOL16	cmd;			/* 82aa */

    VOL8	pattern[256];		/* 82ac */
    
} SisAccel;

typedef struct _sis530 {
    union {
	SisGeneral	general;
	SisLine		line;
	SisTransparent	transparent;
	SisMultiple	multiple;
	SisTrapezoid	trapezoid;
	SisAccel	accel;
    } u;
} SisRec, *SisPtr;

typedef struct _sisCursor {
    int		width, height;
    int		xhot, yhot;
    Bool	has_cursor;
    CursorPtr	pCursor;
} SisCursor;

#define SIS_CURSOR_WIDTH    64
#define SIS_CURSOR_HEIGHT   64

typedef struct _sisClock {
    CARD32	vclk_numerator;
    BOOL	vclk_divide_by_2;
    CARD32	vclk_denominator;
    CARD32	vclk_post_scale;
    BOOL	vclk_post_scale_2;
    BOOL	high_speed_dac;
} SisClockRec, *SisClockPtr;

typedef struct _crtc {

    union {
	struct {
	    CARD8   _io_address_select	    : 1;
	    CARD8   _display_ram_enable	    : 1;
	    CARD8   _clock_select	    : 2;
	    CARD8			    : 1;
	    CARD8   _odd_even_page	    : 1;
	    CARD8   _h_sync_polarity	    : 1;
	    CARD8   _v_sync_polarity	    : 1;
	} _misc_output_s;
	CARD8	_misc_output;
    } _misc_output_u;    			    /* 3CC/3C2 */

#define misc_output	    _misc_output_u._misc_output
#define io_address_select   _misc_output_u._misc_output_s._io_address_select
#define display_ram_enable  _misc_output_u._misc_output_s._display_ram_enable
#define clock_select	    _misc_output_u._misc_output_s._clock_select
#define odd_even_page	    _misc_output_u._misc_output_s._odd_even_page
#define h_sync_polarity	    _misc_output_u._misc_output_s._h_sync_polarity
#define v_sync_polarity	    _misc_output_u._misc_output_s._v_sync_polarity
    
    CARD8   h_total_0_7;			    /* CR0 */
    CARD8   h_display_end_0_7;			    /* CR1 */
    CARD8   h_blank_start_0_7;			    /* CR2 */
    union {
	struct {
	    CARD8   _h_blank_end_0_4  	    : 5;
	    CARD8   _display_skew	    : 2;
	    CARD8			    : 1;
	} _h_blank_end_s;
	CARD8   __h_blank_end;			    /* CR3 */
    } _h_blank_end_u;
#define h_blank_end_0_4 _h_blank_end_u._h_blank_end_s._h_blank_end_0_4
#define display_skew	_h_blank_end_u._h_blank_end_s._display_skew
#define _h_blank_end	_h_blank_end_u.__h_blank_end
    
    CARD8   h_sync_start_0_7;			    /* CR4 */
    
    union {
	struct {
	    CARD8   _h_sync_end_0_4    	    : 5;
	    CARD8   _horizontal_skew	    : 2;
	    CARD8   _h_blank_end_5	    : 1;
	} _h_sync_end_s;
        CARD8   __h_sync_end;			    /* CR5 */
    } _h_sync_end_u;

#define h_sync_end_0_4	_h_sync_end_u._h_sync_end_s._h_sync_end_0_4
#define horizontal_skew	_h_sync_end_u._h_sync_end_s._horizontal_skew
#define h_blank_end_5	_h_sync_end_u._h_sync_end_s._h_blank_end_5
#define _h_sync_end	_h_sync_end_u.__h_sync_end
    
    CARD8   v_total_0_7;			    /* CR6 */
    
    union {
	struct {
	    CARD8   _v_total_8		    : 1;
	    CARD8   _v_display_end_8	    : 1;
	    CARD8   _v_retrace_start_8	    : 1;
	    CARD8   _v_blank_start_8	    : 1;
	    CARD8   _line_compare_8	    : 1;
	    CARD8   _v_total_9		    : 1;
	    CARD8   _v_display_end_9	    : 1;
	    CARD8   _v_retrace_start_9	    : 1;
	} _crtc_overflow_s;
	CARD8	_crtc_overflow;			    /* CR7 */
    } _crtc_overflow_u;

#define v_total_8	    _crtc_overflow_u._crtc_overflow_s._v_total_8
#define v_display_end_8	    _crtc_overflow_u._crtc_overflow_s._v_display_end_8
#define v_retrace_start_8   _crtc_overflow_u._crtc_overflow_s._v_retrace_start_8
#define v_blank_start_8	    _crtc_overflow_u._crtc_overflow_s._v_blank_start_8
#define line_compare_8	    _crtc_overflow_u._crtc_overflow_s._line_compare_8
#define v_total_9	    _crtc_overflow_u._crtc_overflow_s._v_total_9
#define v_display_end_9	    _crtc_overflow_u._crtc_overflow_s._v_display_end_9
#define v_retrace_start_9   _crtc_overflow_u._crtc_overflow_s._v_retrace_start_9
#define crtc_overflow	    _crtc_overflow_u._crtc_overflow

    CARD8   preset_row_scan;			    /* CR8 (unused) */

    union {
	struct {
	    CARD8   ___max_scan_line	    : 5;
	    CARD8   _v_blank_start_9	    : 1;
	    CARD8   _line_compare_9	    : 1;
	    CARD8   _double_scan	    : 1;
	} _max_scan_line_s;
	CARD8   __max_scan_line;		    /* CR9 */
    } _max_scan_line_u;

#define max_scan_line	_max_scan_line_u._max_scan_line_s.___max_scan_line
#define v_blank_start_9	_max_scan_line_u._max_scan_line_s._v_blank_start_9
#define line_compare_9	_max_scan_line_u._max_scan_line_s._line_compare_9
#define double_scan	_max_scan_line_u._max_scan_line_s._double_scan
#define _max_scan_line	_max_scan_line_u.__max_scan_line
    
    CARD8   cursor_start;			    /* CRA */
    CARD8   cursor_end;				    /* CRB */

    CARD8   start_address_8_15;			    /* CRC */
    CARD8   start_address_0_7;			    /* CRD */

    CARD8   text_cursor_15_8;			    /* CRE */
    CARD8   text_cursor_7_0;			    /* CRF */

    CARD8   cursor_loc_high;
    CARD8   cursor_loc_low;

    CARD8   v_retrace_start_0_7;		    /* CR10 */
    union {
	struct {
	    CARD8   _v_retrace_end_0_3	    : 4;
	    CARD8   _clear_v_retrace_int	    : 1;
	    CARD8   _disable_v_retrace_int   : 1;
	    CARD8   _refresh_cycle_select    : 1;
	    CARD8   _lock_crtc		    : 1;
	} _v_retrace_end_s;
	CARD8   __v_retrace_end;		    /* CR11 */
    } _v_retrace_end_u;
    
#define v_retrace_end_0_3	_v_retrace_end_u._v_retrace_end_s._v_retrace_end_0_3
#define clear_v_retrace_int	_v_retrace_end_u._v_retrace_end_s._clear_v_retrace_int
#define disable_v_retrace_int	_v_retrace_end_u._v_retrace_end_s._disable_v_retrace_int
#define refresh_cycle_select	_v_retrace_end_u._v_retrace_end_s._refresh_cycle_select
#define lock_crtc		_v_retrace_end_u._v_retrace_end_s._lock_crtc
#define _v_retrace_end		_v_retrace_end_u.__v_retrace_end
    
    CARD8   v_display_end_0_7;			    /* CR12 */
    
    CARD8   screen_off_0_7;			    /* CR13 */
    
    union {
	struct {
	    CARD8   ___underline_location   : 5;
	    CARD8   _count_by_four	    : 1;
	    CARD8   _doubleword_mode	    : 1;
	    CARD8			    : 1;
	} _underline_location_s;
	CARD8   __underline_location;		    /* CR14 */
    } _underline_location_u;

#define underline_location  _underline_location_u._underline_location_s.___underline_location
#define count_by_four	    _underline_location_u._underline_location_s._count_by_four
#define doubleword_mode	    _underline_location_u._underline_location_s._doubleword_mode
#define _underline_location _underline_location_u.__underline_location
    
    CARD8   v_blank_start_0_7;			    /* CR15 */
    CARD8   v_blank_end_0_7;			    /* CR16 */

    union {
	struct {
	    CARD8   _two_bk_cga		    : 1;
	    CARD8   _four_bk_cga	    : 1;
	    CARD8   _v_total_double	    : 1;
	    CARD8   _count_by_two	    : 1;
	    CARD8			    : 1;
	    CARD8   _address_wrap	    : 1;
	    CARD8   _byte_mode		    : 1;
	    CARD8   _hardware_reset	    : 1;
	} _crtc_mode_s;
	CARD8   _crtc_mode;			    /* CR17 */
    } _crtc_mode_u;

#define crtc_mode	_crtc_mode_u._crtc_mode
#define two_bk_cga	_crtc_mode_u._crtc_mode_s._two_bk_cga
#define four_bk_cga	_crtc_mode_u._crtc_mode_s._four_bk_cga
#define v_total_double	_crtc_mode_u._crtc_mode_s._v_total_double
#define count_by_two	_crtc_mode_u._crtc_mode_s._count_by_two
#define address_wrap	_crtc_mode_u._crtc_mode_s._address_wrap
#define byte_mode	_crtc_mode_u._crtc_mode_s._byte_mode
#define hardware_reset	_crtc_mode_u._crtc_mode_s._hardware_reset

    CARD8   line_compare_0_7;			    /* CR18 (unused) */

    union {
	struct {
	    CARD8	_graphics_mode_enable	: 1;
	    CARD8	_attribute_byte_mda	: 1;
	    CARD8	_line_graphics_enable	: 1;
	    CARD8	_background_blink	: 1;
	    CARD8				: 1;
	    CARD8	_pel_panning_compat	: 1;
	    CARD8	_pixel_clock_double	: 1;
	    CARD8	p4_p5_source_select	: 1;
	} _mode_control_s;
	CARD8	_mode_control;
    } _mode_control_u;				    /* AR10 */

#define mode_control		_mode_control_u._mode_control
#define graphics_mode_enable	_mode_control_u._mode_control_s._graphics_mode_enable
#define pixel_clock_double	_mode_control_u._mode_control_s._pixel_clock_double
    
    CARD8   screen_border_color;		    /* AR11 */
    CARD8   enable_color_plane;			    /* AR12 */
    CARD8   horizontal_pixel_pan;		    /* AR13 */
    
    union {
	struct {
	    CARD8   _write_mode		    : 2;
	    CARD8			    : 1;
	    CARD8   _read_mode		    : 1;
	    CARD8   _odd_even_addressing    : 1;
	    CARD8   _shift_register_mode    : 1;
	    CARD8   _color_mode_256	    : 1;
	    CARD8			    : 1;
	} _mode_register_s;
	CARD8	_mode_register;
    } _mode_register_u;				    /* GR5 */
    
#define mode_register	    _mode_register_u._mode_register
#define color_mode_256	    _mode_register_u._mode_register_s._color_mode_256
#define odd_even_addressing _mode_register_u._mode_register_s._odd_even_addressing

    union {
	struct {
	    CARD8   _graphics_enable	    : 1;
	    CARD8   _chain_odd_even	    : 1;
	    CARD8   _memory_address_select  : 2;
	    CARD8			    : 4;
	} _misc_register_s;
	CARD8   _misc_register;
    } _misc_register_u;				    /* GR6 */

#define misc_register	    _misc_register_u._misc_register
#define graphics_enable	    _misc_register_u._misc_register_s._graphics_enable
#define chain_odd_even	    _misc_register_u._misc_register_s._chain_odd_even
#define memory_address_select _misc_register_u._misc_register_s._memory_address_select
    
    CARD8	color_dont_care;		    /* GR7 */
    
    union {
	struct {
	    CARD8   _dot_clock_8_9	    : 1;
	    CARD8			    : 1;
	    CARD8   _shifter_load_16	    : 1;
	    CARD8   _dot_clock_divide_2	    : 1;
	    CARD8   _shifter_load_32	    : 1;
	    CARD8   _display_off	    : 1;
	    CARD8			    : 2;
	} _clock_mode_s;
	CARD8	_clock_mode;
    } _clock_mode_u;				    /* SR1 */

#define clock_mode	    _clock_mode_u._clock_mode
#define dot_clock_8_9	    _clock_mode_u._clock_mode_s._dot_clock_8_9
#define shifter_load_16	    _clock_mode_u._clock_mode_s._shifter_load_16
#define dot_clock_divide_2  _clock_mode_u._clock_mode_s._dot_clock_divide_2
#define shifter_load_32	    _clock_mode_u._clock_mode_s._shifter_load_32
#define display_off	    _clock_mode_u._clock_mode_s._display_off

    CARD8   color_plane_w_enable;		    /* SR2 */
    
    union {
	struct {
	    CARD8			    : 1;
	    CARD8   _extended_memory_size   : 1;
	    CARD8   _odd_even_disable	    : 1;
	    CARD8   _chain_4_enable	    : 1;
	    CARD8			    : 4;
	} _memory_mode_s;
	CARD8	_memory_mode;
    } _memory_mode_u;				    /* SR4 */

#define memory_mode	    _memory_mode_u._memory_mode
#define extended_memory_sz  _memory_mode_u._memory_mode_s._extended_memory_size
#define	odd_even_disable    _memory_mode_u._memory_mode_s._odd_even_disable
#define chain_4_enable	    _memory_mode_u._memory_mode_s._chain_4_enable
    
    union {
	struct {
	    CARD8   _enhanced_text_mode	    : 1;
	    CARD8   _enhanced_graphics_mode : 1;
	    CARD8   _graphics_mode_32k	    : 1;
	    CARD8   _graphics_mode_64k	    : 1;
	    CARD8   _graphics_mode_true	    : 1;
	    CARD8   _graphics_mode_interlaced: 1;
	    CARD8   _graphics_mode_hw_cursor: 1;
	    CARD8   _graphics_mode_linear   : 1;
	} _graphics_mode_s;
	CARD8	_graphics_mode;
    } _graphics_mode_u;				/* SR6 */

#define graphics_mode		_graphics_mode_u._graphics_mode
#define enhanced_text_mode	_graphics_mode_u._graphics_mode_s._enhanced_text_mode
#define enhanced_graphics_mode  _graphics_mode_u._graphics_mode_s._enhanced_graphics_mode
#define graphics_mode_32k	_graphics_mode_u._graphics_mode_s._graphics_mode_32k
#define graphics_mode_64k	_graphics_mode_u._graphics_mode_s._graphics_mode_64k
#define graphics_mode_true	_graphics_mode_u._graphics_mode_s._graphics_mode_true
#define graphics_mode_interlaced	_graphics_mode_u._graphics_mode_s._graphics_mode_interlaced
#define graphics_mode_hw_cursor	_graphics_mode_u._graphics_mode_s._graphics_mode_hw_cursor
#define graphics_mode_linear	_graphics_mode_u._graphics_mode_s._graphics_mode_linear

    union {
	struct {
	    CARD8   _external_dac_reference : 1;
	    CARD8   _high_speed_dac_0	    : 1;
	    CARD8   _direct_color_24bit	    : 1;
	    CARD8   _multi_line_prefetch    : 1;
	    CARD8   _extended_video_div_2   : 1;
	    CARD8   _ramdac_power_save	    : 1;
	    CARD8			    : 1;
	    CARD8   _merge_video_fifo	    : 1;
	} _misc_control_0_s;
	CARD8	_misc_control_0;
    } _misc_control_0_u;			/* SR7 */

#define misc_control_0	    _misc_control_0_u._misc_control_0
#define external_dac_reference	_misc_control_0_u._misc_control_0_s._external_dac_reference
#define high_speed_dac_0	    _misc_control_0_u._misc_control_0_s._high_speed_dac_0
#define direct_color_24bit	    _misc_control_0_u._misc_control_0_s._direct_color_24bit
#define multi_line_prefetch	    _misc_control_0_u._misc_control_0_s._multi_line_prefetch
#define extended_video_div_2	    _misc_control_0_u._misc_control_0_s._extended_video_div_2
#define ramdac_power_save	    _misc_control_0_u._misc_control_0_s._ramdac_power_save
#define merge_video_fifo	    _misc_control_0_u._misc_control_0_s._merge_video_fifo

    union {
	struct {
	    CARD8   _crt_engine_threshold_high_0_3  : 4;
	    CARD8   _crt_cpu_threshold_low_0_3	    : 4;
	} _crt_cpu_threshold_control_0_s;
	CARD8	_crt_cpu_threshold_control_0;
    } _crt_cpu_threshold_control_0_u;		/* SR8 */
    
#define crt_cpu_threshold_control_0	_crt_cpu_threshold_control_0_u._crt_cpu_threshold_control_0
#define crt_engine_threshold_high_0_3	_crt_cpu_threshold_control_0_u._crt_cpu_threshold_control_0_s._crt_engine_threshold_high_0_3
#define crt_cpu_threshold_low_0_3	_crt_cpu_threshold_control_0_u._crt_cpu_threshold_control_0_s._crt_cpu_threshold_low_0_3

    union {
	struct {
	    CARD8   _crt_cpu_threshold_high_0_3	: 4;
	    CARD8   _ascii_attribute_threshold_0_2 : 3;
	    CARD8   _true_color_32bpp	    : 1;
	} _crt_cpu_threshold_control_1_s;
	CARD8	_crt_cpu_threshold_control_1;
    } _crt_cpu_threshold_control_1_u;		/* SR9 */

#define crt_cpu_threshold_control_1 _crt_cpu_threshold_control_1_u._crt_cpu_threshold_control_1
#define crt_cpu_threshold_high_0_3  _crt_cpu_threshold_control_1_u._crt_cpu_threshold_control_1_s._crt_cpu_threshold_high_0_3
#define ascii_attribute_threshold_0_2	_crt_cpu_threshold_control_1_u._crt_cpu_threshold_control_1_s._ascii_attribute_threshold_0_2
#define true_color_32bpp	    _crt_cpu_threshold_control_1_u._crt_cpu_threshold_control_1_s._true_color_32bpp

    union {
	struct {
	    CARD8   _v_total_10		    : 1;
	    CARD8   _v_display_end_10	    : 1;
	    CARD8   _v_blank_start_10	    : 1;
	    CARD8   _v_retrace_start_10	    : 1;
	    CARD8   _screen_off_8_11	    : 4;
	} _extended_crt_overflow_s;
	CARD8	_extended_crt_overflow;
    } _extended_crt_overflow_u;			    /* SRA */

#define extended_crt_overflow	    _extended_crt_overflow_u._extended_crt_overflow
#define v_total_10		    _extended_crt_overflow_u._extended_crt_overflow_s._v_total_10
#define v_display_end_10	    _extended_crt_overflow_u._extended_crt_overflow_s._v_display_end_10
#define v_blank_start_10	    _extended_crt_overflow_u._extended_crt_overflow_s._v_blank_start_10
#define v_retrace_start_10	    _extended_crt_overflow_u._extended_crt_overflow_s._v_retrace_start_10
#define screen_off_8_11		    _extended_crt_overflow_u._extended_crt_overflow_s._screen_off_8_11

    union {
	struct {
	    CARD8   _cpu_bitblt_enable	    : 1;    /* enable CPU bitblt */
	    CARD8   _packed_16_color_enable : 1;    /* 2 pixels per byte? */
	    CARD8   _io_gating		    : 1;    /* when write buffer not empty */
	    CARD8   _dual_segment_enable    : 1;    /* ? */
	    CARD8   _true_color_modulation  : 1;    /* ? */
	    CARD8   _memory_mapped_mode	    : 2;    /* mmio enable */
	    CARD8   _true_color_order	    : 1;    /* 0: RGB 1: BGR */
	} _misc_control_1_s;
	CARD8   _misc_control_1;	    /* SRB */
    } _misc_control_1_u;
    
#define misc_control_1	_misc_control_1_u._misc_control_1
#define cpu_bitblt_enable   _misc_control_1_u._misc_control_1_s._cpu_bitblt_enable
#define memory_mapped_mode  _misc_control_1_u._misc_control_1_s._memory_mapped_mode
#define true_color_modulation	_misc_control_1_u._misc_control_1_s._true_color_modulation
#define true_color_order    _misc_control_1_u._misc_control_1_s._true_color_order
    
    union {
	struct {
	    CARD8   _sync_reset_enable	    : 1;
	    CARD8   _memory_configuration   : 3;
#define SIS_MEMORY_CONFIG_1M_1BANK  0
#define SIS_MEMORY_CONFIG_2M_2BANK  1
#define SIS_MEMORY_CONFIG_4M_2BANK  2
#define SIS_MEMORY_CONFIG_2M_1BANK  5
#define SIS_MEMORY_CONFIG_4M_1BANK  6
#define SIS_MEMORY_CONFIG_8M_2BANK  7
	    CARD8   _test_mode_enable	    : 1;
	    CARD8   _read_ahead_enable	    : 1;
	    CARD8   _text_mode_16bit_enable : 1;
	    CARD8   _graphics_mode_32bit_enable : 1;
	} _misc_control_2_s;
	CARD8	_misc_control_2;
    } _misc_control_2_u;		    /* SRC */

#define misc_control_2		_misc_control_2_u._misc_control_2
#define sync_reset_enable	_misc_control_2_u._misc_control_2_s._sync_reset_enable
#define memory_configuration	_misc_control_2_u._misc_control_2_s._memory_configuration
#define test_mode_enable	_misc_control_2_u._misc_control_2_s._test_mode_enable
#define read_ahead_enable	_misc_control_2_u._misc_control_2_s._read_ahead_enable
#define text_mode_16bit_enable	_misc_control_2_u._misc_control_2_s._text_mode_16bit_enable
#define graphics_mode_32bit_enable  _misc_control_2_u._misc_control_2_s._graphics_mode_32bit_enable

    union ddc_and_power_control_u {
	struct {
	    CARD8   _ddc_clk_programming    : 1;
	    CARD8   _ddc_data_programming   : 1;
	    CARD8			    : 1;
	    CARD8   _acpi_enable	    : 1;
	    CARD8   _kbd_cursor_activate    : 1;
	    CARD8   _video_memory_activate  : 1;
	    CARD8   _vga_standby	    : 1;
	    CARD8   _vga_suspend	    : 1;
	} _ddc_and_power_control_s;
	CARD8	_ddc_and_power_control;
    } _ddc_and_power_control_u;		    /* SR11 */
    
#define ddc_and_power_control	    _ddc_and_power_control_u._ddc_and_power_control
#define ddc_clk_programming	    _ddc_and_power_control_u._ddc_and_power_control_s._ddc_clk_programming
#define ddc_data_programming	    _ddc_and_power_control_u._ddc_and_power_control_s._ddc_data_programming
#define acpi_enable		    _ddc_and_power_control_u._ddc_and_power_control_s._acpi_enable
#define kbd_cursor_activate	    _ddc_and_power_control_u._ddc_and_power_control_s._kbd_cursor_activate
#define video_memory_activate	    _ddc_and_power_control_u._ddc_and_power_control_s._video_memory_activate
#define vga_standby		    _ddc_and_power_control_u._ddc_and_power_control_s._vga_standby
#define vga_suspend		    _ddc_and_power_control_u._ddc_and_power_control_s._vga_suspend
    
    union {
	struct {
	    CARD8   _h_total_8		    : 1;
	    CARD8   _h_display_end_8	    : 1;
	    CARD8   _h_blank_start_8	    : 1;
	    CARD8   _h_sync_start_8	    : 1;
	    CARD8   _h_blank_end_6	    : 1;
	    CARD8   _h_retrace_skew	    : 3;
	} _extended_horizontal_overflow_s;
	CARD8	_extended_horizontal_overflow;
    } _extended_horizontal_overflow_u;		    /* SR12 */
#define extended_horizontal_overflow	_extended_horizontal_overflow_u._extended_horizontal_overflow
#define h_total_8			_extended_horizontal_overflow_u._extended_horizontal_overflow_s._h_total_8
#define h_display_end_8			_extended_horizontal_overflow_u._extended_horizontal_overflow_s._h_display_end_8
#define h_blank_start_8			_extended_horizontal_overflow_u._extended_horizontal_overflow_s._h_blank_start_8
#define h_sync_start_8		_extended_horizontal_overflow_u._extended_horizontal_overflow_s._h_sync_start_8
#define h_blank_end_6			_extended_horizontal_overflow_u._extended_horizontal_overflow_s._h_blank_end_6
#define h_retrace_skew			_extended_horizontal_overflow_u._extended_horizontal_overflow_s._h_retrace_skew

    union {
	struct {
	    CARD8			    : 6;
	    CARD8   _vclk_post_scale_2	    : 1;
	    CARD8   _mclk_post_scale_2	    : 1;
	} _extended_clock_generator_s;
	CARD8	_extended_clock_generator;
    } _extended_clock_generator_u;		    /* SR13 */

#define extended_clock_generator	 _extended_clock_generator_u._extended_clock_generator
#define vclk_post_scale_2		 _extended_clock_generator_u._extended_clock_generator_s._vclk_post_scale_2
#define mclk_post_scale_2		 _extended_clock_generator_u._extended_clock_generator_s._mclk_post_scale_2

    CARD8   cursor_0_red;			    /* SR14 */
    CARD8   cursor_0_green;			    /* SR15 */
    CARD8   cursor_0_blue;			    /* SR16 */
    
    CARD8   cursor_1_red;			    /* SR17 */
    CARD8   cursor_1_green;			    /* SR18 */
    CARD8   cursor_1_blue;			    /* SR19 */
    
    CARD8   cursor_h_start_0_7;			    /* SR1A */
    union {
	struct {
	    CARD8   _cursor_h_start_8_11    : 4;
	    CARD8			    : 3;
	    CARD8   _cursor_mmio_enable	    : 1;
	} _cusor_h_start_1_s;
	CARD8	_cursor_h_start_1;
    } _cursor_h_start_1_u;			    /* SR1B */

#define cursor_h_start_1		_cursor_h_start_1_u._cursor_h_start_1
#define cursor_h_start_8_11		_cursor_h_start_1_u._cursor_h_start_1_s._cursor_h_start_8_11
    
    CARD8   cursor_h_preset_0_5;		    /* SR1C */
    
    CARD8   cursor_v_start_0_7;			    /* SR1D */
    
    union {
	struct {
	    CARD8   _cursor_v_start_8_10    : 3;
	    CARD8   _cursor_side_pattern    : 1;
	    CARD8   _cursor_pattern	    : 4;
	} _cusor_v_start_1_s;
	CARD8	_cursor_v_start_1;
    } _cursor_v_start_1_u;			    /* SR1E */

#define cursor_v_start_1                _cursor_v_start_1_u._cursor_v_start_1
    
    CARD8   cursor_v_preset_0_5;		    /* SR1F */

    CARD8   linear_base_19_26;			    /* SR20 */

    union {
	struct {
	    CARD8   _linear_base_27_31	    : 5;
	    CARD8   _linear_aperture	    : 3;
#define SIS_LINEAR_APERTURE_512K	0
#define SIS_LINEAR_APERTURE_1M	1
#define SIS_LINEAR_APERTURE_2M	2
#define SIS_LINEAR_APERTURE_4M	3
#define SIS_LINEAR_APERTURE_8M	4
	} _linear_base_1_s;
	CARD8	_linear_base_1;
    } _linear_base_1_u;				    /* SR21 */
    
#define linear_base_1	    _linear_base_1_u._linear_base_1
#define linear_base_27_31   _linear_base_1_u._linear_base_1_s._linear_base_27_31
#define linear_aperture	    _linear_base_1_u._linear_base_1_s._linear_aperture

    union {
	struct {
	    CARD8   _screen_start_addr_20   : 1;
	    CARD8			    : 3;
	    CARD8   _continuous_mem_access  : 1;
	    CARD8			    : 1;
	    CARD8   _power_down_dac	    : 1;
	    CARD8			    : 1;
	} _graphics_engine_0_s;
	CARD8	_graphics_engine_0;
    } _graphics_engine_0_u;			    /* SR26 */

#define graphics_engine_0	_graphics_engine_0_u._graphics_engine_0

    
    union {
	struct {
	    CARD8   _screen_start_addr_16_19: 4;
	    CARD8   _logical_screen_width   : 2;
#define SIS_LOG_SCREEN_WIDTH_1024	0
#define SIS_LOG_SCREEN_WIDTH_2048	1
#define SIS_LOG_SCREEN_WIDTH_4096	2
	    CARD8   _graphics_prog_enable   : 1;
	    CARD8   _turbo_queue_enable	    : 1;
	} _graphics_engine_1_s;
	CARD8	_graphics_engine_1;
    } _graphics_engine_1_u;			    /* SR27 */

#define graphics_engine_1	_graphics_engine_1_u._graphics_engine_1
#define screen_start_addr_16_19	_graphics_engine_1_u._graphics_engine_1_s._screen_start_addr_16_19
#define logical_screen_width	_graphics_engine_1_u._graphics_engine_1_s._logical_screen_width
#define graphics_prog_enable	_graphics_engine_1_u._graphics_engine_1_s._graphics_prog_enable
#define turbo_queue_enable	_graphics_engine_1_u._graphics_engine_1_s._turbo_queue_enable


    union {
	struct {
	    CARD8   _mclk_numerator	    : 7;
	    CARD8   _mclk_divide_by_2	    : 1;
	} _internal_mclk_0_s;
	CARD8	_internal_mclk_0;
    } _internal_mclk_0_u;			    /* SR28 */

#define internal_mclk_0	    _internal_mclk_0_u._internal_mclk_0
#define mclk_numerator	    _internal_mclk_0_u._internal_mclk_0_s._mclk_numerator
#define mclk_divide_by_2    _internal_mclk_0_u._internal_mclk_0_s._mclk_divide_by_2
    
    union {
	struct {
	    CARD8   _mclk_denominator	    : 5;
	    CARD8   _mclk_post_scale	    : 2;
#define SIS_MCLK_POST_SCALE_1	    0
#define SIS_MCLK_POST_SCALE_2	    1
#define SIS_MCLK_POST_SCALE_3	    2
#define SIS_MCLK_POST_SCALE_4	    3
	    CARD8   _mclk_vco_gain	    : 1;
	} _internal_mclk_1_s;
	CARD8	_internal_mclk_1;
    } _internal_mclk_1_u;		    /* SR29 */

#define internal_mclk_1	    _internal_mclk_1_u._internal_mclk_1
#define mclk_denominator    _internal_mclk_1_u._internal_mclk_1_s._mclk_denominator
#define mclk_post_scale	    _internal_mclk_1_u._internal_mclk_1_s._mclk_post_scale
#define mclk_vco_gain	    _internal_mclk_1_u._internal_mclk_1_s._mclk_vco_gain
    
    union {
	struct {
	    CARD8   _vclk_numerator	    : 7;
	    CARD8   _vclk_divide_by_2	    : 1;
	} _internal_vclk_0_s;
	CARD8	_internal_vclk_0;
    } _internal_vclk_0_u;		    /* SR2A */

#define internal_vclk_0	    _internal_vclk_0_u._internal_vclk_0
#define vclk_numerator	    _internal_vclk_0_u._internal_vclk_0_s._vclk_numerator
#define vclk_divide_by_2    _internal_vclk_0_u._internal_vclk_0_s._vclk_divide_by_2
    
    union {
	struct {
	    CARD8   _vclk_denominator	    : 5;
	    CARD8   _vclk_post_scale	    : 2;
#define SIS_VCLK_POST_SCALE_1	    0
#define SIS_VCLK_POST_SCALE_2	    1
#define SIS_VCLK_POST_SCALE_3	    2
#define SIS_VCLK_POST_SCALE_4	    3
	    CARD8   _vclk_vco_gain	    : 1;
	} _internal_vclk_1_s;
	CARD8	_internal_vclk_1;
    } _internal_vclk_1_u;		    /* SR2B */

#define internal_vclk_1	    _internal_vclk_1_u._internal_vclk_1
#define vclk_denominator    _internal_vclk_1_u._internal_vclk_1_s._vclk_denominator
#define vclk_post_scale	    _internal_vclk_1_u._internal_vclk_1_s._vclk_post_scale
#define vclk_vco_gain	    _internal_vclk_1_u._internal_vclk_1_s._vclk_vco_gain
    
    union {
	struct {
	    CARD8   _extended_clock_select  : 2;
#define SIS_CLOCK_SELECT_INTERNAL   0
#define SIS_CLOCK_SELECT_25MHZ	    1
#define SIS_CLOCK_SELECT_28MHZ	    2
	    CARD8   _disable_line_compare   : 1;
	    CARD8   _disable_pci_read_t_o   : 1;
	    CARD8   _cursor_start_addr_18_21: 4;
	} _misc_control_7_s;
	CARD8	_misc_control_7;
    } _misc_control_7_u;			    /* SR38 */

#define misc_control_7		_misc_control_7_u._misc_control_7
#define extended_clock_select	_misc_control_7_u._misc_control_7_s._extended_clock_select
#define disable_line_compare	_misc_control_7_u._misc_control_7_s._disable_line_compare
#define disable_pci_read_t_o	_misc_control_7_u._misc_control_7_s._disable_pci_read_t_o
#define cursor_start_addr_18_21	_misc_control_7_u._misc_control_7_s._cursor_start_addr_18_21

    union {
	struct {
	    CARD8   _high_speed_dclk	    : 1;
	    CARD8   _sgram_block_write	    : 1;
	    CARD8   _cursor_start_addr_22   : 1;
	    CARD8   _dram_texture_read	    : 1;
	    CARD8   _sgram_16mb		    : 1;
	    CARD8   _agp_signal_delay	    : 2;
	    CARD8   _dclk_off		    : 1;
	} _misc_control_11_s;
	CARD8	_misc_control_11;
    } _misc_control_11_u;			    /* SR3E */

#define misc_control_11		_misc_control_11_u._misc_control_11
#define high_speed_dclk		_misc_control_11_u._misc_control_11_s._high_speed_dclk
#define sgram_block_write	_misc_control_11_u._misc_control_11_s.__sgram_block_write
#define cursor_start_addr_22	_misc_control_11_u._misc_control_11_s._cursor_start_addr_22
#define dram_texture_read	_misc_control_11_u._misc_control_11_s._dram_texture_read
#define sgram_16mb		_misc_control_11_u._misc_control_11_s._sgram_16mb
#define agp_signal_delay	_misc_control_11_u._misc_control_11_s._agp_signal_delay
#define dclk_off		_misc_control_11_u._misc_control_11_s._dclk_off
    
    union {
	struct {
	    CARD8			    : 1;
	    CARD8   _flat_panel_low_enable  : 1;
	    CARD8   _crt_cpu_threshold_low_4: 1;
	    CARD8   _crt_engine_threshold_high_4: 1;
	    CARD8   _crt_cpu_threshold_high_4	: 1;
	    CARD8   _crt_threshold_full_control	: 2;
#define SIS_CRT_32_STAGE_THRESHOLD  0
#define SIS_CRT_64_STAGE_THRESHOLD  1
#define SIS_CRT_63_STAGE_THRESHOLD  2
#define SIS_CRT_256_STAGE_THRESHOLD 3
	    CARD8   _high_speed_dac_1	    : 1;
	} _misc_control_12_s;
	CARD8	_misc_control_12;
    } _misc_control_12_u;			    /* SR3F */
#define misc_control_12		_misc_control_12_u._misc_control_12
#define flat_panel_low_enable	_misc_control_12_u._misc_control_12_s._flat_panel_low_enable
#define crt_cpu_threshold_low_4	_misc_control_12_u._misc_control_12_s._crt_cpu_threshold_low_4
#define crt_engine_threshold_high_4 _misc_control_12_u._misc_control_12_s._crt_engine_threshold_high_4
#define crt_cpu_threshold_high_4    _misc_control_12_u._misc_control_12_s._crt_cpu_threshold_high_4
#define crt_threshold_full_control  _misc_control_12_u._misc_control_12_s._crt_threshold_full_control
#define high_speed_dac_1	    _misc_control_12_u._misc_control_12_s._high_speed_dac_1
    
    /* computed values */
    CARD16	    ge_screen_pitch;
    CARD8	    bits_per_pixel;
    CARD8	    depth;
    CARD8	    double_pixel_mode;
    CARD16	    pixel_width;
} SisCrtc;

#define crtc_v_total(crtc)    ((crtc)->v_total_0_7 | \
			       ((crtc)->v_total_8 << 8) | \
			       ((crtc)->v_total_9 << 9) | \
			       ((crtc)->v_total_10 << 10))
    
#define crtc_set_v_total(crtc,v) { \
    ((crtc))->v_total_0_7 = (v); \
    ((crtc))->v_total_8 = (v) >> 8; \
    ((crtc))->v_total_9 = (v) >> 9; \
    ((crtc))->v_total_10 = (v) >> 10; \
}
    
#define crtc_v_display_end(crtc) ((crtc)->v_display_end_0_7 | \
				  ((crtc)->v_display_end_8 << 8) | \
				  ((crtc)->v_display_end_9 << 9) | \
				  ((crtc)->v_display_end_10 << 10))

#define crtc_set_v_display_end(crtc,v) {\
    ((crtc))->v_display_end_0_7 = (v); \
    ((crtc))->v_display_end_8 = (v) >> 8; \
    ((crtc))->v_display_end_9 = (v) >> 9; \
    ((crtc))->v_display_end_10 = (v) >> 10; \
}

#define crtc_v_retrace_start(crtc)  ((crtc)->v_retrace_start_0_7 | \
				     ((crtc)->v_retrace_start_8 << 8) | \
				     ((crtc)->v_retrace_start_9 << 9) | \
				     ((crtc)->v_retrace_start_10 << 10))

#define crtc_set_v_retrace_start(crtc,v) {\
    ((crtc))->v_retrace_start_0_7 = (v); \
    ((crtc))->v_retrace_start_8 = (v) >> 8; \
    ((crtc))->v_retrace_start_9 = (v) >> 9; \
    ((crtc))->v_retrace_start_10 = (v) >> 10; \
}

#define crtc_v_blank_start(crtc)  ((crtc)->v_blank_start_0_7 | \
				   ((crtc)->v_blank_start_8 << 8) | \
				   ((crtc)->v_blank_start_9 << 9) | \
				   ((crtc)->v_blank_start_10 << 10))

#define crtc_set_v_blank_start(crtc,v) {\
    ((crtc))->v_blank_start_0_7 = (v); \
    ((crtc))->v_blank_start_8 = (v) >> 8; \
    ((crtc))->v_blank_start_9 = (v) >> 9; \
    ((crtc))->v_blank_start_10 = (v) >> 10; \
}

#define crtc_h_total(crtc) ((crtc)->h_total_0_7 | \
			    ((crtc)->h_total_8 << 8))

#define crtc_set_h_total(crtc,v) {\
    ((crtc))->h_total_0_7 = (v); \
    ((crtc))->h_total_8 = (v) >> 8; \
}

#define crtc_h_display_end(crtc) ((crtc)->h_display_end_0_7 | \
				  ((crtc)->h_display_end_8 << 8))

#define crtc_set_h_display_end(crtc,v) {\
    ((crtc))->h_display_end_0_7 = (v); \
    ((crtc))->h_display_end_8 = (v) >> 8; \
}

#define crtc_h_blank_start(crtc) ((crtc)->h_blank_start_0_7 | \
				  ((crtc)->h_blank_start_8 << 8))

#define crtc_set_h_blank_start(crtc,v) {\
    ((crtc))->h_blank_start_0_7 = (v); \
    ((crtc))->h_blank_start_8 = (v) >> 8; \
}

#define crtc_h_blank_end(crtc)  ((crtc)->h_blank_end_0_4 | \
				 ((crtc)->h_blank_end_5 << 5))

#define crtc_set_h_blank_end(crtc,v) {\
    ((crtc))->h_blank_end_0_4 = (v); \
    ((crtc))->h_blank_end_5 = (v) >> 5; \
}

#define crtc_h_sync_start(crtc) ((crtc)->h_sync_start_0_7 | \
				 ((crtc)->h_sync_start_8 << 8))

#define crtc_set_h_sync_start(crtc,v) {\
    ((crtc))->h_sync_start_0_7 = (v); \
    ((crtc))->h_sync_start_8 = (v) >> 8; \
}

#define crtc_h_sync_end(crtc) ((crtc)->h_sync_end_0_4)

#define crtc_set_h_sync_end(crtc,v) {\
    ((crtc))->h_sync_end_0_4 = (v); \
}

#define crtc_screen_off(crtc)    ((crtc)->screen_off_0_7 | \
				  ((crtc)->screen_off_8_11 << 8))

#define crtc_set_screen_off(crtc,v) {\
    ((crtc))->screen_off_0_7 = (v); \
    ((crtc))->screen_off_8_11 = (v) >> 8; \
}

#define crtc_ge_screen_width(crtc) ((crtc)->ge_screen_width_0_1 | \
				    ((crtc)->ge_screen_width_2 << 2))

#define crtc_set_ge_screen_width(crtc,v) { \
    (crtc)->ge_screen_width_0_1 = (v); \
    (crtc)->ge_screen_width_2 = (v) >> 2; \
}

#define crtc_h_start_fifo_fetch(crtc) ((crtc)->h_start_fifo_fetch_0_7 | \
				       ((crtc)->h_start_fifo_fetch_8 << 8))

#define crtc_set_h_start_fifo_fetch(crtc,v) {\
    (crtc)->h_start_fifo_fetch_0_7 = (v); \
    (crtc)->h_start_fifo_fetch_8 = (v) >> 8; \
}

#define crtc_start_address(crtc)    ((crtc)->start_address_0_7 | \
				     ((crtc)->start_address_8_15 << 8) | \
				     ((crtc)->start_address_16_19 << 16))

#define crtc_set_start_address(crtc,v) {\
    (crtc)->start_address_0_7 = (v); \
    (crtc)->start_address_8_15 = (v) >> 8; \
    (crtc)->start_address_16_19 = (v) >> 16; \
}

#define crtc_line_compare(crtc)    ((crtc)->line_compare_0_7 | \
				    ((crtc)->line_compare_8 << 8) | \
				    ((crtc)->line_compare_9 << 9) | \
				    ((crtc)->line_compare_10 << 10))

#define crtc_set_line_compare(crtc,v) { \
    ((crtc))->line_compare_0_7 = (v); \
    ((crtc))->line_compare_8 = (v) >> 8; \
    ((crtc))->line_compare_9 = (v) >> 9; \
    ((crtc))->line_compare_10 = (v) >> 10; \
}

#define crtc_set_cursor_start_addr(crtc,v) { \
    (crtc)->cursor_start_addr_18_21 = (v) >> 18; \
    (crtc)->cursor_start_addr_22 = (v) >> 22; \
}

#define _sisOutb(v,r)	    outb(v,r)
#define _sisInb(r)	    inb(r)

#define SIS_DAC_INDEX_READ  0x47
#define SIS_DAC_INDEX_WRITE 0x48
#define SIS_DAC_DATA	    0x49

#define GetCrtc(sisc,i)	    _sisReadIndexRegister ((sisc)->io_base+0x54,i)
#define PutCrtc(sisc,i,v)   _sisWriteIndexRegister ((sisc)->io_base+0x54,i,v)

#define GetSrtc(sisc,i)	    _sisReadIndexRegister ((sisc)->io_base+0x44,i)
#define PutSrtc(sisc,i,v)   _sisWriteIndexRegister ((sisc)->io_base+0x44,i,v)

#define GetArtc(sisc,i)	    _sisReadArtc ((sisc)->io_base+0x40,i)
#define PutArtc(sisc,i,v)   _sisWriteArtc ((sisc)->io_base+0x40,i,v)

#define GetGrtc(sisc,i)	    _sisReadIndexRegister ((sisc)->io_base+0x4e,i)
#define PutGrtc(sisc,i,v)   _sisWriteIndexRegister ((sisc)->io_base+0x4e,i,v)

#define _sisWaitVRetrace(sisc)

#define LockSis(sisc)
#define UnlockSis(sisc)

typedef struct _sisTiming {
    /* label */
    int		horizontal;
    int		vertical;
    int		rate;
    /* horizontal timing */
    int		hfp;	    /* front porch */
    int		hbp;	    /* back porch */
    int		hblank;	    /* blanking */
    /* vertical timing */
    int		vfp;	    /* front porch */
    int		vbp;	    /* back porch */
    int		vblank;	    /* blanking */
} SisTiming;

#define SIS_TEXT_SAVE	(64*1024)

typedef struct _sisSave {
    CARD8	srb;
    CARD8	sr5;
    SisCrtc	crtc;
    CARD8	text_save[SIS_TEXT_SAVE];
} SisSave;

typedef struct _sisCardInfo {
    SisPtr  sis;
    int	    memory;
    CARD8   *frameBuffer;
    CARD8   *registers;
    VOL32   *cpu_bitblt;
    CARD32  io_base;
    SisSave save;
} SisCardInfo;

typedef struct _sisScreenInfo {
    CARD8	*cursor_base;
    CARD32	cursor_off;
    CARD8	*expand;
    CARD32	expand_off;
    int		expand_len;
    SisCursor	cursor;
} SisScreenInfo;

#define getSisCardInfo(kd)	((SisCardInfo *) ((kd)->card->driver))
#define sisCardInfo(kd)		SisCardInfo *sisc = getSisCardInfo(kd)

#define getSisScreenInfo(kd)	((SisScreenInfo *) ((kd)->screen->driver))
#define sisScreenInfo(kd)	SisScreenInfo *siss = getSisScreenInfo(kd)

Bool	sisCardInit (KdCardInfo *);
Bool	sisScreenInit (KdScreenInfo *);
Bool	sisEnable (ScreenPtr pScreen);
void	sisDisable (ScreenPtr pScreen);
void	sisFini (ScreenPtr pScreen);

Bool	sisCursorInit (ScreenPtr pScreen);
void	sisCursorEnable (ScreenPtr pScreen);
void	sisCursorDisable (ScreenPtr pScreen);
void	sisCursorFini (ScreenPtr pScreen);
void	sisRecolorCursor (ScreenPtr pScreen, int ndef, xColorItem *pdefs);

Bool	sisDrawInit (ScreenPtr pScreen);
void	sisDrawEnable (ScreenPtr pScreen);
void	sisDrawSync (ScreenPtr pScreen);
void	sisDrawDisable (ScreenPtr pScreen);
void	sisDrawFini (ScreenPtr pScreen);

void	sisGetColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs);
void	sisPutColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs);

void	SISInitCard (KdCardAttr *attr);

CARD8	_sisReadIndexRegister (CARD32 base, CARD8 index);
void	_sisWriteIndexRegister (CARD32 base, CARD8 index, CARD8 value);
CARD8	_sisReadArtc (CARD32 base, CARD8 index);
void	_sisWriteArtc (CARD32 base, CARD8 index, CARD8 value);

extern KdCardFuncs  sisFuncs;

/*
 * sisclock.c
 */
void 
sisGetClock (unsigned long clock, SisCrtc *crtc);

void
sisEngThresh (SisCrtc *crtc, unsigned long vclk, int bpp);
    
/*
 * siscurs.c
 */

Bool
sisCursorInit (ScreenPtr pScreen);

void
sisCursorEnable (ScreenPtr pScreen);

void
sisCursorDisable (ScreenPtr pScreen);

void
sisCursorFini (ScreenPtr pScreen);

/* sisdraw.c */
Bool
sisDrawInit (ScreenPtr pScreen);

void
sisDrawEnable (ScreenPtr pScreen);

void
sisDrawDisable (ScreenPtr pScreen);

void
sisDrawFini (ScreenPtr pScreen);

#endif /* _SIS_H_ */

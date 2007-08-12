/*
 * Copyright 1999 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "s3reg.h"

#define CR00 S3_CR+0x00
#define CR01 S3_CR+0x01
#define CR02 S3_CR+0x02
#define CR03 S3_CR+0x03
#define CR04 S3_CR+0x04
#define CR05 S3_CR+0x05
#define CR06 S3_CR+0x06
#define CR07 S3_CR+0x07
#define CR08 S3_CR+0x08
#define CR09 S3_CR+0x09
#define CR0A S3_CR+0x0A
#define CR0B S3_CR+0x0B
#define CR0C S3_CR+0x0C
#define CR0D S3_CR+0x0D
#define CR0E S3_CR+0x0E
#define CR0F S3_CR+0x0F
#define CR10 S3_CR+0x10
#define CR11 S3_CR+0x11
#define CR12 S3_CR+0x12
#define CR13 S3_CR+0x13
#define CR14 S3_CR+0x14
#define CR15 S3_CR+0x15
#define CR16 S3_CR+0x16
#define CR17 S3_CR+0x17
#define CR18 S3_CR+0x18
#define CR19 S3_CR+0x19
#define CR1A S3_CR+0x1A
#define CR1B S3_CR+0x1B
#define CR1C S3_CR+0x1C
#define CR1D S3_CR+0x1D
#define CR1E S3_CR+0x1E
#define CR1F S3_CR+0x1F
#define CR20 S3_CR+0x20
#define CR21 S3_CR+0x21
#define CR22 S3_CR+0x22
#define CR23 S3_CR+0x23
#define CR24 S3_CR+0x24
#define CR25 S3_CR+0x25
#define CR26 S3_CR+0x26
#define CR27 S3_CR+0x27
#define CR28 S3_CR+0x28
#define CR29 S3_CR+0x29
#define CR2A S3_CR+0x2A
#define CR2B S3_CR+0x2B
#define CR2C S3_CR+0x2C
#define CR2D S3_CR+0x2D
#define CR2E S3_CR+0x2E
#define CR2F S3_CR+0x2F
#define CR30 S3_CR+0x30
#define CR31 S3_CR+0x31
#define CR32 S3_CR+0x32
#define CR33 S3_CR+0x33
#define CR34 S3_CR+0x34
#define CR35 S3_CR+0x35
#define CR36 S3_CR+0x36
#define CR37 S3_CR+0x37
#define CR38 S3_CR+0x38
#define CR39 S3_CR+0x39
#define CR3A S3_CR+0x3A
#define CR3B S3_CR+0x3B
#define CR3C S3_CR+0x3C
#define CR3D S3_CR+0x3D
#define CR3E S3_CR+0x3E
#define CR3F S3_CR+0x3F
#define CR40 S3_CR+0x40
#define CR41 S3_CR+0x41
#define CR42 S3_CR+0x42
#define CR43 S3_CR+0x43
#define CR44 S3_CR+0x44
#define CR45 S3_CR+0x45
#define CR46 S3_CR+0x46
#define CR47 S3_CR+0x47
#define CR48 S3_CR+0x48
#define CR49 S3_CR+0x49
#define CR4A S3_CR+0x4A
#define CR4B S3_CR+0x4B
#define CR4C S3_CR+0x4C
#define CR4D S3_CR+0x4D
#define CR4E S3_CR+0x4E
#define CR4F S3_CR+0x4F
#define CR50 S3_CR+0x50
#define CR51 S3_CR+0x51
#define CR52 S3_CR+0x52
#define CR53 S3_CR+0x53
#define CR54 S3_CR+0x54
#define CR55 S3_CR+0x55
#define CR56 S3_CR+0x56
#define CR57 S3_CR+0x57
#define CR58 S3_CR+0x58
#define CR59 S3_CR+0x59
#define CR5A S3_CR+0x5A
#define CR5B S3_CR+0x5B
#define CR5C S3_CR+0x5C
#define CR5D S3_CR+0x5D
#define CR5E S3_CR+0x5E
#define CR5F S3_CR+0x5F
#define CR60 S3_CR+0x60
#define CR61 S3_CR+0x61
#define CR62 S3_CR+0x62
#define CR63 S3_CR+0x63
#define CR64 S3_CR+0x64
#define CR65 S3_CR+0x65
#define CR66 S3_CR+0x66
#define CR67 S3_CR+0x67
#define CR68 S3_CR+0x68
#define CR69 S3_CR+0x69
#define CR6A S3_CR+0x6A
#define CR6B S3_CR+0x6B
#define CR6C S3_CR+0x6C
#define CR6D S3_CR+0x6D
#define CR6E S3_CR+0x6E
#define CR6F S3_CR+0x6F
#define CR70 S3_CR+0x70
#define CR71 S3_CR+0x71
#define CR72 S3_CR+0x72
#define CR73 S3_CR+0x73
#define CR74 S3_CR+0x74
#define CR75 S3_CR+0x75
#define CR76 S3_CR+0x76
#define CR77 S3_CR+0x77
#define CR78 S3_CR+0x78
#define CR79 S3_CR+0x79
#define CR7A S3_CR+0x7A
#define CR7B S3_CR+0x7B
#define CR7C S3_CR+0x7C
#define CR7D S3_CR+0x7D
#define CR7E S3_CR+0x7E
#define CR7F S3_CR+0x7F
#define CR80 S3_CR+0x80
#define CR81 S3_CR+0x81
#define CR82 S3_CR+0x82
#define CR83 S3_CR+0x83
#define CR84 S3_CR+0x84
#define CR85 S3_CR+0x85
#define CR86 S3_CR+0x86
#define CR87 S3_CR+0x87
#define CR88 S3_CR+0x88
#define CR89 S3_CR+0x89
#define CR8A S3_CR+0x8A
#define CR8B S3_CR+0x8B
#define CR8C S3_CR+0x8C
#define CR8D S3_CR+0x8D
#define CR8E S3_CR+0x8E
#define CR8F S3_CR+0x8F
#define CR90 S3_CR+0x90
#define CR91 S3_CR+0x91
#define CR92 S3_CR+0x92
#define CR93 S3_CR+0x93
#define CR94 S3_CR+0x94
#define CR95 S3_CR+0x95
#define CR96 S3_CR+0x96
#define CR97 S3_CR+0x97
#define CR98 S3_CR+0x98
#define CR99 S3_CR+0x99
#define CR9A S3_CR+0x9A
#define CR9B S3_CR+0x9B
#define CR9C S3_CR+0x9C
#define CR9D S3_CR+0x9D
#define CR9E S3_CR+0x9E
#define CR9F S3_CR+0x9F
#define CRA0 S3_CR+0xA0
#define CRA1 S3_CR+0xA1
#define CRA2 S3_CR+0xA2
#define CRA3 S3_CR+0xA3
#define CRA4 S3_CR+0xA4
#define CRA5 S3_CR+0xA5
#define CRA6 S3_CR+0xA6
#define CRA7 S3_CR+0xA7
#define CRA8 S3_CR+0xA8
#define CRA9 S3_CR+0xA9
#define CRAA S3_CR+0xAA
#define CRAB S3_CR+0xAB
#define CRAC S3_CR+0xAC
#define CRAD S3_CR+0xAD
#define CRAE S3_CR+0xAE
#define CRAF S3_CR+0xAF
#define CRB0 S3_CR+0xB0
#define CRB1 S3_CR+0xB1
#define CRB2 S3_CR+0xB2
#define CRB3 S3_CR+0xB3
#define CRB4 S3_CR+0xB4
#define CRB5 S3_CR+0xB5
#define CRB6 S3_CR+0xB6
#define CRB7 S3_CR+0xB7
#define CRB8 S3_CR+0xB8
#define CRB9 S3_CR+0xB9
#define CRBA S3_CR+0xBA
#define CRBB S3_CR+0xBB
#define CRBC S3_CR+0xBC
#define CRBD S3_CR+0xBD
#define CRBE S3_CR+0xBE
#define CRBF S3_CR+0xBF

#define CR_FIRST	CR00

VgaReg	s3_h_total[] = {
    CR00, 0, 8,
    CR5D, 0, 1,
    CR5F, 0, 2,
    VGA_REG_END
};

VgaReg	s3_h_display_end[] = {
    CR01, 0, 8,
    CR5D, 1, 1,
    CR5F, 2, 2,
    VGA_REG_END
};

VgaReg s3_h_blank_start[] = {
    CR02, 0, 8,
    CR5D, 2, 1,
    CR5F, 4, 2, 
    VGA_REG_END
};

VgaReg s3_h_blank_end[] = {
    CR03, 0, 5,
    CR05, 7, 1,
    VGA_REG_END
};

VgaReg s3_display_skew[] = {
    CR03, 5, 2,
    VGA_REG_END
};

VgaReg s3_h_sync_start[] = {
    CR04, 0, 8,
    CR5D, 4, 1,
    CR5F, 6, 2,
    VGA_REG_END
};

VgaReg s3_h_sync_end[] = {
    CR05, 0, 5,
    VGA_REG_END
};

VgaReg s3_h_skew[] = {
    CR05, 5, 2,
    VGA_REG_END
};

VgaReg	s3_v_total[] = {
    CR06, 0, 8,
    CR07, 0, 1,
    CR07, 5, 1,
    CR5E, 0, 1,
    VGA_REG_END
};

VgaReg s3_preset_row_scan[] = {
    CR08, 0, 8,
    VGA_REG_END
};

VgaReg s3_max_scan_line[] = {
    CR09, 0, 5,
    VGA_REG_END
};

VgaReg s3_start_address[] = {
    CR0D, 0, 8,
    CR0C, 0, 8,
    CR69, 0, 7,
    VGA_REG_END
};

VgaReg s3_v_retrace_start[] = {
    CR10, 0, 8,
    CR07, 2, 1,
    CR07, 7, 1,
    CR5E, 4, 1,
    VGA_REG_END
};

VgaReg s3_v_retrace_end[] = {
    CR11, 0, 4,
    VGA_REG_END
};

VgaReg s3_clear_v_retrace_int[] = {
    CR11, 4, 1,
    VGA_REG_END
};

VgaReg s3_disable_v_retrace_int[] = {
    CR11, 5, 1,
    VGA_REG_END
};

VgaReg s3_lock_crtc[] = {
    CR11, 7, 1,
    VGA_REG_END
};

VgaReg	s3_v_display_end[] = {
    CR12, 0, 8,
    CR07, 1, 1,
    CR07, 6, 1,
    CR5E, 1, 1,
    VGA_REG_END
};

VgaReg s3_screen_offset[] = {
    CR13, 0, 8,
    CR51, 4, 2,
    VGA_REG_END
};

VgaReg s3_count_by_4_mode[] = {
    CR14, 5, 1,
    VGA_REG_END
};

VgaReg s3_doubleword_mode[] = {
    CR14, 6, 1,
    VGA_REG_END
};

VgaReg s3_v_blank_start[] = {
    CR15, 0, 8,
    CR07, 3, 1,
    CR09, 5, 1,
    CR5E, 2, 1,
    VGA_REG_END
};

VgaReg s3_v_blank_end[] = {
    CR16, 0, 8,
    VGA_REG_END
};

VgaReg s3_2bk_cga[] = {
    CR17, 0, 1,
    VGA_REG_END
};

VgaReg s3_4bk_hga[] = {
    CR17, 1, 1,
    VGA_REG_END
};

VgaReg s3_v_total_double[] = {
    CR17, 2, 1,
    VGA_REG_END
};

VgaReg s3_word_mode[] = {
    CR17, 3, 1,
    VGA_REG_END
};

VgaReg s3_address_16k_wrap[] = {
    CR17, 5, 1,
    VGA_REG_END
};

VgaReg s3_byte_mode[] = {
    CR17, 6, 1,
    VGA_REG_END
};

VgaReg s3_hardware_reset[] = {
    CR17, 7, 1,
    VGA_REG_END
};

VgaReg s3_line_compare[] = {
    CR18, 0, 8,
    CR07, 4, 1,
    CR09, 6, 1,
    CR5E, 6, 1,
    VGA_REG_END
};

VgaReg s3_delay_primary_load[] = {
    CR21, 1, 1,
    VGA_REG_END
};

VgaReg s3_device_id[] = {
    CR2E, 0, 8,
    CR2D, 0, 8,
    VGA_REG_END
};

VgaReg s3_revision[] = {
    CR2F, 0, 8,
    VGA_REG_END
};

VgaReg s3_enable_vga_16bit[] = {
    CR31, 2, 1,
    VGA_REG_END
};

VgaReg s3_enhanced_memory_mapping[] = {
    CR31, 3, 1,
    VGA_REG_END
};

VgaReg s3_lock_dac_writes[] = {
    CR33, 4, 1,
    VGA_REG_END
};

VgaReg s3_border_select[] = {
    CR33, 5, 1,
    VGA_REG_END
};
    
VgaReg s3_lock_palette[] = {
    CR33, 6, 1,
    VGA_REG_END
};

VgaReg s3_enable_sff[] = {
    CR34, 4, 1,
    VGA_REG_END
};

VgaReg s3_lock_vert[] = {
    CR35, 4, 1,
    VGA_REG_END
};

VgaReg s3_lock_horz[] = {
    CR35, 5, 1,
    VGA_REG_END
};

VgaReg s3_io_disable[] = {
    CR36, 4, 1,
    VGA_REG_END
};

VgaReg s3_mem_size[] = {
    CR36, 5, 3,
    VGA_REG_END
};

VgaReg s3_register_lock_1 [] = {
    CR38, 0, 8,	    /* load with 0x48 */
    VGA_REG_END
};

VgaReg s3_register_lock_2 [] = {
    CR39, 0, 8,	    /* load with 0xa0 */
    VGA_REG_END
};

VgaReg s3_refresh_control[] = {
    CR3A, 0, 2,
    VGA_REG_END
};

VgaReg s3_enable_256[] = {
    CR3A, 4, 1,
    VGA_REG_END
};

VgaReg s3_disable_pci_read_bursts[] = {
    CR3A, 7, 1,
    VGA_REG_END
};

VgaReg s3_h_start_fifo_fetch[] = {
    CR3B, 0, 8,
    CR5D, 6, 1,
    CR5B, 2, 2,
    VGA_REG_END
};

VgaReg s3_enable_2d_access[] = {
    CR40, 0, 1,
    VGA_REG_END
};

VgaReg s3_interlace[] = {
    CR42, 5, 1,
    VGA_REG_END
};

VgaReg s3_old_screen_off_8[] = {
    CR43, 2, 1,
    VGA_REG_END
};

VgaReg s3_h_counter_double_mode[] = {
    CR43, 7, 1,
    VGA_REG_END
};

VgaReg s3_cursor_enable[] = {
    CR45, 0, 1,
    VGA_REG_END
};

VgaReg s3_cursor_right[] = {
    CR45, 4, 1,
    VGA_REG_END
};

VgaReg s3_cursor_xhigh[] = {
    CR46, 0, 3,
    VGA_REG_END
};

VgaReg s3_cursor_xlow[] = {
    CR47, 0, 8,
    VGA_REG_END
};

VgaReg s3_cursor_yhigh[] = {
    CR48, 0, 3,
    VGA_REG_END
};

VgaReg s3_cursor_ylow[] = {
    CR49, 0, 8,
    VGA_REG_END
};

VgaReg s3_cursor_fg[] = {
    CR4A, 0, 8,
    VGA_REG_END
};

VgaReg s3_cursor_bg[] = {
    CR4B, 0, 8,
    VGA_REG_END
};

VgaReg s3_cursor_address[] = {
    CR4D, 0, 8,
    CR4C, 0, 8,
    VGA_REG_END
};

VgaReg s3_cursor_xoff[] = {
    CR4E, 0, 6,
    VGA_REG_END
};

VgaReg s3_cursor_yoff[] = {
    CR4F, 0, 6,
    VGA_REG_END
};

VgaReg s3_ge_screen_width[] = {
    CR50, 6, 2,
    CR50, 0, 1,
    VGA_REG_END
};

VgaReg s3_pixel_length[] = {
    CR50, 4, 2,
    VGA_REG_END
};

VgaReg s3_big_endian_linear[] = {
    CR53, 1, 2,
    VGA_REG_END
};

VgaReg s3_mmio_select[] = {
    CR53, 3, 2,
    VGA_REG_END
};

VgaReg s3_mmio_window[] = {
    CR53, 5, 1,
    VGA_REG_END
};

VgaReg s3_swap_nibbles[] = {
    CR53, 6, 1,
    VGA_REG_END
};

VgaReg s3_cursor_ms_x11[] = {
    CR55, 4, 1,
    VGA_REG_END
};

VgaReg s3_linear_window_size[] = {
    CR58, 0, 2,
    VGA_REG_END
};

VgaReg s3_enable_linear[] = {
    CR58, 4, 1,
    VGA_REG_END
};

VgaReg s3_h_blank_extend[] = {
    CR5D, 3, 1,
    VGA_REG_END
};

VgaReg s3_h_sync_extend[] = {
    CR5D, 5, 1,
    VGA_REG_END
};

VgaReg s3_sdclk_skew[] = {
    CR60, 0, 4,
    VGA_REG_END
};

VgaReg s3_delay_blank[] = {
    CR65, 3, 2,
    VGA_REG_END
};

VgaReg s3_delay_h_enable[] = {
    CR65, 6, 2,
    CR65, 0, 1,
    VGA_REG_END
};

VgaReg s3_enable_2d_3d[] = {
    CR66, 0, 1,
    VGA_REG_END
};

VgaReg s3_pci_disconnect_enable[] = {
    CR66, 3, 1,
    VGA_REG_END
};

VgaReg s3_primary_load_control[] = {
    CR66, 4, 1,
    VGA_REG_END
};

VgaReg s3_secondary_load_control[] = {
    CR66, 5, 1,
    VGA_REG_END
};

VgaReg s3_pci_retry_enable[] = {
    CR66, 7, 1,
    VGA_REG_END
};

VgaReg s3_streams_mode[] = {
    CR67, 2, 2,
    VGA_REG_END
};

VgaReg s3_color_mode[] = {
    CR67, 4, 4,
    VGA_REG_END
};

VgaReg s3_primary_stream_definition[] = {
    CR69, 7, 1,
    VGA_REG_END
};

VgaReg s3_primary_stream_timeout[] = {
    CR71, 0, 8,
    VGA_REG_END
};

VgaReg s3_master_control_unit_timeout[] = {
    CR74, 0, 8,
    VGA_REG_END
};

VgaReg s3_command_buffer_timeout[] = {
    CR75, 0, 8,
    VGA_REG_END
};

VgaReg s3_lpb_timeout[] = {
    CR76, 0, 8,
    VGA_REG_END
};

VgaReg s3_cpu_timeout[] = {
    CR78, 0, 8,
    VGA_REG_END
};

VgaReg s3_2d_graphics_engine_timeout[] = {
    CR79, 0, 8,
    VGA_REG_END
};

VgaReg s3_fifo_drain_delay[] = {
    CR85, 0, 3,
    VGA_REG_END
};

VgaReg s3_fifo_fetch_timing[] = {
    CR85, 4, 1,
    VGA_REG_END
};

VgaReg s3_dac_power_up_time[] = {
    CR86, 0, 7,
    VGA_REG_END
};

VgaReg s3_dac_power_saving_disable[] = {
    CR86, 7, 1,
    VGA_REG_END
};

VgaReg s3_flat_panel_output_control_1[] = {
    CR90, 3, 1,
    VGA_REG_END
};

VgaReg s3_streams_fifo_delay[] = {
    CR90, 4, 2,
    VGA_REG_END
};

VgaReg s3_flat_panel_output_control_2[] = {
    CR90, 6, 1,
    VGA_REG_END
};

VgaReg s3_enable_l1_parameter[] = {
    CR90, 7, 1,
    VGA_REG_END
};

VgaReg s3_primary_stream_l1[] = {
    CR91, 0, 8,
    CR90, 0, 3,
    VGA_REG_END
};

#define CR_LAST	CR91

#define SR00 S3_SR+0x00
#define SR01 S3_SR+0x01
#define SR02 S3_SR+0x02
#define SR03 S3_SR+0x03
#define SR04 S3_SR+0x04
#define SR05 S3_SR+0x05
#define SR06 S3_SR+0x06
#define SR07 S3_SR+0x07
#define SR08 S3_SR+0x08
#define SR09 S3_SR+0x09
#define SR0A S3_SR+0x0A
#define SR0B S3_SR+0x0B
#define SR0C S3_SR+0x0C
#define SR0D S3_SR+0x0D
#define SR0E S3_SR+0x0E
#define SR0F S3_SR+0x0F
#define SR10 S3_SR+0x10
#define SR11 S3_SR+0x11
#define SR12 S3_SR+0x12
#define SR13 S3_SR+0x13
#define SR14 S3_SR+0x14
#define SR15 S3_SR+0x15
#define SR16 S3_SR+0x16
#define SR17 S3_SR+0x17
#define SR18 S3_SR+0x18
#define SR19 S3_SR+0x19
#define SR1A S3_SR+0x1A
#define SR1B S3_SR+0x1B
#define SR1C S3_SR+0x1C
#define SR1D S3_SR+0x1D
#define SR1E S3_SR+0x1E
#define SR1F S3_SR+0x1F
#define SR20 S3_SR+0x20
#define SR21 S3_SR+0x21
#define SR22 S3_SR+0x22
#define SR23 S3_SR+0x23
#define SR24 S3_SR+0x24
#define SR25 S3_SR+0x25
#define SR26 S3_SR+0x26
#define SR27 S3_SR+0x27
#define SR28 S3_SR+0x28
#define SR29 S3_SR+0x29
#define SR2A S3_SR+0x2A
#define SR2B S3_SR+0x2B
#define SR2C S3_SR+0x2C
#define SR2D S3_SR+0x2D
#define SR2E S3_SR+0x2E
#define SR2F S3_SR+0x2F
#define SR30 S3_SR+0x30
#define SR31 S3_SR+0x31
#define SR32 S3_SR+0x32
#define SR33 S3_SR+0x33
#define SR34 S3_SR+0x34
#define SR35 S3_SR+0x35
#define SR36 S3_SR+0x36
#define SR37 S3_SR+0x37
#define SR38 S3_SR+0x38
#define SR39 S3_SR+0x39
#define SR3A S3_SR+0x3A
#define SR3B S3_SR+0x3B
#define SR3C S3_SR+0x3C
#define SR3D S3_SR+0x3D
#define SR3E S3_SR+0x3E
#define SR3F S3_SR+0x3F
#define SR40 S3_SR+0x40
#define SR41 S3_SR+0x41
#define SR42 S3_SR+0x42
#define SR43 S3_SR+0x43
#define SR44 S3_SR+0x44
#define SR45 S3_SR+0x45
#define SR46 S3_SR+0x46
#define SR47 S3_SR+0x47
#define SR48 S3_SR+0x48
#define SR49 S3_SR+0x49
#define SR4A S3_SR+0x4A
#define SR4B S3_SR+0x4B
#define SR4C S3_SR+0x4C
#define SR4D S3_SR+0x4D
#define SR4E S3_SR+0x4E
#define SR4F S3_SR+0x4F
#define SR50 S3_SR+0x50
#define SR51 S3_SR+0x51
#define SR52 S3_SR+0x52
#define SR53 S3_SR+0x53
#define SR54 S3_SR+0x54
#define SR55 S3_SR+0x55
#define SR56 S3_SR+0x56
#define SR57 S3_SR+0x57
#define SR58 S3_SR+0x58
#define SR59 S3_SR+0x59
#define SR5A S3_SR+0x5A
#define SR5B S3_SR+0x5B
#define SR5C S3_SR+0x5C
#define SR5D S3_SR+0x5D
#define SR5E S3_SR+0x5E
#define SR5F S3_SR+0x5F
#define SR60 S3_SR+0x60
#define SR61 S3_SR+0x61
#define SR62 S3_SR+0x62
#define SR63 S3_SR+0x63
#define SR64 S3_SR+0x64
#define SR65 S3_SR+0x65
#define SR66 S3_SR+0x66
#define SR67 S3_SR+0x67
#define SR68 S3_SR+0x68
#define SR69 S3_SR+0x69
#define SR6A S3_SR+0x6A
#define SR6B S3_SR+0x6B
#define SR6C S3_SR+0x6C
#define SR6D S3_SR+0x6D
#define SR6E S3_SR+0x6E
#define SR6F S3_SR+0x6F

#define SR_FIRST    SR02

VgaReg s3_dot_clock_8[] = {
    SR01, 0, 1,
    VGA_REG_END
};

VgaReg s3_screen_off[] = {
    SR01, 5, 1,
    VGA_REG_END
};

VgaReg s3_enable_write_plane[] = {
    SR02, 0, 4,
    VGA_REG_END
};

VgaReg s3_extended_memory_access[] = {
    SR04, 1, 1,
    VGA_REG_END
};

VgaReg s3_sequential_addressing_mode[] = {
    SR04, 2, 1,
    VGA_REG_END
};

VgaReg s3_select_chain_4_mode[] = {
    SR04, 3, 1,
    VGA_REG_END
};

VgaReg s3_unlock_extended_sequencer[] = {
    SR08, 0, 8,	/* write 0x06 */
    VGA_REG_END
};

VgaReg s3_linear_addressing_control[] = {
    SR09, 0, 1,
    VGA_REG_END
};

VgaReg s3_disable_io_ports[] = {
    SR09, 7, 1,
    VGA_REG_END
};

VgaReg s3_hsync_control[] = {
    SR0D, 4, 2,
    VGA_REG_END
};

VgaReg s3_vsync_control[] = {
    SR0D, 6, 2,
    VGA_REG_END
};

VgaReg s3_mclk_n[] = {
    SR10, 0, 5,
    VGA_REG_END
};

VgaReg s3_mclk_r[] = {
    SR10, 5, 2,
    VGA_REG_END
};

VgaReg s3_mclk_m[] = {
    SR11, 0, 7,
    VGA_REG_END
};

VgaReg s3_dclk_n[] = {
    SR12, 0, 6,
    SR29, 4, 1,
    VGA_REG_END
};

VgaReg s3_dclk_r[] = {
    SR12, 6, 2,
    SR29, 2, 1,
    VGA_REG_END
};

VgaReg s3_dclk_m[] = {
    SR13, 0, 8,
    SR29, 3, 1,
    VGA_REG_END
};

VgaReg s3_mclk_load[] = {
    SR15, 0, 1,
    VGA_REG_END
};

VgaReg s3_dclk_load[] = {
    SR15, 1, 1,
    VGA_REG_END
};

VgaReg s3_dclk_over_2[] = {
    SR15, 4, 1,
    VGA_REG_END
};

VgaReg s3_clock_load_imm[] = {
    SR15, 5, 1,
    VGA_REG_END
};

VgaReg s3_dclk_invert[] = {
    SR15, 6, 1,
    VGA_REG_END
};

VgaReg s3_enable_clock_double[] = {
    SR18, 7, 1,
    VGA_REG_END
};

VgaReg s3_dclk_double_15_16_invert[] = {
    SR1A, 0, 1,
    VGA_REG_END
};

VgaReg s3_enable_gamma_correction[] = {
    SR1B, 3, 1,
    VGA_REG_END
};

VgaReg s3_enable_8_bit_luts[] = {
    SR1B, 4, 1,
    VGA_REG_END
};

VgaReg s3_dclk_control[] = {
    SR1B, 7, 1,
    VGA_REG_END
};

VgaReg s3_eclk_n[] = {
    SR32, 0, 5,
    VGA_REG_END
};

VgaReg s3_eclk_r[] = {
    SR32, 5, 2,
    VGA_REG_END
};

VgaReg s3_eclk_m[] = {
    SR32, 0, 5,
    VGA_REG_END
};

VgaReg s3_vga_dclk_n[] = {
    SR36, 0, 6,
    SR39, 4, 1,
    VGA_REG_END
};

VgaReg s3_vga_dclk_r[] = {
    SR36, 6, 2,
    SR39, 2, 1,
    VGA_REG_END
};

VgaReg s3_vga_dclk_m1[] = {
    SR37, 0, 8,
    SR39, 3, 1,
    VGA_REG_END
};

VgaReg s3_vga_dclk_m2[] = {
    SR38, 0, 8,
    SR39, 3, 1,
    VGA_REG_END
};

VgaReg s3_vga_clk_select[] = {
    SR39, 0, 1,
    VGA_REG_END
};

#define SR_LAST	    SR39

#define AR00	(S3_AR+0x00)
#define AR10	(S3_AR+0x10)
#define AR11	(S3_AR+0x11)
#define AR12	(S3_AR+0x12)
#define AR13	(S3_AR+0x13)
#define AR14	(S3_AR+0x14)

#define AR_FIRST    AR00

VgaReg s3_select_graphics_mode[] = {
    AR10, 0, 1,
    VGA_REG_END
};

VgaReg s3_enable_blinking[] = {
    AR10, 3, 1,
    VGA_REG_END
};

VgaReg s3_border_color[] = {
    AR11, 0, 8,
    VGA_REG_END
};

#define AR_LAST	    AR11

VgaReg s3_io_addr_select[] = {
    S3_MISC_OUT, 0, 1,
    VGA_REG_END
};

VgaReg s3_enable_ram[] = {
    S3_MISC_OUT, 1, 1,
    VGA_REG_END
};

VgaReg s3_clock_select[] = {
    S3_MISC_OUT, 2, 2,
    VGA_REG_END
};

VgaReg s3_horz_sync_neg[] = {
    S3_MISC_OUT, 6, 1,
    VGA_REG_END
};

VgaReg s3_vert_sync_neg[] = {
    S3_MISC_OUT, 7, 1,
    VGA_REG_END
};

VgaReg s3_display_mode_inactive[] = {
    S3_INPUT_STATUS_1,	0, 1,
    VGA_REG_END
};

VgaReg s3_vertical_sync_active[] = {
    S3_INPUT_STATUS_1, 3, 1,
    VGA_REG_END
};

VgaReg s3_dac_mask[] = {
    S3_DAC + 0,	0, 8,
    VGA_REG_END
};

VgaReg s3_dac_read_index[] = {
    S3_DAC + 1, 0, 8,
    VGA_REG_END
};

VgaReg s3_dac_write_index[] = {
    S3_DAC + 2, 0, 8,
    VGA_REG_END
};

VgaReg s3_dac_data[] = {
    S3_DAC + 3, 0, 8,
    VGA_REG_END
};

VGA8
_s3Inb (VgaCard *card, VGA16 port)
{
    VGAVOL8 *reg;
    
    if (card->closure)
	return VgaReadMemb ((VGA32) card->closure + port);
    else
	return VgaInb (port);
}

void
_s3Outb (VgaCard *card, VGA8 value, VGA16 port)
{
    if (card->closure)
	VgaWriteMemb (value, (VGA32) card->closure + port);
    else
	VgaOutb (value, port);
}

void
_s3RegMap (VgaCard *card, VGA16 reg, VgaMap *map, VGABOOL write)
{
    
    if (reg < S3_SR + S3_NSR)
    {
	map->access = VgaAccessIndIo;
	map->port = 0x3c4;
	map->addr = 0;
	map->value = 1;
	map->index = reg - S3_SR;
    }
    else if (reg < S3_GR + S3_NGR)
    {
	map->access = VgaAccessIndIo;
	map->port = 0x3ce;
	map->addr = 0;
	map->value = 1;
	map->index = reg - S3_GR;
    }
    else if (reg < S3_AR + S3_NAR)
    {
	reg -= S3_AR;
	map->access = VgaAccessDone;
	/* reset AFF to index */
	(void) _s3Inb (card, 0x3da);
	if (reg >= 16)
	    reg |= 0x20;
	_s3Outb (card, reg, 0x3c0);
	if (write)
	    _s3Outb (card, map->value, 0x3c0);
	else
	    map->value = _s3Inb (card, 0x3c1);
	if (!(reg & 0x20))
	{
	    /* enable video display again */
	    (void) _s3Inb (card, 0x3da);
	    _s3Outb (card, 0x20, 0x3c0);
	}
	return;
    }
    else if (reg < S3_CR + S3_NCR)
    {
	map->access = VgaAccessIndIo;
	map->port = 0x3d4;
	map->addr = 0;
	map->value = 1;
	map->index = reg - S3_CR;
    }
    else if (reg < S3_DAC + S3_NDAC)
    {
	map->access = VgaAccessIo;
	map->port = 0x3c6 + reg - S3_DAC;
    }
    else switch (reg) {
    case S3_MISC_OUT:
	map->access = VgaAccessIo;
	if (write)
	    map->port = 0x3c2;
	else
	    map->port = 0x3cc;
	break;
    case S3_INPUT_STATUS_1:
	map->access = VgaAccessIo;
	map->port = 0x3da;
	break;
    }
    if (card->closure)
    {
	map->port = map->port + (VGA32) card->closure;
	if (map->access == VgaAccessIo)
	    map->access = VgaAccessMem;
	if (map->access == VgaAccessIndIo)
	    map->access = VgaAccessIndMem;
    }
}

VgaSave	    s3Saves[] = {
    CR_FIRST, CR18,
    CR31, CR_LAST,
    SR_FIRST, SR15,
    SR18, SR_LAST,
    AR_FIRST, AR_LAST,
    S3_MISC_OUT, S3_MISC_OUT,
    VGA_SAVE_END
};

void
s3RegInit (S3Vga *s3vga, VGAVOL8 *mmio)
{
    s3vga->card.map = _s3RegMap;
    s3vga->card.closure = (void *) mmio;
    s3vga->card.max = S3_NREG;
    s3vga->card.values = s3vga->values;
    s3vga->card.saves = s3Saves;
}

void
s3Save (S3Vga *s3vga)
{
    s3vga->save_lock_crtc = s3Get(s3vga, s3_lock_crtc);
    s3SetImm (s3vga, s3_lock_crtc, 0);
    s3vga->save_register_lock_1 = s3Get (s3vga, s3_register_lock_1);
    s3SetImm (s3vga, s3_register_lock_1, 0x48);
    s3vga->save_register_lock_2 = s3Get (s3vga, s3_register_lock_2);
    s3SetImm (s3vga, s3_register_lock_2, 0xa5);
    s3vga->save_unlock_extended_sequencer = s3Get (s3vga, s3_unlock_extended_sequencer);
    s3SetImm (s3vga, s3_unlock_extended_sequencer, 0x06);
    s3vga->save_lock_horz = s3Get (s3vga, s3_lock_horz);
    s3SetImm (s3vga, s3_lock_horz, 0);
    s3vga->save_lock_vert = s3Get (s3vga, s3_lock_vert);
    s3SetImm (s3vga, s3_lock_vert, 0);
    s3vga->save_dot_clock_8 = s3Get (s3vga, s3_dot_clock_8);
    VgaPreserve (&s3vga->card);
}

void
s3Reset (S3Vga *s3vga)
{
    VgaRestore (&s3vga->card);
    s3SetImm (s3vga, s3_clock_load_imm, 1);
    s3SetImm (s3vga, s3_clock_load_imm, 0);
    s3SetImm (s3vga, s3_dot_clock_8, s3vga->save_dot_clock_8);
    s3SetImm (s3vga, s3_lock_vert, s3vga->save_lock_vert);
    s3SetImm (s3vga, s3_lock_horz, s3vga->save_lock_horz);
    s3SetImm (s3vga, s3_lock_dac_writes, s3vga->save_lock_dac_writes);
    s3SetImm (s3vga, s3_unlock_extended_sequencer, s3vga->save_unlock_extended_sequencer);
    s3SetImm (s3vga, s3_register_lock_2, s3vga->save_register_lock_2);
    s3SetImm (s3vga, s3_register_lock_1, s3vga->save_register_lock_1);
    s3SetImm (s3vga, s3_lock_crtc, s3vga->save_lock_crtc);
    VgaFinish (&s3vga->card);
}

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
#include <stdio.h>
#include "s3reg.h"

typedef struct {
    VgaReg  *reg;
    char    *name;
} NamedVgaReg;

NamedVgaReg s3VRegs[] = {
    s3_h_total, "h_total",
    s3_h_display_end, "h_display_end",
    s3_h_blank_start, "h_blank_start",
    s3_h_blank_end, "h_blank_end",
    s3_display_skew, "display_skew",
    s3_h_sync_start, "h_sync_start",
    s3_h_sync_end, "h_sync_end",
    s3_h_skew, "h_skew",
    s3_v_total, "v_total",
    s3_preset_row_scan, "preset_row_scan",
    s3_max_scan_line, "max_scan_line",
    s3_start_address, "start_address",
    s3_v_retrace_start, "v_retrace_start",
    s3_v_retrace_end, "v_retrace_end",
    s3_clear_v_retrace_int, "clear_v_retrace_int",
    s3_disable_v_retrace_int, "disable_v_retrace_int",
    s3_lock_crtc, "lock_crtc",
    s3_v_display_end, "v_display_end",
    s3_screen_offset, "screen_offset",
    s3_count_by_4_mode, "count_by_4_mode",
    s3_doubleword_mode, "doubleword_mode",
    s3_v_blank_start, "v_blank_start",
    s3_v_blank_end, "v_blank_end",
    s3_v_total_double, "v_total_double",
    s3_word_mode, "word_mode",
    s3_byte_mode, "byte_mode",
    s3_line_compare, "line_compare",
    s3_device_id, "device_id",
    s3_revision, "revision",
    s3_lock_vert, "lock_vert",
    s3_lock_horz, "lock_horz",
    s3_io_disable, "io_disable",
    s3_mem_size, "mem_size",
    s3_register_lock_1 , "register_lock_1 ",
    s3_register_lock_2 , "register_lock_2 ",
    s3_refresh_control, "refresh_control",
    s3_enable_256, "enable_256",
    s3_enable_pci_read_bursts, "enable_pci_read_bursts",
    s3_h_start_fifo_fetch, "h_start_fifo_fetch",
    s3_interlace, "interlace",
    s3_old_screen_off_8, "old_screen_off_8",
    s3_h_counter_double_mode, "h_counter_double_mode",
    s3_hardware_cursor_enable, "hardware_cursor_enable",
    s3_hardware_cursor_right, "hardware_cursor_right",
    s3_hardware_cursor_x, "hardware_cursor_x",
    s3_hardware_cursor_y, "hardware_cursor_y",
    s3_hardware_cursor_fg, "hardware_cursor_fg",
    s3_cursor_address, "cursor_address",
    s3_cursor_start_x, "cursor_start_x",
    s3_cursor_start_y, "cursor_start_y",
    s3_ge_screen_width, "ge_screen_width",
    s3_pixel_length, "pixel_length",
    s3_big_endian_linear, "big_endian_linear",
    s3_mmio_select, "mmio_select",
    s3_mmio_window, "mmio_window",
    s3_swap_nibbles, "swap_nibbles",
    s3_hardware_cursor_ms_x11, "hardware_cursor_ms_x11",
    s3_h_blank_extend, "h_blank_extend",
    s3_h_sync_extend, "h_sync_extend",
    s3_enable_2d_3d, "enable_2d_3d",
    s3_pci_disconnect_enable, "pci_disconnect_enable",
    s3_pci_retry_enable, "pci_retry_enable",
    s3_color_mode, "color_mode",
    s3_screen_off, "screen_off",
    s3_unlock_extended_sequencer, "unlock_extended_sequencer",
    s3_disable_io_ports, "disable_io_ports",
    s3_hsync_control, "hsync_control",
    s3_vsync_control, "vsync_control",
    s3_mclk_n, "mclk_n",
    s3_mclk_r, "mclk_r",
    s3_mclk_m, "mclk_m",
    s3_dclk_n, "dclk_n",
    s3_dclk_r, "dclk_r",
    s3_dclk_m, "dclk_m",
    s3_mclk_load, "mclk_load",
    s3_dclk_load, "dclk_load",
    s3_dclk_over_2, "dclk_over_2",
    s3_clock_load_imm, "clock_load_imm",
    s3_dclk_invert, "dclk_invert",
    s3_enable_clock_double, "enable_clock_double",
    s3_dclk_double_15_16_invert, "dclk_double_15_16_invert",
    s3_enable_gamma_correction, "enable_gamma_correction",
    s3_enable_8_bit_luts, "enable_8_bit_luts",
    s3_dclk_control, "dclk_control",
    s3_vga_dclk_n, "vga_dclk_n",
    s3_vga_dclk_r, "vga_dclk_r",
    s3_vga_dclk_m1, "vga_dclk_m1",
    s3_vga_dclk_m2, "vga_dclk_m2",
    s3_vga_clk_select, "vga_clk_select",
    s3_clock_select, "clock_select",
};

#define NUM_S3_VREGS (sizeof (s3VRegs)/ sizeof (s3VRegs[0]))

main (int argc, char **argv)
{
    int	i;

    iopl(3);
    s3SetImm(s3_register_lock_1, 0x48);
    s3SetImm(s3_register_lock_2, 0xa0);
    s3SetImm(s3_unlock_extended_sequencer, 0x06);
    for (i = 0; i < NUM_S3_VREGS; i++)
	printf ("%-20.20s %8x\n", s3VRegs[i].name, s3Get (s3VRegs[i].reg));
    s3Restore ();
}

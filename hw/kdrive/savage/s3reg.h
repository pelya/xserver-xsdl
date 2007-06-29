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

#ifndef _S3REG_H_
#define _S3REG_H_

#include "vga.h"

#define S3_SR	0
#define S3_NSR	0x70
#define S3_GR	(S3_SR+S3_NSR)
#define S3_NGR	0x09
#define S3_AR	(S3_GR+S3_NGR)
#define S3_NAR	0x15
#define S3_CR	(S3_AR+S3_NAR)
#define S3_NCR	0xc0
#define S3_DAC	(S3_CR+S3_NCR)
#define S3_NDAC	4
#define S3_MISC_OUT	    (S3_DAC + S3_NDAC)
#define S3_INPUT_STATUS_1   (S3_MISC_OUT+1)
#define S3_NREG		    (S3_INPUT_STATUS_1+1)

extern VgaReg s3_h_total[];
extern VgaReg s3_h_display_end[];
extern VgaReg s3_h_blank_start[];
extern VgaReg s3_h_blank_end[];
extern VgaReg s3_display_skew[];
extern VgaReg s3_h_sync_start[];
extern VgaReg s3_h_sync_end[];
extern VgaReg s3_h_skew[];
extern VgaReg s3_v_total[];
extern VgaReg s3_preset_row_scan[];
extern VgaReg s3_max_scan_line[];
extern VgaReg s3_start_address[];
extern VgaReg s3_v_retrace_start[];
extern VgaReg s3_v_retrace_end[];
extern VgaReg s3_clear_v_retrace_int[];
extern VgaReg s3_disable_v_retrace_int[];
extern VgaReg s3_lock_crtc[];
extern VgaReg s3_v_display_end[];
extern VgaReg s3_screen_offset[];
extern VgaReg s3_count_by_4_mode[];
extern VgaReg s3_doubleword_mode[];
extern VgaReg s3_v_blank_start[];
extern VgaReg s3_v_blank_end[];
extern VgaReg s3_2bk_cga[];
extern VgaReg s3_4bk_hga[];
extern VgaReg s3_v_total_double[];
extern VgaReg s3_address_16k_wrap[];
extern VgaReg s3_word_mode[];
extern VgaReg s3_byte_mode[];
extern VgaReg s3_hardware_reset[];
extern VgaReg s3_line_compare[];
extern VgaReg s3_delay_primary_load[];
extern VgaReg s3_device_id[];
extern VgaReg s3_revision[];
extern VgaReg s3_enable_vga_16bit[];
extern VgaReg s3_enhanced_memory_mapping[];
extern VgaReg s3_enable_sff[];
extern VgaReg s3_lock_dac_writes[];
extern VgaReg s3_border_select[];
extern VgaReg s3_lock_palette[];
extern VgaReg s3_lock_vert[];
extern VgaReg s3_lock_horz[];
extern VgaReg s3_io_disable[];
extern VgaReg s3_mem_size[];
extern VgaReg s3_register_lock_1 [];
extern VgaReg s3_register_lock_2 [];
extern VgaReg s3_refresh_control[];
extern VgaReg s3_enable_256[];
extern VgaReg s3_disable_pci_read_bursts[];
extern VgaReg s3_h_start_fifo_fetch[];
extern VgaReg s3_enable_2d_access[];
extern VgaReg s3_interlace[];
extern VgaReg s3_old_screen_off_8[];
extern VgaReg s3_h_counter_double_mode[];
extern VgaReg s3_cursor_enable[];
extern VgaReg s3_cursor_right[];
extern VgaReg s3_cursor_xhigh[];
extern VgaReg s3_cursor_xlow[];
extern VgaReg s3_cursor_yhigh[];
extern VgaReg s3_cursor_ylow[];
extern VgaReg s3_cursor_fg[];
extern VgaReg s3_cursor_bg[];
extern VgaReg s3_cursor_address[];
extern VgaReg s3_cursor_xoff[];
extern VgaReg s3_cursor_yoff[];
extern VgaReg s3_ge_screen_width[];
extern VgaReg s3_pixel_length[];
extern VgaReg s3_big_endian_linear[];
extern VgaReg s3_mmio_select[];
extern VgaReg s3_mmio_window[];
extern VgaReg s3_swap_nibbles[];
extern VgaReg s3_cursor_ms_x11[];
extern VgaReg s3_linear_window_size[];
extern VgaReg s3_enable_linear[];
extern VgaReg s3_h_blank_extend[];
extern VgaReg s3_h_sync_extend[];
extern VgaReg s3_sdclk_skew[];
extern VgaReg s3_delay_blank[];
extern VgaReg s3_delay_h_enable[];
extern VgaReg s3_enable_2d_3d[];
extern VgaReg s3_pci_disconnect_enable[];
extern VgaReg s3_primary_load_control[];
extern VgaReg s3_secondary_load_control[];
extern VgaReg s3_pci_retry_enable[];
extern VgaReg s3_streams_mode[];
extern VgaReg s3_color_mode[];
extern VgaReg s3_primary_stream_definition[];
extern VgaReg s3_primary_stream_timeout[];
extern VgaReg s3_master_control_unit_timeout[];
extern VgaReg s3_command_buffer_timeout[];
extern VgaReg s3_lpb_timeout[];
extern VgaReg s3_cpu_timeout[];
extern VgaReg s3_2d_graphics_engine_timeout[];
extern VgaReg s3_fifo_drain_delay[];
extern VgaReg s3_fifo_fetch_timing[];
extern VgaReg s3_dac_power_up_time[];
extern VgaReg s3_dac_power_saving_disable[];
extern VgaReg s3_flat_panel_output_control_1[];
extern VgaReg s3_streams_fifo_delay[];
extern VgaReg s3_flat_panel_output_control_2[];
extern VgaReg s3_enable_l1_parameter[];
extern VgaReg s3_primary_stream_l1[];

extern VgaReg s3_dot_clock_8[];
extern VgaReg s3_screen_off[];
extern VgaReg s3_enable_write_plane[];
extern VgaReg s3_extended_memory_access[];
extern VgaReg s3_sequential_addressing_mode[];
extern VgaReg s3_select_chain_4_mode[];

extern VgaReg s3_unlock_extended_sequencer[];
extern VgaReg s3_linear_addressing_control[];
extern VgaReg s3_disable_io_ports[];
extern VgaReg s3_hsync_control[];
extern VgaReg s3_vsync_control[];
extern VgaReg s3_mclk_n[];
extern VgaReg s3_mclk_r[];
extern VgaReg s3_mclk_m[];
extern VgaReg s3_dclk_n[];
extern VgaReg s3_dclk_r[];
extern VgaReg s3_dclk_m[];
extern VgaReg s3_mclk_load[];
extern VgaReg s3_dclk_load[];
extern VgaReg s3_dclk_over_2[];
extern VgaReg s3_clock_load_imm[];
extern VgaReg s3_dclk_invert[];
extern VgaReg s3_enable_clock_double[];
extern VgaReg s3_dclk_double_15_16_invert[];
extern VgaReg s3_enable_gamma_correction[];
extern VgaReg s3_enable_8_bit_luts[];
extern VgaReg s3_dclk_control[];
extern VgaReg s3_eclk_n[];
extern VgaReg s3_eclk_r[];
extern VgaReg s3_eclk_m[];
extern VgaReg s3_vga_dclk_n[];
extern VgaReg s3_vga_dclk_r[];
extern VgaReg s3_vga_dclk_m1[];
extern VgaReg s3_vga_dclk_m2[];
extern VgaReg s3_vga_clk_select[];
extern VgaReg s3_select_graphics_mode[];
extern VgaReg s3_enable_blinking[];
extern VgaReg s3_border_color[];

extern VgaReg s3_io_addr_select[];
extern VgaReg s3_enable_ram[];
extern VgaReg s3_clock_select[];
extern VgaReg s3_horz_sync_neg[];
extern VgaReg s3_vert_sync_neg[];

extern VgaReg s3_display_mode_inactive[];
extern VgaReg s3_vertical_sync_active[];

extern VgaReg s3_dac_mask[];
extern VgaReg s3_dac_read_index[];
extern VgaReg s3_dac_write_index[];
extern VgaReg s3_dac_data[];

#define s3Get(sv,r)	    VgaGet(&(sv)->card, (r))
#define s3GetImm(sv,r)	    VgaGetImm(&(sv)->card, (r))
#define s3Set(sv,r,v)	    VgaSet(&(sv)->card, (r), (v))
#define s3SetImm(sv,r,v)    VgaSetImm(&(sv)->card, (r), (v))

typedef struct _s3Vga {
    VgaCard	card;
    VgaValue	values[S3_NREG];
    VGA32	save_lock_crtc;
    VGA32	save_register_lock_1;
    VGA32	save_register_lock_2;
    VGA32	save_unlock_extended_sequencer;
    VGA32	save_lock_dac_writes;
    VGA32	save_lock_horz;
    VGA32	save_lock_vert;
    VGA32	save_dot_clock_8;
} S3Vga;

void
s3RegInit (S3Vga *s3vga, VGAVOL8 *mmio);

void
s3Save (S3Vga *s3vga);

void
s3Reset (S3Vga *s3vga);

#endif /* _S3REG_H_ */

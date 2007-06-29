/*
 * Copyright © 2000 Keith Packard
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

#ifndef _IGSREG_H_
#define _IGSREG_H_

#include "vga.h"

#define IGS_SR	    0
#define IGS_NSR	    5
#define IGS_GR	    (IGS_SR+IGS_NSR)
#define IGS_NGR	    0xC0
#define IGS_GREX    (IGS_GR+IGS_NGR)
#define IGS_GREXBASE	0x3c
#define IGS_NGREX   1
#define IGS_AR	    (IGS_GREX+IGS_NGREX)
#define IGS_NAR	    0x15
#define IGS_CR	    (IGS_AR+IGS_NAR)
#define IGS_NCR	    0x48
#define IGS_DAC	    (IGS_CR+IGS_NCR)
#define IGS_NDAC    4
#define IGS_DACEX   (IGS_DAC+IGS_NDAC)
#define IGS_NDACEX  4
#define IGS_MISC_OUT	    (IGS_DACEX + IGS_NDACEX)
#define IGS_INPUT_STATUS_1  (IGS_MISC_OUT+1)
#define IGS_NREG	    (IGS_INPUT_STATUS_1+1)

extern VgaReg igs_h_total[];
extern VgaReg igs_h_de_end[];
extern VgaReg igs_h_bstart[];
extern VgaReg igs_h_bend[];
extern VgaReg igs_de_skew[];
extern VgaReg igs_ena_vr_access[];
extern VgaReg igs_h_rstart[];
extern VgaReg igs_h_rend[];
extern VgaReg igs_h_rdelay[];
extern VgaReg igs_v_total[];
extern VgaReg igs_v_rstart[];
extern VgaReg igs_v_rend[];
extern VgaReg igs_clear_v_int[];
extern VgaReg igs_disable_v_int[];
extern VgaReg igs_bandwidth[];
extern VgaReg igs_crt_protect[];
extern VgaReg igs_v_de_end[];
extern VgaReg igs_offset[];
extern VgaReg igs_v_bstart[];
extern VgaReg igs_v_bend[];
extern VgaReg igs_linecomp[];
extern VgaReg igs_ivideo[];
extern VgaReg igs_num_fetch[];
extern VgaReg igs_wcrt0[];
extern VgaReg igs_wcrt1[];
extern VgaReg igs_rcrts1[];
extern VgaReg igs_selwk[];
extern VgaReg igs_dot_clock_8[];
extern VgaReg igs_screen_off[];
extern VgaReg igs_enable_write_plane[];
extern VgaReg igs_mexhsyn[];
extern VgaReg igs_mexvsyn[];
extern VgaReg igs_pci_burst_write[];
extern VgaReg igs_pci_burst_read[];
extern VgaReg	igs_iow_retry[];
extern VgaReg  igs_mw_retry[];
extern VgaReg  igs_mr_retry[];
extern VgaReg	igs_biga22en[];
extern VgaReg	igs_biga24en[];
extern VgaReg	igs_biga22force[];
extern VgaReg	igs_bigswap[];
#define IGS_BIGSWAP_8	0x3f
#define IGS_BIGSWAP_16	0x2a
#define IGS_BIGSWAP_32	0x00
extern VgaReg  igs_sprite_x[];
extern VgaReg  igs_sprite_preset_x[];
extern VgaReg  igs_sprite_y[];
extern VgaReg  igs_sprite_preset_y[];
extern VgaReg	igs_sprite_visible[];
extern VgaReg  igs_sprite_64x64[];
extern VgaReg  igs_mgrext[];
extern VgaReg  igs_hcshf[];
extern VgaReg	igs_mbpfix[];
extern VgaReg igs_overscan_red[];
extern VgaReg igs_overscan_green[];
extern VgaReg igs_overscan_blue[];
extern VgaReg igs_memgopg[];
extern VgaReg igs_memr2wpg[];
extern VgaReg igs_crtff16[];
extern VgaReg igs_fifomust[];
extern VgaReg igs_fifogen[];
extern VgaReg igs_mode_sel[];
#define IGS_MODE_TEXT	0
#define IGS_MODE_8	1
#define IGS_MODE_565	2
#define IGS_MODE_5551	6
#define IGS_MODE_8888	3
#define IGS_MODE_888	4
#define IGS_MODE_332	9
#define IGS_MODE_4444	10
extern VgaReg  igs_sprite_addr[];
extern VgaReg  igs_fastmpie[];
extern VgaReg  igs_vclk_m[];
extern VgaReg  igs_vclk_n[];
extern VgaReg  igs_vfsel[];
extern VgaReg  igs_vclk_p[];
extern VgaReg  igs_frqlat[];
extern VgaReg igs_dac_mask[];
extern VgaReg igs_dac_read_index[];
extern VgaReg igs_dac_write_index[];
extern VgaReg igs_dac_data[];
extern VgaReg	igs_rampwdn[];
extern VgaReg	igs_dac6_8[];
extern VgaReg  igs_ramdacbypass[];
extern VgaReg	igs_dacpwdn[];
extern VgaReg	igs_cursor_read_index[];
extern VgaReg igs_cursor_write_index[];
extern VgaReg igs_cursor_data[];

#define igsGet(sv,r)	    VgaGet(&(sv)->card, (r))
#define igsGetImm(sv,r)	    VgaGetImm(&(sv)->card, (r))
#define igsSet(sv,r,v)	    VgaSet(&(sv)->card, (r), (v))
#define igsSetImm(sv,r,v)    VgaSetImm(&(sv)->card, (r), (v))

typedef struct _igsVga {
    VgaCard	card;
    VgaValue	values[IGS_NREG];
} IgsVga;

void
igsRegInit (IgsVga *igsvga, VGAVOL8 *mmio);

void
igsSave (IgsVga *igsvga);

void
igsReset (IgsVga *igsvga);

#endif /* _IGSREG_H_ */

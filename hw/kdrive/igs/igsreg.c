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

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "igsreg.h"
#include <stdio.h>

#define CR00 IGS_CR+0x00
#define CR01 IGS_CR+0x01
#define CR02 IGS_CR+0x02
#define CR03 IGS_CR+0x03
#define CR04 IGS_CR+0x04
#define CR05 IGS_CR+0x05
#define CR06 IGS_CR+0x06
#define CR07 IGS_CR+0x07
#define CR08 IGS_CR+0x08
#define CR09 IGS_CR+0x09
#define CR0A IGS_CR+0x0A
#define CR0B IGS_CR+0x0B
#define CR0C IGS_CR+0x0C
#define CR0D IGS_CR+0x0D
#define CR0E IGS_CR+0x0E
#define CR0F IGS_CR+0x0F
#define CR10 IGS_CR+0x10
#define CR11 IGS_CR+0x11
#define CR12 IGS_CR+0x12
#define CR13 IGS_CR+0x13
#define CR14 IGS_CR+0x14
#define CR15 IGS_CR+0x15
#define CR16 IGS_CR+0x16
#define CR17 IGS_CR+0x17
#define CR18 IGS_CR+0x18
#define CR19 IGS_CR+0x19
#define CR1A IGS_CR+0x1A
#define CR1B IGS_CR+0x1B
#define CR1C IGS_CR+0x1C
#define CR1D IGS_CR+0x1D
#define CR1E IGS_CR+0x1E
#define CR1F IGS_CR+0x1F
#define CR20 IGS_CR+0x20
#define CR21 IGS_CR+0x21
#define CR22 IGS_CR+0x22
#define CR23 IGS_CR+0x23
#define CR24 IGS_CR+0x24
#define CR25 IGS_CR+0x25
#define CR26 IGS_CR+0x26
#define CR27 IGS_CR+0x27
#define CR28 IGS_CR+0x28
#define CR29 IGS_CR+0x29
#define CR2A IGS_CR+0x2A
#define CR2B IGS_CR+0x2B
#define CR2C IGS_CR+0x2C
#define CR2D IGS_CR+0x2D
#define CR2E IGS_CR+0x2E
#define CR2F IGS_CR+0x2F
#define CR30 IGS_CR+0x30
#define CR31 IGS_CR+0x31
#define CR32 IGS_CR+0x32
#define CR33 IGS_CR+0x33
#define CR34 IGS_CR+0x34
#define CR35 IGS_CR+0x35
#define CR36 IGS_CR+0x36
#define CR37 IGS_CR+0x37
#define CR38 IGS_CR+0x38
#define CR39 IGS_CR+0x39
#define CR3A IGS_CR+0x3A
#define CR3B IGS_CR+0x3B
#define CR3C IGS_CR+0x3C
#define CR3D IGS_CR+0x3D
#define CR3E IGS_CR+0x3E
#define CR3F IGS_CR+0x3F
#define CR40 IGS_CR+0x40
#define CR41 IGS_CR+0x41
#define CR42 IGS_CR+0x42
#define CR43 IGS_CR+0x43
#define CR44 IGS_CR+0x44
#define CR45 IGS_CR+0x45
#define CR46 IGS_CR+0x46
#define CR47 IGS_CR+0x47
#define CR48 IGS_CR+0x48

#define CR_FIRST    CR00
#define CR_LAST	    CR48

#define SR00 IGS_SR+0x00
#define SR01 IGS_SR+0x01
#define SR02 IGS_SR+0x02
#define SR03 IGS_SR+0x03
#define SR04 IGS_SR+0x04

#define SR_FIRST    SR00
#define SR_LAST	    SR04

#define AR00 IGS_AR+0x00
#define AR01 IGS_AR+0x01
#define AR02 IGS_AR+0x02
#define AR03 IGS_AR+0x03
#define AR04 IGS_AR+0x04
#define AR05 IGS_AR+0x05
#define AR06 IGS_AR+0x06
#define AR07 IGS_AR+0x07
#define AR08 IGS_AR+0x08
#define AR09 IGS_AR+0x09
#define AR0A IGS_AR+0x0A
#define AR0B IGS_AR+0x0B
#define AR0C IGS_AR+0x0C
#define AR0D IGS_AR+0x0D
#define AR0E IGS_AR+0x0E
#define AR0F IGS_AR+0x0F
#define AR10 IGS_AR+0x10
#define AR11 IGS_AR+0x11
#define AR12 IGS_AR+0x12
#define AR13 IGS_AR+0x13
#define AR14 IGS_AR+0x14

#define AR_FIRST    AR00
#define AR_LAST	    AR14

#define GR00 IGS_GR+0x00
#define GR01 IGS_GR+0x01
#define GR02 IGS_GR+0x02
#define GR03 IGS_GR+0x03
#define GR04 IGS_GR+0x04
#define GR05 IGS_GR+0x05
#define GR06 IGS_GR+0x06
#define GR07 IGS_GR+0x07
#define GR08 IGS_GR+0x08
#define GR09 IGS_GR+0x09
#define GR0A IGS_GR+0x0A
#define GR0B IGS_GR+0x0B
#define GR0C IGS_GR+0x0C
#define GR0D IGS_GR+0x0D
#define GR0E IGS_GR+0x0E
#define GR0F IGS_GR+0x0F
#define GR10 IGS_GR+0x10
#define GR11 IGS_GR+0x11
#define GR12 IGS_GR+0x12
#define GR13 IGS_GR+0x13
#define GR14 IGS_GR+0x14
#define GR15 IGS_GR+0x15
#define GR16 IGS_GR+0x16
#define GR17 IGS_GR+0x17
#define GR18 IGS_GR+0x18
#define GR19 IGS_GR+0x19
#define GR1A IGS_GR+0x1A
#define GR1B IGS_GR+0x1B
#define GR1C IGS_GR+0x1C
#define GR1D IGS_GR+0x1D
#define GR1E IGS_GR+0x1E
#define GR1F IGS_GR+0x1F
#define GR20 IGS_GR+0x20
#define GR21 IGS_GR+0x21
#define GR22 IGS_GR+0x22
#define GR23 IGS_GR+0x23
#define GR24 IGS_GR+0x24
#define GR25 IGS_GR+0x25
#define GR26 IGS_GR+0x26
#define GR27 IGS_GR+0x27
#define GR28 IGS_GR+0x28
#define GR29 IGS_GR+0x29
#define GR2A IGS_GR+0x2A
#define GR2B IGS_GR+0x2B
#define GR2C IGS_GR+0x2C
#define GR2D IGS_GR+0x2D
#define GR2E IGS_GR+0x2E
#define GR2F IGS_GR+0x2F
#define GR30 IGS_GR+0x30
#define GR31 IGS_GR+0x31
#define GR32 IGS_GR+0x32
#define GR33 IGS_GR+0x33
#define GR34 IGS_GR+0x34
#define GR35 IGS_GR+0x35
#define GR36 IGS_GR+0x36
#define GR37 IGS_GR+0x37
#define GR38 IGS_GR+0x38
#define GR39 IGS_GR+0x39
#define GR3A IGS_GR+0x3A
#define GR3B IGS_GR+0x3B
#define GR3C IGS_GR+0x3C
#define GR3D IGS_GR+0x3D
#define GR3E IGS_GR+0x3E
#define GR3F IGS_GR+0x3F
#define GR40 IGS_GR+0x40
#define GR41 IGS_GR+0x41
#define GR42 IGS_GR+0x42
#define GR43 IGS_GR+0x43
#define GR44 IGS_GR+0x44
#define GR45 IGS_GR+0x45
#define GR46 IGS_GR+0x46
#define GR47 IGS_GR+0x47
#define GR48 IGS_GR+0x48
#define GR49 IGS_GR+0x49
#define GR4A IGS_GR+0x4A
#define GR4B IGS_GR+0x4B
#define GR4C IGS_GR+0x4C
#define GR4D IGS_GR+0x4D
#define GR4E IGS_GR+0x4E
#define GR4F IGS_GR+0x4F
#define GR50 IGS_GR+0x50
#define GR51 IGS_GR+0x51
#define GR52 IGS_GR+0x52
#define GR53 IGS_GR+0x53
#define GR54 IGS_GR+0x54
#define GR55 IGS_GR+0x55
#define GR56 IGS_GR+0x56
#define GR57 IGS_GR+0x57
#define GR58 IGS_GR+0x58
#define GR59 IGS_GR+0x59
#define GR5A IGS_GR+0x5A
#define GR5B IGS_GR+0x5B
#define GR5C IGS_GR+0x5C
#define GR5D IGS_GR+0x5D
#define GR5E IGS_GR+0x5E
#define GR5F IGS_GR+0x5F
#define GR60 IGS_GR+0x60
#define GR61 IGS_GR+0x61
#define GR62 IGS_GR+0x62
#define GR63 IGS_GR+0x63
#define GR64 IGS_GR+0x64
#define GR65 IGS_GR+0x65
#define GR66 IGS_GR+0x66
#define GR67 IGS_GR+0x67
#define GR68 IGS_GR+0x68
#define GR69 IGS_GR+0x69
#define GR6A IGS_GR+0x6A
#define GR6B IGS_GR+0x6B
#define GR6C IGS_GR+0x6C
#define GR6D IGS_GR+0x6D
#define GR6E IGS_GR+0x6E
#define GR6F IGS_GR+0x6F
#define GR70 IGS_GR+0x70
#define GR71 IGS_GR+0x71
#define GR72 IGS_GR+0x72
#define GR73 IGS_GR+0x73
#define GR74 IGS_GR+0x74
#define GR75 IGS_GR+0x75
#define GR76 IGS_GR+0x76
#define GR77 IGS_GR+0x77
#define GR78 IGS_GR+0x78
#define GR79 IGS_GR+0x79
#define GR7A IGS_GR+0x7A
#define GR7B IGS_GR+0x7B
#define GR7C IGS_GR+0x7C
#define GR7D IGS_GR+0x7D
#define GR7E IGS_GR+0x7E
#define GR7F IGS_GR+0x7F
#define GR80 IGS_GR+0x80
#define GR81 IGS_GR+0x81
#define GR82 IGS_GR+0x82
#define GR83 IGS_GR+0x83
#define GR84 IGS_GR+0x84
#define GR85 IGS_GR+0x85
#define GR86 IGS_GR+0x86
#define GR87 IGS_GR+0x87
#define GR88 IGS_GR+0x88
#define GR89 IGS_GR+0x89
#define GR8A IGS_GR+0x8A
#define GR8B IGS_GR+0x8B
#define GR8C IGS_GR+0x8C
#define GR8D IGS_GR+0x8D
#define GR8E IGS_GR+0x8E
#define GR8F IGS_GR+0x8F
#define GR90 IGS_GR+0x90
#define GR91 IGS_GR+0x91
#define GR92 IGS_GR+0x92
#define GR93 IGS_GR+0x93
#define GR94 IGS_GR+0x94
#define GR95 IGS_GR+0x95
#define GR96 IGS_GR+0x96
#define GR97 IGS_GR+0x97
#define GR98 IGS_GR+0x98
#define GR99 IGS_GR+0x99
#define GR9A IGS_GR+0x9A
#define GR9B IGS_GR+0x9B
#define GR9C IGS_GR+0x9C
#define GR9D IGS_GR+0x9D
#define GR9E IGS_GR+0x9E
#define GR9F IGS_GR+0x9F
#define GRA0 IGS_GR+0xA0
#define GRA1 IGS_GR+0xA1
#define GRA2 IGS_GR+0xA2
#define GRA3 IGS_GR+0xA3
#define GRA4 IGS_GR+0xA4
#define GRA5 IGS_GR+0xA5
#define GRA6 IGS_GR+0xA6
#define GRA7 IGS_GR+0xA7
#define GRA8 IGS_GR+0xA8
#define GRA9 IGS_GR+0xA9
#define GRAA IGS_GR+0xAA
#define GRAB IGS_GR+0xAB
#define GRAC IGS_GR+0xAC
#define GRAD IGS_GR+0xAD
#define GRAE IGS_GR+0xAE
#define GRAF IGS_GR+0xAF
#define GRB0 IGS_GR+0xB0
#define GRB1 IGS_GR+0xB1
#define GRB2 IGS_GR+0xB2
#define GRB3 IGS_GR+0xB3
#define GRB4 IGS_GR+0xB4
#define GRB5 IGS_GR+0xB5
#define GRB6 IGS_GR+0xB6
#define GRB7 IGS_GR+0xB7
#define GRB8 IGS_GR+0xB8
#define GRB9 IGS_GR+0xB9
#define GRBA IGS_GR+0xBA
#define GRBB IGS_GR+0xBB
#define GRBC IGS_GR+0xBC
#define GRBD IGS_GR+0xBD
#define GRBE IGS_GR+0xBE
#define GRBF IGS_GR+0xBF

#define GR_FIRST    GR00
#define GR_LAST	    GRBF

#define GREX3C	IGS_GREX+(0x3c-IGS_GREXBASE)

VgaReg igs_h_total[] = {
    CR00,   0, 8,
    VGA_REG_END
};

VgaReg igs_h_de_end[] = {
    CR01,   0, 8,
    VGA_REG_END
};

VgaReg igs_h_bstart[] = {
    CR02,   0, 8,
    VGA_REG_END
};

VgaReg igs_h_bend[] = {
    CR03,   0, 5,
    CR05,   7, 1,
    VGA_REG_END
};

VgaReg igs_de_skew[] = {
    CR03,   5, 2,
    VGA_REG_END
};

VgaReg igs_ena_vr_access[] = {
    CR03,   7, 1,
    VGA_REG_END
};

VgaReg igs_h_rstart[] = {
    CR04,   0, 8,
    VGA_REG_END
};

VgaReg igs_h_rend[] = {
    CR05,   0, 5,
    VGA_REG_END
};

VgaReg igs_h_rdelay[] = {
    CR05,   5, 2,
    VGA_REG_END
};

VgaReg igs_v_total[] = {
    CR06,   0, 8,
    CR07,   0, 1,
    CR07,   5, 1,
    GR11,   0, 1,
    VGA_REG_END
};

VgaReg igs_v_rstart[] = {
    CR10,   0, 8,
    CR07,   2, 1,
    CR07,   7, 1,
    GR11,   2, 1,
    VGA_REG_END
};

VgaReg igs_v_rend[] = {
    CR11,   0, 4,
    VGA_REG_END
};

VgaReg igs_clear_v_int[] = {
    CR11,   4, 1,
    VGA_REG_END
};

VgaReg igs_disable_v_int[] = {
    CR11,   5, 1,
    VGA_REG_END
};

VgaReg igs_bandwidth[] = {
    CR11,   6, 1,
    VGA_REG_END
};

VgaReg igs_crt_protect[] = {
    CR11,   7, 1,
    VGA_REG_END
};

VgaReg igs_v_de_end[] = {
    CR12,   0, 8,
    CR07,   1, 1,
    CR07,   6, 1,
    GR11,   1, 1,
    VGA_REG_END
};

VgaReg igs_offset[] = {
    CR13,   0, 8,
    GR15,   4, 2,
    VGA_REG_END
};

VgaReg igs_v_bstart[] = {
    CR15,   0, 8,
    CR07,   3, 1,
    CR09,   5, 1,
    GR11,   3, 1,
    VGA_REG_END
};

VgaReg igs_v_bend[] = {
    CR16,   0, 7,
    VGA_REG_END
};

VgaReg igs_linecomp[] = {
    CR18,   0, 8,
    CR07,   4, 1,
    CR09,   6, 1,
    GR11,   4, 1,
    VGA_REG_END
};

VgaReg igs_ivideo[] = {
    GR11,   5, 1,
    VGA_REG_END
};

VgaReg igs_num_fetch[] = {
    GR14,   0, 8,
    GR15,   0, 2,
    VGA_REG_END
};

VgaReg igs_wcrt0[] = {
    CR1F, 0, 1,
    VGA_REG_END
};

VgaReg igs_wcrt1[] = {
    CR1F, 1, 1,
    VGA_REG_END
};

VgaReg igs_rcrts1[] = {
    CR1F, 4, 1,
    VGA_REG_END
};

VgaReg igs_selwk[] = {
    CR1F, 6, 1,
    VGA_REG_END
};

VgaReg igs_dot_clock_8[] = {
    SR01, 0, 1,
    VGA_REG_END
};

VgaReg igs_screen_off[] = {
    SR01, 5, 1,
    VGA_REG_END
};

VgaReg igs_enable_write_plane[] = {
    SR02, 0, 4,
    VGA_REG_END
};

VgaReg igs_mexhsyn[] = {
    GR16,   0, 2,
    VGA_REG_END
};

VgaReg igs_mexvsyn[] = {
    GR16,   2, 2,
    VGA_REG_END
};

VgaReg igs_pci_burst_write[] = {
    GR30,   5, 1,
    VGA_REG_END
};

VgaReg igs_pci_burst_read[] = {
    GR30,   7, 1,
    VGA_REG_END
};

VgaReg	igs_iow_retry[] = {
    GREX3C, 0, 1,
    VGA_REG_END
};

VgaReg  igs_mw_retry[] = {
    GREX3C, 1, 1,
    VGA_REG_END
};

VgaReg  igs_mr_retry[] = {
    GREX3C, 2, 1,
    VGA_REG_END
};



VgaReg	igs_biga22en[] = {
    GR3D,   4, 1,
    VGA_REG_END
};

VgaReg	igs_biga24en[] = {
    GR3D,   5, 1,
    VGA_REG_END
};

VgaReg	igs_biga22force[] = {
    GR3D,   6, 1,
    VGA_REG_END
};

VgaReg	igs_bigswap[] = {
    GR3F,   0, 6,
    VGA_REG_END
};

/* #define IGS_BIGSWAP_8	0x3f */
/* #define IGS_BIGSWAP_16	0x2a */
/* #define IGS_BIGSWAP_32	0x00 */

VgaReg  igs_sprite_x[] = {
    GR50,   0, 8,
    GR51,   0, 3,
    VGA_REG_END
};

VgaReg  igs_sprite_preset_x[] = {
    GR52,   0, 6,
    VGA_REG_END
};

VgaReg  igs_sprite_y[] = {
    GR53,   0, 8,
    GR54,   0, 3,
    VGA_REG_END
};

VgaReg  igs_sprite_preset_y[] = {
    GR55,   0, 6,
    VGA_REG_END
};

VgaReg	igs_sprite_visible[] = {
    GR56,   0, 1,
    VGA_REG_END
};

VgaReg  igs_sprite_64x64[] = {
    GR56,   1, 1,
    VGA_REG_END
};

VgaReg  igs_mgrext[] = {
    GR57,   0, 1,
    VGA_REG_END
};

VgaReg  igs_hcshf[] = {
    GR57,   4, 2,
    VGA_REG_END
};

VgaReg	igs_mbpfix[] = {
    GR57,   6, 2,
    VGA_REG_END
};

VgaReg igs_overscan_red[] = {
    GR58, 0, 8,
    VGA_REG_END
};

VgaReg igs_overscan_green[] = {
    GR59, 0, 8,
    VGA_REG_END
};

VgaReg igs_overscan_blue[] = {
    GR5A, 0, 8,
    VGA_REG_END
};

VgaReg igs_memgopg[] = {
    GR73,   2, 1,
    VGA_REG_END
};

VgaReg igs_memr2wpg[] = {
    GR73,   1, 1,
    VGA_REG_END
};

VgaReg igs_crtff16[] = {
    GR73,   3, 1,
    VGA_REG_END
};

VgaReg igs_fifomust[] = {
    GR74,   0, 5,
    VGA_REG_END
};

VgaReg igs_fifogen[] = {
    GR75,   0, 5,
    VGA_REG_END
};

VgaReg igs_mode_sel[] = {
    GR77,   0, 4,
    VGA_REG_END
};

/* #define IGS_MODE_TEXT	0 */
/* #define IGS_MODE_8	1 */
/* #define IGS_MODE_565	2 */
/* #define IGS_MODE_5551	6 */
/* #define IGS_MODE_8888	3 */
/* #define IGS_MODE_888	4 */
/* #define IGS_MODE_332	9 */
/* #define IGS_MODE_4444	10 */

VgaReg  igs_sprite_addr[] = {
    GR7E,   0, 8,
    GR7F,   0, 4,
    VGA_REG_END
};

VgaReg  igs_fastmpie[] = {
    GR9E,   0, 1,
    VGA_REG_END
};

VgaReg  igs_vclk_m[] = {
    GRB0,   0, 8,
    GRBA,   0, 3,
    VGA_REG_END
};

VgaReg  igs_vclk_n[] = {
    GRB1,   0, 5,
    GRBA,   3, 3,
    VGA_REG_END
};

VgaReg  igs_vfsel[] = {
    GRB1,   5, 1,
    VGA_REG_END
};

VgaReg  igs_vclk_p[] = {
    GRB1,   6, 2,
    GRBA,   6, 1,
    VGA_REG_END
};

VgaReg  igs_frqlat[] = {
    GRB9, 7, 1,
    VGA_REG_END
};


VgaReg igs_dac_mask[] = {
    IGS_DAC + 0,	0, 8,
    VGA_REG_END
};

VgaReg igs_dac_read_index[] = {
    IGS_DAC + 1, 0, 8,
    VGA_REG_END
};

VgaReg igs_dac_write_index[] = {
    IGS_DAC + 2, 0, 8,
    VGA_REG_END
};

VgaReg igs_dac_data[] = {
    IGS_DAC + 3, 0, 8,
    VGA_REG_END
};

VgaReg	igs_rampwdn[] = {
    IGS_DACEX + 0,   0, 1,
    VGA_REG_END
};

VgaReg	igs_dac6_8[] = {
    IGS_DACEX + 0,   1, 1,
    VGA_REG_END
};

VgaReg  igs_ramdacbypass[] = {
    IGS_DACEX + 0,   4, 1,
    VGA_REG_END
};

VgaReg	igs_dacpwdn[] = {
    IGS_DACEX + 0,   6, 1,
    VGA_REG_END
};

VgaReg	igs_cursor_read_index[] = {
    IGS_DACEX + 1, 0, 8,
    VGA_REG_END
};

VgaReg igs_cursor_write_index[] = {
    IGS_DACEX + 2, 0, 8,
    VGA_REG_END
};

VgaReg igs_cursor_data[] = {
    IGS_DACEX + 3, 0, 8,
    VGA_REG_END
};

VGA8
_igsInb (VgaCard *card, VGA16 port)
{
    VGAVOL8 *reg;
    
    if (card->closure)
	return VgaReadMemb ((VGA32) card->closure + port);
    else
	return VgaInb (port);
}

void
_igsOutb (VgaCard *card, VGA8 value, VGA16 port)
{
    if (card->closure)
	VgaWriteMemb (value, (VGA32) card->closure + port);
    else
	VgaOutb (value, port);
}

void
_igsRegMap (VgaCard *card, VGA16 reg, VgaMap *map, VGABOOL write)
{
    if (reg < IGS_SR + IGS_NSR)
    {
	map->access = VgaAccessIndIo;
	map->port = 0x3c4;
	map->addr = 0;
	map->value = 1;
	map->index = reg - IGS_SR;
    }
    else if (reg < IGS_GR + IGS_NGR)
    {
	map->access = VgaAccessIndIo;
	map->port = 0x3ce;
	map->addr = 0;
	map->value = 1;
	map->index = reg - IGS_GR;
    }
    else if (reg < IGS_GREX + IGS_NGREX)
    {
	VGA8	gr33;

	map->access = VgaAccessDone;
	_igsOutb (card, 0x33, 0x3ce);
	gr33 = _igsInb (card, 0x3cf);
	_igsOutb (card, gr33 | 0x40, 0x3cf);
	_igsOutb (card, IGS_GREXBASE + reg - IGS_GREX, 0x3ce);
	if (write)
	    _igsOutb (card, map->value, 0x3cf);
	else
	    map->value = _igsInb (card, 0x3cf);
	_igsOutb (card, 0x33, 0x3ce);
	_igsOutb (card, gr33, 0x3cf);
	return;
    }
    else if (reg < IGS_AR + IGS_NAR)
    {
	reg -= IGS_AR;
	map->access = VgaAccessDone;
	/* reset AFF to index */
	(void) _igsInb (card, 0x3da);
	if (reg >= 16)
	    reg |= 0x20;
	_igsOutb (card, reg, 0x3c0);
	if (write)
	    _igsOutb (card, map->value, 0x3c0);
	else
	    map->value = _igsInb (card, 0x3c1);
	if (!(reg & 0x20))
	{
	    /* enable video display again */
	    (void) _igsInb (card, 0x3da);
	    _igsOutb (card, 0x20, 0x3c0);
	}
	return;
    }
    else if (reg < IGS_CR + IGS_NCR)
    {
	map->access = VgaAccessIndIo;
	map->port = 0x3d4;
	map->addr = 0;
	map->value = 1;
	map->index = reg - IGS_CR;
    }
    else if (reg < IGS_DAC + IGS_NDAC)
    {
	map->access = VgaAccessIo;
	map->port = 0x3c6 + reg - IGS_DAC;
    }
    else if (reg < IGS_DACEX + IGS_NDACEX)
    {
	VGA8	gr56;
	reg = 0x3c6 + reg - IGS_DACEX;
	map->access = VgaAccessDone;
	_igsOutb (card, 0x56, 0x3ce);
	gr56 = _igsInb (card, 0x3cf);
	_igsOutb (card, gr56 | 4, 0x3cf);
	if (write)
	    _igsOutb (card, map->value, reg);
	else
	    map->value = _igsInb (card, reg);
	_igsOutb (card, gr56, 0x3cf);
	return;
    }
    else switch (reg) {
    case IGS_MISC_OUT:
	map->access = VgaAccessIo;
	if (write)
	    map->port = 0x3c2;
	else
	    map->port = 0x3cc;
	break;
    case IGS_INPUT_STATUS_1:
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

VgaSave	    igsSaves[] = {
    CR00, CR18,
    SR01, SR02,
    GR11, GRBA,
    IGS_MISC_OUT, IGS_MISC_OUT,
    VGA_SAVE_END
};

void
igsRegInit (IgsVga *igsvga, VGAVOL8 *mmio)
{
    igsvga->card.map = _igsRegMap;
    igsvga->card.closure = (void *) mmio;
    igsvga->card.max = IGS_NREG;
    igsvga->card.values = igsvga->values;
    igsvga->card.saves = igsSaves;
}

void
igsSave (IgsVga *igsvga)
{
    igsSetImm (igsvga, igs_wcrt0, 1);
    igsSetImm (igsvga, igs_wcrt1, 1);
    igsSetImm (igsvga, igs_rcrts1, 1);
    igsSetImm (igsvga, igs_selwk, 1);
    VgaPreserve (&igsvga->card);
}

void
igsReset (IgsVga *igsvga)
{
    VgaRestore (&igsvga->card);
    igsSetImm (igsvga, igs_frqlat, 0);
    igsSetImm (igsvga, igs_frqlat, 1);
    igsSetImm (igsvga, igs_frqlat, 0);    
    VgaFinish (&igsvga->card);
}

char *
igsRegName(char *buf, VGA16 reg)
{
    if (reg < IGS_SR + IGS_NSR)
    {
	sprintf (buf, "  SR%02X", reg - IGS_SR);
    }
    else if (reg < IGS_GR + IGS_NGR)
    {
	sprintf (buf, "  GR%02X", reg - IGS_GR);
    }
    else if (reg < IGS_GREX + IGS_NGREX)
    {
	sprintf (buf, " GRX%02X", reg - IGS_GREX + IGS_GREXBASE);
    }
    else if (reg < IGS_AR + IGS_NAR)
    {
	sprintf (buf, "  AR%02X", reg - IGS_AR);
    }
    else if (reg < IGS_CR + IGS_NCR)
    {
	sprintf (buf, "  CR%02X", reg - IGS_CR);
    }
    else if (reg < IGS_DAC + IGS_NDAC)
    {
	sprintf (buf, " DAC%02X", reg - IGS_DAC);
    }
    else if (reg < IGS_DACEX + IGS_NDACEX)
    {
	sprintf (buf, "DACX%02X", reg - IGS_DACEX);
    }
    else switch (reg) {
    case IGS_MISC_OUT:
	sprintf (buf, "MISC_O");
	break;
    case IGS_INPUT_STATUS_1:
	sprintf (buf, "IN_S_1");
	break;
    }
    return buf;
}

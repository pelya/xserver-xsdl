/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/xf86Axp.c,v 1.2 2000/11/06 21:57:11 dawes Exp $ */

#include "xf86Axp.h"

axpParams xf86AXPParams[] = { 
  {SYS_NONE,     0,         0,         0},
  {TSUNAMI,      0,         0,         0},
  {LCA,      1<<24,0xf8000000, 1UL << 32},
  {APECS,    1<<24,0xf8000000, 1UL << 32},
  {T2,           0,0xFC000000, 1UL << 31},
  {T2_GAMMA,     0,0xFC000000, 1UL << 31},
  {CIA,          0,0xE0000000, 1UL << 34},
  {MCPCIA,       0,0xf8000000, 1UL << 31},
  {JENSEN,       0, 0xE000000, 1UL << 32},
  {POLARIS,      0,         0,         0},
  {PYXIS,        0,         0,         0},
  {PYXIS_CIA,    0,0xE0000000, 1UL << 34},
  {IRONGATE,     0,         0,         0}
};


/* 
Copyright (c) 2000 by Juliusz Chroboczek
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions: 
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software. 

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef _VBE_H
#define _VBE_H

#define VBE_WINDOW_RELOCATE 1
#define VBE_WINDOW_READ 2
#define VBE_WINDOW_WRITE 4

#ifndef U8
#define U8 unsigned char
#define U16 unsigned short
#define U32 unsigned int
#endif

/* The whole addressable memory */
#define SYSMEM_BASE 0x00000
#define SYSMEM_SIZE 0x100000

/* Interrupt vectors and BIOS data area */
/* This is allocated privately from /dev/mem */
#define MAGICMEM_BASE 0x00000
#define MAGICMEM_SIZE 0x01000

/* The low memory, allocated privately from /dev/zero */
/* 64KB should be enough for anyone, as they used to say */
#define LOMEM_BASE 0x10000
#define LOMEM_SIZE 0x10000

/* The video memory and BIOS ROM, allocated shared from /dev/mem */
#define HIMEM_BASE 0xA0000
#define HIMEM_SIZE (SYSMEM_BASE + SYSMEM_SIZE - HIMEM_BASE)

/* The BIOS ROM */
#define ROM_BASE 0xC0000
#define ROM_SIZE 0x30000

#define STACK_SIZE 0x1000

#define POINTER_SEGMENT(ptr) (((unsigned int)ptr)>>4)
#define POINTER_OFFSET(ptr) (((unsigned int)ptr)&0x000F)
#define MAKE_POINTER(seg, off) (((((unsigned int)(seg))<<4) + (unsigned int)(off)))
#define MAKE_POINTER_1(lw) MAKE_POINTER(((lw)&0xFFFF0000)/0x10000, (lw)&0xFFFF)
#define ALLOC_FAIL ((U32)-1)

typedef struct _VbeInfoRec {
    int devmem, devzero;
    void *magicMem, *loMem, *hiMem;
    U32 brk;
    struct vm86_struct vms;
    U32 ret_code, stack_base, vib_base, vmib_base, statebuffer_base, palette_scratch_base;
    U8 palette_format;
    int palette_wait;
    int windowA_offset;
    int windowB_offset;
    int last_window;
    int vga_palette;
} VbeInfoRec, *VbeInfoPtr;

typedef struct _VbeInfoBlock {
    U8 VbeSignature[4];         /* VBE Signature */
    U16 VbeVersion;             /* VBE Version */
    U32 OemStringPtr;           /* Pointer to OEM String */
    U8 Capabilities[4];         /* Capabilities of graphics controller */
    U32 VideoModePtr;           /* Pointer to VideoModeList */
    U16 TotalMemory;            /* Number of 64kb memory blocks */
/* Added for VBE 2.0 */
    U16 OemSoftwareRev;         /* VBE implementation Software revision */
    U32 OemVendorNamePtr;       /* Pointer to Vendor Name String */
    U32 OemProductNamePtr;	/* Pointer to Product Name String */
    U32 OemProductRevPtr;       /* Pointer to Product Revision String */
    U8 Reserved[222];           /* Reserved for VBE implementation */
    U8 OemData[256];            /* Data Area for OEM Strings*/
} __attribute__((packed)) VbeInfoBlock;

typedef struct _VbeModeInfoBlock {
/* Mandatory information for all VBE revisions */
    U16 ModeAttributes;         /* mode attributes */
    U8 WinAAttributes;          /* window A attributes */
    U8 WinBAttributes;          /* window B attributes */
    U16 WinGranularity;         /* window granularity */
    U16 WinSize;                /* window size */
    U16 WinASegment;            /* window A start segment */
    U16 WinBSegment;            /* window B start segment */
    U32 WinFuncPtr;             /* pointer to window function */
    U16 BytesPerScanLine;       /* bytes per scan line */
/* Mandatory information for VBE 1.2 and above */
    U16 XResolution;            /* horizontal resolution */
    U16 YResolution;            /* vertical resolution */
    U8 XCharSize;               /* character cell width in pixels */
    U8 YCharSize;               /* character cell height in pixels */
    U8 NumberOfPlanes;          /* number of memory planes */
    U8 BitsPerPixel;            /* bits per pixel */
    U8 NumberOfBanks;           /* number of banks */
    U8 MemoryModel;             /* memory model type */
    U8 BankSize;                /* bank size in KB */
    U8 NumberOfImagePages;      /* number of images */
    U8 Reserved;                /* reserved for page function */
/* Direct Color fields (required for direct/6 and YUV/7 memory models) */
    U8 RedMaskSize;             /* size of direct color red mask in bits */
    U8 RedFieldPosition;        /* bit position of lsb of red mask */
    U8 GreenMaskSize;           /* size of direct color green mask in bits */
    U8 GreenFieldPosition;      /* bit position of lsb of green mask */
    U8 BlueMaskSize;            /* size of direct color blue mask in bits */
    U8 BlueFieldPosition;       /* bit position of lsb of blue mask */
    U8 RsvdMaskSize;            /* size of direct color reserved mask bits*/
    U8 RsvdFieldPosition;       /* bit position of lsb of reserved mask */
    U8 DirectColorModeInfo;     /* direct color mode attributes */
/* Mandatory information for VBE 2.0 and above */
    U32 PhysBasePtr;            /* physical address for flat memory fb */
    U32 OffScreenMemOffset;     /* pointer to start of off screen memory */
    U16 OffScreenMemSize;       /* amount of off screen memory in 1k units */
    U8 Reserved2[206];          /* remainder of ModeInfoBlock */
} __attribute__((packed)) VbeModeInfoBlock;


typedef struct _SupVbeInfoBlock {
    U8 SupVbeSignature[7];      /* Supplemental VBE Signature */
    U16 SupVbeVersion;          /* Supplemental VBE Version*/
    U8 SupVbeSubFunc[8];	/* Bitfield of supported subfunctions */
    U16 OemSoftwareRev;         /* OEM Software revision */
    U32 OemVendorNamePtr;       /* Pointer to Vendor Name String */
    U32 OemProductNamePtr;	/* Pointer to Product Name String */
    U32 OemProductRevPtr;       /* Pointer to Product Revision String */
    U32 OemStringPtr;           /* Pointer to OEM String */
    U8 Reserved[221];           /* Reserved */
} __attribute__((packed)) SupVbeInfoBlock;

VbeInfoPtr VbeSetup(void);
void VbeCleanup(VbeInfoPtr vi);
VbeInfoBlock *VbeGetInfo(VbeInfoPtr vi);
VbeModeInfoBlock *VbeGetModeInfo(VbeInfoPtr vi, int mode);
int VbeSetMode(VbeInfoPtr vi, int mode, int linear);
int  VbeGetMode(VbeInfoPtr vi, int *mode);
int VbeSetupStateBuffer(VbeInfoPtr vi);
int VbeSaveState(VbeInfoPtr vi);
int VbeRestoreState(VbeInfoPtr vi);
void *VbeMapFramebuffer(VbeInfoPtr vi, VbeModeInfoBlock *vmib);
int VbeUnmapFrambuffer(VbeInfoPtr vi, VbeModeInfoBlock *vmib, void *fb);
int VbeSetPalette(VbeInfoPtr vi, int first, int number, U8 *entries);
int VbeSetPaletteOptions(VbeInfoPtr vi, U8 bits, int wait);
void *VbeSetWindow(VbeInfoPtr vi, int offset, int purpose, int *size_return);
int VbeReportInfo(VbeInfoPtr, VbeInfoBlock *);
int VbeReportModeInfo(VbeInfoPtr, U16 mode, VbeModeInfoBlock *);

int VbeDoInterrupt(VbeInfoPtr, int num);
int VbeDoInterrupt10(VbeInfoPtr vi);
int  VbeIsMemory(VbeInfoPtr vi, U32 i);
U8 VbeMemory(VbeInfoPtr, U32);
U16 VbeMemoryW(VbeInfoPtr, U32);
U32 VbeMemoryL(VbeInfoPtr, U32);
void VbeWriteMemory(VbeInfoPtr, U32, U8);
void VbeWriteMemoryW(VbeInfoPtr, U32, U16);
void VbeWriteMemoryL(VbeInfoPtr, U32, U32);
int VbeAllocateMemory(VbeInfoPtr, int);
void VbeDebug(VbeInfoPtr vi);
#endif

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

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/vm86.h>
#include <sys/io.h>
#include "vbe.h"

#ifdef NOT_IN_X_SERVER
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
static void ErrorF(char*, ...);
#define xalloc(a) malloc(a)
#define xcalloc(a,b) calloc(a,b)
#define xfree(a) free(a)
#else
#include "X.h"
#include "Xproto.h"
#include "Xos.h"
#include "os.h"
#endif

static int vm86old(struct vm86_struct *vms);
static int vm86_loop(VbeInfoPtr vi);

static U8 rev_ints[32] =
{ 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0x80,
};

static U8 retcode_data[2] =
{ 0xCD, 0xFF };

#define LM(vi,i) (((char*)vi->loMem)[i-LOMEM_BASE])
#define LMW(vi,i) (*(U16*)(&LM(vi,i)))
#define LML(vi,i) (*(U32*)(&LM(vi,i)))
#define MM(vi,i) (((char*)vi->magicMem)[i-MAGICMEM_BASE])
#define MMW(vi,i) (*(U16*)(&MM(vi,i)))
#define MML(vi,i) (*(U32*)(&MM(vi,i)))
#define HM(vi,i) (((char*)vi->hiMem)[i-HIMEM_BASE])
#define HMW(vi,i) (*(U16*)(&MM(vi,i)))
#define HML(vi,i) (*(U32*)(&MM(vi,i)))

#define PUSHW(vi, i) \
{ vi->vms.regs.esp -= 2;\
  LMW(vi,MAKE_POINTER(vi->vms.regs.ss, vi->vms.regs.esp)) = i;}

VbeInfoPtr
VbeSetup()
{
    int devmem = -1, devzero = -1;
    void *magicMem, *loMem, *hiMem;
    U32 stack_base, vib_base, vmib_base, ret_code;
    VbeInfoPtr vi = NULL;

    devmem = open("/dev/mem", O_RDWR);
    if(devmem < 0) {
        perror("open /dev/mem");
        goto fail;
    }

    devzero = open("/dev/zero", O_RDWR);
    if(devmem < 0) {
        perror("open /dev/zero");
        goto fail;
    }


    magicMem = mmap((void*)MAGICMEM_BASE, MAGICMEM_SIZE,
                   PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_FIXED, devmem, MAGICMEM_BASE);
    if(magicMem == MAP_FAILED) {
        ErrorF("Couldn't map magic memory\n");
        goto fail;
    }

    loMem = mmap((void*)LOMEM_BASE, LOMEM_SIZE,
                 PROT_READ | PROT_WRITE | PROT_EXEC,
                 MAP_PRIVATE | MAP_FIXED, devzero, LOMEM_BASE);
    if(loMem == MAP_FAILED) {
        ErrorF("Couldn't map low memory\n");
        munmap(magicMem, MAGICMEM_SIZE);
        goto fail;
    }

    hiMem = mmap((void*)HIMEM_BASE, HIMEM_SIZE,
                 PROT_READ | PROT_WRITE | PROT_EXEC,
                 MAP_SHARED | MAP_FIXED,
                 devmem, HIMEM_BASE);
    if(hiMem == MAP_FAILED) {
        munmap(magicMem, MAGICMEM_SIZE);
        munmap(loMem, LOMEM_SIZE);
        goto fail;
    }

    vi = xalloc(sizeof(VbeInfoRec));
    if (!vi)
        goto unmapfail;

    vi->devmem = devmem;
    vi->devzero = devzero;
    vi->magicMem = magicMem;
    vi->loMem = loMem;
    vi->hiMem = hiMem;
    vi->fb = NULL;
    vi->brk = LOMEM_BASE;

    stack_base = VbeAllocateMemory(vi, STACK_SIZE);
    if(stack_base == ALLOC_FAIL)
        goto unmapfail;
    ret_code = VbeAllocateMemory(vi, sizeof(retcode_data));
    if(ret_code == ALLOC_FAIL)
        goto unmapfail;
    vib_base = VbeAllocateMemory(vi, sizeof(VbeInfoBlock));
    if(vib_base == ALLOC_FAIL)
        goto unmapfail;
    vmib_base = VbeAllocateMemory(vi, sizeof(VbeModeInfoBlock));
    if(vmib_base == ALLOC_FAIL)
        goto unmapfail;

    vi->stack_base = stack_base;
    vi->ret_code = ret_code;
    vi->vib_base = vib_base;
    vi->vmib_base = vmib_base;
    vi->statebuffer_base = ~0;
    vi->palette_scratch_base = ~0;
    vi->palette_format = 6;
    vi->palette_wait = 0;

    memset(&vi->vms, 0, sizeof(struct vm86_struct));
    vi->vms.flags = 0;
    vi->vms.screen_bitmap = 0;
    vi->vms.cpu_type = CPU_586;
    memcpy(&vi->vms.int_revectored, rev_ints, sizeof(rev_ints));

    ioperm(0, 0x400, 1);
    iopl(3);

    return vi;

  unmapfail:
    munmap(magicMem, MAGICMEM_SIZE);
    munmap(loMem, LOMEM_SIZE);
    munmap(hiMem, HIMEM_SIZE);
  fail:
    if(devmem >= 0)
        close(devmem);
    if(devzero >= 0)
        close(devzero);
    if(vi)
        xfree(vi);
    return NULL;
}

void
VbeCleanup(VbeInfoPtr vi)
{
    if(vi->fb)
        VbeUnmapFramebuffer(vi);
    munmap(vi->magicMem, MAGICMEM_SIZE);
    munmap(vi->loMem, LOMEM_SIZE);
    munmap(vi->hiMem, HIMEM_SIZE);
    xfree(vi);
}

VbeInfoBlock *
VbeGetInfo(VbeInfoPtr vi)
{
    int code;
    VbeInfoBlock *vib = (VbeInfoBlock*)&(LM(vi, vi->vib_base));
    vi->vms.regs.eax = 0x4F00;
    vi->vms.regs.es = POINTER_SEGMENT(vi->vib_base);
    vi->vms.regs.edi = POINTER_OFFSET(vi->vib_base);
    memcpy(vib->VbeSignature, "VBE2", 4);
    code = VbeDoInterrupt10(vi);
    if(code < 0)
        return NULL;
    if(memcmp(vib->VbeSignature, "VESA", 4) != 0) {
        ErrorF("Int 10 didn't return VESA signature in info block");
        return NULL;
    }
    return vib;
}

VbeModeInfoBlock *
VbeGetModeInfo(VbeInfoPtr vi, int mode)
{
    int code;
    U32 p;
    VbeInfoBlock *vib = (VbeInfoBlock*)&(LM(vi, vi->vib_base));
    VbeModeInfoBlock *vmib = (VbeModeInfoBlock*)&(LM(vi, vi->vmib_base));
    p = MAKE_POINTER_1(vib->VideoModePtr);
    if(!VbeIsMemory(vi, p)) {
        ErrorF("VideoModePtr 0x%08X doesn't point at low memory\n",
               vib->VideoModePtr);
        return NULL;
    }
    vi->vms.regs.eax = 0x4F01;
    vi->vms.regs.ecx = mode&0xFFFF;
    vi->vms.regs.es = POINTER_SEGMENT(vi->vmib_base);
    vi->vms.regs.edi = POINTER_OFFSET(vi->vmib_base);
    code = VbeDoInterrupt10(vi);
    if(code < 0)
        return NULL;
    else
        return vmib;
}

int
VbeSetMode(VbeInfoPtr vi, int mode)
{
    int code;

    vi->vms.regs.eax = 0x4F02;
    vi->vms.regs.ebx = (mode & 0xFFFF) | 0xC000;
    code = VbeDoInterrupt10(vi);
    if(code < 0)
        return -1;
    return 0;
}

int 
VbeGetMode(VbeInfoPtr vi, int *mode)
{
    int code;
    vi->vms.regs.eax = 0x4F03;
    code = VbeDoInterrupt10(vi);
    if(code < 0)
        return - 1;
    *mode = vi->vms.regs.ebx & 0xFFFF;
    return 0;
}

int
VbeSetupStateBuffer(VbeInfoPtr vi)
{
    int code;
    if(vi->statebuffer_base != ~0)
        return 0;
    vi->vms.regs.eax = 0x4F04;
    vi->vms.regs.edx = 0x0000;
    vi->vms.regs.ecx = 0x000F;
    code = VbeDoInterrupt10(vi);
    if(code < 0)
        return -1;
    vi->statebuffer_base = VbeAllocateMemory(vi, vi->vms.regs.ebx & 0xFFFF);
    return 0;
}

int
VbeSaveState(VbeInfoPtr vi)
{
    int code;
    code = VbeSetupStateBuffer(vi);
    if(code < 0)
        return -1;
    vi->vms.regs.eax = 0x4F04;
    vi->vms.regs.edx = 0x0001;
    vi->vms.regs.ecx = 0x000F;
    vi->vms.regs.es = POINTER_SEGMENT(vi->statebuffer_base);
    vi->vms.regs.ebx = POINTER_OFFSET(vi->statebuffer_base);
    code = VbeDoInterrupt10(vi);
    if(code < 0)
        return -1;
    return 0;
}

int
VbeRestoreState(VbeInfoPtr vi)
{
    int code;
    vi->vms.regs.eax = 0x4F04;
    vi->vms.regs.edx = 0x0002;
    vi->vms.regs.ecx = 0x000F;
    vi->vms.regs.es = POINTER_SEGMENT(vi->statebuffer_base);
    vi->vms.regs.ebx = POINTER_OFFSET(vi->statebuffer_base);
    code = VbeDoInterrupt10(vi);
    if(code < 0)
        return -1;
    return 0;
}

void *
VbeMapFramebuffer(VbeInfoPtr vi) {
    U8 *fb;
    VbeInfoBlock *vib = (VbeInfoBlock*)&(LM(vi, vi->vib_base));
    VbeModeInfoBlock *vmib = (VbeModeInfoBlock*)&(LM(vi, vi->vmib_base));
    int size;
    int pagesize = getpagesize(), before, after;

    size = 1024 * 64L * vib->TotalMemory;

    before = vmib->PhysBasePtr % pagesize;
    after = pagesize - ((vmib->PhysBasePtr + size) % pagesize);
    if(after == pagesize)
        after = 0;

    fb = mmap(0, before + size + after,
              PROT_READ | PROT_WRITE, MAP_SHARED, 
              vi->devmem, vmib->PhysBasePtr - before);
    if(fb == MAP_FAILED) {
        ErrorF("Failed to map framebuffer: %d\n", errno);
        return NULL;
    }

    vi->fb = fb;
    vi->fb_size = before + size + after;
    return fb + before;
}

int 
VbeUnmapFramebuffer(VbeInfoPtr vi)
{
    int code;
    if(!vi->fb)
        ErrorF("Unmapping frambuffer not mapped\n");
    code = munmap(vi->fb, vi->fb_size);
    if(code) {
        ErrorF("Couldn't unmap framebuffer: %d\n", errno);
        return -1;
    }
    return 0;
}

static int
PreparePalette(VbeInfoPtr vi)
{
    int code;
    if(vi->palette_scratch_base == ~0) {
        vi->palette_scratch_base = VbeAllocateMemory(vi, 4*256);
        if(vi->palette_scratch_base == ALLOC_FAIL) {
            ErrorF("Couldn't allocate scratch area for palette transfer\n");
            return -1;
        }
    }
    if(!vi->palette_format) {
        /* This isn't used currently */
        vi->vms.regs.eax = 0x4F08;
        vi->vms.regs.ebx = 0x01;
        code = VbeDoInterrupt10(vi);
        if(code < 0)
            return -1;
        vi->palette_format = vi->vms.regs.ebx & 0xFF;
    }
    return 0;
}

int 
VbeSetPalette(VbeInfoPtr vi, int first, int number, U8 *entries)
{
    U8 *palette_scratch;
    int i, code;

    if(number == 0)
        return 0;

    code = PreparePalette(vi);
    if(code < 0)
        return -1;

    if(first < 0 || number < 0 || first + number > 256) {
        ErrorF("Cannot set %d, %d palette entries\n", first, number);
        return -1;
    }
    palette_scratch = &LM(vi, vi->palette_scratch_base);

    if(vi->palette_format < 6 || vi->palette_format > 8) {
        ErrorF("Impossible palette format %d\n", vi->palette_format);
        return -1;
    }

    for(i=0; i<number*4; i++)
        palette_scratch[i] = entries[i] >> (8 - vi->palette_format);

    vi->vms.regs.eax = 0x4F09;
    if(vi->palette_wait)
        vi->vms.regs.ebx = 0x80;
    else
        vi->vms.regs.ebx = 0x00;
    vi->vms.regs.ecx = number;
    vi->vms.regs.edx = first;
    vi->vms.regs.es = POINTER_SEGMENT(vi->palette_scratch_base);
    vi->vms.regs.edi = POINTER_OFFSET(vi->palette_scratch_base);
    code = VbeDoInterrupt10(vi);
    if(code < 0)
        return -1;
    return 0;
}   
        
int 
VbeGetPalette(VbeInfoPtr vi, int first, int number, U8 *entries)
{
    U8 *palette_scratch;
    int i, code;

    code = PreparePalette(vi);
    if(code < 0)
        return -1;

    if(first< 0 || number < 0 || first + number > 256) {
        ErrorF("Cannot get %d, %d palette entries\n", first, number);
        return -1;
    }
    palette_scratch = &LM(vi, vi->palette_scratch_base);

    if(vi->palette_format < 6 || vi->palette_format > 8) {
        ErrorF("Impossible palette format %d\n", vi->palette_format);
        return -1;
    }

    vi->vms.regs.eax = 0x4F09;
    vi->vms.regs.ebx = 0x01;
    vi->vms.regs.ecx = number;
    vi->vms.regs.edx = first;
    vi->vms.regs.es = POINTER_SEGMENT(vi->palette_scratch_base);
    vi->vms.regs.edi = POINTER_OFFSET(vi->palette_scratch_base);
    code = VbeDoInterrupt10(vi);
    if(code < 0)
        return -1;

    for(i=0; i<number*4; i++)
         entries[i] = palette_scratch[i] << (8-vi->palette_format);

    return 0;
}   
        
int 
VbeSetPaletteOptions(VbeInfoPtr vi, U8 bits, int wait)
{
    int code;
    if(bits < 6 || bits > 8) {
        ErrorF("Impossible palette format %d\n", vi->palette_format);
        return -1;
    }
    ErrorF("Setting palette format to %d\n", vi->palette_format);
    if(bits != vi->palette_format) {
        vi->palette_format = 0;
        vi->vms.regs.eax = 0x4F08;
        vi->vms.regs.ebx = bits << 8;
        code = VbeDoInterrupt10(vi);
        if(code < 0)
            return -1;
        vi->palette_format = bits;
    }
    vi->palette_wait = wait;
    return 0;
}

int 
VbeReportInfo(VbeInfoPtr vi, VbeInfoBlock *vib)
{
    U32 i, p;
    unsigned char c;
    int error;
    ErrorF("VBE version %c.%c (", 
           ((vib->VbeVersion >> 8) & 0xFF) + '0',
           (vib->VbeVersion & 0xFF)+'0');
    p = vib->OemStringPtr;
    for(i = 0; 1; i++) {
        c = VbeMemory(vi, MAKE_POINTER_1(p+i));
        if(!c) break;
        ErrorF("%c", c);
        if (i > 32000) {
            error = 1;
            break;
        }
    }
    ErrorF(")\n");
    ErrorF("DAC is %s, controller is %sVGA compatible%s\n",
           (vib->Capabilities[0]&1)?"fixed":"switchable",
           (vib->Capabilities[0]&2)?"not ":"",
           (vib->Capabilities[0]&3)?", RAMDAC causes snow":"");
    ErrorF("Total memory: %lu bytes\n", 64L*vib->TotalMemory);
    if(error)
        return -1;
    return 0;
}
    
int 
VbeReportModeInfo(VbeInfoPtr vi, U16 mode, VbeModeInfoBlock *vmib)
{
    int supported = (vmib->ModeAttributes&0x1)?1:0;
    int colour = (vmib->ModeAttributes&0x8)?1:0;
    int graphics = (vmib->ModeAttributes&0x10)?1:0;
    int vga_compatible = !((vmib->ModeAttributes&0x20)?1:0);
    int linear_fb = (vmib->ModeAttributes&0x80)?1:0;

    ErrorF("0x%04X: %dx%dx%d%s",
           (unsigned)mode, 
           (int)vmib->XResolution, (int)vmib->YResolution,
           (int)vmib->BitsPerPixel,
           colour?"":" (monochrome)");
    switch(vmib->MemoryModel) {
    case 0:
        ErrorF(" text mode (%dx%d)",
               (int)vmib->XCharSize, (int)vmib->YCharSize);
        break;
    case 1:
        ErrorF(" CGA graphics");
        break;
    case 2:
        ErrorF(" Hercules graphics");
        break;
    case 3:
        ErrorF(" Planar (%d planes)", vmib->NumberOfPlanes);
        break;
    case 4:
        ErrorF(" PseudoColor");
        break;
    case 5:
        ErrorF(" Non-chain 4, 256 colour");
        break;
    case 6:
        if(vmib->DirectColorModeInfo & 1)
            ErrorF(" DirectColor");
        else
            ErrorF(" TrueColor");
        ErrorF(" [%d:%d:%d:%d]",
               vmib->RedMaskSize, vmib->GreenMaskSize, vmib->BlueMaskSize,
               vmib->RsvdMaskSize);
        if(vmib->DirectColorModeInfo & 2)
            ErrorF(" (reserved bits are reserved)");
        break;
    case 7: ErrorF("YUV");
        break;
    default:
        ErrorF("unknown MemoryModel 0x%X ", vmib->MemoryModel);
    }
    if(!supported)
        ErrorF(" (unsupported)");
    else if(!linear_fb)
        ErrorF(" (no linear framebuffer)");
    ErrorF("\n");
    return 0;
}       
int
VbeDoInterrupt10(VbeInfoPtr vi)
{
    int code;
    int oldax;

    oldax = vi->vms.regs.eax & 0xFFFF;

    code = VbeDoInterrupt(vi, 0x10);

    if(code < 0)
        return -1;

    if((vi->vms.regs.eax & 0xFFFF) != 0x4F) {
        ErrorF("Int 10h (0x%04X) failed: 0x%04X",
               oldax, vi->vms.regs.eax & 0xFFFF);
        if((oldax & 0xFF00) == 0x4F00) {
            switch((vi->vms.regs.eax & 0xFF00)>>8) {
            case 0: 
                ErrorF(" (success)\n"); 
                break;
            case 1: 
                ErrorF(" (function call failed)\n"); 
                break;
            case 2: 
                ErrorF(" (function not supported on this hardware)\n"); 
                break;
            case 3: 
                ErrorF(" (function call invalid in this video mode)\n"); 
                break;
            default: 
                ErrorF(" (unknown error)\n"); 
                break;
            }
            return -1;
        } else {
            ErrorF("\n");
        }
    }
    return code;
}

int
VbeDoInterrupt(VbeInfoPtr vi, int num)
{
    U16 seg, off;
    int code;

    if(num < 0 || num>256) {
        ErrorF("Interrupt %d doesn't exist\n");
        return -1;
    }
    seg = MMW(vi,num * 4 + 2);
    off = MMW(vi,num * 4);
    if(MAKE_POINTER(seg, off) < ROM_BASE ||
       MAKE_POINTER(seg, off) >= ROM_BASE + ROM_SIZE) {
        ErrorF("Interrupt pointer doesn't point at ROM\n");
        return -1;
    }
    memcpy(&(LM(vi,vi->ret_code)), retcode_data, sizeof(retcode_data));
    vi->vms.regs.eflags = IF_MASK | IOPL_MASK;
    vi->vms.regs.ss = POINTER_SEGMENT(vi->stack_base);
    vi->vms.regs.esp = STACK_SIZE;
    PUSHW(vi, IF_MASK | IOPL_MASK);
    PUSHW(vi, POINTER_SEGMENT(vi->ret_code));
    PUSHW(vi, POINTER_OFFSET(vi->ret_code));
    vi->vms.regs.cs = seg;
    vi->vms.regs.eip = off;
    code = vm86_loop(vi);
    if(code < 0) {
        perror("vm86 failed");
        return -1;
    } else if(code != 0) {
        ErrorF("vm86 returned 0x%04X\n", code);
        return -1;
    } else
        return 0;
}

static inline U8 
vm86_inb(U16 port)
{
    U8 value;
    asm volatile ("inb %w1,%b0" : "=a" (value) : "d" (port));
    return value;
}

static inline U16
vm86_inw(U16 port)
{
    U16 value;
    asm volatile ("inw %w1,%w0" : "=a" (value) : "d" (port));
    return value;
}

static inline U32
vm86_inl(U16 port)
{
    U32 value;
    asm volatile ("inl %w1,%0" : "=a" (value) : "d" (port));
    return value;
}

static inline void
vm86_outb(U16 port, U8 value)
{
    asm volatile ("outb %b0,%w1" : : "a" (value), "d" (port));
}

static inline void
vm86_outw(U16 port, U16 value)
{
    asm volatile ("outw %w0,%w1" : : "a" (value), "d" (port));
}

static inline void
vm86_outl(U16 port, U32 value)
{
    asm volatile ("outl %0,%w1" : : "a" (value), "d" (port));
}

#define SEG_CS 1
#define SEG_DS 2
#define SEG_ES 3
#define SEG_SS 4
#define SEG_GS 5
#define SEG_FS 6
#define REP 1
#define REPNZ 2
#define SET_8(_x, _y) (_x) = (_x & ~0xFF) | (_y & 0xFF);
#define SET_16(_x, _y) (_x) = (_x & ~0xFFFF) | (_y & 0xFFFF);
#define INC_IP(_i) SET_16(regs->eip, (regs->eip + _i))
#define AGAIN INC_IP(1); goto again;

static int
vm86_emulate(VbeInfoPtr vi)
{
    struct vm86_regs *regs = &vi->vms.regs;
    U8 opcode;
    int size;
    int pref_seg = 0, pref_rep = 0, pref_66 = 0, pref_67 = 0;
    U32 count;
    int code;

  again:
    if(!VbeIsMemory(vi, MAKE_POINTER(regs->cs, regs->eip))) {
        ErrorF("Trying to execute unmapped memory\n");
        return -1;
    }
    opcode = VbeMemory(vi, MAKE_POINTER(regs->cs, regs->eip));
    switch(opcode) {
    case 0x2E: pref_seg = SEG_CS; AGAIN;
    case 0x3E: pref_seg = SEG_DS; AGAIN;
    case 0x26: pref_seg = SEG_ES; AGAIN;
    case 0x36: pref_seg = SEG_SS; AGAIN;
    case 0x65: pref_seg = SEG_GS; AGAIN;
    case 0x64: pref_seg = SEG_FS; AGAIN;
    case 0x66: pref_66 = 1; AGAIN;
    case 0x67: pref_67 = 1; AGAIN;
    case 0xF2: pref_rep = REPNZ; AGAIN;
    case 0xF3: pref_rep = REP; AGAIN;

    case 0xEC:                  /* IN AL, DX */
        SET_8(regs->eax, vm86_inb(regs->edx & 0xFFFF));
        INC_IP(1);
        break;
    case 0xED:                  /* IN AX, DX */
        if(pref_66)
            regs->eax = vm86_inl(regs->edx & 0xFFFF);
        else
            SET_16(regs->eax, vm86_inw(regs->edx & 0xFFFF));
	INC_IP(1);
        break;
    case 0xE4:                  /* IN AL, imm8 */
        SET_8(regs->eax, 
              vm86_inb(VbeMemory(vi, MAKE_POINTER(regs->cs, regs->eip+1))));
        INC_IP(2);
        break;
    case 0xE5:                  /* IN AX, imm8 */
        if(pref_66)
            regs->eax =
                vm86_inl(VbeMemory(vi, MAKE_POINTER(regs->cs, regs->eip+1)));
        else
            SET_16(regs->eax, 
                   vm86_inw(VbeMemory(vi, MAKE_POINTER(regs->cs, regs->eip+1))));
        INC_IP(2);
        break;
    case 0x6C:                  /* INSB */
    case 0x6D:                  /* INSW */
        if(opcode == 0x6C) {
            VbeWriteMemory(vi, MAKE_POINTER(regs->es, regs->edi),
                vm86_inb(regs->edx & 0xFFFF));
            size = 1;
        } else if(pref_66) {
            VbeWriteMemoryL(vi, MAKE_POINTER(regs->es, regs->edi),
                vm86_inl(regs->edx & 0xFFFF));
            size = 4;
        } else {
            VbeWriteMemoryW(vi, MAKE_POINTER(regs->es, regs->edi),
                vm86_inw(regs->edx & 0xFFFF));
            size = 2;
        }
        if(regs->eflags & (1<<10))
            regs->edi -= size;
        else
            regs->edi += size;
        if(pref_rep) {
            if(pref_66) {
                regs->ecx--;
                if(regs->ecx != 0) {
                    goto again;
                } else {
                    SET_16(regs->ecx, regs->ecx - 1);
                    if(regs->ecx & 0xFFFF != 0)
                        goto again;
                }
            }
        }
        INC_IP(1);
        break;

    case 0xEE:                  /* OUT DX, AL */
        vm86_outb(regs->edx & 0xFFFF, regs->eax & 0xFF);
        INC_IP(1);
        break;
    case 0xEF:                  /* OUT DX, AX */
        if(pref_66)
            vm86_outl(regs->edx & 0xFFFF, regs->eax);
        else
            vm86_outw(regs->edx & 0xFFFF, regs->eax & 0xFFFF);
        INC_IP(1);
        break;
    case 0xE6:                  /* OUT imm8, AL */
        vm86_outb(VbeMemory(vi, MAKE_POINTER(regs->cs, regs->eip+1)),
             regs->eax & 0xFF);
        INC_IP(2);
        break;
    case 0xE7:                  /* OUT imm8, AX */
        if(pref_66)
            vm86_outl(VbeMemory(vi, MAKE_POINTER(regs->cs, regs->eip+1)),
                  regs->eax);
        else
            vm86_outw(VbeMemory(vi, MAKE_POINTER(regs->cs, regs->eip+1)),
                 regs->eax & 0xFFFF);
        INC_IP(2);
        break;
    case 0x6E:                  /* OUTSB */
    case 0x6F:                  /* OUTSW */
        if(opcode == 0x6E) {
            vm86_outb(regs->edx & 0xFFFF, 
                 VbeMemory(vi, MAKE_POINTER(regs->es, regs->edi)));
            size = 1;
        } else if(pref_66) {
            vm86_outl(regs->edx & 0xFFFF, 
                 VbeMemory(vi, MAKE_POINTER(regs->es, regs->edi)));
            size = 4;
        } else {
            vm86_outw(regs->edx & 0xFFFF, 
                 VbeMemory(vi, MAKE_POINTER(regs->es, regs->edi)));
            size = 2;
        }
        if(regs->eflags & (1<<10))
            regs->edi -= size;
        else
            regs->edi += size;
        if(pref_rep) {
            if(pref_66) {
                regs->ecx--;
                if(regs->ecx != 0) {
                    goto again;
                } else {
                    SET_16(regs->ecx, regs->ecx - 1);
                    if(regs->ecx & 0xFFFF != 0)
                        goto again;
                }
            }
        }
        INC_IP(1);
        break;

    case 0x0F:
        ErrorF("Hit 0F trap in VM86 code\n");
        return -1;
    case 0xF0:
        ErrorF("Hit lock prefix in VM86 code\n");
        return -1;
    case 0xF4:
        ErrorF("Hit HLT in VM86 code\n");
        return -1;

    default:
        ErrorF("Unhandled GP fault in VM86 code (opcode = 0x%02X)\n",
               opcode);
        return -1;
    }
    return 0;
}
#undef SEG_CS
#undef SEG_DS
#undef SEG_ES
#undef SEG_SS
#undef SEG_GS
#undef SEG_FS
#undef REP
#undef REPNZ
#undef SET_8
#undef SET_16
#undef INC_IP
#undef AGAIN

static int
vm86_loop(VbeInfoPtr vi)
{
    int code;
    while(1) {
        code = vm86old(&vi->vms);
        switch(VM86_TYPE(code)) {
        case VM86_SIGNAL:
            continue;
        case VM86_UNKNOWN:
            code = vm86_emulate(vi);
            if(code < 0) {
                VbeDebug(vi);
                return -1;
            }
            break;
        case VM86_INTx:
            if(VM86_ARG(code) == 0xFF)
                return 0;
            else {
                PUSHW(vi, vi->vms.regs.eflags)
                PUSHW(vi, vi->vms.regs.cs);
                PUSHW(vi, vi->vms.regs.eip);
                vi->vms.regs.cs = MMW(vi,VM86_ARG(code) * 4 + 2);
                vi->vms.regs.eip = MMW(vi,VM86_ARG(code) * 4);
            }
            break;
        case VM86_STI:
            ErrorF("VM86 code enabled interrupts\n");
            VbeDebug(vi);
            return -1;
        default:
            ErrorF("Unexpected result code 0x%X from vm86\n", code);
            VbeDebug(vi);
            return -1;
        }
    }
}

int 
VbeIsMemory(VbeInfoPtr vi, U32 i) 
{
    if(i >= MAGICMEM_BASE && i< MAGICMEM_BASE + MAGICMEM_SIZE)
        return 1;
    else if(i >= LOMEM_BASE && i< LOMEM_BASE + LOMEM_SIZE)
        return 1;
    else if(i >= HIMEM_BASE && i< HIMEM_BASE + HIMEM_SIZE)
        return 1;
    else
        return 0;
}

U8 
VbeMemory(VbeInfoPtr vi, U32 i)
{
    if(i >= MAGICMEM_BASE && i< MAGICMEM_BASE + MAGICMEM_SIZE)
        return MM(vi, i);
    else if(i >= LOMEM_BASE && i< LOMEM_BASE + LOMEM_SIZE)
        return LM(vi, i);
    else if(i >= HIMEM_BASE && i< HIMEM_BASE + HIMEM_SIZE)
        return HM(vi, i);
    else {
        ErrorF("Reading unmapped memory at 0x%08X\n", i);
    }
}

U16
VbeMemoryW(VbeInfoPtr vi, U32 i)
{
    if(i >= MAGICMEM_BASE && i< MAGICMEM_BASE + MAGICMEM_SIZE)
        return MMW(vi, i);
    else if(i >= LOMEM_BASE && i< LOMEM_BASE + LOMEM_SIZE)
        return LMW(vi, i);
    else if(i >= HIMEM_BASE && i< HIMEM_BASE + HIMEM_SIZE)
        return HMW(vi, i);
    else {
        ErrorF("Reading unmapped memory at 0x%08X\n", i);
        return 0;
    }
}

U32
VbeMemoryL(VbeInfoPtr vi, U32 i)
{
    if(i >= MAGICMEM_BASE && i< MAGICMEM_BASE + MAGICMEM_SIZE)
        return MML(vi, i);
    else if(i >= LOMEM_BASE && i< LOMEM_BASE + LOMEM_SIZE)
        return LML(vi, i);
    else if(i >= HIMEM_BASE && i< HIMEM_BASE + HIMEM_SIZE)
        return HML(vi, i);
    else {
        ErrorF("Reading unmapped memory at 0x%08X\n", i);
        return 0;
    }
}

void
VbeWriteMemory(VbeInfoPtr vi, U32 i, U8 val)
{
    if(i >= MAGICMEM_BASE && i< MAGICMEM_BASE + MAGICMEM_SIZE)
        MM(vi, i) = val;
    else if(i >= LOMEM_BASE && i< LOMEM_BASE + LOMEM_SIZE)
        LM(vi, i) = val;
    else if(i >= HIMEM_BASE && i< HIMEM_BASE + HIMEM_SIZE)
        HM(vi, i) = val;
    else {
        ErrorF("Writing unmapped memory at 0x%08X\n", i);
    }
}

void
VbeWriteMemoryW(VbeInfoPtr vi, U32 i, U16 val)
{
    if(i >= MAGICMEM_BASE && i< MAGICMEM_BASE + MAGICMEM_SIZE)
        MMW(vi, i) = val;
    else if(i >= LOMEM_BASE && i< LOMEM_BASE + LOMEM_SIZE)
        LMW(vi, i) = val;
    else if(i >= HIMEM_BASE && i< HIMEM_BASE + HIMEM_SIZE)
        HMW(vi, i) = val;
    else {
        ErrorF("Writing unmapped memory at 0x%08X\n", i);
    }
}

void
VbeWriteMemoryL(VbeInfoPtr vi, U32 i, U32 val)
{
    if(i >= MAGICMEM_BASE && i< MAGICMEM_BASE + MAGICMEM_SIZE)
        MML(vi, i) = val;
    else if(i >= LOMEM_BASE && i< LOMEM_BASE + LOMEM_SIZE)
        LML(vi, i) = val;
    else if(i >= HIMEM_BASE && i< HIMEM_BASE + HIMEM_SIZE)
        HML(vi, i) = val;
    else {
        ErrorF("Writing unmapped memory at 0x%08X\n", i);
    }
}

int
VbeAllocateMemory(VbeInfoPtr vi, int n)
{
    int ret;
    if(n<0) {
        ErrorF("Asked to allocate negative amount of memory\n");
        return vi->brk;
    }
      
    n = (n + 15) & ~15;
    if(vi->brk + n > LOMEM_BASE + LOMEM_SIZE) {
        ErrorF("Out of low memory\n");
        exit(2);
    }
    ret = vi->brk;
    vi->brk += n;
    return ret;
}

static int
vm86old(struct vm86_struct *vm)
{
    int res;
    asm volatile (
        "pushl %%ebx\n\t"
        "movl %2, %%ebx\n\t"
        "movl %1,%%eax\n\t"
        "int $0x80\n\t"
        "popl %%ebx"
        : "=a" (res)  : "n" (113), "r" (vm));
    if(res < 0) {
        errno = -res;
        res = -1;
    } else 
        errno = 0;
    return res;
}

void
VbeDebug(VbeInfoPtr vi)
{
    struct vm86_regs *regs = &vi->vms.regs;
    int i;

    ErrorF("eax=0x%08lX ebx=0x%08lX ecx=0x%08lX edx=0x%08lX\n",
           regs->eax, regs->ebx, regs->ecx, regs->edx);
    ErrorF("esi=0x%08lX edi=0x%08lX ebp=0x%08lX\n",
           regs->esi, regs->edi, regs->ebp);
    ErrorF("eip=0x%08lX esp=0x%08lX eflags=0x%08lX\n",
           regs->eip, regs->esp, regs->eflags);
    ErrorF("cs=0x%04lX      ds=0x%04lX      es=0x%04lX      fs=0x%04lX      gs=0x%04lX\n",
           regs->cs, regs->ds, regs->es, regs->fs, regs->gs);
    for(i=-7; i<8; i++) {
        ErrorF(" %s%02X",
               i==0?"->":"",
               VbeMemory(vi, MAKE_POINTER(regs->cs, regs->eip + i)));
    }
    ErrorF("\n");
}

#ifdef NOT_IN_X_SERVER
static void 
ErrorF(char *f, ...)
{
    va_list args;
    va_start(args, f);
    vfprintf(stderr, f, args);
    va_end(args);
}
#endif

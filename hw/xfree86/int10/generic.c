/* $XFree86: xc/programs/Xserver/hw/xfree86/int10/generic.c,v 1.29 2003/09/24 02:43:33 dawes Exp $ */
/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"
#define _INT10_PRIVATE
#include "xf86int10.h"
#include "int10Defines.h"

#define ALLOC_ENTRIES(x) ((V_RAM / x) - 1)

static CARD8 read_b(xf86Int10InfoPtr pInt,int addr);
static CARD16 read_w(xf86Int10InfoPtr pInt,int addr);
static CARD32 read_l(xf86Int10InfoPtr pInt,int addr);
static void write_b(xf86Int10InfoPtr pInt,int addr, CARD8 val);
static void write_w(xf86Int10InfoPtr pInt,int addr, CARD16 val);
static void write_l(xf86Int10InfoPtr pInt,int addr, CARD32 val);

/*
 * the emulator cannot pass a pointer to the current xf86Int10InfoRec
 * to the memory access functions therefore store it here.
 */

typedef struct {
    int shift;
    int entries;
    void* base;
    void* vRam;
    int highMemory;
    void* sysMem;
    char* alloc;
} genericInt10Priv;

#define INTPriv(x) ((genericInt10Priv*)x->private)

int10MemRec genericMem = {
    read_b,
    read_w,
    read_l,
    write_b,
    write_w,
    write_l
};

static void MapVRam(xf86Int10InfoPtr pInt);
static void UnmapVRam(xf86Int10InfoPtr pInt);
#ifdef _PC
#define GET_HIGH_BASE(x) (((V_BIOS + size + getpagesize() - 1)/getpagesize()) \
                             * getpagesize())
#endif

static void *sysMem = NULL;

xf86Int10InfoPtr
xf86InitInt10(int entityIndex)
{
    return xf86ExtendedInitInt10(entityIndex, 0);
}

xf86Int10InfoPtr
xf86ExtendedInitInt10(int entityIndex, int Flags)
{
    xf86Int10InfoPtr pInt;
    void* base = 0;
    void* vbiosMem = 0;
    void* options = NULL;
    pciVideoPtr pvp;
    int screen;
    legacyVGARec vga;
    xf86int10BiosLocation bios;
    
#ifdef _PC
    int size;
    CARD32 cs;
#endif

    screen = (xf86FindScreenForEntity(entityIndex))->scrnIndex;

    options = xf86HandleInt10Options(xf86Screens[screen],entityIndex);
    
    if (int10skip(options)) {
	xfree(options);
	return NULL;
    }
    
    pInt = (xf86Int10InfoPtr)xnfcalloc(1, sizeof(xf86Int10InfoRec));
    pInt->entityIndex = entityIndex;
    if (!xf86Int10ExecSetup(pInt))
	goto error0;
    pInt->mem = &genericMem;
    pInt->private = (pointer)xnfcalloc(1, sizeof(genericInt10Priv));
    INTPriv(pInt)->alloc = (pointer)xnfcalloc(1, ALLOC_ENTRIES(getpagesize()));
    pInt->scrnIndex = screen;
    base = INTPriv(pInt)->base = xnfalloc(SYS_BIOS);

    pvp = xf86GetPciInfoForEntity(entityIndex);
    if (pvp) pInt->Tag = ((pciConfigPtr)(pvp->thisCard))->tag;

    /*
     * we need to map video RAM MMIO as some chipsets map mmio
     * registers into this range.
     */
    MapVRam(pInt);
#ifdef _PC
    if (!sysMem)
	sysMem = xf86MapVidMem(screen, VIDMEM_MMIO, V_BIOS,
			       BIOS_SIZE + SYS_BIOS - V_BIOS);
    INTPriv(pInt)->sysMem = sysMem;

    if (xf86ReadBIOS(0, 0, base, LOW_PAGE_SIZE) < 0) {
	xf86DrvMsg(screen, X_ERROR, "Cannot read int vect\n");
	goto error1;
    }

    /*
     * Retrieve everything between V_BIOS and SYS_BIOS as some system BIOSes
     * have executable code there.  Note that xf86ReadBIOS() can only read in
     * 64kB at a time.
     */
    (void)memset((char *)base + V_BIOS, 0, SYS_BIOS - V_BIOS);
#if 0
    for (cs = V_BIOS;  cs < SYS_BIOS;  cs += V_BIOS_SIZE)
	if (xf86ReadBIOS(cs, 0, (unsigned char *)base + cs, V_BIOS_SIZE) <
		V_BIOS_SIZE)
	    xf86DrvMsg(screen, X_WARNING,
		       "Unable to retrieve all of segment 0x%06X.\n", cs);
#endif
    INTPriv(pInt)->highMemory = V_BIOS;
    
    xf86int10ParseBiosLocation(options,&bios);
    
    if (xf86IsEntityPrimary(entityIndex) 
	&& !(initPrimary(options))) {
	
	if (bios.bus == BUS_ISA && bios.location.legacy) {
	    xf86DrvMsg(screen, X_CONFIG,
			   "Overriding BIOS location: 0x%x\n",
		       bios.location.legacy);
	    cs = bios.location.legacy >> 4;
#define CHECK_V_SEGMENT_RANGE(x)   \
               if (((x) << 4) < V_BIOS) {\
		   xf86DrvMsg(screen, X_ERROR, \
		              "V_BIOS address 0x%lx out of range\n", \
			      (unsigned long)(x) << 4); \
		    goto error1; \
	       }
	    CHECK_V_SEGMENT_RANGE(cs);
	    vbiosMem = (unsigned char *)sysMem - V_BIOS + (cs << 4);
	    if (!int10_check_bios(screen, cs, vbiosMem)) {
		xf86DrvMsg(screen, X_ERROR,
			   "No V_BIOS at specified address 0x%lx\n",
			   (unsigned long)cs << 4);
		goto error1;
	    }
	} else {
	    if (bios.bus == BUS_PCI) {
		xf86DrvMsg(screen, X_WARNING,
			   "Option BiosLocation for primary device ignored: "
			   "It points to PCI.\n");
		xf86DrvMsg(screen, X_WARNING,
			   "You must set Option InitPrimary also\n");
	    }
	    
	    cs = MEM_RW(pInt,((0x10<<2)+2));
	    CHECK_V_SEGMENT_RANGE(cs);
	    vbiosMem = (unsigned char *)sysMem - V_BIOS + (cs << 4);
	    if (!int10_check_bios(screen, cs, vbiosMem)) {
		cs = MEM_RW(pInt, (0x42 << 2) + 2);
		CHECK_V_SEGMENT_RANGE(cs);
		vbiosMem = (unsigned char *)sysMem - V_BIOS + (cs << 4);
		if (!int10_check_bios(screen, cs, vbiosMem)) {
		    cs = V_BIOS >> 4;
		    vbiosMem = (unsigned char *)sysMem - V_BIOS + (cs << 4);
		    if (!int10_check_bios(screen, cs, vbiosMem)) {
			xf86DrvMsg(screen, X_ERROR, "No V_BIOS found\n");
			goto error1;
		    }
		}
	    }
	}
	
	xf86DrvMsg(screen, X_INFO, "Primary V_BIOS segment is: 0x%lx\n",
		   (unsigned long)cs);

	set_return_trap(pInt);
	pInt->BIOSseg = cs;

	pInt->Flags = Flags & (SET_BIOS_SCRATCH | RESTORE_BIOS_SCRATCH);
	if (! (pInt->Flags & SET_BIOS_SCRATCH))
	    pInt->Flags &= ~RESTORE_BIOS_SCRATCH;
	xf86Int10SaveRestoreBIOSVars(pInt, TRUE);
	
    } else {
	BusType location_type;
	int bios_location = V_BIOS;
	int pci_entity;
	
        EntityInfoPtr pEnt = xf86GetEntityInfo(pInt->entityIndex);
        reset_int_vect(pInt);
	set_return_trap(pInt);

	if (bios.bus != BUS_NONE) {
	    switch (location_type = bios.bus) {
	    case BUS_PCI:
		xf86DrvMsg(screen,X_CONFIG,"Overriding bios location: "
			   "PCI:%i:%i%i\n",bios.location.pci.bus,
			   bios.location.pci.dev,bios.location.pci.func);
		break;
	    case BUS_ISA:
		bios_location = bios.location.legacy;
		if (bios.location.legacy)
		    xf86DrvMsg(screen,X_CONFIG,"Overriding bios location: "
			       "Legacy:0x%x\n",bios.location.legacy);
		else
		    xf86DrvMsg(screen,X_CONFIG,"Overriding bios location: "
			       "Legacy\n");
		break;
	    default:
		break;
	    }
	} else
	    location_type = pEnt->location.type;
	
	switch (location_type) {
	case BUS_PCI:
	    vbiosMem = (unsigned char *)base + bios_location;
	    if (bios.bus == BUS_PCI)
		pci_entity = xf86GetPciEntity(bios.location.pci.bus,
					      bios.location.pci.dev,
					      bios.location.pci.func);
	    else 
		pci_entity = pInt->entityIndex;
	    if (!(size = mapPciRom(pci_entity,(unsigned char *)(vbiosMem)))) {
		xf86DrvMsg(screen,X_ERROR,"Cannot read V_BIOS (3)\n");
		goto error1;
	    }
	    INTPriv(pInt)->highMemory = GET_HIGH_BASE(size);
	    break;
	case BUS_ISA:
	    vbiosMem = (unsigned char *)sysMem + bios_location;
#if 0
	    (void)memset(vbiosMem, 0, V_BIOS_SIZE);
	    if (xf86ReadBIOS(bios_location, 0, vbiosMem, V_BIOS_SIZE)
		< V_BIOS_SIZE)
		xf86DrvMsg(screen, X_WARNING,
		    "Unable to retrieve all of segment 0x%x.\n",bios_location);
#endif
	    if (!int10_check_bios(screen, bios_location >> 4, vbiosMem)) {
	        xf86DrvMsg(screen,X_ERROR,"Cannot read V_BIOS (4)\n");
		goto error1;
	    }
	default:
	    goto error1;
	}
	xfree(pEnt);
	pInt->BIOSseg = V_BIOS >> 4;
	pInt->num = 0xe6;
	LockLegacyVGA(pInt, &vga);
	xf86ExecX86int10(pInt);
	UnlockLegacyVGA(pInt, &vga);
    }
#else
    if (!sysMem) {
	sysMem = xnfalloc(BIOS_SIZE);
	setup_system_bios(sysMem);
    }
    INTPriv(pInt)->sysMem = sysMem;
    setup_int_vect(pInt);
    set_return_trap(pInt);

    /*
     * Retrieve two segments:  one at V_BIOS, the other 64kB beyond the first.
     * This'll catch any BIOS that might have been initialised before server
     * entry.
     */
    vbiosMem = (char *)base + V_BIOS;
    (void)memset(vbiosMem, 0, 2 * V_BIOS_SIZE);
    if (xf86ReadDomainMemory(pInt->Tag, V_BIOS, V_BIOS_SIZE, vbiosMem) <
	V_BIOS_SIZE)
	xf86DrvMsg(screen, X_WARNING,
	    "Unable to retrieve all of segment 0x0C0000.\n");
    else if ((((unsigned char *)vbiosMem)[0] == 0x55) &&
	     (((unsigned char *)vbiosMem)[1] == 0xAA) &&
	     (((unsigned char *)vbiosMem)[2] > 0x80))
    if (xf86ReadDomainMemory(pInt->Tag, V_BIOS + V_BIOS_SIZE, V_BIOS_SIZE,
	    (unsigned char *)vbiosMem + V_BIOS_SIZE) < V_BIOS_SIZE)
	xf86DrvMsg(screen, X_WARNING,
	    "Unable to retrieve all of segment 0x0D0000.\n");

    /*
     * If this adapter is the primary, use its post-init BIOS (if we can find
     * it).
     */
    xf86int10ParseBiosLocation(options,&bios);
    
    {
	int bios_location = V_BIOS;
	Bool done = FALSE;
	vbiosMem = (unsigned char *)base + bios_location;
	
	if ((bios.bus == BUS_ISA)
	    || (bios.bus != BUS_PCI && xf86IsEntityPrimary(entityIndex))) {
		if (bios.bus == BUS_ISA && bios.location.legacy) {
		    xf86DrvMsg(screen, X_CONFIG,"Looking for legacy V_BIOS "
			       "at 0x%x for %sprimary device\n",
			       bios.location.legacy,
			       xf86IsEntityPrimary(entityIndex) ? "" : "non-");
		    bios_location = bios.location.legacy;
		    vbiosMem = (unsigned char *)base + bios_location;
		}
		if (int10_check_bios(screen, bios_location >> 4, vbiosMem)) 
		    done = TRUE;
		else 
		    xf86DrvMsg(screen,X_INFO,
			       "No legacy BIOS found -- trying PCI\n");
	} 
	if (!done) {
	    int pci_entity;
	    
	    if (bios.bus == BUS_PCI) {
		xf86DrvMsg(screen,X_CONFIG,"Looking for BIOS at PCI:%i%i%i\n",
			   bios.location.pci.bus,bios.location.pci.dev,
			   bios.location.pci.func);		
		pci_entity = xf86GetPciEntity(bios.location.pci.bus,
					      bios.location.pci.dev,
					      bios.location.pci.func);
	    } else 
		pci_entity = pInt->entityIndex;

	    if (!mapPciRom(pci_entity, vbiosMem)) {
		    xf86DrvMsg(screen, X_ERROR, "Cannot read V_BIOS (5)\n");
		    goto error1;
	    }
	} 

    }

    pInt->BIOSseg = V_BIOS >> 4;
    pInt->num = 0xe6;
    LockLegacyVGA(pInt, &vga);
    xf86ExecX86int10(pInt);
    UnlockLegacyVGA(pInt, &vga);
#endif
    xfree(options);
    return pInt;

 error1:
    xfree(base);
    UnmapVRam(pInt);
    xfree(INTPriv(pInt)->alloc);
    xfree(pInt->private);
 error0:
    xfree(pInt);
    xfree(options);
    
    return NULL;
}

static void
MapVRam(xf86Int10InfoPtr pInt)
{
    int pagesize = getpagesize();
    int size = ((VRAM_SIZE + pagesize - 1) / pagesize) * pagesize;

    INTPriv(pInt)->vRam = xf86MapDomainMemory(pInt->scrnIndex, VIDMEM_MMIO,
					      pInt->Tag, V_RAM, size);

    pInt->ioBase = xf86Screens[pInt->scrnIndex]->domainIOBase;
}

static void
UnmapVRam(xf86Int10InfoPtr pInt)
{
    int screen = pInt->scrnIndex;
    int pagesize = getpagesize();
    int size = ((VRAM_SIZE + pagesize - 1)/pagesize) * pagesize;

    xf86UnMapVidMem(screen, INTPriv(pInt)->vRam, size);
}

Bool
MapCurrentInt10(xf86Int10InfoPtr pInt)
{
    /* nothing to do here */
    return TRUE;
}

void
xf86FreeInt10(xf86Int10InfoPtr pInt)
{
    if (!pInt)
      return;
#if defined (_PC)
    xf86Int10SaveRestoreBIOSVars(pInt, FALSE);
#endif
    if (Int10Current == pInt)
	Int10Current = NULL;
    xfree(INTPriv(pInt)->base);
    UnmapVRam(pInt);
    xfree(INTPriv(pInt)->alloc);
    xfree(pInt->private);
    xfree(pInt);
}

void *
xf86Int10AllocPages(xf86Int10InfoPtr pInt, int num, int *off)
{
    int pagesize = getpagesize();
    int num_pages = ALLOC_ENTRIES(pagesize);
    int i,j;

    for (i = 0; i < (num_pages - num); i++) {
	if (INTPriv(pInt)->alloc[i] == 0) {
	    for (j = i; j < (num + i); j++)
		if (INTPriv(pInt)->alloc[j] != 0)
		    break;
	    if (j == (num + i))
		break;
	    i += num;
	}
    }
    if (i == (num_pages - num))
	return NULL;

    for (j = i; j < (i + num); j++)
	INTPriv(pInt)->alloc[j] = 1;

    *off = (i + 1) * pagesize;

    return (char *)INTPriv(pInt)->base + *off;
}

void
xf86Int10FreePages(xf86Int10InfoPtr pInt, void *pbase, int num)
{
    int pagesize = getpagesize();
    int first = (((char *)pbase - (char *)INTPriv(pInt)->base) / pagesize) - 1;
    int i;

    for (i = first; i < (first + num); i++)
	INTPriv(pInt)->alloc[i] = 0;
}

#define OFF(addr) ((addr) & 0xffff)
#if defined _PC
# define HIGH_OFFSET (INTPriv(pInt)->highMemory)
# define HIGH_BASE   V_BIOS
#else
# define HIGH_OFFSET SYS_BIOS
# define HIGH_BASE   SYS_BIOS
#endif
# define SYS(addr) ((addr) >= HIGH_OFFSET)
#define V_ADDR(addr) \
	  (SYS(addr) ? ((char*)INTPriv(pInt)->sysMem) + (addr - HIGH_BASE) \
	   : (((char*)(INTPriv(pInt)->base) + addr)))
#define VRAM_ADDR(addr) (addr - V_RAM)
#define VRAM_BASE (INTPriv(pInt)->vRam)

#define VRAM(addr) ((addr >= V_RAM) && (addr < (V_RAM + VRAM_SIZE)))
#define V_ADDR_RB(addr) \
	(VRAM(addr)) ? MMIO_IN8((CARD8*)VRAM_BASE,VRAM_ADDR(addr)) \
	   : *(CARD8*) V_ADDR(addr)
#define V_ADDR_RW(addr) \
	(VRAM(addr)) ? MMIO_IN16((CARD16*)VRAM_BASE,VRAM_ADDR(addr)) \
	   : ldw_u((pointer)V_ADDR(addr))
#define V_ADDR_RL(addr) \
	(VRAM(addr)) ? MMIO_IN32((CARD32*)VRAM_BASE,VRAM_ADDR(addr)) \
	   : ldl_u((pointer)V_ADDR(addr))

#define V_ADDR_WB(addr,val) \
	if(VRAM(addr)) \
	    MMIO_OUT8((CARD8*)VRAM_BASE,VRAM_ADDR(addr),val); \
	else \
	    *(CARD8*) V_ADDR(addr) = val;
#define V_ADDR_WW(addr,val) \
	if(VRAM(addr)) \
	    MMIO_OUT16((CARD16*)VRAM_BASE,VRAM_ADDR(addr),val); \
	else \
	    stw_u((val),(pointer)(V_ADDR(addr)));

#define V_ADDR_WL(addr,val) \
	if (VRAM(addr)) \
	    MMIO_OUT32((CARD32*)VRAM_BASE,VRAM_ADDR(addr),val); \
	else \
	    stl_u(val,(pointer)(V_ADDR(addr)));

static CARD8
read_b(xf86Int10InfoPtr pInt, int addr)
{
    return V_ADDR_RB(addr);
}

static CARD16
read_w(xf86Int10InfoPtr pInt, int addr)
{
#if X_BYTE_ORDER == X_LITTLE_ENDIAN
    if (OFF(addr + 1) > 0)
	return V_ADDR_RW(addr);
#endif
    return V_ADDR_RB(addr) | (V_ADDR_RB(addr + 1) << 8);
}

static CARD32
read_l(xf86Int10InfoPtr pInt, int addr)
{
#if X_BYTE_ORDER == X_LITTLE_ENDIAN
    if (OFF(addr + 3) > 2)
	return V_ADDR_RL(addr);
#endif
    return V_ADDR_RB(addr) |
	   (V_ADDR_RB(addr + 1) << 8) |
	   (V_ADDR_RB(addr + 2) << 16) |
	   (V_ADDR_RB(addr + 3) << 24);
}

static void
write_b(xf86Int10InfoPtr pInt, int addr, CARD8 val)
{
    V_ADDR_WB(addr,val);
}

static void
write_w(xf86Int10InfoPtr pInt, int addr, CARD16 val)
{
#if X_BYTE_ORDER == X_LITTLE_ENDIAN
    if (OFF(addr + 1) > 0)
      { V_ADDR_WW(addr, val); }
#endif
    V_ADDR_WB(addr, val);
    V_ADDR_WB(addr + 1, val >> 8);
}

static void
write_l(xf86Int10InfoPtr pInt, int addr, CARD32 val)
{
#if X_BYTE_ORDER == X_LITTLE_ENDIAN
    if (OFF(addr + 3) > 2)
      { V_ADDR_WL(addr, val); }
#endif
    V_ADDR_WB(addr, val);
    V_ADDR_WB(addr + 1, val >> 8);
    V_ADDR_WB(addr + 2, val >> 16);
    V_ADDR_WB(addr + 3, val >> 24);
}

pointer
xf86int10Addr(xf86Int10InfoPtr pInt, CARD32 addr)
{
    return V_ADDR(addr);
}

#ifndef _PM2_H_
#define _PM2_H_
#include <vesa.h>
#include "kxv.h"
#include "klinux.h"

#include "glint_regs.h"

typedef volatile CARD8	VOL8;
typedef volatile CARD16	VOL16;
typedef volatile CARD32	VOL32;

#if 0
typedef struct {
    VOL32 StartXDom;
    VOL32 dXDom;
    VOL32 StartXSub;
    VOL32 dXSub;
    VOL32 StartY;
    VOL32 dY;
    VOL32 GLINTCount;
    VOL32 Render;
    VOL32 ContinueNewLine;
    VOL32 ContinueNewDom;
    VOL32 ContinueNewSub;
    VOL32 Continue;
    VOL32 FlushSpan;
    VOL32 BitMaskPattern;
} PMRender;

typedef struct {
    VOL32 PointTable0;
    VOL32 PointTable1;
    VOL32 PointTable2;
    VOL32 PointTable3;
    VOL32 RasterizerMode;
    VOL32 YLimits;
    VOL32 ScanLineOwnership;
    VOL32 WaitForCompletion;
    VOL32 PixelSize;
    VOL32 XLimits;
    VOL32 RectangleOrigin;
    VOL32 RectangleSize;
} PMRectangle;

typedef struct {
    VOL32 FilterMode;
    VOL32 StatisticMode;
    VOL32 MinRegion;
    VOL32 MaxRegion;
    VOL32 ResetPickResult;
    VOL32 MitHitRegion;
    VOL32 MaxHitRegion;
    VOL32 PickResult;
    VOL32 GlintSync;
    VOL32 reserved00;
    VOL32 reserved01;
    VOL32 reserved02;
    VOL32 reserved03;
    VOL32 FBBlockColorU;
    VOL32 FBBlockColorL;
    VOL32 SuspendUntilFrameBlank;
} PMMode;

typedef struct {
    VOL32 ScissorMode;
    VOL32 ScissorMinXY;
    VOL32 ScissorMaxXY;
    VOL32 ScreenSize;
    VOL32 AreaStippleMode;
    VOL32 LineStippleMode;
    VOL32 LoadLineStippleCounters;
    VOL32 UpdateLineStippleCounters;
    VOL32 SaveLineStippleState;
    VOL32 WindowOrigin;
} PMScissor;

typedef struct {
    VOL32 RStart;
    VOL32 dRdx;
    VOL32 dRdyDom;
    VOL32 GStart;
    VOL32 dGdx;
    VOL32 dGdyDom;
    VOL32 BStart;
    VOL32 dBdx;
    VOL32 dBdyDom;
    VOL32 AStart;
    VOL32 dAdx;
    VOL32 dAdyDom;
    VOL32 ColorDDAMode;
    VOL32 ConstantColor;
    VOL32 GLINTColor;
} PMColor;
#endif

#define PM2_REG_BASE(c)		((c)->attr.address[0] & 0xFFFFC000)
#define PM2_REG_SIZE(c)		(0x10000)

#define minb(p) *(volatile CARD8 *)(pm2c->reg_base + (p))
#define moutb(p,v) *(volatile CARD8 *)(pm2c->reg_base + (p)) = (v)


/* Memory mapped register access macros */
#define INREG8(addr)        *(volatile CARD8  *)(pm2c->reg_base + (addr))
#define INREG16(addr)       *(volatile CARD16 *)(pm2c->reg_base + (addr))
#define INREG(addr)         *(volatile CARD32 *)(pm2c->reg_base + (addr))

#define OUTREG8(addr, val) do {				\
   *(volatile CARD8 *)(pm2c->reg_base  + (addr)) = (val);	\
} while (0)

#define OUTREG16(addr, val) do {			\
   *(volatile CARD16 *)(pm2c->reg_base + (addr)) = (val);	\
} while (0)

#define OUTREG(addr, val) do {				\
   *(volatile CARD32 *)(pm2c->reg_base + (addr)) = (val);	\
} while (0)

typedef struct _PM2CardInfo {
    VesaCardPrivRec	vesa;
    CARD8		*reg_base;

    int			in_fifo_space;
    int			fifo_size;

    int			pprod;
    int			bppalign;

    int			clipping_on;

    int			ROP;

} PM2CardInfo;

#define getPM2CardInfo(kd)	((PM2CardInfo *) ((kd)->card->driver))
#define pmCardInfo(kd)	PM2CardInfo	*pm2c = getPM2CardInfo(kd)

typedef struct _PM2ScreenInfo {
    VesaScreenPrivRec		vesa;
    CARD8			*cursor_base;
    CARD8			*screen;
    CARD8			*off_screen;
    int				off_screen_size;
    KdVideoAdaptorPtr		pAdaptor;
} PM2ScreenInfo;

#define getPM2ScreenInfo(kd) ((PM2ScreenInfo *) ((kd)->screen->driver))
#define pmScreenInfo(kd)    PM2ScreenInfo *pm2s = getPM2ScreenInfo(kd)

Bool 
pmCardInit (KdCardInfo *card);

Bool 
pmScreenInit (KdScreenInfo *screen);

Bool        
pmDrawInit(ScreenPtr);

void
pmDrawEnable (ScreenPtr);

void
pmDrawSync (ScreenPtr);

void
pmDrawDisable (ScreenPtr);

void
pmDrawFini (ScreenPtr);


extern KdCardFuncs  PM2Funcs;

#define PM2R_MEM_CONFIG					0x10c0

#define PM2F_MEM_CONFIG_RAM_MASK			(3L<<29)
#define PM2F_MEM_BANKS_1				0L
#define PM2F_MEM_BANKS_2				(1L<<29)
#define PM2F_MEM_BANKS_3				(2L<<29)
#define PM2F_MEM_BANKS_4				(3L<<29)

#endif /* _PM2_H_ */

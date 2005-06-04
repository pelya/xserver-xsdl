#include "randrstr.h"

#define KD_BUTTON_1     0x01
#define KD_BUTTON_2     0x02
#define KD_BUTTON_3     0x04
#define KD_BUTTON_4     0x08
#define KD_BUTTON_5     0x10
#define KD_MOUSE_DELTA  0x80000000

typedef struct _KdMouseFuncs {
    Bool            (*Init) (void);
    void            (*Fini) (void);
} KdMouseFuncs;

typedef struct _KdKeyboardFuncs {
    void            (*Load) (void);
    int             (*Init) (void);
    void            (*Leds) (int);
    void            (*Bell) (int, int, int);
    void            (*Fini) (void);
    int             LockLed;
} KdKeyboardFuncs;

typedef struct _KdOsFuncs {
    int             (*Init) (void);
    void            (*Enable) (void);
    Bool            (*SpecialKey) (KeySym);
    void            (*Disable) (void);
    void            (*Fini) (void);
    void            (*pollEvents) (void);
} KdOsFuncs;

typedef struct _KdMouseMatrix {
    int     matrix[2][3];
} KdMouseMatrix;

typedef enum _kdMouseState {
    start,
    button_1_pend,
    button_1_down,
    button_2_down,
    button_3_pend,
    button_3_down,
    synth_2_down_13,
    synth_2_down_3,
    synth_2_down_1,
    num_input_states
} KdMouseState;

#define KD_MAX_BUTTON  7

typedef struct _KdMouseInfo {
    struct _KdMouseInfo *next;
    void                *driver;
    void                *closure;
    char                *name;
    char                *prot;
    char                map[KD_MAX_BUTTON];
    int                 nbutton;
    Bool                emulateMiddleButton;
    unsigned long       emulationTimeout;
    Bool                timeoutPending;
    KdMouseState        mouseState;
    Bool                eventHeld;
    xEvent              heldEvent;
    unsigned char       buttonState;
    int                 emulationDx, emulationDy;
    int                 inputType;
    Bool                transformCoordinates;
} KdMouseInfo;

typedef struct _KdScreenInfo {
    struct _KdScreenInfo    *next;
    ScreenPtr   pScreen;
    void        *driver;
    Rotation    randr;  /* rotation and reflection */
    int         width;
    int         height;
    int         rate;
    int         width_mm;
    int         height_mm;
    int         subpixel_order;
    Bool        dumb;
    Bool        softCursor;
    int         mynum;
    DDXPointRec origin;
} KdScreenInfo;

#define KD_MAX_FB 2
#define KD_MAX_PSEUDO_DEPTH 8
#define KD_MAX_PSEUDO_SIZE           (1 << KD_MAX_PSEUDO_DEPTH)

typedef struct _xeglScreen {
    Window             win;
    Colormap           colormap;
    CloseScreenProcPtr CloseScreen;
    KdScreenInfo       *screen;
    ColormapPtr     pInstalledmap[KD_MAX_FB];         /* current colormap */
} xeglScreenRec, *xeglScreenPtr;

extern KdMouseInfo *kdMouseInfo;
extern KdOsFuncs *kdOsFuncs;
extern Bool kdDontZap;
extern Bool kdDisableZaphod;
extern int kdScreenPrivateIndex;
extern KdMouseFuncs LinuxEvdevMouseFuncs;
extern KdKeyboardFuncs LinuxEvdevKeyboardFuncs;

#define RR_Rotate_All   (RR_Rotate_0|RR_Rotate_90|RR_Rotate_180|RR_Rotate_270)
#define RR_Reflect_All  (RR_Reflect_X|RR_Reflect_Y)

#define KdGetScreenPriv(pScreen) ((xeglScreenPtr) \
                                  (pScreen)->devPrivates[kdScreenPrivateIndex].ptr)
#define KdScreenPriv(pScreen) xeglScreenPtr pScreenPriv = KdGetScreenPriv(pScreen)

extern void eglInitInput(KdMouseFuncs *pMouseFuncs, KdKeyboardFuncs *pKeyboardFuncs);
extern void KdParseMouse(char *arg);
extern KdMouseInfo *KdMouseInfoAdd(void);
extern void KdMouseInfoDispose(KdMouseInfo *mi);
extern int KdAllocInputType(void);
extern char *KdSaveString (char *str);
extern Bool KdRegisterFd(int type, int fd, void (*read) (int fd, void *closure), void *closure);
extern void KdUnregisterFds(int type, Bool do_close);
extern void KdEnqueueKeyboardEvent(unsigned char scan_code, unsigned char is_up);
extern void KdEnqueueMouseEvent(KdMouseInfo *mi, unsigned long flags, int rx, int ry);
extern void KdRegisterFdEnableDisable(int fd,
                                int (*enable)(int fd, void *closure),
                                void (*disable)(int fd, void *closure));
extern void KdWakeupHandler(int screen, pointer data, unsigned long lresult, pointer readmask);



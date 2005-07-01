/*************************************************************************************
 * $Id$
 * 
 * Created by Bogdan D. bogdand@users.sourceforge.net 
 * License: GPL
 *
 * $Log$
 * Revision 1.2  2005/07/01 22:43:11  daniels
 * Change all misc.h and os.h references to <X11/foo.h>.
 *
 *
 ************************************************************************************/

#ifndef __UDA1380_H__
#define __UDA1380_H__

#include "xf86i2c.h"

typedef struct {
	I2CDevRec d;
	
	CARD16 analog_mixer_settings;	/* register 0x03 */
	
	} UDA1380Rec, *UDA1380Ptr;

#define UDA1380_ADDR_1   0x30
#define UDA1380_ADDR_2   0x34

UDA1380Ptr Detect_uda1380(I2CBusPtr b, I2CSlaveAddr addr);
Bool uda1380_init(UDA1380Ptr t);
void uda1380_shutdown(UDA1380Ptr t);
void uda1380_setvolume(UDA1380Ptr t, INT32);
void uda1380_mute(UDA1380Ptr t, Bool);
void uda1380_setparameters(UDA1380Ptr t);
void uda1380_getstatus(UDA1380Ptr t);
void uda1380_dumpstatus(UDA1380Ptr t);

#define UDA1380SymbolsList  \
		"Detect_uda1380", \
		"uda1380_init", \
		"uda1380_shutdown", \
		"uda1380_setvolume", \
		"uda1380_mute", \
		"uda1380_setparameters", \
		"uda1380_getstatus", \
		"uda1380_dumpstatus"

#ifdef XFree86LOADER

#define xf86_Detect_uda1380       ((UDA1380Ptr (*)(I2CBusPtr, I2CSlaveAddr))LoaderSymbol("Detect_uda1380"))
#define xf86_uda1380_init         ((Bool (*)(UDA1380Ptr))LoaderSymbol("uda1380_init"))
#define xf86_uda1380_shutdown     ((void (*)(UDA1380Ptr))LoaderSymbol("uda1380_shutdown"))
#define xf86_uda1380_setvolume         ((void (*)(UDA1380Ptr, CARD16))LoaderSymbol("uda1380_setvolume"))
#define xf86_uda1380_mute         ((void (*)(UDA1380Ptr, Bool))LoaderSymbol("uda1380_mute"))
#define xf86_uda1380_setparameters     ((void (*)(UDA1380Ptr))LoaderSymbol("uda1380_setparameters"))
#define xf86_uda1380_getstatus    ((void (*)(UDA1380Ptr))LoaderSymbol("uda1380_getstatus"))
#define xf86_uda1380_dumpstatus    ((void (*)(UDA1380Ptr))LoaderSymbol("uda1380_dumpstatus"))

#else

#define xf86_Detect_uda1380       Detect_uda1380
#define xf86_uda1380_init         uda1380_init
#define xf86_uda1380_shutdown         uda1380_shutdown
#define xf86_uda1380_setvolume    uda1380_setvolume
#define xf86_uda1380_mute         uda1380_mute
#define xf86_uda1380_setparameters     uda1380_setparameters
#define xf86_uda1380_getstatus    uda1380_getstatus
#define xf86_uda1380_dumpstatus    uda1380_dumpstatus

#endif

#endif

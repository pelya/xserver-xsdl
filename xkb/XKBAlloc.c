/* $Xorg: XKBAlloc.c,v 1.4 2000/08/17 19:44:59 cpqbld Exp $ */
/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be 
used in advertising or publicity pertaining to distribution 
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability 
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, 
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/
/* $XFree86: xc/lib/X11/XKBAlloc.c,v 3.5 2001/01/17 19:41:48 dawes Exp $ */

#ifndef XKB_IN_SERVER

#include <stdio.h>
#define NEED_REPLIES
#define NEED_EVENTS
#include "Xlibint.h"
#include "XKBlibint.h"
#include <X11/extensions/XKBgeom.h>
#include <X11/extensions/XKBproto.h>
#include "XKBlibint.h"

#else 

#include <stdio.h>
#include "X.h"
#define	NEED_EVENTS
#define	NEED_REPLIES
#include "Xproto.h"
#include "misc.h"
#include "inputstr.h"
#include "XKBsrv.h"
#include "XKBgeom.h"

#endif /* XKB_IN_SERVER */

/***===================================================================***/

/*ARGSUSED*/
Status
#if NeedFunctionPrototypes
XkbAllocCompatMap(XkbDescPtr xkb,unsigned which,unsigned nSI)
#else
XkbAllocCompatMap(xkb,which,nSI)
    XkbDescPtr	xkb;
    unsigned	which;
    unsigned	nSI;
#endif
{
XkbCompatMapPtr	compat;
XkbSymInterpretRec *prev_interpret;

    if (!xkb)
	return BadMatch;
    if (xkb->compat) {
	if (xkb->compat->size_si>=nSI)
	    return Success;
	compat= xkb->compat;
	compat->size_si= nSI;
	if (compat->sym_interpret==NULL)
	    compat->num_si= 0;
	prev_interpret = compat->sym_interpret;
	compat->sym_interpret= _XkbTypedRealloc(compat->sym_interpret,
						     nSI,XkbSymInterpretRec);
	if (compat->sym_interpret==NULL) {
	    _XkbFree(prev_interpret);
	    compat->size_si= compat->num_si= 0;
	    return BadAlloc;
	}
	if (compat->num_si!=0) {
	    _XkbClearElems(compat->sym_interpret,compat->num_si,
					compat->size_si-1,XkbSymInterpretRec);
	}
	return Success;
    }
    compat= _XkbTypedCalloc(1,XkbCompatMapRec);
    if (compat==NULL)
	return BadAlloc;
    if (nSI>0) {
	compat->sym_interpret= _XkbTypedCalloc(nSI,XkbSymInterpretRec);
	if (!compat->sym_interpret) {
	    _XkbFree(compat);
	    return BadAlloc;
	}
    }
    compat->size_si= nSI;
    compat->num_si= 0;
    bzero((char *)&compat->groups[0],XkbNumKbdGroups*sizeof(XkbModsRec));
    xkb->compat= compat;
    return Success;
}


void
#if NeedFunctionPrototypes
XkbFreeCompatMap(XkbDescPtr xkb,unsigned which,Bool freeMap)
#else
XkbFreeCompatMap(xkb,which,freeMap)
    XkbDescPtr	xkb;
    unsigned	which;
    Bool	freeMap;
#endif
{
register XkbCompatMapPtr compat;

    if ((xkb==NULL)||(xkb->compat==NULL))
	return;
    compat= xkb->compat;
    if (freeMap)
	which= XkbAllCompatMask;
    if (which&XkbGroupCompatMask)
	bzero((char *)&compat->groups[0],XkbNumKbdGroups*sizeof(XkbModsRec));
    if (which&XkbSymInterpMask) {
	if ((compat->sym_interpret)&&(compat->size_si>0))
	    _XkbFree(compat->sym_interpret);
	compat->size_si= compat->num_si= 0;
	compat->sym_interpret= NULL;
    }
    if (freeMap) {
	_XkbFree(compat);
	xkb->compat= NULL;
    }
    return;
}

/***===================================================================***/

Status
#if NeedFunctionPrototypes
XkbAllocNames(XkbDescPtr xkb,unsigned which,int nTotalRG,int nTotalAliases)
#else
XkbAllocNames(xkb,which,nTotalRG,nTotalAliases)
    XkbDescPtr	xkb;
    unsigned	which;
    int		nTotalRG;
    int		nTotalAliases;
#endif
{
XkbNamesPtr	names;

    if (xkb==NULL)
	return BadMatch;
    if (xkb->names==NULL) {
	xkb->names = _XkbTypedCalloc(1,XkbNamesRec);
	if (xkb->names==NULL)
	    return BadAlloc;
    }
    names= xkb->names;
    if ((which&XkbKTLevelNamesMask)&&(xkb->map!=NULL)&&(xkb->map->types!=NULL)){
	register int	i;
	XkbKeyTypePtr	type;

	type= xkb->map->types;
	for (i=0;i<xkb->map->num_types;i++,type++) {
	    if (type->level_names==NULL) {
		type->level_names= _XkbTypedCalloc(type->num_levels,Atom);
		if (type->level_names==NULL)
		    return BadAlloc;
	    }
	}
    }
    if ((which&XkbKeyNamesMask)&&(names->keys==NULL)) {
	if ((!XkbIsLegalKeycode(xkb->min_key_code))||
	    (!XkbIsLegalKeycode(xkb->max_key_code))||
	    (xkb->max_key_code<xkb->min_key_code)) 
	    return BadValue;
	names->keys= _XkbTypedCalloc((xkb->max_key_code+1),XkbKeyNameRec);
	if (names->keys==NULL)
	    return BadAlloc;
    }
    if ((which&XkbKeyAliasesMask)&&(nTotalAliases>0)) {
	if (names->key_aliases==NULL) {
	    names->key_aliases= _XkbTypedCalloc(nTotalAliases,XkbKeyAliasRec);
	}
	else if (nTotalAliases>names->num_key_aliases) {
	    XkbKeyAliasRec *prev_aliases = names->key_aliases;

	    names->key_aliases= _XkbTypedRealloc(names->key_aliases,
						nTotalAliases,XkbKeyAliasRec);
	    if (names->key_aliases!=NULL) {
		_XkbClearElems(names->key_aliases,names->num_key_aliases,
						nTotalAliases-1,XkbKeyAliasRec);
	    } else {
		_XkbFree(prev_aliases);
	    }
	}
	if (names->key_aliases==NULL) {
	    names->num_key_aliases= 0;
	    return BadAlloc;
	}
	names->num_key_aliases= nTotalAliases;
    }
    if ((which&XkbRGNamesMask)&&(nTotalRG>0)) {
	if (names->radio_groups==NULL) {
	    names->radio_groups= _XkbTypedCalloc(nTotalRG,Atom);
	}
	else if (nTotalRG>names->num_rg) {
	    Atom *prev_radio_groups = names->radio_groups;

	    names->radio_groups= _XkbTypedRealloc(names->radio_groups,nTotalRG,
									Atom);
	    if (names->radio_groups!=NULL) {
		_XkbClearElems(names->radio_groups,names->num_rg,nTotalRG-1,
									Atom);
	    } else {
		_XkbFree(prev_radio_groups);
	    }
	}
	if (names->radio_groups==NULL)
	    return BadAlloc;
	names->num_rg= nTotalRG;
    }
    return Success;
}

void
#if NeedFunctionPrototypes
XkbFreeNames(XkbDescPtr xkb,unsigned which,Bool freeMap)
#else
XkbFreeNames(xkb,which,freeMap)
    XkbDescPtr	xkb;
    unsigned	which;
    Bool	freeMap;
#endif
{
XkbNamesPtr	names;

    if ((xkb==NULL)||(xkb->names==NULL))
	return;
    names= xkb->names;
    if (freeMap)
	which= XkbAllNamesMask; 
    if (which&XkbKTLevelNamesMask) {
	XkbClientMapPtr	map= xkb->map;
	if ((map!=NULL)&&(map->types!=NULL)) {
	    register int 		i;
	    register XkbKeyTypePtr	type;
	    type= map->types;
	    for (i=0;i<map->num_types;i++,type++) {
		if (type->level_names!=NULL) {
		    _XkbFree(type->level_names);
		    type->level_names= NULL;
		}
	    }
	}
    }
    if ((which&XkbKeyNamesMask)&&(names->keys!=NULL)) {
	_XkbFree(names->keys);
	names->keys= NULL;
	names->num_keys= 0;
    }
    if ((which&XkbKeyAliasesMask)&&(names->key_aliases)){
	_XkbFree(names->key_aliases);
	names->key_aliases=NULL;
	names->num_key_aliases=0;
    }
    if ((which&XkbRGNamesMask)&&(names->radio_groups)) {
	_XkbFree(names->radio_groups);
	names->radio_groups= NULL;
	names->num_rg= 0;
    }
    if (freeMap) {
	_XkbFree(names);
	xkb->names= NULL;
    }
    return;
}

/***===================================================================***/

/*ARGSUSED*/
Status
#if NeedFunctionPrototypes
XkbAllocControls(XkbDescPtr xkb,unsigned which)
#else
XkbAllocControls(xkb,which)
    XkbDescPtr		xkb;
    unsigned		which;
#endif
{
    if (xkb==NULL)
	return BadMatch;

    if (xkb->ctrls==NULL) {
	xkb->ctrls= _XkbTypedCalloc(1,XkbControlsRec);
	if (!xkb->ctrls)
	    return BadAlloc;
    }
    return Success;
}

/*ARGSUSED*/
void
#if NeedFunctionPrototypes
XkbFreeControls(XkbDescPtr xkb,unsigned which,Bool freeMap)
#else
XkbFreeControls(xkb,which,freeMap)
    XkbDescPtr		xkb;
    unsigned		which;
    Bool		freeMap;
#endif
{
    if (freeMap && (xkb!=NULL) && (xkb->ctrls!=NULL)) {
	_XkbFree(xkb->ctrls);
	xkb->ctrls= NULL;
    }
    return;
}

/***===================================================================***/

Status 
#if NeedFunctionPrototypes
XkbAllocIndicatorMaps(XkbDescPtr xkb)
#else
XkbAllocIndicatorMaps(xkb)
    XkbDescPtr	xkb;
#endif
{
    if (xkb==NULL)
	return BadMatch;
    if (xkb->indicators==NULL) {
	xkb->indicators= _XkbTypedCalloc(1,XkbIndicatorRec);
	if (!xkb->indicators)
	    return BadAlloc;
    }
    return Success;
}

void
#if NeedFunctionPrototypes
XkbFreeIndicatorMaps(XkbDescPtr xkb)
#else
XkbFreeIndicatorMaps(xkb)
    XkbDescPtr	xkb;
#endif
{
    if ((xkb!=NULL)&&(xkb->indicators!=NULL)) {
	_XkbFree(xkb->indicators);
	xkb->indicators= NULL;
    }
    return;
}

/***====================================================================***/

XkbDescRec	*
#if NeedFunctionPrototypes
XkbAllocKeyboard(void)
#else
XkbAllocKeyboard()
#endif
{
XkbDescRec *xkb;

    xkb = _XkbTypedCalloc(1,XkbDescRec);
    if (xkb)
	xkb->device_spec= XkbUseCoreKbd;
    return xkb;
}

void
#if NeedFunctionPrototypes
XkbFreeKeyboard(XkbDescPtr xkb,unsigned which,Bool freeAll)
#else
XkbFreeKeyboard(xkb,which,freeAll)
    XkbDescPtr	xkb;
    unsigned	which;
    Bool	freeAll;
#endif
{
    if (xkb==NULL)
	return;
    if (freeAll)
	which= XkbAllComponentsMask;
    if (which&XkbClientMapMask)
	XkbFreeClientMap(xkb,XkbAllClientInfoMask,True);
    if (which&XkbServerMapMask)
	XkbFreeServerMap(xkb,XkbAllServerInfoMask,True);
    if (which&XkbCompatMapMask)
	XkbFreeCompatMap(xkb,XkbAllCompatMask,True);
    if (which&XkbIndicatorMapMask)
	XkbFreeIndicatorMaps(xkb);
    if (which&XkbNamesMask)
	XkbFreeNames(xkb,XkbAllNamesMask,True);
    if ((which&XkbGeometryMask) && (xkb->geom!=NULL))
	XkbFreeGeometry(xkb->geom,XkbGeomAllMask,True);
    if (which&XkbControlsMask)
	XkbFreeControls(xkb,XkbAllControlsMask,True);
    if (freeAll)
	_XkbFree(xkb);
    return;
}

/***====================================================================***/

XkbDeviceLedInfoPtr
#if NeedFunctionPrototypes
XkbAddDeviceLedInfo(XkbDeviceInfoPtr devi,unsigned ledClass,unsigned ledId)
#else
XkbAddDeviceLedInfo(devi,ledClass,ledId)
    XkbDeviceInfoPtr	devi;
    unsigned		ledClass;
    unsigned		ledId;
#endif
{
XkbDeviceLedInfoPtr	devli;
register int		i;

    if ((!devi)||(!XkbSingleXIClass(ledClass))||(!XkbSingleXIId(ledId)))
	return NULL;
    for (i=0,devli=devi->leds;i<devi->num_leds;i++,devli++) {
	if ((devli->led_class==ledClass)&&(devli->led_id==ledId))
	    return devli;
    }
    if (devi->num_leds>=devi->sz_leds) {
	XkbDeviceLedInfoRec *prev_leds = devi->leds;
	
	if (devi->sz_leds>0)	devi->sz_leds*= 2;
	else			devi->sz_leds= 1;
	devi->leds= _XkbTypedRealloc(devi->leds,devi->sz_leds,
							XkbDeviceLedInfoRec);
	if (!devi->leds) {
	    _XkbFree(prev_leds);
	    devi->sz_leds= devi->num_leds= 0;
	    return NULL;
	}
	i= devi->num_leds;
	for (devli=&devi->leds[i];i<devi->sz_leds;i++,devli++) {
	    bzero(devli,sizeof(XkbDeviceLedInfoRec));
	    devli->led_class= XkbXINone;
	    devli->led_id= XkbXINone;
	}
    }
    devli= &devi->leds[devi->num_leds++];
    bzero(devli,sizeof(XkbDeviceLedInfoRec));
    devli->led_class= ledClass;
    devli->led_id= ledId;
    return devli;
}

Status
#if NeedFunctionPrototypes
XkbResizeDeviceButtonActions(XkbDeviceInfoPtr devi,unsigned newTotal)
#else
XkbResizeDeviceButtonActions(devi,newTotal)
    XkbDeviceInfoPtr	devi;
    unsigned		newTotal;
#endif
{
    XkbAction *prev_btn_acts;

    if ((!devi)||(newTotal>255))
	return BadValue;
    if ((devi->btn_acts!=NULL)&&(newTotal==devi->num_btns))
	return Success;
    if (newTotal==0) {
	if (devi->btn_acts!=NULL) {
	    _XkbFree(devi->btn_acts);
	    devi->btn_acts= NULL;
	}
	devi->num_btns= 0;
	return Success;
    }
    prev_btn_acts = devi->btn_acts;
    devi->btn_acts= _XkbTypedRealloc(devi->btn_acts,newTotal,XkbAction);
    if (devi->btn_acts==NULL) {
	_XkbFree(prev_btn_acts);
	devi->num_btns= 0;
	return BadAlloc;
    }
    if (newTotal>devi->num_btns) {
	XkbAction *act;
	act= &devi->btn_acts[devi->num_btns];
	bzero((char *)act,(newTotal-devi->num_btns)*sizeof(XkbAction));
    }
    devi->num_btns= newTotal;
    return Success;
}

/*ARGSUSED*/
XkbDeviceInfoPtr
#if NeedFunctionPrototypes
XkbAllocDeviceInfo(unsigned deviceSpec,unsigned nButtons,unsigned szLeds)
#else
XkbAllocDeviceInfo(deviceSpec,nButtons,szLeds)
    unsigned	deviceSpec;
    unsigned	nButtons;
    unsigned	szLeds;
#endif
{
XkbDeviceInfoPtr	devi;

    devi= _XkbTypedCalloc(1,XkbDeviceInfoRec);
    if (devi!=NULL) {
	devi->device_spec= deviceSpec;
	devi->has_own_state= False;
	devi->num_btns= 0;
	devi->btn_acts= NULL;
	if (nButtons>0) {
	    devi->num_btns= nButtons;
	    devi->btn_acts= _XkbTypedCalloc(nButtons,XkbAction);
	    if (!devi->btn_acts) {
		_XkbFree(devi);
		return NULL;
	    }
	}
	devi->dflt_kbd_fb= XkbXINone;
	devi->dflt_led_fb= XkbXINone;
	devi->num_leds= 0;
	devi->sz_leds= 0;
	devi->leds= NULL;
	if (szLeds>0) {
	    devi->sz_leds= szLeds;
	    devi->leds= _XkbTypedCalloc(szLeds,XkbDeviceLedInfoRec);
	    if (!devi->leds) {
		if (devi->btn_acts)
		    _XkbFree(devi->btn_acts);
		_XkbFree(devi);
		return NULL;
	    }
	}
    }
    return devi;
}


void 
#if NeedFunctionPrototypes
XkbFreeDeviceInfo(XkbDeviceInfoPtr devi,unsigned which,Bool freeDevI)
#else
XkbFreeDeviceInfo(devi,which,freeDevI)
    XkbDeviceInfoPtr	devi;
    unsigned		which;
    Bool		freeDevI;
#endif
{
    if (devi) {
	if (freeDevI) {
	    which= XkbXI_AllDeviceFeaturesMask;
	    if (devi->name) {
		_XkbFree(devi->name);
		devi->name= NULL;
	    }
	}
	if ((which&XkbXI_ButtonActionsMask)&&(devi->btn_acts)) {
	    _XkbFree(devi->btn_acts);
	    devi->num_btns= 0;
	    devi->btn_acts= NULL;
	}
	if ((which&XkbXI_IndicatorsMask)&&(devi->leds)) {
	    register int i;
	    if ((which&XkbXI_IndicatorsMask)==XkbXI_IndicatorsMask) {
		_XkbFree(devi->leds);
		devi->sz_leds= devi->num_leds= 0;
		devi->leds= NULL;
	    }
	    else {
		XkbDeviceLedInfoPtr	devli;
		for (i=0,devli=devi->leds;i<devi->num_leds;i++,devli++) {
		    if (which&XkbXI_IndicatorMapsMask)
			 bzero((char *)&devli->maps[0],sizeof(devli->maps));
		    else bzero((char *)&devli->names[0],sizeof(devli->names));
		}
	    }
	}
	if (freeDevI)
	    _XkbFree(devi);
    }
    return;
}

/* $Xorg: ddxList.c,v 1.3 2000/08/17 19:53:46 cpqbld Exp $ */
/************************************************************
Copyright (c) 1995 by Silicon Graphics Computer Systems, Inc.

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
/* $XFree86: xc/programs/Xserver/xkb/ddxList.c,v 3.7 2001/10/28 03:34:19 tsi Exp $ */

#include <stdio.h>
#include <ctype.h>
#define	NEED_EVENTS 1
#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/extensions/XKM.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#define	XKBSRV_NEED_FILE_FUNCS
#include "XKBsrv.h"
#include "XI.h"

#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define	PATH_MAX MAXPATHLEN
#else
#define	PATH_MAX 1024
#endif
#endif

/***====================================================================***/

static char *componentDirs[_XkbListNumComponents] = {
	"keymap", "keycodes", "types", "compat", "symbols", "geometry"
};

/***====================================================================***/

Status
#if NeedFunctionPrototypes
_AddListComponent(	XkbSrvListInfoPtr	list,
			int			what,
			unsigned		flags,
			char *			str,
			ClientPtr		client)
#else
_AddListComponent(list,what,flags,str,client)
    XkbSrvListInfoPtr	list;
    int			what;
    unsigned		flags;
    char *		str;
    ClientPtr		client;
#endif
{
int		slen,wlen;
unsigned char *	wire8;
unsigned short *wire16;
char *		tmp;

    if (list->nTotal>=list->maxRtrn) {
	list->nTotal++;
	return Success;
    }
    tmp= strchr(str,')');
    if ((tmp==NULL)&&((tmp=strchr(str,'('))==NULL)) {
	slen= strlen(str);
	while ((slen>0) && isspace(str[slen-1])) {
	    slen--;
	}
    }
    else {
	slen= (tmp-str+1);
    }
    wlen= (((slen+1)/2)*2)+4;	/* four bytes for flags and length, pad to */
				/* 2-byte boundary */
    if ((list->szPool-list->nPool)<wlen) {
	if (wlen>1024)	list->szPool+= XkbPaddedSize(wlen*2);
	else		list->szPool+= 1024;
	list->pool= _XkbTypedRealloc(list->pool,list->szPool,char);
	if (!list->pool)
	    return BadAlloc;
    }
    wire16= (unsigned short *)&list->pool[list->nPool];
    wire8= (unsigned char *)&wire16[2];
    wire16[0]= flags;
    wire16[1]= slen;
    memcpy(wire8,str,slen);
    if (client->swapped) {
	register int n;
	swaps(&wire16[0],n);
	swaps(&wire16[1],n);
    }
    list->nPool+= wlen;
    list->nFound[what]++;
    list->nTotal++;
    return Success;
}

/***====================================================================***/
static Status
#if NeedFunctionPrototypes
XkbDDXListComponent(	DeviceIntPtr 		dev,
			int			what,
			XkbSrvListInfoPtr	list,
			ClientPtr		client)
#else
XkbDDXListComponent(dev,what,list,client)
    DeviceIntPtr	dev;
    int			what;
    XkbSrvListInfoPtr	list;
    ClientPtr		client;
#endif
{
char 	*file,*map,*tmp,buf[PATH_MAX];
FILE 	*in;
Status	status;
int	rval;
Bool	haveDir;
#ifdef WIN32
char tmpname[32];
#endif    

    if ((list->pattern[what]==NULL)||(list->pattern[what][0]=='\0'))
	return Success;
    file= list->pattern[what];
    map= strrchr(file,'(');
    if (map!=NULL) {
	char *tmp;
	map++;
	tmp= strrchr(map,')');
	if ((tmp==NULL)||(tmp[1]!='\0')) {
	    /* illegal pattern.  No error, but no match */
	    return Success;
	}
    }

    in= NULL;
    haveDir= True;
#ifdef WIN32
    strcpy(tmpname, "\\temp\\xkb_XXXXXX");
    (void) mktemp(tmpname);
#endif
    if (XkbBaseDirectory!=NULL) {
	if (strlen(XkbBaseDirectory)+strlen(componentDirs[what])+6 > PATH_MAX)
	    return BadImplementation;
	if ((list->pattern[what][0]=='*')&&(list->pattern[what][1]=='\0')) {
	    sprintf(buf,"%s/%s.dir",XkbBaseDirectory,componentDirs[what]);
	    in= fopen(buf,"r");
	}
	if (!in) {
	    haveDir= False;
	    if (strlen(XkbBaseDirectory)*2+strlen(componentDirs[what])
		    +(xkbDebugFlags>9?2:1)+strlen(file)+31 > PATH_MAX)
		return BadImplementation;
#ifndef WIN32
	    sprintf(buf,"%s/xkbcomp -R%s/%s -w %ld -l -vlfhpR '%s'",
		XkbBaseDirectory,XkbBaseDirectory,componentDirs[what],(long)
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:xkbDebugFlags)),
		file);
#else
	    sprintf(buf,"%s/xkbcomp -R%s/%s -w %ld -l -vlfhpR '%s' %s",
		XkbBaseDirectory,XkbBaseDirectory,componentDirs[what],(long)
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:xkbDebugFlags)),
		file, tmpname);
#endif
	}
    }
    else {
	if (strlen(XkbBaseDirectory)+strlen(componentDirs[what])+6 > PATH_MAX)
	    return BadImplementation;
	if ((list->pattern[what][0]=='*')&&(list->pattern[what][1]=='\0')) {
	    sprintf(buf,"%s.dir",componentDirs[what]);
	    in= fopen(buf,"r");
	}
	if (!in) {
	    haveDir= False;
	    if (strlen(componentDirs[what])
		    +(xkbDebugFlags>9?2:1)+strlen(file)+29 > PATH_MAX)
		return BadImplementation;
#ifndef WIN32
	    sprintf(buf,"xkbcomp -R%s -w %ld -l -vlfhpR '%s'",
		componentDirs[what],(long)
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:xkbDebugFlags)),
		file);
#else
	    sprintf(buf,"xkbcomp -R%s -w %ld -l -vlfhpR '%s' %s",
		componentDirs[what],(long)
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:xkbDebugFlags)),
		file, tmpname);
#endif
	}
    }
    status= Success;
    if (!haveDir)
#ifndef WIN32
	in= Popen(buf,"r");
#else
    {
	if (System(buf) < 0)
	    ErrorF("Could not invoke keymap compiler\n");
	else
	    in= fopen(tmpname, "r");
    }
#endif
    if (!in)
	return BadImplementation;
    list->nFound[what]= 0;
    while ((status==Success)&&((tmp=fgets(buf,PATH_MAX,in))!=NULL)) {
	unsigned flags;
	register unsigned int i;
	if (*tmp=='#') /* comment, skip it */
	    continue;
	if (!strncmp(tmp, "Warning:", 8) || !strncmp(tmp, "        ", 8))
	    /* skip warnings too */
	    continue;
	flags= 0;
	/* each line in the listing is supposed to start with two */
	/* groups of eight characters, which specify the general  */
	/* flags and the flags that are specific to the component */
	/* if they're missing, fail with BadImplementation	  */
	for (i=0;(i<8)&&(status==Success);i++) { /* read the general flags */
	   if (isalpha(*tmp))	flags|= (1L<<i);
	   else if (*tmp!='-')	status= BadImplementation;
	   tmp++;
	}
	if (status != Success)  break;
	if (!isspace(*tmp)) {
	     status= BadImplementation;
	     break;
	}
	else tmp++;
	for (i=0;(i<8)&&(status==Success);i++) { /* read the component flags */
	   if (isalpha(*tmp))	flags|= (1L<<(i+8));
	   else if (*tmp!='-')	status= BadImplementation;
	   tmp++;
	}
	if (status != Success)  break;
	if (isspace(*tmp)) {
	    while (isspace(*tmp)) {
		tmp++;
	    }
	}
	else {
	    status= BadImplementation;
	    break;
	}
	status= _AddListComponent(list,what,flags,tmp,client);
    }
#ifndef WIN32
    if (haveDir)
	fclose(in);
    else if ((rval=pclose(in))!=0) {
	if (xkbDebugFlags)
	    ErrorF("xkbcomp returned exit code %d\n",rval);
    }
#else
    fclose(in);
#endif
    return status;
}

/***====================================================================***/

/* ARGSUSED */
Status
#if NeedFunctionPrototypes
XkbDDXList(DeviceIntPtr	dev,XkbSrvListInfoPtr list,ClientPtr client)
#else
XkbDDXList(dev,list,client)
    DeviceIntPtr		dev;
    XkbSrvListInfoPtr		list;
    ClientPtr			client;
#endif
{
Status	status;

    status= XkbDDXListComponent(dev,_XkbListKeymaps,list,client);
    if (status==Success)
	status= XkbDDXListComponent(dev,_XkbListKeycodes,list,client);
    if (status==Success)
	status= XkbDDXListComponent(dev,_XkbListTypes,list,client);
    if (status==Success)
	status= XkbDDXListComponent(dev,_XkbListCompat,list,client);
    if (status==Success)
	status= XkbDDXListComponent(dev,_XkbListSymbols,list,client);
    if (status==Success)
	status= XkbDDXListComponent(dev,_XkbListGeometry,list,client);
    return status;
}

/* $Xorg: maprules.c,v 1.4 2000/08/17 19:46:43 cpqbld Exp $ */
/************************************************************
 Copyright (c) 1996 by Silicon Graphics Computer Systems, Inc.

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

#include <stdio.h>
#include <ctype.h>
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif


#define X_INCLUDE_STRING_H
#define XOS_USE_NO_LOCKING
#include <X11/Xos_r.h>

#ifndef XKB_IN_SERVER

#include <X11/Xproto.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xfuncs.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>
#include "XKMformat.h"
#include "XKBfileInt.h"
#include "XKBrules.h"

#else

#define NEED_EVENTS
#include <X11/Xproto.h>
#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xfuncs.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include "misc.h"
#include "inputstr.h"
#include "dix.h"
#include "XKBstr.h"
#define XKBSRV_NEED_FILE_FUNCS
#include "XKBsrv.h"

#endif

#ifdef DEBUG
#define PR_DEBUG(s)	fprintf(stderr,s)
#define PR_DEBUG1(s,a)	fprintf(stderr,s,a)
#else
#define PR_DEBUG(s)
#define PR_DEBUG1(s,a)
#endif

/***====================================================================***/

#define DFLT_LINE_SIZE	128

typedef struct {
	int	line_num;
	int	sz_line;
	int	num_line;
	char	buf[DFLT_LINE_SIZE];
	char *	line;
} InputLine;

static void
#if NeedFunctionPrototypes
InitInputLine(InputLine *line)
#else
InitInputLine(line)
    InputLine *	line;
#endif
{
    line->line_num= 1;
    line->num_line= 0;
    line->sz_line= DFLT_LINE_SIZE;
    line->line=	line->buf;
    return;
}

static void
#if NeedFunctionPrototypes
FreeInputLine(InputLine *line)
#else
FreeInputLine(line)
    InputLine *line;
#endif
{
    if (line->line!=line->buf)
	_XkbFree(line->line);
    line->line_num= 1;
    line->num_line= 0;
    line->sz_line= DFLT_LINE_SIZE;
    line->line= line->buf;
    return;
}

static int
#if NeedFunctionPrototypes
InputLineAddChar(InputLine *line,int ch)
#else
InputLineAddChar(line,ch)
    InputLine *	line;
    int		ch;
#endif
{
    if (line->num_line>=line->sz_line) {
	if (line->line==line->buf) {
	    line->line= (char *)_XkbAlloc(line->sz_line*2);
	    memcpy(line->line,line->buf,line->sz_line);
	}
	else {
	    line->line=(char *)_XkbRealloc((char *)line->line,line->sz_line*2);
	}
	line->sz_line*= 2;
    }
    line->line[line->num_line++]= ch;
    return ch;
}

#define	ADD_CHAR(l,c)	((l)->num_line<(l)->sz_line?\
				(int)((l)->line[(l)->num_line++]= (c)):\
				InputLineAddChar(l,c))

static Bool
#if NeedFunctionPrototypes
GetInputLine(FILE *file,InputLine *line,Bool checkbang)
#else
GetInputLine(file,line,checkbang)
    FILE *	file;
    InputLine *	line;
    Bool	checkbang;
#endif
{
int	ch;
Bool	endOfFile,spacePending,slashPending,inComment;

     endOfFile= False;
     while ((!endOfFile)&&(line->num_line==0)) {
	spacePending= slashPending= inComment= False;
	while (((ch=getc(file))!='\n')&&(ch!=EOF)) {
	    if (ch=='\\') {
		if ((ch=getc(file))==EOF)
		    break;
		if (ch=='\n') {
		    inComment= False;
		    ch= ' ';
		    line->line_num++;
		}
	    }
	    if (inComment)
		continue;
	    if (ch=='/') {
		if (slashPending) {
		    inComment= True;
		    slashPending= False;
		}
		else {
		    slashPending= True;
		}
		continue;
	    }
	    else if (slashPending) {
		if (spacePending) {
		    ADD_CHAR(line,' ');
		    spacePending= False;
		}
		ADD_CHAR(line,'/');
		slashPending= False;
	    }
	    if (isspace(ch)) {
		while (isspace(ch)&&(ch!='\n')&&(ch!=EOF)) {
		    ch= getc(file);
		}
		if (ch==EOF)
		    break;
		if ((ch!='\n')&&(line->num_line>0))
		    spacePending= True;
		ungetc(ch,file);
	    }
	    else {
		if (spacePending) {
		    ADD_CHAR(line,' ');
		    spacePending= False;
		}
		if (checkbang && ch=='!') {
		    if (line->num_line!=0) {
			PR_DEBUG("The '!' legal only at start of line\n");
			PR_DEBUG("Line containing '!' ignored\n");
			line->num_line= 0;
			inComment= 0;
			break;
		    }

		}
		ADD_CHAR(line,ch);
	    }
	}
	if (ch==EOF)
	     endOfFile= True;
/*	else line->num_line++;*/
     }
     if ((line->num_line==0)&&(endOfFile))
	return False;
      ADD_CHAR(line,'\0');
      return True;
}

/***====================================================================***/

#define	MODEL		0
#define	LAYOUT		1
#define	VARIANT		2
#define	OPTION		3
#define	KEYCODES	4
#define SYMBOLS		5
#define	TYPES		6
#define	COMPAT		7
#define	GEOMETRY	8
#define	KEYMAP		9
#define	MAX_WORDS	10

#define	PART_MASK	0x000F
#define	COMPONENT_MASK	0x03F0

static	char *	cname[MAX_WORDS] = {
	"model", "layout", "variant", "option", 
	"keycodes", "symbols", "types", "compat", "geometry", "keymap"
};

typedef	struct _RemapSpec {
	int			num_remap;
	int			remap[MAX_WORDS];
} RemapSpec;

typedef struct _FileSpec {
	char *			name[MAX_WORDS];
	struct _FileSpec *	pending;
} FileSpec;

/***====================================================================***/

static void
#if NeedFunctionPrototypes
SetUpRemap(InputLine *line,RemapSpec *remap)
#else
SetUpRemap(line,remap)
   InputLine *	line;
   RemapSpec *	remap;
#endif
{
char *		tok,*str;
unsigned	present;
register int	i;
#ifdef DEBUG
Bool		found;
#endif
_Xstrtokparams	strtok_buf;

   present= 0;
   str= &line->line[1];
   bzero((char *)remap,sizeof(RemapSpec));
   while ((tok=_XStrtok(str," ",strtok_buf))!=NULL) {
#ifdef DEBUG
	found= False;
#endif
	str= NULL;
	if (strcmp(tok,"=")==0)
	    continue;
	for (i=0;i<MAX_WORDS;i++) {
	    if (strcmp(cname[i],tok)==0) {
#ifdef DEBUG
		found= True;
#endif
		if (present&(1<<i)) {
		    PR_DEBUG1("Component \"%s\" listed twice\n",tok);
		    PR_DEBUG("Second definition ignored\n");
		    break;
		}
		present|= (1<<i);
		remap->remap[remap->num_remap++]= i;
		break;
	    }
	}
#ifdef DEBUG
	if (!found) {
	    fprintf(stderr,"Unknown component \"%s\" ignored\n",tok);
	}
#endif
   }
   if ((present&PART_MASK)==0) {
#ifdef DEBUG
	unsigned mask= PART_MASK;
	fprintf(stderr,"Mapping needs at one of ");
	for (i=0;(i<MAX_WORDS)&mask;i++) {
	    if ((1L<<i)&mask) {
		mask&= ~(1L<<i);
		if (mask)	fprintf(stderr,"\"%s,\" ",cname[i]);
		else		fprintf(stderr,"or \"%s\"\n",cname[i]);
	    }
	}
	fprintf(stderr,"Illegal mapping ignored\n");
#endif
	remap->num_remap= 0;
	return;
   }
   if ((present&COMPONENT_MASK)==0) {
	PR_DEBUG("Mapping needs at least one component\n");
	PR_DEBUG("Illegal mapping ignored\n");
	remap->num_remap= 0;
	return;
   }
   if (((present&PART_MASK)&(1<<OPTION))&&
				((present&PART_MASK)!=(1<<OPTION))) {
	PR_DEBUG("Options cannot appear with other parts\n");
	PR_DEBUG("Illegal mapping ignored\n");
	remap->num_remap= 0;
	return;
   }
   if (((present&COMPONENT_MASK)&(1<<KEYMAP))&&
				((present&COMPONENT_MASK)!=(1<<KEYMAP))) {
	PR_DEBUG("Keymap cannot appear with other components\n");
	PR_DEBUG("Illegal mapping ignored\n");
	remap->num_remap= 0;
	return;
   }
   return;
}

static Bool
#if NeedFunctionPrototypes
MatchOneOf(char *wanted,char *vals_defined)
#else
MatchOneOf(wanted,vals_defined)
    char *	wanted;
    char *	vals_defined;
#endif
{
char	*str,*next;
int	want_len= strlen(wanted);

    for (str=vals_defined,next=NULL;str!=NULL;str=next) {
	int len;
	next= strchr(str,',');
	if (next) {
	    len= next-str;
	    next++;
	}
	else {
	    len= strlen(str);
	}
	if ((len==want_len)&&(strncmp(wanted,str,len)==0))
	    return True;
    }
    return False;
}

/***====================================================================***/

static Bool
#if NeedFunctionPrototypes
CheckLine(	InputLine *		line,
		RemapSpec *		remap,
		XkbRF_RulePtr		rule)
#else
CheckLine(line,remap,rule)
    InputLine *		line;
    RemapSpec *		remap;
    XkbRF_RulePtr	rule;
#endif
{
char *		str,*tok;
register int	nread;
FileSpec	tmp;
_Xstrtokparams	strtok_buf;

    if (line->line[0]=='!') {
	SetUpRemap(line,remap);
	return False;
    }
    if (remap->num_remap==0) {
	PR_DEBUG("Must have a mapping before first line of data\n");
	PR_DEBUG("Illegal line of data ignored\n");
	return False;
    }
    bzero((char *)&tmp,sizeof(FileSpec));
    str= line->line;
    for (nread= 0;(tok=_XStrtok(str," ",strtok_buf))!=NULL;nread++) {
	str= NULL;
	if (strcmp(tok,"=")==0) {
	    nread--;
	    continue;
	}
	if (nread>remap->num_remap) {
	    PR_DEBUG("Too many words on a line\n");
	    PR_DEBUG1("Extra word \"%s\" ignored\n",tok);
	    continue;
	}
	tmp.name[remap->remap[nread]]= tok;
    }
    if (nread<remap->num_remap) {
	PR_DEBUG("Too few words on a line\n");
	PR_DEBUG("line ignored\n");
	return False;
    }
    if ((tmp.name[MODEL]!=NULL)&&(strcmp(tmp.name[MODEL],"*")==0))
	tmp.name[MODEL]= NULL;
    if ((tmp.name[LAYOUT]!=NULL)&&(strcmp(tmp.name[LAYOUT],"*")==0))
	tmp.name[LAYOUT]= NULL;
    if ((tmp.name[VARIANT]!=NULL)&&(strcmp(tmp.name[VARIANT],"*")==0))
	tmp.name[VARIANT]= NULL;

    rule->flags= 0;
    if (tmp.name[OPTION])
	 rule->flags|= XkbRF_Delayed|XkbRF_Append;
    rule->model= _XkbDupString(tmp.name[MODEL]);
    rule->layout= _XkbDupString(tmp.name[LAYOUT]);
    rule->variant= _XkbDupString(tmp.name[VARIANT]);
    rule->option= _XkbDupString(tmp.name[OPTION]);

    rule->keycodes= _XkbDupString(tmp.name[KEYCODES]);
    rule->symbols= _XkbDupString(tmp.name[SYMBOLS]);
    rule->types= _XkbDupString(tmp.name[TYPES]);
    rule->compat= _XkbDupString(tmp.name[COMPAT]);
    rule->geometry= _XkbDupString(tmp.name[GEOMETRY]);
    rule->keymap= _XkbDupString(tmp.name[KEYMAP]);
    return True;
}

static char *
#if NeedFunctionPrototypes
_Concat(char *str1,char *str2)
#else
_Concat(str1,str2)
    char *	str1;
    char *	str2;
#endif
{
int len;

    if ((!str1)||(!str2))
	return str1;
   len= strlen(str1)+strlen(str2)+1;
    str1= _XkbTypedRealloc(str1,len,char);
    if (str1)
	strcat(str1,str2);
    return str1;
}

Bool
#if NeedFunctionPrototypes
XkbRF_ApplyRule(	XkbRF_RulePtr 		rule,
			XkbComponentNamesPtr	names)
#else
XkbRF_ApplyRule(rule,names)
    XkbRF_RulePtr		rule;
    XkbComponentNamesPtr	names;
#endif
{
    rule->flags&= ~XkbRF_PendingMatch; /* clear the flag because it's applied */
    if ((rule->flags&XkbRF_Append)==0) {
	if ((names->keycodes==NULL)&&(rule->keycodes!=NULL))
	    names->keycodes= _XkbDupString(rule->keycodes);

	if ((names->symbols==NULL)&&(rule->symbols!=NULL))
	    names->symbols= _XkbDupString(rule->symbols);

	if ((names->types==NULL)&&(rule->types!=NULL))
	    names->types= _XkbDupString(rule->types);

	if ((names->compat==NULL)&&(rule->compat!=NULL))
	    names->compat= _XkbDupString(rule->compat);

	if ((names->geometry==NULL)&&(rule->geometry!=NULL))
	    names->geometry= _XkbDupString(rule->geometry);

	if ((names->keymap==NULL)&&(rule->keymap!=NULL))
	    names->keymap= _XkbDupString(rule->keymap);
    }
    else {
	if (rule->keycodes)
	    names->keycodes= _Concat(names->keycodes,rule->keycodes);
	if (rule->symbols)
	    names->symbols= _Concat(names->symbols,rule->symbols);
	if (rule->types)
	    names->types= _Concat(names->types,rule->types);
	if (rule->compat)
	    names->compat= _Concat(names->compat,rule->compat);
	if (rule->geometry)
	    names->geometry= _Concat(names->geometry,rule->geometry);
	if (rule->keymap)
	    names->keymap= _Concat(names->keymap,rule->keymap);
    }
    return (names->keycodes && names->symbols && names->types &&
		names->compat && names->geometry ) || names->keymap;
}

#define	CHECK_MATCH(r,d) ((((r)[0]=='?')&&((r)[1]=='\0'))||(strcmp(r,d)==0))

Bool
#if NeedFunctionPrototypes
XkbRF_CheckApplyRule(	XkbRF_RulePtr 		rule,
			XkbRF_VarDefsPtr	defs,
			XkbComponentNamesPtr	names)
#else
XkbRF_CheckApplyRule(rule,defs,names)
    XkbRF_RulePtr		rule;
    XkbRF_VarDefsPtr		defs;
    XkbComponentNamesPtr	names;
#endif
{
    if (rule->model!=NULL) {
	if ((!defs->model)||(!CHECK_MATCH(rule->model,defs->model)))
	    return False; 
    }
    if (rule->layout!=NULL) {
	if ((!defs->layout)||(!CHECK_MATCH(rule->layout,defs->layout)))
	    return False;
    }
    if (rule->variant!=NULL) {
	if ((!defs->variant)||(!CHECK_MATCH(rule->variant,defs->variant)))
	    return False;
    }
    if (rule->option!=NULL) {
	if ((!defs->options)||(!MatchOneOf(rule->option,defs->options)))
	    return False;
    }

    if ((!rule->option)&&
   	 ((!rule->model)||(!rule->layout)||(!rule->variant))) {
	/* partial map -- partial maps are applied in the order they */
	/* appear, but all partial maps come before any options. */
	rule->flags|= XkbRF_PendingMatch;
	return False;
    }
    /* exact match, apply it now */
    return XkbRF_ApplyRule(rule,names);
}

void
#if NeedFunctionPrototypes
XkbRF_ClearPartialMatches(XkbRF_RulesPtr rules)
#else
XkbRF_ClearPartialMatches(rules)
    XkbRF_RulesPtr 	rules;
#endif
{
register int 	i;
XkbRF_RulePtr	rule;

    for (i=0,rule=rules->rules;i<rules->num_rules;i++,rule++) {
	rule->flags&= ~XkbRF_PendingMatch;
    }
}

Bool
#if NeedFunctionPrototypes
XkbRF_ApplyPartialMatches(XkbRF_RulesPtr rules,XkbComponentNamesPtr names)
#else
XkbRF_ApplyPartialMatches(rules,names)
    XkbRF_RulesPtr 		rules;
    XkbComponentNamesPtr	names;
#endif
{
int		i;
XkbRF_RulePtr	rule;
Bool		complete;

    complete= False;
    for (rule=rules->rules,i=0;(i<rules->num_rules)&&(!complete);i++,rule++) {
	if ((rule->flags&XkbRF_PendingMatch)==0)
	    continue;
	complete= XkbRF_ApplyRule(rule,names);
    }
    return complete;
}

void
#if NeedFunctionPrototypes
XkbRF_CheckApplyDelayedRules(	XkbRF_RulesPtr 		rules,
				XkbRF_VarDefsPtr	defs,
				XkbComponentNamesPtr	names)
#else
XkbRF_CheckApplyDelayedRules(rules,defs,names)
    XkbRF_RulesPtr 		rules;
    XkbRF_VarDefsPtr		defs;
    XkbComponentNamesPtr	names;
#endif
{
int		i;
XkbRF_RulePtr	rule;

    for (rule=rules->rules,i=0;(i<rules->num_rules);i++,rule++) {
	if ((rule->flags&XkbRF_Delayed)==0)
	    continue;
	XkbRF_CheckApplyRule(rule,defs,names);
    }
    return;
}

Bool
#if NeedFunctionPrototypes
XkbRF_CheckApplyRules(	XkbRF_RulesPtr 		rules,
			XkbRF_VarDefsPtr	defs,
			XkbComponentNamesPtr	names)
#else
XkbRF_CheckApplyRules(rules,defs,names)
    XkbRF_RulesPtr 		rules;
    XkbRF_VarDefsPtr		defs;
    XkbComponentNamesPtr	names;
#endif
{
int		i;
XkbRF_RulePtr	rule;
Bool		complete;

    complete= False;
    for (rule=rules->rules,i=0;(i<rules->num_rules)&&(!complete);i++,rule++) {
	if ((rule->flags&XkbRF_Delayed)!=0)
	    continue;
	complete= XkbRF_CheckApplyRule(rule,defs,names);
    }
    return complete;
}

/***====================================================================***/

char *
#if NeedFunctionPrototypes
XkbRF_SubstituteVars(char *name,XkbRF_VarDefsPtr defs)
#else
XkbRF_SubstituteVars(name,defs)
    char *		name;
    XkbRF_VarDefsPtr	defs;
#endif
{
char 	*str,*outstr,*orig;
int	len;

    orig= name;
    str= index(name,'%');
    if (str==NULL)
	return name;
    len= strlen(name);
    while (str!=NULL) {
	char pfx= str[1];
	int   extra_len= 0;
	if ((pfx=='+')||(pfx=='|')||(pfx=='_')||(pfx=='-')) {
	    extra_len= 1;
	    str++;
	}
	else if (pfx=='(') {
	    extra_len= 2;
	    str++;
	}

	if ((str[1]=='l')&&defs->layout)
	    len+= strlen(defs->layout)+extra_len;
	else if ((str[1]=='m')&&defs->model)
	    len+= strlen(defs->model)+extra_len;
	else if ((str[1]=='v')&&defs->variant)
	    len+= strlen(defs->variant)+extra_len;
	if ((pfx=='(')&&(str[2]==')')) {
	    str++;
	}
	str= index(&str[1],'%');
    }
    name= (char *)_XkbAlloc(len+1);
    str= orig;
    outstr= name;
    while (*str!='\0') {
	if (str[0]=='%') {
	    char pfx,sfx;
	    str++;
	    pfx= str[0];
	    sfx= '\0';
	    if ((pfx=='+')||(pfx=='|')||(pfx=='_')||(pfx=='-')) {
		str++;
	    }
	    else if (pfx=='(') {
		sfx= ')';
		str++;
	    }
	    else pfx= '\0';

	    if ((str[0]=='l')&&(defs->layout)) {
		if (pfx) *outstr++= pfx;
		strcpy(outstr,defs->layout);
		outstr+= strlen(defs->layout);
		if (sfx) *outstr++= sfx;
	    }
	    else if ((str[0]=='m')&&(defs->model)) {
		if (pfx) *outstr++= pfx;
		strcpy(outstr,defs->model);
		outstr+= strlen(defs->model);
		if (sfx) *outstr++= sfx;
	    }
	    else if ((str[0]=='v')&&(defs->variant)) {
		if (pfx) *outstr++= pfx;
		strcpy(outstr,defs->variant);
		outstr+= strlen(defs->variant);
		if (sfx) *outstr++= sfx;
	    }
	    str++;
	    if ((pfx=='(')&&(str[0]==')'))
		str++;
	}
	else {
	    *outstr++= *str++;
	}
    }
    *outstr++= '\0';
    if (orig!=name)
	_XkbFree(orig);
    return name;
}

/***====================================================================***/

Bool
#if NeedFunctionPrototypes
XkbRF_GetComponents(	XkbRF_RulesPtr		rules,
			XkbRF_VarDefsPtr	defs,
			XkbComponentNamesPtr	names)
#else
XkbRF_GetComponents(rules,defs,names)
    XkbRF_RulesPtr		rules;
    XkbRF_VarDefsPtr		defs;
    XkbComponentNamesPtr	names;
#endif
{
Bool		complete;

    bzero((char *)names,sizeof(XkbComponentNamesRec));
    XkbRF_ClearPartialMatches(rules);
    complete= XkbRF_CheckApplyRules(rules,defs,names);
    if (!complete)
	complete= XkbRF_ApplyPartialMatches(rules,names);
    XkbRF_CheckApplyDelayedRules(rules,defs,names);
    if (names->keycodes)
	names->keycodes= XkbRF_SubstituteVars(names->keycodes,defs);
    if (names->symbols)	
	names->symbols=	XkbRF_SubstituteVars(names->symbols,defs);
    if (names->types)
	names->types= XkbRF_SubstituteVars(names->types,defs);
    if (names->compat)
	names->compat= XkbRF_SubstituteVars(names->compat,defs);
    if (names->geometry)
	names->geometry= XkbRF_SubstituteVars(names->geometry,defs);
    if (names->keymap)	
	names->keymap= XkbRF_SubstituteVars(names->keymap,defs);
    return (names->keycodes && names->symbols && names->types &&
		names->compat && names->geometry ) || names->keymap;
}

XkbRF_RulePtr
#if NeedFunctionPrototypes
XkbRF_AddRule(XkbRF_RulesPtr	rules)
#else
XkbRF_AddRule(rules)
    XkbRF_RulesPtr	rules;
#endif
{
    if (rules->sz_rules<1) {
	rules->sz_rules= 16;
	rules->num_rules= 0;
	rules->rules= _XkbTypedCalloc(rules->sz_rules,XkbRF_RuleRec);
    }
    else if (rules->num_rules>=rules->sz_rules) {
	rules->sz_rules*= 2;
	rules->rules= _XkbTypedRealloc(rules->rules,rules->sz_rules,
							XkbRF_RuleRec);
    }
    if (!rules->rules) {
	rules->sz_rules= rules->num_rules= 0;
#ifdef DEBUG
	fprintf(stderr,"Allocation failure in XkbRF_AddRule\n");
#endif
	return NULL;
    }
    bzero((char *)&rules->rules[rules->num_rules],sizeof(XkbRF_RuleRec));
    return &rules->rules[rules->num_rules++];
}

Bool
#if NeedFunctionPrototypes
XkbRF_LoadRules(FILE *file, XkbRF_RulesPtr rules)
#else
XkbRF_LoadRules(file,rules)
    FILE *			file;
    XkbRF_RulesPtr		rules;
#endif
{
InputLine	line;
RemapSpec	remap;
XkbRF_RuleRec	trule,*rule;

    if (!(rules && file))
	return False;
    bzero((char *)&remap,sizeof(RemapSpec));
    InitInputLine(&line);
    while (GetInputLine(file,&line,True)) {
	if (CheckLine(&line,&remap,&trule)) {
	    if ((rule= XkbRF_AddRule(rules))!=NULL) {
		*rule= trule;
		bzero((char *)&trule,sizeof(XkbRF_RuleRec));
	    }
	}
	line.num_line= 0;
    }
    FreeInputLine(&line);
    return True;
}

Bool
#if NeedFunctionPrototypes
XkbRF_LoadRulesByName(char *base,char *locale,XkbRF_RulesPtr rules)
#else
XkbRF_LoadRulesByName(base,locale,rules)
    char *		base;
    char *		locale;
    XkbRF_RulesPtr	rules;
#endif
{
FILE *		file;
char		buf[PATH_MAX];
Bool		ok;

    if ((!base)||(!rules))
	return False;
    if (locale) {
	if (strlen(base)+strlen(locale)+2 > PATH_MAX)
	    return False;
	sprintf(buf,"%s-%s", base, locale);
    }
    else {
	if (strlen(base)+1 > PATH_MAX)
	    return False;
	strcpy(buf,base);
    }

    file= fopen(buf, "r");
    if ((!file)&&(locale)) { /* fallback if locale was specified */
	strcpy(buf,base);
	file= fopen(buf, "r");
    }
    if (!file)
	return False;
    ok= XkbRF_LoadRules(file,rules);
    fclose(file);
    return ok;
}

/***====================================================================***/

#define HEAD_NONE	0
#define HEAD_MODEL	1
#define HEAD_LAYOUT	2
#define HEAD_VARIANT	3
#define HEAD_OPTION	4
#define	HEAD_EXTRA	5

XkbRF_VarDescPtr
#if NeedFunctionPrototypes
XkbRF_AddVarDesc(XkbRF_DescribeVarsPtr	vars)
#else
XkbRF_AddVarDesc(vars)
    XkbRF_DescribeVarsPtr 	vars;
#endif
{
    if (vars->sz_desc<1) {
	vars->sz_desc= 16;
	vars->num_desc= 0;
	vars->desc= _XkbTypedCalloc(vars->sz_desc,XkbRF_VarDescRec);
    }
    else if (vars->num_desc>=vars->sz_desc) {
	vars->sz_desc*= 2;
	vars->desc= _XkbTypedRealloc(vars->desc,vars->sz_desc,XkbRF_VarDescRec);
    }
    if (!vars->desc) {
	vars->sz_desc= vars->num_desc= 0;
	PR_DEBUG("Allocation failure in XkbRF_AddVarDesc\n");
	return NULL;
    }
    vars->desc[vars->num_desc].name= NULL;
    vars->desc[vars->num_desc].desc= NULL;
    return &vars->desc[vars->num_desc++];
}

XkbRF_VarDescPtr
#if NeedFunctionPrototypes
XkbRF_AddVarDescCopy(XkbRF_DescribeVarsPtr vars,XkbRF_VarDescPtr from)
#else
XkbRF_AddVarDescCopy(vars,from)
    XkbRF_DescribeVarsPtr 	vars;
    XkbRF_VarDescPtr		from;
#endif
{
XkbRF_VarDescPtr	nd;

    if ((nd=XkbRF_AddVarDesc(vars))!=NULL) {
	nd->name= _XkbDupString(from->name);
	nd->desc= _XkbDupString(from->desc);
    }
    return nd;
}

XkbRF_DescribeVarsPtr 
#if NeedFunctionPrototypes
XkbRF_AddVarToDescribe(XkbRF_RulesPtr rules,char *name)
#else
XkbRF_AddVarToDescribe(rules,name)
    XkbRF_RulesPtr	rules;
    char *		name;
#endif
{
    if (rules->sz_extra<1) {
	rules->num_extra= 0;
	rules->sz_extra= 1;
	rules->extra_names= _XkbTypedCalloc(rules->sz_extra,char *);
	rules->extra= _XkbTypedCalloc(rules->sz_extra, XkbRF_DescribeVarsRec);
    }
    else if (rules->num_extra>=rules->sz_extra) {
	rules->sz_extra*= 2;
	rules->extra_names= _XkbTypedRealloc(rules->extra_names,rules->sz_extra,
								char *);
	rules->extra=_XkbTypedRealloc(rules->extra, rules->sz_extra,
							XkbRF_DescribeVarsRec);
    }
    if ((!rules->extra_names)||(!rules->extra)) {
	PR_DEBUG("allocation error in extra parts\n");
	rules->sz_extra= rules->num_extra= 0;
	rules->extra_names= NULL;
	rules->extra= NULL;
	return NULL;
    }
    rules->extra_names[rules->num_extra]= _XkbDupString(name);
    bzero(&rules->extra[rules->num_extra],sizeof(XkbRF_DescribeVarsRec));
    return &rules->extra[rules->num_extra++];
}

Bool
#if NeedFunctionPrototypes
XkbRF_LoadDescriptions(FILE *file,XkbRF_RulesPtr rules)
#else
XkbRF_LoadDescriptions(file,rules)
    FILE *		file;
    XkbRF_RulesPtr	rules;
#endif
{
InputLine		line;
XkbRF_VarDescRec	tmp;
char			*tok;
int			len,headingtype,extra_ndx;

    bzero((char *)&tmp, sizeof(XkbRF_VarDescRec));
    headingtype = HEAD_NONE;
    InitInputLine(&line);
    for ( ; GetInputLine(file,&line,False); line.num_line= 0) {
	if (line.line[0]=='!') {
	    tok = strtok(&(line.line[1]), " \t");
	    if (!_XkbStrCaseCmp(tok,"model"))
		headingtype = HEAD_MODEL;
	    else if (!_XkbStrCaseCmp(tok,"layout"))
		headingtype = HEAD_LAYOUT;
	    else if (!_XkbStrCaseCmp(tok,"variant"))
		headingtype = HEAD_VARIANT;
	    else if (!_XkbStrCaseCmp(tok,"option"))
		headingtype = HEAD_OPTION;
	    else {
		int i;
		headingtype = HEAD_EXTRA;
		extra_ndx= -1;
		for (i=0;(i<rules->num_extra)&&(extra_ndx<0);i++) {
		    if (!_XkbStrCaseCmp(tok,rules->extra_names[i]))
			extra_ndx= i;
		}
		if (extra_ndx<0) {
		    XkbRF_DescribeVarsPtr	var;
		    PR_DEBUG1("Extra heading \"%s\" encountered\n",tok);
		    var= XkbRF_AddVarToDescribe(rules,tok);
		    if (var)
			 extra_ndx= var-rules->extra;
		    else headingtype= HEAD_NONE;
		}
	    }
	    continue;
	}

	if (headingtype == HEAD_NONE) {
	    PR_DEBUG("Must have a heading before first line of data\n");
	    PR_DEBUG("Illegal line of data ignored\n");
	    continue;
	}

	len = strlen(line.line);
	if ((tmp.name= strtok(line.line, " \t")) == NULL) {
	    PR_DEBUG("Huh? No token on line\n");
	    PR_DEBUG("Illegal line of data ignored\n");
	    continue;
	}
	if (strlen(tmp.name) == len) {
	    PR_DEBUG("No description found\n");
	    PR_DEBUG("Illegal line of data ignored\n");
	    continue;
	}

	tok = line.line + strlen(tmp.name) + 1;
	while ((*tok!='\n')&&isspace(*tok))
		tok++;
	if (*tok == '\0') {
	    PR_DEBUG("No description found\n");
	    PR_DEBUG("Illegal line of data ignored\n");
	    continue;
	}
	tmp.desc= tok;
	switch (headingtype) {
	    case HEAD_MODEL:
		XkbRF_AddVarDescCopy(&rules->models,&tmp);
		break;
	    case HEAD_LAYOUT:
		XkbRF_AddVarDescCopy(&rules->layouts,&tmp);
		break;
	    case HEAD_VARIANT:
		XkbRF_AddVarDescCopy(&rules->variants,&tmp);
		break;
	    case HEAD_OPTION:
		XkbRF_AddVarDescCopy(&rules->options,&tmp);
		break;
	    case HEAD_EXTRA:
		XkbRF_AddVarDescCopy(&rules->extra[extra_ndx],&tmp);
		break;
	}
    }
    FreeInputLine(&line);
    if ((rules->models.num_desc==0) && (rules->layouts.num_desc==0) &&
	(rules->variants.num_desc==0) && (rules->options.num_desc==0) &&
	(rules->num_extra==0)) {
	return False;
    }
    return True;
}

Bool
#if NeedFunctionPrototypes
XkbRF_LoadDescriptionsByName(char *base,char *locale,XkbRF_RulesPtr rules)
#else
XkbRF_LoadDescriptionsByName(base,locale,rules)
    char *		base;
    char *		locale;
    XkbRF_RulesPtr	rules;
#endif
{
FILE *		file;
char		buf[PATH_MAX];
Bool		ok;

    if ((!base)||(!rules))
	return False;
    if (locale) {
	if (strlen(base)+strlen(locale)+6 > PATH_MAX)
	    return False;
	sprintf(buf,"%s-%s.lst", base, locale);
    }
    else {
	if (strlen(base)+5 > PATH_MAX)
	    return False;
	sprintf(buf,"%s.lst", base);
    }

    file= fopen(buf, "r");
    if ((!file)&&(locale)) { /* fallback if locale was specified */
	sprintf(buf,"%s.lst", base);

	file= fopen(buf, "r");
    }
    if (!file)
	return False;
    ok= XkbRF_LoadDescriptions(file,rules);
    fclose(file);
    return ok;
}

/***====================================================================***/

XkbRF_RulesPtr
#if NeedFunctionPrototypes
XkbRF_Load(char *base,char *locale,Bool wantDesc,Bool wantRules)
#else
XkbRF_Load(base,locale,wantDesc,wantRules)
    char *base;
    char *locale;
    Bool wantDesc;
    Bool wantRules;
#endif
{
XkbRF_RulesPtr	rules;

    if ((!base)||((!wantDesc)&&(!wantRules)))
	return NULL;
    if ((rules=_XkbTypedCalloc(1,XkbRF_RulesRec))==NULL)
	return NULL;
    if (wantDesc&&(!XkbRF_LoadDescriptionsByName(base,locale,rules))) {
	XkbRF_Free(rules,True);
	return NULL;
    }
    if (wantRules&&(!XkbRF_LoadRulesByName(base,locale,rules))) {
	XkbRF_Free(rules,True);
	return NULL;
    }
    return rules;
}

XkbRF_RulesPtr
XkbRF_Create(int szRules,int szExtra) 
{
XkbRF_RulesPtr rules;

    if ((rules=_XkbTypedCalloc(1,XkbRF_RulesRec))==NULL)
	return NULL;
    if (szRules>0) {
	rules->sz_rules= szRules; 
	rules->rules= _XkbTypedCalloc(rules->sz_rules,XkbRF_RuleRec);
	if (!rules->rules) {
	    _XkbFree(rules);
	    return NULL;
	}
    }
    if (szExtra>0) {
	rules->sz_extra= szExtra; 
	rules->extra= _XkbTypedCalloc(rules->sz_extra,XkbRF_DescribeVarsRec);
	if (!rules->extra) {
	    if (rules->rules)
		_XkbFree(rules->rules);
	    _XkbFree(rules);
	    return NULL;
	}
    }
    return rules;
}

/***====================================================================***/

static void
#if NeedFunctionPrototypes
XkbRF_ClearVarDescriptions(XkbRF_DescribeVarsPtr var)
#else
XkbRF_ClearVarDescriptions(var)
    XkbRF_DescribeVarsPtr var;
#endif
{
register int i;
    
    for (i=0;i<var->num_desc;i++) {
	if (var->desc[i].name)
	    _XkbFree(var->desc[i].name);
	if (var->desc[i].desc)
	    _XkbFree(var->desc[i].desc);
	var->desc[i].name= var->desc[i].desc= NULL;
    }
    if (var->desc)
	_XkbFree(var->desc);
    var->desc= NULL;
    return;
}

void
#if NeedFunctionPrototypes
XkbRF_Free(XkbRF_RulesPtr rules,Bool freeRules)
#else
XkbRF_Free(rules,freeRules)
    XkbRF_RulesPtr 	rules;
    Bool		freeRules;
#endif
{
int		i;
XkbRF_RulePtr	rule;

    if (!rules)
	return;
    XkbRF_ClearVarDescriptions(&rules->models);
    XkbRF_ClearVarDescriptions(&rules->layouts);
    XkbRF_ClearVarDescriptions(&rules->variants);
    XkbRF_ClearVarDescriptions(&rules->options);
    if (rules->extra) {
	for (i = 0; i < rules->num_extra; i++) {
	    XkbRF_ClearVarDescriptions(&rules->extra[i]);
	}
	_XkbFree(rules->extra);
	rules->num_extra= rules->sz_extra= 0;
	rules->extra= NULL;
    }
    if (rules->rules) {
	for (i=0,rule=rules->rules;i<rules->num_rules;i++,rule++) {
	    if (rule->model)	_XkbFree(rule->model);
	    if (rule->layout)	_XkbFree(rule->layout);
	    if (rule->variant)	_XkbFree(rule->variant);
	    if (rule->option)	_XkbFree(rule->option);
	    if (rule->keycodes)	_XkbFree(rule->keycodes);
	    if (rule->symbols)	_XkbFree(rule->symbols);
	    if (rule->types)	_XkbFree(rule->types);
	    if (rule->compat)	_XkbFree(rule->compat);
	    if (rule->geometry)	_XkbFree(rule->geometry);
	    if (rule->keymap)	_XkbFree(rule->keymap);
	    bzero((char *)rule,sizeof(XkbRF_RuleRec));
	}
	_XkbFree(rules->rules);
	rules->num_rules= rules->sz_rules= 0;
	rules->rules= NULL;
    }
    if (freeRules)
	_XkbFree(rules);
    return;
}

#ifndef XKB_IN_SERVER

Bool 
#if NeedFunctionPrototypes
XkbRF_GetNamesProp(Display *dpy,char **rf_rtrn,XkbRF_VarDefsPtr vd_rtrn)
#else
XkbRF_GetNamesProp(dpy,rf_rtrn,vd_rtrn)
   Display *		dpy;
   char **		rf_rtrn;
   XkbRF_VarDefsPtr	vd_rtrn;
#endif
{
Atom		rules_atom,actual_type;
int		fmt;
unsigned long	nitems,bytes_after;
char            *data,*out;
Status		rtrn;

    rules_atom= XInternAtom(dpy,_XKB_RF_NAMES_PROP_ATOM,True);
    if (rules_atom==None)	/* property cannot exist */
	return False; 
    rtrn= XGetWindowProperty(dpy,DefaultRootWindow(dpy),rules_atom,
                                0L,_XKB_RF_NAMES_PROP_MAXLEN,False,
                                XA_STRING,&actual_type,
                                &fmt,&nitems,&bytes_after,
                                (unsigned char **)&data);
    if (rtrn!=Success)
	return False;
    if (rf_rtrn)
	*rf_rtrn= NULL;
    (void)bzero((char *)vd_rtrn,sizeof(XkbRF_VarDefsRec));
    if ((bytes_after>0)||(actual_type!=XA_STRING)||(fmt!=8)) {
	if (data) XFree(data);
	return (fmt==0?True:False);
    }

    out= data;
    if (out && (*out) && rf_rtrn)
	 *rf_rtrn= _XkbDupString(out);
    out+=strlen(out)+1;

    if ((out-data)<nitems) {
	if (*out)
	    vd_rtrn->model= _XkbDupString(out);
	out+=strlen(out)+1;
    }

    if ((out-data)<nitems) {
	if (*out)
	    vd_rtrn->layout= _XkbDupString(out);
	out+=strlen(out)+1;
    }

    if ((out-data)<nitems) {
	if (*out)
	    vd_rtrn->variant= _XkbDupString(out);
	out+=strlen(out)+1;
    }


    if ((out-data)<nitems) {
	if (*out)
	    vd_rtrn->options= _XkbDupString(out);
	out+=strlen(out)+1;
    }
    XFree(data);
    return True;
}

Bool 
#if NeedFunctionPrototypes
XkbRF_SetNamesProp(Display *dpy,char *rules_file,XkbRF_VarDefsPtr var_defs)
#else
XkbRF_SetNamesProp(dpy,rules_file,var_defs)
   Display *		dpy;
   char *		rules_file;
   XkbRF_VarDefsPtr	var_defs;
#endif
{
int	len,out;
Atom	name;
char *	pval;

    len= (rules_file?strlen(rules_file):0);
    len+= (var_defs->model?strlen(var_defs->model):0);
    len+= (var_defs->layout?strlen(var_defs->layout):0);
    len+= (var_defs->variant?strlen(var_defs->variant):0);
    len+= (var_defs->options?strlen(var_defs->options):0);
    if (len<1)
        return True;

    len+= 5; /* trailing NULs */

    name= XInternAtom(dpy,_XKB_RF_NAMES_PROP_ATOM,False);
    if (name==None)  { /* should never happen */
	_XkbLibError(_XkbErrXReqFailure,"XkbRF_SetNamesProp",X_InternAtom);
        return False;
    }
    pval= (char *)_XkbAlloc(len);
    if (!pval) {
	_XkbLibError(_XkbErrBadAlloc,"XkbRF_SetNamesProp",len);
        return False;
    }
    out= 0;
    if (rules_file) {
        strcpy(&pval[out],rules_file);
        out+= strlen(rules_file);
    }
    pval[out++]= '\0';
    if (var_defs->model) {
        strcpy(&pval[out],var_defs->model);
        out+= strlen(var_defs->model);
    }
    pval[out++]= '\0';
    if (var_defs->layout) {
        strcpy(&pval[out],var_defs->layout);
        out+= strlen(var_defs->layout);
    }
    pval[out++]= '\0';
    if (var_defs->variant) {
        strcpy(&pval[out],var_defs->variant);
        out+= strlen(var_defs->variant);
    }
    pval[out++]= '\0';
    if (var_defs->options) {
        strcpy(&pval[out],var_defs->options);
        out+= strlen(var_defs->options);
    }
    pval[out++]= '\0';
    if (out!=len) {
	_XkbLibError(_XkbErrBadLength,"XkbRF_SetNamesProp",out);
	_XkbFree(pval);
	return False;
    }

    XChangeProperty(dpy,DefaultRootWindow(dpy),name,XA_STRING,8,PropModeReplace,
                                                (unsigned char *)pval,len);
    _XkbFree(pval);
    return True;
}

#endif

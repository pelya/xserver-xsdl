/*
 * $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Config.c,v 3.113.2.17 1998/02/24 19:05:54 hohndel Exp $
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $Xorg: xf86Config.c,v 1.3 2000/08/17 19:50:28 cpqbld Exp $ */

#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#else
extern double atof();
extern char *getenv();
#endif

#define NEED_EVENTS 1
#include "X.h"
#include "Xproto.h"
#include "Xmd.h"
#include "input.h"
#include "servermd.h"
#include "scrnintstr.h"

#ifdef DPMSExtension
#include "opaque.h"
extern CARD32 DPMSStandbyTime;
extern CARD32 DPMSSuspendTime;
extern CARD32 DPMSOffTime;
#endif

#define NO_COMPILER_H_EXTRAS
#include "xf86Procs.h"
#include "xf86_OSlib.h"

#define INIT_CONFIG
#include "xf86_Config.h"

#ifdef XKB
#include "inputstr.h"
#include "XKBsrv.h"
#endif

#ifdef XF86SETUP
#include "xfsconf.h"
#endif

#ifdef XINPUT
#include "xf86Xinput.h"

#ifndef XF86SETUP
extern DeviceAssocRec	mouse_assoc;
#endif
#endif

#ifdef NEED_RETURN_VALUE
#define HANDLE_RETURN(xx)	if (xx == RET_ERROR) return RET_ERROR
#else
#define HANDLE_RETURN(xx)	xx
#endif

#define CONFIG_BUF_LEN     1024

static FILE * configFile   = NULL;
static int    configStart  = 0;           /* start of the current token */
static int    configPos    = 0;           /* current readers position */
static int    configLineNo = 0;           /* linenumber */
static char   *configBuf,*configRBuf;     /* buffer for lines */
static char   *configPath;                /* path to config file */
static char   *fontPath = NULL;           /* font path */
static char   *modulePath = NULL;	  /* module path */
static int    pushToken = LOCK_TOKEN;
static LexRec val;                        /* global return value */
static char   DCerr;  
static int scr_index = 0;

#ifdef XF86SETUP
#define STATIC_OR_NOT
#else
#define STATIC_OR_NOT static
#endif
STATIC_OR_NOT int n_monitors = 0;
STATIC_OR_NOT MonPtr monitor_list = NULL;
STATIC_OR_NOT int n_devices = 0;
STATIC_OR_NOT GDevPtr device_list = NULL;

static int screenno = -100;      /* some little number ... */

extern char *defaultFontPath;
extern char *rgbPath;

extern Bool xf86fpFlag, xf86coFlag, xf86sFlag;
extern Bool xf86ScreensOpen;

extern int defaultColorVisualClass;
extern CARD32 defaultScreenSaverTime, ScreenSaverTime;

char *xf86VisualNames[] = {
    "StaticGray",
    "GrayScale",
    "StaticColor",
    "PseudoColor",
    "TrueColor",
    "DirectColor"
};

static CONFIG_RETURN_TYPE configFilesSection(
#if NeedFunctionPrototypes
    void
#endif
);
static CONFIG_RETURN_TYPE configServerFlagsSection(
#if NeedFunctionPrototypes
    void
#endif
);
static CONFIG_RETURN_TYPE configKeyboardSection(
#if NeedFunctionPrototypes
    void
#endif
);
static CONFIG_RETURN_TYPE configDeviceSection(
#if NeedFunctionPrototypes
    void
#endif
);
static CONFIG_RETURN_TYPE configScreenSection(
#if NeedFunctionPrototypes
    void
#endif
);
static CONFIG_RETURN_TYPE configDisplaySubsection(
#if NeedFunctionPrototypes
    DispPtr disp
#endif
);
static CONFIG_RETURN_TYPE configMonitorSection(
#if NeedFunctionPrototypes
    void
#endif
);
static CONFIG_RETURN_TYPE configDynamicModuleSection(
#if NeedFunctionPrototypes
    void
#endif
);
static char *xf86DCSaveLine(
#if NeedFunctionPrototypes
char *,
int
#endif
);
static char *xf86DCOption(
#if NeedFunctionPrototypes
char *,
LexRec
#endif
);
static char * xf86DCConcatOption(
#if NeedFunctionPrototypes
char *,
char *
#endif
);
#ifndef XF86SETUP
static
#endif
CONFIG_RETURN_TYPE findConfigFile(
#if NeedFunctionPrototypes
      char *filename,
      FILE **fp
#endif
);
static int getScreenIndex(
#if NeedFunctionPrototypes
     int token
#endif
);
static int getStringToken(
#if NeedFunctionPrototypes
     SymTabRec tab[]
#endif
);
static CONFIG_RETURN_TYPE readVerboseMode(
#if NeedFunctionPrototypes
    MonPtr monp
#endif
);
static Bool validateGraphicsToken(
#if NeedFunctionPrototypes
     int *validTokens,
     int token
#endif
);
extern char * xf86GetPathElem(
#if NeedFunctionPrototypes
     char **pnt
#endif
);
static DisplayModePtr xf86PruneModes(
#if NeedFunctionPrototypes
    MonPtr monp,
    DisplayModePtr allmodes,
    ScrnInfoPtr scrp,
    Bool card
#endif
);
static char * xf86ValidateFontPath(
#if NeedFunctionPrototypes
     char * /* path */
#endif
);
#ifdef XINPUT
extern CONFIG_RETURN_TYPE xf86ConfigExtendedInputSection(
#if NeedFunctionPrototypes
    LexPtr      pval
#endif
);
#endif

#ifdef XKB
extern char *XkbInitialMap;
#endif

#define DIR_FILE	"/fonts.dir"

/*
 * xf86GetPathElem --
 *	Extract a single element from the font path string starting at
 *	pnt.  The font path element will be returned, and pnt will be
 *	updated to point to the start of the next element, or set to
 *	NULL if there are no more.
 */
char *
xf86GetPathElem(pnt)
     char **pnt;
{
  char *p1;

  p1 = *pnt;
  *pnt = index(*pnt, ',');
  if (*pnt != NULL) {
    **pnt = '\0';
    *pnt += 1;
  }
  return(p1);
}

/*
 * StrToUL --
 *
 *	A portable, but restricted, version of strtoul().  It only understands
 *	hex, octal, and decimal.  But it's good enough for our needs.
 */
unsigned int StrToUL(str)
char *str;
{
  int base = 10;
  char *p = str;
  unsigned int tot = 0;

  if (*p == '0') {
    p++;
    if (*p == 'x') {
      p++;
      base = 16;
    }
    else
      base = 8;
  }
  while (*p) {
    if ((*p >= '0') && (*p <= ((base == 8)?'7':'9'))) {
      tot = tot * base + (*p - '0');
    }
    else if ((base == 16) && (*p >= 'a') && (*p <= 'f')) {
      tot = tot * base + 10 + (*p - 'a');
    }
    else if ((base == 16) && (*p >= 'A') && (*p <= 'F')) {
      tot = tot * base + 10 + (*p - 'A');
    }
    else {
      return(tot);
    }
    p++;
  }
  return(tot);
}

/*
 * xf86ValidateFontPath --
 *	Validates the user-specified font path.  Each element that
 *	begins with a '/' is checked to make sure the directory exists.
 *	If the directory exists, the existence of a file named 'fonts.dir'
 *	is checked.  If either check fails, an error is printed and the
 *	element is removed from the font path.
 */
#define CHECK_TYPE(mode, type) ((S_IFMT & (mode)) == (type))
static char *
xf86ValidateFontPath(path)
     char *path;
{
  char *tmp_path, *out_pnt, *path_elem, *next, *p1, *dir_elem;
  struct stat stat_buf;
  int flag;
  int dirlen;

  tmp_path = (char *)Xcalloc(strlen(path)+1);
  out_pnt = tmp_path;
  path_elem = NULL;
  next = path;
  while (next != NULL) {
    path_elem = xf86GetPathElem(&next);
#ifndef __EMX__
    if (*path_elem == '/') {
      dir_elem = (char *)Xcalloc(strlen(path_elem) + 1);
      if ((p1 = strchr(path_elem, ':')) != 0)
#else
    /* OS/2 must prepend X11ROOT */
    if (*path_elem == '/') {
      path_elem = (char*)__XOS2RedirRoot(path_elem);
      dir_elem = (char*)xcalloc(1, strlen(path_elem) + 1);
      if (p1 = strchr(path_elem+2, ':'))
#endif
	dirlen = p1 - path_elem;
      else
	dirlen = strlen(path_elem);
      strncpy(dir_elem, path_elem, dirlen);
      dir_elem[dirlen] = '\0';
      flag = stat(dir_elem, &stat_buf);
      if (flag == 0)
	if (!CHECK_TYPE(stat_buf.st_mode, S_IFDIR))
	  flag = -1;
      if (flag != 0) {
        ErrorF("Warning: The directory \"%s\" does not exist.\n", dir_elem);
	ErrorF("         Entry deleted from font path.\n");
	continue;
      }
      else {
	p1 = (char *)xalloc(strlen(dir_elem)+strlen(DIR_FILE)+1);
	strcpy(p1, dir_elem);
	strcat(p1, DIR_FILE);
	flag = stat(p1, &stat_buf);
	if (flag == 0)
	  if (!CHECK_TYPE(stat_buf.st_mode, S_IFREG))
	    flag = -1;
#ifndef __EMX__
	xfree(p1);
#endif
	if (flag != 0) {
	  ErrorF("Warning: 'fonts.dir' not found (or not valid) in \"%s\".\n", 
		 dir_elem);
	  ErrorF("          Entry deleted from font path.\n");
	  ErrorF("          (Run 'mkfontdir' on \"%s\").\n", dir_elem);
	  continue;
	}
      }
      xfree(dir_elem);
    }

    /*
     * Either an OK directory, or a font server name.  So add it to
     * the path.
     */
    if (out_pnt != tmp_path)
      *out_pnt++ = ',';
    strcat(out_pnt, path_elem);
    out_pnt += strlen(path_elem);
  }
  return(tmp_path);
}

/*
 * xf86GetToken --
 *      Read next Token form the config file. Handle the global variable
 *      pushToken.
 */
int
xf86GetToken(tab)
     SymTabRec tab[];
{
  int          c, i;

  /*
   * First check whether pushToken has a different value than LOCK_TOKEN.
   * In this case rBuf[] contains a valid STRING/TOKEN/NUMBER. But in the other
   * case the next token must be read from the input.
   */
  if (pushToken == EOF) return(EOF);
  else if (pushToken == LOCK_TOKEN)
    {
      
      c = configBuf[configPos];
      
      /*
       * Get start of next Token. EOF is handled, whitespaces & comments are
       * skipped. 
       */
      do {
	if (!c)  {
	  if (fgets(configBuf,CONFIG_BUF_LEN-1,configFile) == NULL)
	    {
	      return( pushToken = EOF );
	    }
	  configLineNo++;
	  configStart = configPos = 0;
	}
#ifndef __EMX__
	while (((c=configBuf[configPos++])==' ') || ( c=='\t') || ( c=='\n'));
#else
	while (((c=configBuf[configPos++])==' ') || ( c=='\t') || ( c=='\n') 
		|| (c=='\r'));
#endif
	if (c == '#') c = '\0'; 
      } while (!c);
      
      /* GJA -- handle '-' and ',' 
       * Be careful: "-hsync" is a keyword.
       */
      if ( (c == ',') && !isalpha(configBuf[configPos]) ) {
         configStart = configPos; return COMMA;
      } else if ( (c == '-') && !isalpha(configBuf[configPos]) ) {
         configStart = configPos; return DASH;
      }

      configStart = configPos;
      /*
       * Numbers are returned immediately ...
       */
      if (isdigit(c))
	{
	  int base;

	  if (c == '0')
	    if ((configBuf[configPos] == 'x') || 
		(configBuf[configPos] == 'X'))
	      base = 16;
	    else
	      base = 8;
	  else
	    base = 10;

	  configRBuf[0] = c; i = 1;
	  while (isdigit(c = configBuf[configPos++]) || 
		 (c == '.') || (c == 'x') || 
		 ((base == 16) && (((c >= 'a') && (c <= 'f')) ||
				   ((c >= 'A') && (c <= 'F')))))
            configRBuf[i++] = c;
          configPos--; /* GJA -- one too far */
	  configRBuf[i] = '\0';
	  val.num = StrToUL(configRBuf);
          val.realnum = atof(configRBuf);
	  return(NUMBER);
	}
      
      /*
       * All Strings START with a \" ...
       */
      else if (c == '\"')
	{
	  i = -1;
	  do {
	    configRBuf[++i] = (c = configBuf[configPos++]);
#ifndef __EMX__
	  } while ((c != '\"') && (c != '\n') && (c != '\0'));
#else
	  } while ((c != '\"') && (c != '\n') && (c != '\r') && (c != '\0'));
#endif
	  configRBuf[i] = '\0';
	  val.str = (char *)xalloc(strlen(configRBuf) + 1);
	  strcpy(val.str, configRBuf);      /* private copy ! */
	  return(STRING);
	}
      
      /*
       * ... and now we MUST have a valid token.  The search is
       * handled later along with the pushed tokens.
       */
      else
	{
          configRBuf[0] = c;
          i = 0;
	  do {
	    configRBuf[++i] = (c = configBuf[configPos++]);;
#ifndef __EMX__
	  } while ((c != ' ') && (c != '\t') && (c != '\n') && (c != '\0'));
#else
	  } while ((c != ' ') && (c != '\t') && (c != '\n') && (c != '\r') && (c != '\0') );
#endif
	  configRBuf[i] = '\0'; i=0;
	}
      
    }
  else
    {
    
      /*
       * Here we deal with pushed tokens. Reinitialize pushToken again. If
       * the pushed token was NUMBER || STRING return them again ...
       */
      int temp = pushToken;
      pushToken = LOCK_TOKEN;
    
      if (temp == COMMA || temp == DASH) return(temp);
      if (temp == NUMBER || temp == STRING) return(temp);
    }
  
  /*
   * Joop, at last we have to lookup the token ...
   */
  if (tab)
    {
      i = 0;
      while (tab[i].token != -1)
	if (StrCaseCmp(configRBuf,tab[i].name) == 0)
	  return(tab[i].token);
	else
	  i++;
    }
  
  return(ERROR_TOKEN);       /* Error catcher */
}

/*
 * xf86GetToken --
 *	Lookup a string if it is actually a token in disguise.
 */
static int
getStringToken(tab)
     SymTabRec tab[];
{
  int i;

  for ( i = 0 ; tab[i].token != -1 ; i++ ) {
    if ( ! StrCaseCmp(tab[i].name,val.str) ) return tab[i].token;
  }
  return(ERROR_TOKEN);
}

/*
 * getScreenIndex --
 *	Given the screen token, returns the index in xf86Screens, or -1 if
 *	the screen type is not applicable to this server.
 */
static int
getScreenIndex(token)
     int token;
{
  int i;

  for (i = 0; xf86ScreenNames[i] >= 0 && xf86ScreenNames[i] != token; i++)
    ;
  if (xf86ScreenNames[i] < 0)
    return(-1);
  else
    return(i);
}

/*
 * validateGraphicsToken --
 *	If token is a graphics token, check it is in the list of validTokens
 * XXXX This needs modifying to work as it did with the old format
 */
static Bool
validateGraphicsToken(validTokens, token)
     int *validTokens;
     int token;
{
  int i;

  for (i = 0; ScreenTab[i].token >= 0 && ScreenTab[i].token != token; i++)
    ;
  if (ScreenTab[i].token < 0)
    return(FALSE);        /* Not a graphics token */

  for (i = 0; validTokens[i] >= 0 && validTokens[i] != token; i++)
    ;
  return(validTokens[i] >= 0);
}

/*
 * xf86TokenToString --
 *	returns the string corresponding to token
 */
char *
xf86TokenToString(table, token)
     SymTabPtr table;
     int token;
{
  int i;

  for (i = 0; table[i].token >= 0 && table[i].token != token; i++)
    ;
  if (table[i].token < 0)
    return("unknown");
  else
    return(table[i].name);
}
 
/*
 * xf86StringToToken --
 *	returns the string corresponding to token
 */
int
xf86StringToToken(table, string)
     SymTabPtr table;
     char *string;
{
  int i;

  for (i = 0; table[i].token >= 0 && StrCaseCmp(string, table[i].name); i++)
    ;
  return(table[i].token);
}
 
/*
 * xf86ConfigError --
 *      Print a READABLE ErrorMessage!!!  All information that is 
 *      interesting is printed.  Even a pointer to the erroneous place is
 *      printed.  Maybe our e-mail will be less :-)
 */
#ifdef XF86SETUP
int
XF86SetupXF86ConfigError(msg)
#else
void
xf86ConfigError(msg)
#endif
     char *msg;
{
  int i,j;

  ErrorF( "\nConfig Error: %s:%d\n\n%s", configPath, configLineNo, configBuf);
  for (i = 1, j = 1; i < configStart; i++, j++) 
    if (configBuf[i-1] != '\t')
      ErrorF(" ");
    else
      do
	ErrorF(" ");
      while (((j++)%8) != 0);
  for (i = configStart; i <= configPos; i++) ErrorF("^");
  ErrorF("\n%s\n", msg);
#ifdef NEED_RETURN_VALUE
  return RET_ERROR;
#else
  exit(-1);                 /* simple exit ... */
#endif
}

#ifndef XF86SETUP
void
xf86DeleteMode(infoptr, dispmp)
ScrnInfoPtr	infoptr;
DisplayModePtr	dispmp;
{
	if(infoptr->modes == dispmp)
		infoptr->modes = dispmp->next;

	if(dispmp->next == dispmp)
		FatalError("No valid modes found.\n");

	ErrorF("%s %s: Removing mode \"%s\" from list of valid modes.\n",
	       XCONFIG_PROBED, infoptr->name, dispmp->name);
	dispmp->prev->next = dispmp->next;
	dispmp->next->prev = dispmp->prev;

	xfree(dispmp->name);
	xfree(dispmp);
}
#endif

/*
 * findConfigFile --
 * 	Locate the XF86Config file.  Abort if not found.
 */
#ifndef XF86SETUP
static
#endif
CONFIG_RETURN_TYPE
findConfigFile(filename, fp)
      char *filename;
      FILE **fp;
{
#define configFile (*fp)
#define MAXPTRIES   6
  char           *home = NULL;
  char           *xconfig = NULL;
  char           *xwinhome = NULL;
  char           *configPaths[MAXPTRIES];
  int            pcount = 0, idx;

  /*
   * First open if necessary the config file.
   * If the -xf86config flag was used, use the name supplied there (root only).
   * If $XF86CONFIG is a pathname, use it as the name of the config file (root)
   * If $XF86CONFIG is set but doesn't contain a '/', append it to 'XF86Config'
   *   and search the standard places (root only).
   * If $XF86CONFIG is not set, just search the standard places.
   */
  while (!configFile) {
    
    /*
     * configPaths[0]			is used as a buffer for -xf86config
     *					and $XF86CONFIG if it contains a path
     * configPaths[1...MAXPTRIES-1]	is used to store the paths of each of
     *					the other attempts
     */
    for (pcount = idx = 0; idx < MAXPTRIES; idx++)
      configPaths[idx] = NULL;

    /*
     * First check if the -xf86config option was used.
     */
    configPaths[pcount] = (char *)xalloc(PATH_MAX);
#ifndef __EMX__
    if (getuid() == 0 && xf86ConfigFile[0])
#else
    if (xf86ConfigFile[0])
#endif
    {
      strcpy(configPaths[pcount], xf86ConfigFile);
      if ((configFile = fopen(configPaths[pcount], "r")) != 0)
        break;
      else
        FatalError(
             "Cannot read file \"%s\" specified by the -xf86config flag\n",
             configPaths[pcount]);
    }
    /*
     * Check if XF86CONFIG is set.
     */
#ifndef __EMX__
    if (getuid() == 0
     && (xconfig = getenv("XF86CONFIG")) != 0
     && index(xconfig, '/'))
#else
    /* no root available, and filenames start with drive letter */
    if ((xconfig = getenv("XF86CONFIG")) != 0
      && isalpha(xconfig[0])
      && xconfig[1]==':')
#endif
    {
        strcpy(configPaths[pcount], xconfig);
        if ((configFile = fopen(configPaths[pcount], "r")) != 0)
          break;
        else
          FatalError(
               "Cannot read file \"%s\" specified by XF86CONFIG variable\n",
               configPaths[pcount]);
    }
     
#ifndef __EMX__
    /*
     * ~/XF86Config ...
     */
    if (getuid() == 0 && (home = getenv("HOME"))) {
      configPaths[++pcount] = (char *)xalloc(PATH_MAX);
      strcpy(configPaths[pcount],home);
      strcat(configPaths[pcount],"/XF86Config");
      if (xconfig) strcat(configPaths[pcount],xconfig);
      if ((configFile = fopen( configPaths[pcount], "r" )) != 0) break;
    }
    
    /*
     * /etc/XF86Config
     */
    configPaths[++pcount] = (char *)xalloc(PATH_MAX);
    strcpy(configPaths[pcount], "/etc/XF86Config");
    if (xconfig) strcat(configPaths[pcount],xconfig);
    if ((configFile = fopen( configPaths[pcount], "r" )) != 0) break;
    
    /*
     * $(LIBDIR)/XF86Config.<hostname>
     */

    configPaths[++pcount] = (char *)xalloc(PATH_MAX);
    if (getuid() == 0 && (xwinhome = getenv("XWINHOME")) != NULL)
	sprintf(configPaths[pcount], "%s/lib/X11/XF86Config", xwinhome);
    else
	strcpy(configPaths[pcount], SERVER_CONFIG_FILE);
    if (getuid() == 0 && xconfig) strcat(configPaths[pcount],xconfig);
    strcat(configPaths[pcount], ".");
#ifdef AMOEBA
    {
      extern char *XServerHostName;

      strcat(configPaths[pcount], XServerHostName);
    }
#else
    gethostname(configPaths[pcount]+strlen(configPaths[pcount]),
                MAXHOSTNAMELEN);
#endif
    if ((configFile = fopen( configPaths[pcount], "r" )) != 0) break;
#endif /* !__EMX__  */
    
    /*
     * $(LIBDIR)/XF86Config
     */
    configPaths[++pcount] = (char *)xalloc(PATH_MAX);
#ifndef __EMX__
    if (getuid() == 0 && xwinhome)
	sprintf(configPaths[pcount], "%s/lib/X11/XF86Config", xwinhome);
    else
	strcpy(configPaths[pcount], SERVER_CONFIG_FILE);
    if (getuid() == 0 && xconfig) strcat(configPaths[pcount],xconfig);
#else
    /* we explicitly forbid numerous config files everywhere for OS/2;
     * users should consider them lucky to have one in a standard place
     * and another one with the -xf86config option
     */
    xwinhome = getenv("X11ROOT"); /* get drive letter */
    if (!xwinhome) FatalError("X11ROOT environment variable not set\n");
    strcpy(configPaths[pcount], __XOS2RedirRoot("/XFree86/lib/X11/XConfig"));
#endif

    if ((configFile = fopen( configPaths[pcount], "r" )) != 0) break;
    
    ErrorF("\nCould not find config file!\n");
    ErrorF("- Tried:\n");
    for (idx = 1; idx <= pcount; idx++)
        if (configPaths[idx] != NULL)
            ErrorF("      %s\n", configPaths[idx]);
    FatalError("No config file found!\n%s", getuid() == 0 ? "" :
               "Note, the X server no longer looks for XF86Config in $HOME");
  }
  strcpy(filename, configPaths[pcount]);
  if (xf86Verbose) {
    ErrorF("XF86Config: %s\n", filename);
    ErrorF("%s stands for supplied, %s stands for probed/default values\n",
       XCONFIG_GIVEN, XCONFIG_PROBED);
  }
  for (idx = 0; idx <= pcount; idx++)
      if (configPaths[idx] != NULL)
          xfree(configPaths[idx]);
#undef configFile
#undef MAXPTRIES
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}

static DisplayModePtr pNew, pLast;
static Bool graphFound = FALSE;

/*
 * xf86GetNearestClock --
 *	Find closest clock to given frequency (in kHz).  This assumes the
 *	number of clocks is greater than zero.
 */
int
xf86GetNearestClock(Screen, Frequency)
	ScrnInfoPtr Screen;
	int Frequency;
{
  int NearestClock = 0;
  int MinimumGap = abs(Frequency - Screen->clock[0]);
  int i;
  for (i = 1;  i < Screen->clocks;  i++)
  {
    int Gap = abs(Frequency - Screen->clock[i]);
    if (Gap < MinimumGap)
    {
      MinimumGap = Gap;
      NearestClock = i;
    }
  }
  return NearestClock;
}

/*
 * xf86Config --
 *	Fill some internal structure with userdefined setups. Many internal
 *      Structs are initialized.  The drivers are selected and initialized.
 *	if (! vtopen), XF86Config is read, but devices are not probed.
 *	if (vtopen), devices are probed (and modes resolved).
 *	The vtopen argument was added so that XF86Config information could be
 *	made available before the VT is opened.
 */
CONFIG_RETURN_TYPE
xf86Config (vtopen)
     int vtopen;
{
  int            token;
  int            i, j;
#if defined(SYSV) || defined(linux)
  int            xcpipe[2];
#endif
#ifdef XINPUT
  LocalDevicePtr	local;
#endif
  
 if (!vtopen)
 {

  OFLG_ZERO(&GenericXF86ConfigFlag);
  configBuf  = (char*)xalloc(CONFIG_BUF_LEN);
  configRBuf = (char*)xalloc(CONFIG_BUF_LEN);
  configPath = (char*)xalloc(PATH_MAX);
  
  configBuf[0] = '\0';                    /* sanity ... */
  
  /*
   * Read the XF86Config file with the real uid to avoid security problems
   *
   * For SYSV we fork, and send the data back to the parent through a pipe
   */
#if defined(SYSV) || defined(linux)
  if (getuid() != 0) {
    if (pipe(xcpipe))
      FatalError("Pipe failed (%s)\n", strerror(errno));
    switch (fork()) {
      case -1:
        FatalError("Fork failed (%s)\n", strerror(errno));
        break;
      case 0:   /* child */
        close(xcpipe[0]);
        setuid(getuid());  
        HANDLE_RETURN(findConfigFile(configPath, &configFile));
        {
          unsigned char pbuf[CONFIG_BUF_LEN];
          int nbytes;

          /* Pass the filename back as the first line */
          strcat(configPath, "\n");
          if (write(xcpipe[1], configPath, strlen(configPath)) < 0)
            FatalError("Child error writing to pipe (%s)\n", strerror(errno));
          while ((nbytes = fread(pbuf, 1, CONFIG_BUF_LEN, configFile)) > 0)
            if (write(xcpipe[1], pbuf, nbytes) < 0)
              FatalError("Child error writing to pipe (%s)\n", strerror(errno));
        }
        close(xcpipe[1]);
        fclose(configFile);
        exit(0);
        break;
      default: /* parent */
        close(xcpipe[1]);
        configFile = (FILE *)fdopen(xcpipe[0], "r");
        if (fgets(configPath, PATH_MAX, configFile) == NULL)
          FatalError("Error reading config file\n");
        configPath[strlen(configPath) - 1] = '\0';
    }
  }
  else {
    HANDLE_RETURN(findConfigFile(configPath, &configFile));
  }
#else /* ! (SYSV || linux) */
  {
#ifndef __EMX__ /* in OS/2 we don't care about uids */
    int real_uid = getuid();

    if (real_uid) {
#ifdef MINIX
      setuid(getuid());
#else
#if !defined(SVR4) && !defined(__NetBSD__) && !defined(__OpenBSD__) && !defined(__FreeBSD__)
      setruid(0);
#endif
      seteuid(real_uid);
#endif /* MINIX */
    }
#endif /* __EMX__ */

    HANDLE_RETURN(findConfigFile(configPath, &configFile));
#if defined(MINIX) || defined(__EMX__)
    /* no need to restore the uid to root */
#else
    if (real_uid) {
      seteuid(0);
#if !defined(SVR4) && !defined(__NetBSD__) && !defined(__OpenBSD__) && !defined(__FreeBSD__)
      setruid(real_uid);
#endif
    }
#endif /* MINIX */
  }
#endif /* SYSV || linux */
  xf86Info.sharedMonitor = FALSE;
  xf86Info.kbdProc = NULL;
  xf86Info.notrapSignals = FALSE;
  xf86Info.caughtSignal = FALSE;

  /* Allocate mouse device */
#if defined(XINPUT) && !defined(XF86SETUP)
  local = mouse_assoc.device_allocate();
  xf86Info.mouseLocal = (pointer) local;
  xf86Info.mouseDev = (MouseDevPtr) local->private;
  xf86Info.mouseDev->mseProc = NULL;
#else
  xf86Info.mouseDev = (MouseDevPtr) Xcalloc(sizeof(MouseDevRec));
#endif
  
  while ((token = xf86GetToken(TopLevelTab)) != EOF) {
      switch(token) {
      case SECTION:
	  if (xf86GetToken(NULL) != STRING)
	      xf86ConfigError("section name string expected");
	  if ( StrCaseCmp(val.str, "files") == 0 ) {
	      HANDLE_RETURN(configFilesSection());
	  } else if ( StrCaseCmp(val.str, "serverflags") == 0 ) {
	      HANDLE_RETURN(configServerFlagsSection());
	  } else if ( StrCaseCmp(val.str, "keyboard") == 0 ) {
	      HANDLE_RETURN(configKeyboardSection());
	  } else if ( StrCaseCmp(val.str, "pointer") == 0 ) {
	      HANDLE_RETURN(configPointerSection(xf86Info.mouseDev, ENDSECTION, NULL));
	  } else if ( StrCaseCmp(val.str, "device") == 0 ) {
	      HANDLE_RETURN(configDeviceSection());
	  } else if ( StrCaseCmp(val.str, "monitor") == 0 ) {
	      HANDLE_RETURN(configMonitorSection());
	  } else if ( StrCaseCmp(val.str, "screen") == 0 ) {
	      HANDLE_RETURN(configScreenSection());
#ifdef XINPUT
	  } else if ( StrCaseCmp(val.str, "xinput") == 0 ) {
	      HANDLE_RETURN(xf86ConfigExtendedInputSection(&val));
#endif
	  } else if ( StrCaseCmp(val.str, "module") == 0 ) {
	      HANDLE_RETURN(configDynamicModuleSection());
	  } else {
	      xf86ConfigError("not a recognized section name");
	  }
	  break;
      }
  }
  
  fclose(configFile);
  xfree(configBuf);
  xfree(configRBuf);
  xfree(configPath);

  /* These aren't needed after the XF86Config file has been read */
#ifndef XF86SETUP
  if (monitor_list)
    xfree(monitor_list);
  if (device_list)
    xfree(device_list);
#endif
  if (modulePath)
      xfree(modulePath);
  
#if defined(SYSV) || defined(linux)
  if (getuid() != 0) {
    /* Wait for the child */
    wait(NULL);
  }
#endif
  
  /* Try XF86Config FontPath first */
  if (!xf86fpFlag)
    if (fontPath) {
      char *f = xf86ValidateFontPath(fontPath);
      if (*f)
        defaultFontPath = f;
      else
        ErrorF(
        "Warning: FontPath is completely invalid.  Using compiled-in default.\n"
              );
      xfree(fontPath);
      fontPath = (char *)NULL;
    }
    else
      ErrorF("Warning: No FontPath specified, using compiled-in default.\n");
  else    /* Use fontpath specified with '-fp' */
  {
    OFLG_CLR (XCONFIG_FONTPATH, &GenericXF86ConfigFlag);
    if (fontPath)
    {
      xfree(fontPath);
      fontPath = (char *)NULL;
    }
  }
  if (!fontPath) {
    /* xf86ValidateFontPath will write into it's arg, but defaultFontPath
       could be static, so we make a copy. */
    char *f = (char *)xalloc(strlen(defaultFontPath) + 1);
    f[0] = '\0';
    strcpy (f, defaultFontPath);
    defaultFontPath = xf86ValidateFontPath(f);
    xfree(f);
  }
  else
    xfree(fontPath);

  /* If defaultFontPath is still empty, exit here */

  if (! *defaultFontPath)
    FatalError("No valid FontPath could be found\n");
  if (xf86Verbose)
    ErrorF("%s FontPath set to \"%s\"\n", 
      OFLG_ISSET(XCONFIG_FONTPATH, &GenericXF86ConfigFlag) ? XCONFIG_GIVEN :
      XCONFIG_PROBED, defaultFontPath);

  if (!xf86Info.kbdProc)
    FatalError("You must specify a keyboard in XF86Config");
  if (!xf86Info.mouseDev->mseProc)
    FatalError("You must specify a mouse in XF86Config");

  if (!graphFound)
  {
    Bool needcomma = FALSE;

    ErrorF("\nYou must provide a \"Screen\" section in XF86Config for at\n");
    ErrorF("least one of the following graphics drivers: ");
    for (i = 0; i < xf86MaxScreens; i++)
    {
      if (xf86Screens[i])
      {
        ErrorF("%s%s", needcomma ? ", " : "",
               xf86TokenToString(DriverTab, xf86ScreenNames[i]));
        needcomma = TRUE;
      }
    }
    ErrorF("\n");
    FatalError("No configured graphics devices");
  }
 }
#ifndef XF86SETUP
 else	/* if (vtopen) */
 {
  /*
   * Probe all configured screens for letting them resolve their modes
   */
  xf86ScreensOpen = TRUE;
  for ( i=0; i < xf86MaxScreens; i++ )
    if (xf86Screens[i] && xf86Screens[i]->configured &&
	(xf86Screens[i]->configured = (xf86Screens[i]->Probe)())){
      /* if driver doesn't report error do it here */
      if(xf86DCGetToken(xf86Screens[i]->DCConfig,NULL,DeviceTab) != EOF){
	xf86DCConfigError("Unknown device section keyword");
	FatalError("\n");
      }
      if(xf86Screens[i]->DCOptions){
	xf86DCGetOption(xf86Screens[i]->DCOptions,NULL);
	FatalError("\n");
      }
      xf86InitViewport(xf86Screens[i]);
    }

  /*
   * Now sort the drivers to match the order of the ScreenNumbers
   * requested by the user. (sorry, slow bubble-sort here)
   * Note, that after this sorting the first driver that is not configured
   * can be used as last-mark for all configured ones.
   */
  for ( j = 0; j < xf86MaxScreens-1; j++)
    for ( i=0; i < xf86MaxScreens-j-1; i++ )
      if (!xf86Screens[i] || !xf86Screens[i]->configured ||
	  (xf86Screens[i+1] && xf86Screens[i+1]->configured &&
	   (xf86Screens[i+1]->tmpIndex < xf86Screens[i]->tmpIndex)))
	{
	  ScrnInfoPtr temp = xf86Screens[i+1];
	  xf86Screens[i+1] = xf86Screens[i];
	  xf86Screens[i] = temp;
	}

 }
#endif  /* XF86SETUP */

#ifdef NEED_RETURN_VALUE
 return RET_OKAY;
#endif
}

static char* prependRoot(char *pathname) 
{
#ifndef __EMX__
	return pathname;
#else
	/* XXXX caveat: multiple path components in line */
	return (char*)__XOS2RedirRoot(pathname);
#endif
}
    
static CONFIG_RETURN_TYPE
configFilesSection()
{
  int            token;
  int            i, j;
  int            k, l;
  char           *str;

  while ((token = xf86GetToken(FilesTab)) != ENDSECTION) {
    switch (token) {
    case FONTPATH:
      OFLG_SET(XCONFIG_FONTPATH,&GenericXF86ConfigFlag);
      if (xf86GetToken(NULL) != STRING)
	xf86ConfigError("Font path component expected");
      j = FALSE;
      str = prependRoot(val.str);
      if (fontPath == NULL)
	{
	  fontPath = (char *)xalloc(1);
	  fontPath[0] = '\0';
	  i = strlen(str) + 1;
	}
      else
	{
          i = strlen(fontPath) + strlen(str) + 1;
          if (fontPath[strlen(fontPath)-1] != ',') 
	    {
	      i++;
	      j = TRUE;
	    }
	}
      fontPath = (char *)xrealloc(fontPath, i);
      if (j)
        strcat(fontPath, ",");

      strcat(fontPath, str);
      xfree(val.str);
      break;
      
    case RGBPATH:
      OFLG_SET(XCONFIG_RGBPATH, &GenericXF86ConfigFlag);
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("RGB path expected");
      if (!xf86coFlag)
        rgbPath = val.str;
      break;

    case MODULEPATH:
      OFLG_SET(XCONFIG_MODULEPATH, &GenericXF86ConfigFlag);
      if (xf86GetToken(NULL) != STRING)
	xf86ConfigError("Module path expected");
      l = FALSE;
      str = prependRoot(val.str);
      if (modulePath == NULL) {
	modulePath = (char *)xalloc(1);
	  modulePath[0] = '\0';
	  k = strlen(str) + 1;
	}
      else
	{
          k = strlen(modulePath) + strlen(str) + 1;
          if (modulePath[strlen(modulePath)-1] != ',') 
	    {
	      k++;
	      l = TRUE;
	    }
	}
      modulePath = (char *)xrealloc(modulePath, k);
      if (l)
        strcat(modulePath, ",");

      strcat(modulePath, str);
      xfree(val.str);
      break;

    case EOF:
      FatalError("Unexpected EOF (missing EndSection?)");
      break; /* :-) */
    default:
      xf86ConfigError("File section keyword expected");
      break;
    }
  }
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}

static CONFIG_RETURN_TYPE
configServerFlagsSection()
{
  int            token;

  xf86Info.dontZap       = FALSE;
  xf86Info.dontZoom      = FALSE;

  while ((token = xf86GetToken(ServerFlagsTab)) != ENDSECTION) {
    switch (token) {
    case NOTRAPSIGNALS:
      xf86Info.notrapSignals=TRUE;
      break;
    case DONTZAP:
      xf86Info.dontZap = TRUE;
      break;
    case DONTZOOM:
      xf86Info.dontZoom = TRUE;
      break;
#ifdef XF86VIDMODE
    case DISABLEVIDMODE:
      xf86VidModeEnabled = FALSE;
      break;
    case ALLOWNONLOCAL:
      xf86VidModeAllowNonLocal = TRUE;
      break;
#endif
#ifdef XF86MISC
    case DISABLEMODINDEV:
      xf86MiscModInDevEnabled = FALSE;
      break;
    case MODINDEVALLOWNONLOCAL:
      xf86MiscModInDevAllowNonLocal = TRUE;
      break;
#endif
    case ALLOWMOUSEOPENFAIL:
      xf86AllowMouseOpenFail = TRUE;
      break;
    case PCIPROBE1:
      xf86PCIFlags = PCIProbe1;
      break;
    case PCIPROBE2:
      xf86PCIFlags = PCIProbe2;
      break;
    case PCIFORCECONFIG1:
      xf86PCIFlags = PCIForceConfig1;
      break;
    case PCIFORCECONFIG2:
      xf86PCIFlags = PCIForceConfig2;
      break;
    case EOF:
      FatalError("Unexpected EOF (missing EndSection?)");
      break; /* :-) */
    default:
      xf86ConfigError("Server flags section keyword expected");
      break;
    }
  }
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}

static CONFIG_RETURN_TYPE
configKeyboardSection()
{
  int            token, ntoken;
 
  /* Initialize defaults */
  xf86Info.serverNumLock = FALSE;
  xf86Info.xleds         = 0L;
  xf86Info.kbdDelay      = 500;
  xf86Info.kbdRate       = 30;
  xf86Info.kbdProc       = (DeviceProc)0;
  xf86Info.vtinit        = NULL;
  xf86Info.vtSysreq      = VT_SYSREQ_DEFAULT;
  xf86Info.specialKeyMap = (int *)xalloc((RIGHTCTL - LEFTALT + 1) *
                                            sizeof(int));
  xf86Info.specialKeyMap[LEFTALT - LEFTALT] = KM_META;
  xf86Info.specialKeyMap[RIGHTALT - LEFTALT] = KM_META;
  xf86Info.specialKeyMap[SCROLLLOCK - LEFTALT] = KM_COMPOSE;
  xf86Info.specialKeyMap[RIGHTCTL - LEFTALT] = KM_CONTROL;
#if defined(SVR4) && defined(i386) && !defined(PC98)
  xf86Info.panix106      = FALSE;
#endif
#ifdef XKB
  xf86Info.xkbkeymap   = NULL;
  xf86Info.xkbtypes    = "default";
#ifndef PC98
  xf86Info.xkbcompat   = "default";
  xf86Info.xkbkeycodes = "xfree86";
  xf86Info.xkbsymbols  = "us(pc101)";
  xf86Info.xkbgeometry = "pc";
#else
  xf86Info.xkbcompat   = "pc98";
  xf86Info.xkbkeycodes = "xfree98";
  xf86Info.xkbsymbols  = "nec/jp(pc98)";
  xf86Info.xkbgeometry = "nec(pc98)";
#endif
  xf86Info.xkbcomponents_specified    = False;
  xf86Info.xkbrules    = "xfree86";
  xf86Info.xkbmodel    = NULL;
  xf86Info.xkblayout   = NULL;
  xf86Info.xkbvariant  = NULL;
  xf86Info.xkboptions  = NULL;
#endif

  while ((token = xf86GetToken(KeyboardTab)) != ENDSECTION) {
    switch (token) {
    case KPROTOCOL:
      if (xf86GetToken(NULL) != STRING)
        xf86ConfigError("Keyboard protocol name expected");
      if ( StrCaseCmp(val.str,"standard") == 0 ) {
         xf86Info.kbdProc    = xf86KbdProc;
#ifdef AMOEBA
         xf86Info.kbdEvents  = NULL;
#else
         xf86Info.kbdEvents  = xf86KbdEvents;
#endif
      } else if ( StrCaseCmp(val.str,"xqueue") == 0 ) {
#ifdef XQUEUE
        xf86Info.kbdProc = xf86XqueKbdProc;
        xf86Info.kbdEvents = xf86XqueEvents;
        xf86Info.mouseDev->xqueSema  = 0;
        if (xf86Verbose)
          ErrorF("%s Xqueue selected for keyboard input\n",
  	         XCONFIG_GIVEN);
#endif
      } else {
        xf86ConfigError("Not a valid keyboard protocol name");
      }
      break;
    case AUTOREPEAT:
      if (xf86GetToken(NULL) != NUMBER)
	xf86ConfigError("Autorepeat delay expected");
      xf86Info.kbdDelay = val.num;
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Autorepeat rate expected");
      xf86Info.kbdRate = val.num;
      break;
    case SERVERNUM:
      xf86Info.serverNumLock = TRUE;
      break;

    case XLEDS:
      while ((token= xf86GetToken(NULL)) == NUMBER)
	xf86Info.xleds |= 1L << (val.num-1);
      pushToken = token;
      break;
    case LEFTALT:
    case RIGHTALT:
    case SCROLLLOCK:
    case RIGHTCTL:
      ntoken = xf86GetToken(KeyMapTab);
      if ((ntoken == EOF) || (ntoken == STRING) || (ntoken == NUMBER)) 
	xf86ConfigError("KeyMap type token expected");
      else {
	switch(ntoken) {
	case KM_META:
	case KM_COMPOSE:
	case KM_MODESHIFT:
	case KM_MODELOCK:
	case KM_SCROLLLOCK:
	case KM_CONTROL:
          xf86Info.specialKeyMap[token - LEFTALT] = ntoken;
	  break;
	default:
	  xf86ConfigError("Illegal KeyMap type");
	  break;
	}
      }
      break;
    case VTINIT:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("VTInit string expected");
      xf86Info.vtinit = val.str;
      if (xf86Verbose)
        ErrorF("%s VTInit: \"%s\"\n", XCONFIG_GIVEN, val.str);
      break;

    case VTSYSREQ:
#ifdef USE_VT_SYSREQ
      xf86Info.vtSysreq = TRUE;
      if (xf86Verbose && !VT_SYSREQ_DEFAULT)
        ErrorF("%s VTSysReq enabled\n", XCONFIG_GIVEN);
#else
      xf86ConfigError("VTSysReq not supported on this OS");
#endif
      break;

#ifdef XKB
    case XKBDISABLE:
      noXkbExtension = TRUE;
      if (xf86Verbose)
        ErrorF("%s XKB: disabled\n", XCONFIG_GIVEN);
      break;

    case XKBKEYMAP:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("XKBKeymap string expected");
      xf86Info.xkbkeymap = val.str;
      if (xf86Verbose && !XkbInitialMap)
        ErrorF("%s XKB: keymap: \"%s\" (overrides other XKB settings)\n",
	       XCONFIG_GIVEN, val.str);
      break;

    case XKBCOMPAT:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("XKBCompat string expected");
      xf86Info.xkbcompat = val.str;
      xf86Info.xkbcomponents_specified = True;
      if (xf86Verbose && !XkbInitialMap)
        ErrorF("%s XKB: compat: \"%s\"\n", XCONFIG_GIVEN, val.str);
      break;

    case XKBTYPES:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("XKBTypes string expected");
      xf86Info.xkbtypes = val.str;
      xf86Info.xkbcomponents_specified = True;
      if (xf86Verbose && !XkbInitialMap)
        ErrorF("%s XKB: types: \"%s\"\n", XCONFIG_GIVEN, val.str);
      break;

    case XKBKEYCODES:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("XKBKeycodes string expected");
      xf86Info.xkbkeycodes = val.str;
      xf86Info.xkbcomponents_specified = True;
      if (xf86Verbose && !XkbInitialMap)
        ErrorF("%s XKB: keycodes: \"%s\"\n", XCONFIG_GIVEN, val.str);
      break;

    case XKBGEOMETRY:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("XKBGeometry string expected");
      xf86Info.xkbgeometry = val.str;
      xf86Info.xkbcomponents_specified = True;
      if (xf86Verbose && !XkbInitialMap)
        ErrorF("%s XKB: geometry: \"%s\"\n", XCONFIG_GIVEN, val.str);
      break;

    case XKBSYMBOLS:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("XKBSymbols string expected");
      xf86Info.xkbsymbols = val.str;
      xf86Info.xkbcomponents_specified = True;
      if (xf86Verbose && !XkbInitialMap)
        ErrorF("%s XKB: symbols: \"%s\"\n", XCONFIG_GIVEN, val.str);
      break;

    case XKBRULES:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("XKBRules string expected");
      xf86Info.xkbrules = val.str;
      if (xf86Verbose && !XkbInitialMap)
        ErrorF("%s XKB: rules: \"%s\"\n", XCONFIG_GIVEN, val.str);
      break;

    case XKBMODEL:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("XKBModel string expected");
      xf86Info.xkbmodel = val.str;
      if (xf86Verbose && !XkbInitialMap)
        ErrorF("%s XKB: model: \"%s\"\n", XCONFIG_GIVEN, val.str);
      break;

    case XKBLAYOUT:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("XKBLayout string expected");
      xf86Info.xkblayout = val.str;
      if (xf86Verbose && !XkbInitialMap)
        ErrorF("%s XKB: layout: \"%s\"\n", XCONFIG_GIVEN, val.str);
      break;

    case XKBVARIANT:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("XKBVariant string expected");
      xf86Info.xkbvariant = val.str;
      if (xf86Verbose && !XkbInitialMap)
        ErrorF("%s XKB: variant: \"%s\"\n", XCONFIG_GIVEN, val.str);
      break;

    case XKBOPTIONS:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("XKBOptions string expected");
      xf86Info.xkboptions = val.str;
      if (xf86Verbose && !XkbInitialMap)
        ErrorF("%s XKB: options: \"%s\"\n", XCONFIG_GIVEN, val.str);
      break;
#endif
#if defined(SVR4) && defined(i386) && !defined(PC98)
    case PANIX106:
      xf86Info.panix106      = TRUE;
      if (xf86Verbose)
        ErrorF("%s PANIX106: enabled\n", XCONFIG_GIVEN);
      break;
#endif

    case EOF:
      FatalError("Unexpected EOF (missing EndSection?)");
      break; /* :-) */

    default:
      xf86ConfigError("Keyboard section keyword expected");
      break;
    }
  }
  if (xf86Info.kbdProc == (DeviceProc)0)
  {
    xf86ConfigError("No keyboard device given");
  }
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}
      
CONFIG_RETURN_TYPE
configPointerSection(MouseDevPtr	mouse_dev,
		     int		end_tag,
		     char		**devicename) /* used by extended device */
{
  int            token;
  int		 mtoken;
  int            i;
  char *mouseType = "unknown";

  /* Set defaults */
  mouse_dev->baudRate        = 1200;
  mouse_dev->oldBaudRate     = -1;
  mouse_dev->sampleRate      = 0;
  mouse_dev->resolution      = 0;
  mouse_dev->buttons         = MSE_DFLTBUTTONS;
  mouse_dev->emulate3Buttons = FALSE;
  mouse_dev->emulate3Timeout = 50;
  mouse_dev->chordMiddle     = FALSE;
  mouse_dev->mouseFlags      = 0;
  mouse_dev->mseProc         = (DeviceProc)0;
  mouse_dev->mseDevice       = NULL;
  mouse_dev->mseType         = -1;
  mouse_dev->mseModel        = 0;
  mouse_dev->negativeZ       = 0;
  mouse_dev->positiveZ       = 0;
      
  while ((token = xf86GetToken(PointerTab)) != end_tag) {
    switch (token) {

    case PROTOCOL:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("Mouse name expected");
#if defined(USE_OSMOUSE) || defined(OSMOUSE_ONLY)
      if ( StrCaseCmp(val.str,"osmouse") == 0 ) {
        if (xf86Verbose)
          ErrorF("%s OsMouse selected for mouse input\n", XCONFIG_GIVEN);
        /*
         *  allow an option to be passed to the OsMouse routines
         */
        if ((i = xf86GetToken(NULL)) != ERROR_TOKEN)
    	  xf86OsMouseOption(i, (pointer) &val);
        else
  	  pushToken = i;
        mouse_dev->mseProc   = xf86OsMouseProc;
        mouse_dev->mseEvents = (void(*)(MouseDevPtr))xf86OsMouseEvents;
	break;
      }
#endif
#ifdef XQUEUE
      if ( StrCaseCmp(val.str,"xqueue") == 0 ) {
        mouse_dev->mseProc   = xf86XqueMseProc;
        mouse_dev->mseEvents = (void(*)(MouseDevPtr))xf86XqueEvents;
        mouse_dev->xqueSema  = 0;
        if (xf86Verbose)
          ErrorF("%s Xqueue selected for mouse input\n",
	         XCONFIG_GIVEN);
        break;
      }
#endif

#ifndef OSMOUSE_ONLY
#if defined(MACH) || defined(AMOEBA)
      mouseType = (char *) xalloc (strlen (val.str) + 1);
      strcpy (mouseType, val.str);
#else
      mouseType = (char *)strdup(val.str); /* GJA -- should we free this? */
#endif
      mtoken = getStringToken(MouseTab); /* Which mouse? */
#ifdef AMOEBA
      mouse_dev->mseProc    = xf86MseProc;
      mouse_dev->mseEvents  = NULL;
#else
      mouse_dev->mseProc    = xf86MseProc;
      mouse_dev->mseEvents  = xf86MseEvents;
#endif
      mouse_dev->mseType    = mtoken - MICROSOFT;
      if (!xf86MouseSupported(mouse_dev->mseType))
      {
        xf86ConfigError("Mouse type not supported by this OS");
      }
#else /* OSMOUSE_ONLY */
      xf86ConfigError("Mouse type not supported by this OS");
#endif /* OSMOUSE_ONLY */

#ifdef MACH386
      /* Don't need to specify the device for MACH -- should always be this */
      mouse_dev->mseDevice  = "/dev/mouse";
#endif
      break;
#ifndef OSMOUSE_ONLY
    case PDEVICE:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("Mouse device expected");
      mouse_dev->mseDevice  = val.str;
      break;
    case BAUDRATE:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Baudrate expected");
      if (mouse_dev->mseType + MICROSOFT == LOGIMAN)
	{
	  /*
	   * XXXX This should be extended to other mouse types -- most
	   * support only 1200.  Should also disallow baudrate for bus mice
	   */
	  /* Moan if illegal baud rate!  [CHRIS-211092] */
	  if ((val.num != 1200) && (val.num != 9600))
	    xf86ConfigError("Only 1200 or 9600 Baud are supported by MouseMan");
	}
      else if (val.num%1200 != 0 || val.num < 1200 || val.num > 9600)
	    xf86ConfigError("Baud rate must be one of 1200, 2400, 4800, or 9600");
      mouse_dev->baudRate = val.num;
      break;

    case SAMPLERATE:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Sample rate expected");
#if 0
      if (mouse_dev->mseType + MICROSOFT == LOGIMAN)
	{
	  /* XXXX Most mice don't allow this */
	  /* Moan about illegal sample rate!  [CHRIS-211092] */
	  xf86ConfigError("Selection of sample rate is not supported by MouseMan");
	}
#endif
      mouse_dev->sampleRate = val.num;
      break;

    case PRESOLUTION:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Resolution expected");
      if (val.num <= 0)
	    xf86ConfigError("Resolution must be a positive value");
      mouse_dev->resolution = val.num;
      break;
#endif /* OSMOUSE_ONLY */
    case EMULATE3:
      if (mouse_dev->chordMiddle)
        xf86ConfigError("Can't use Emulate3Buttons with ChordMiddle");
      mouse_dev->emulate3Buttons = TRUE;
      break;

    case EM3TIMEOUT:
      if (xf86GetToken(NULL) != NUMBER)
        xf86ConfigError("3 button emulation timeout expected");
      mouse_dev->emulate3Timeout = val.num;
      break;

#ifndef OSMOUSE_ONLY
    case CHORDMIDDLE:
      if (mouse_dev->mseType + MICROSOFT == MICROSOFT ||
          mouse_dev->mseType + MICROSOFT == LOGIMAN)
      {
        if (mouse_dev->emulate3Buttons)
          xf86ConfigError("Can't use ChordMiddle with Emulate3Buttons");
        mouse_dev->chordMiddle = TRUE;
      }
      else
        xf86ConfigError("ChordMiddle is only supported for Microsoft and MouseMan");
      break;

    case CLEARDTR:
#ifdef CLEARDTR_SUPPORT
      if (mouse_dev->mseType + MICROSOFT == MOUSESYS)
        mouse_dev->mouseFlags |= MF_CLEAR_DTR;
      else
        xf86ConfigError("ClearDTR only supported for MouseSystems mouse");
#else
      xf86ConfigError("ClearDTR not supported on this OS");
#endif
      break;
    case CLEARRTS:
#ifdef CLEARDTR_SUPPORT
      if (mouse_dev->mseType + MICROSOFT == MOUSESYS)
        mouse_dev->mouseFlags |= MF_CLEAR_RTS;
      else
        xf86ConfigError("ClearRTS only supported for MouseSystems mouse");
#else
      xf86ConfigError("ClearRTS not supported on this OS");
#endif
      break;
#endif /* OSMOUSE_ONLY */

    case DEVICE_NAME:
	if (!devicename)		/* not called for an extended device */
	    xf86ConfigError("Pointer section keyword expected");

	if (xf86GetToken(NULL) != STRING)
	    xf86ConfigError("Option string expected");
	*devicename = strdup(val.str);
	break;

#ifndef XF86SETUP
#ifdef XINPUT
    case ALWAYSCORE:
	xf86AlwaysCore(mouse_dev->local, TRUE);
	break;
#endif
#endif
	
    case ZAXISMAPPING:
      switch (xf86GetToken(ZMapTab)) {
      case NUMBER:
        if (val.num <= 0 || val.num > MSE_MAXBUTTONS)
	  xf86ConfigError("Button number (1..12) expected");
        mouse_dev->negativeZ = 1 << (val.num - 1);
        if (xf86GetToken(NULL) != NUMBER || 
	    val.num <= 0 || val.num > MSE_MAXBUTTONS)
	  xf86ConfigError("Button number (1..12) expected");
        mouse_dev->positiveZ = 1 << (val.num - 1);
        break;
      case XAXIS:
        mouse_dev->negativeZ = mouse_dev->positiveZ = MSE_MAPTOX;
	break;
      case YAXIS:
        mouse_dev->negativeZ = mouse_dev->positiveZ = MSE_MAPTOY;
	break;
      default:
	xf86ConfigError("Button number (1..12), X or Y expected");
      }
      break;

    case PBUTTONS:
      if (xf86GetToken(NULL) != NUMBER)
	xf86ConfigError("Number of buttons (1..12) expected");
      if (val.num <= 0 || val.num > MSE_MAXBUTTONS)
	xf86ConfigError("Number of buttons must be a positive value (1..12)");
      mouse_dev->buttons = val.num;
      break;

    case EOF:
      FatalError("Unexpected EOF (missing EndSection?)");
      break; /* :-) */
      
    default:
      xf86ConfigError("Pointer section keyword expected");
      break;
    }

  }
  /* Print log and make sanity checks */

  if (mouse_dev->mseProc == (DeviceProc)0)
  {
    xf86ConfigError("No mouse protocol given");
  }
  
  /*
   * if mseProc is set and mseType isn't, then using Xqueue or OSmouse.
   * Otherwise, a mouse device is required.
   */
  if (mouse_dev->mseType >= 0 && !mouse_dev->mseDevice)
  {
    xf86ConfigError("No mouse device given");
  }

  switch (mouse_dev->negativeZ) {
  case 0: /* none */
  case MSE_MAPTOX:
  case MSE_MAPTOY:
    break;
  default: /* buttons */
    for (i = 0; mouse_dev->negativeZ != (1 << i); ++i)
      ;
    if (i + 1 > mouse_dev->buttons)
      mouse_dev->buttons = i + 1;
    for (i = 0; mouse_dev->positiveZ != (1 << i); ++i)
      ;
    if (i + 1 > mouse_dev->buttons)
      mouse_dev->buttons = i + 1;
    break;
  }

  if (xf86Verbose && mouse_dev->mseType >= 0)
  {
    Bool formatFlag = FALSE;
    ErrorF("%s Mouse: type: %s, device: %s", 
       XCONFIG_GIVEN, mouseType, mouse_dev->mseDevice);
    if (mouse_dev->mseType != P_BM
	&& mouse_dev->mseType != P_PS2
	&& mouse_dev->mseType != P_IMPS2
	&& mouse_dev->mseType != P_THINKINGPS2
	&& mouse_dev->mseType != P_MMANPLUSPS2
	&& mouse_dev->mseType != P_GLIDEPOINTPS2
	&& mouse_dev->mseType != P_NETPS2
	&& mouse_dev->mseType != P_NETSCROLLPS2
	&& mouse_dev->mseType != P_SYSMOUSE)
    {
      formatFlag = TRUE;
      ErrorF(", baudrate: %d", mouse_dev->baudRate);
    }
    if (mouse_dev->sampleRate)
    {
      ErrorF(formatFlag ? "\n%s Mouse: samplerate: %d" : "%ssamplerate: %d", 
	     formatFlag ? XCONFIG_GIVEN : ", ", mouse_dev->sampleRate);
      formatFlag = !formatFlag;
    }
    if (mouse_dev->resolution)
    {
      ErrorF(formatFlag ? "\n%s Mouse: resolution: %d" : "%sresolution: %d", 
	     formatFlag ? XCONFIG_GIVEN : ", ", mouse_dev->resolution);
      formatFlag = !formatFlag;
    }
    ErrorF(formatFlag ? "\n%s Mouse: buttons: %d" : "%sbuttons: %d",
	   formatFlag ? XCONFIG_GIVEN : ", ", mouse_dev->buttons);
    formatFlag = !formatFlag;
    if (mouse_dev->emulate3Buttons)
    {
      ErrorF(formatFlag ? "\n%s Mouse: 3 button emulation (timeout: %dms)" :
			  "%s3 button emulation (timeout: %dms)",
             formatFlag ? XCONFIG_GIVEN : ", ", mouse_dev->emulate3Timeout);
      formatFlag = !formatFlag;
    }
    if (mouse_dev->chordMiddle)
      ErrorF(formatFlag ? "\n%s Mouse: Chorded middle button" : 
                          "%sChorded middle button",
             formatFlag ? XCONFIG_GIVEN : ", ");
    ErrorF("\n");

    switch (mouse_dev->negativeZ) {
    case 0: /* none */
      break;
    case MSE_MAPTOX:
      ErrorF("%s Mouse: zaxismapping: X\n", XCONFIG_GIVEN);
      break;
    case MSE_MAPTOY:
      ErrorF("%s Mouse: zaxismapping: Y\n", XCONFIG_GIVEN);
      break;
    default: /* buttons */
      for (i = 0; mouse_dev->negativeZ != (1 << i); ++i)
	;
      ErrorF("%s Mouse: zaxismapping: (-)%d", XCONFIG_GIVEN, i + 1);
      for (i = 0; mouse_dev->positiveZ != (1 << i); ++i)
	;
      ErrorF(" (+)%d\n", i + 1);
      break;
    }
  }
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}
      
static CONFIG_RETURN_TYPE
configDeviceSection()
{
  int            token;
  int            i;
  GDevPtr devp;

  /* Allocate one more device */
  if ( device_list == NULL ) {
    device_list = (GDevPtr) xalloc(sizeof(GDevRec));
  } else {
    device_list = (GDevPtr) xrealloc(device_list,
				     (n_devices+1) * sizeof(GDevRec));
  }
  devp = &(device_list[n_devices]); /* Point to the last device */
  n_devices++; 
  
  /* Pre-init the newly created device */
  devp->identifier = NULL;
  devp->board = NULL;
  devp->vendor = NULL;
  devp->chipset = NULL;
  devp->ramdac = NULL;
  for (i=0; i<MAXDACSPEEDS; i++)
     devp->dacSpeeds[i] = 0;
  OFLG_ZERO(&(devp->options));
  OFLG_ZERO(&(devp->xconfigFlag));
  devp->videoRam = 0;
  devp->speedup = SPEEDUP_DEFAULT;
  OFLG_ZERO(&(devp->clockOptions));
  devp->clocks = 0;
  devp->clockprog = NULL;
  devp->textClockValue = -1;
  /* GJA -- We initialize the following fields to known values.
   * If later on we find they contain different values,
   * they might be interesting to print.
   */
  devp->IObase = 0;
  devp->DACbase = 0;
  devp->COPbase = 0;
  devp->POSbase = 0;
  devp->instance = 0;
  devp->BIOSbase = 0;
  devp->VGAbase = 0;
  devp->MemBase = 0;
  devp->s3Madjust = 0;
  devp->s3Nadjust = 0;
  devp->s3MClk = 0;
  devp->chipID = 0;
  devp->chipRev = 0;
  devp->s3RefClk = 0;
  devp->s3BlankDelay = -1;
  devp->DCConfig = NULL;
  devp->DCOptions = NULL;
  devp->MemClk = 0;
  devp->LCDClk = 0;

  while ((token = xf86GetToken(DeviceTab)) != ENDSECTION) {
    devp->DCConfig = xf86DCSaveLine(devp->DCConfig, token);
    switch (token) {

    case IDENTIFIER:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("identifier name expected");
      devp->identifier = val.str;
      break;

    case VENDOR:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("vendor name expected");
      devp->vendor = val.str;
      break;

    case BOARD:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("board name expected");
      devp->board = val.str;
      break;

    case CHIPSET:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("Chipset string expected");
      devp->chipset = val.str;
      OFLG_SET(XCONFIG_CHIPSET,&(devp->xconfigFlag));
      break;

    case RAMDAC:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("RAMDAC string expected");
      devp->ramdac = val.str;
      OFLG_SET(XCONFIG_RAMDAC,&(devp->xconfigFlag));
      break;

    case DACSPEED:
      for (i=0; i<MAXDACSPEEDS; i++) 
	 devp->dacSpeeds[i] = 0;
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("DAC speed(s) expected");
      else {
	 devp->dacSpeeds[0] = (int)(val.realnum * 1000.0 + 0.5);
	 for(i=1; i<MAXDACSPEEDS; i++) {
	    if (xf86GetToken(NULL) == NUMBER) 
	       devp->dacSpeeds[i] = (int)(val.realnum * 1000.0 + 0.5);
	    else {
	       pushToken = token;
	       break;
	    }
	 }
      }
      OFLG_SET(XCONFIG_DACSPEED,&(devp->xconfigFlag));
      break;

    case CLOCKCHIP:
       /* Only allow one Clock string */
       if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &(devp->clockOptions)))
       {
	  xf86ConfigError("Only one Clock chip may be specified.");
	  break;
       }
       if (devp->clocks == 0)
       {
          if (xf86GetToken(NULL) != STRING) xf86ConfigError("Option string expected");
	  i = 0;
	  while (xf86_ClockOptionTab[i].token != -1)
	  {
	     if (StrCaseCmp(val.str, xf86_ClockOptionTab[i].name) == 0)
	     {
		OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &(devp->clockOptions));
		OFLG_SET(xf86_ClockOptionTab[i].token,
			 &(devp->clockOptions));
		break;
	     }
	     i++;
	  }
	  if (xf86_ClockOptionTab[i].token == -1) {
	     xf86ConfigError("Unknown clock chip");
	     break;
	  }
       }
       else
       {
	  xf86ConfigError("Clocks previously specified by value");
       }
       break;

    case CLOCKS:
      OFLG_SET(XCONFIG_CLOCKS,&(devp->xconfigFlag));
      if ((token = xf86GetToken(NULL)) == STRING)
      {
	 xf86ConfigError("Use ClockChip to specify a programmable clock");
	 break;
      }
      if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &(devp->clockOptions)))
      {
	 xf86ConfigError("Clock previously specified as programmable");
	 break;
      }
      for (i = devp->clocks; token == NUMBER && i < MAXCLOCKS; i++) {
	devp->clock[i] = (int)(val.realnum * 1000.0 + 0.5);
	token = xf86GetToken(NULL);
      }

      devp->clocks = i;
      pushToken = token;
      break;

    case OPTION:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("Option string expected");
      i = 0;
      while (xf86_OptionTab[i].token != -1) 
      {
	if (StrCaseCmp(val.str, xf86_OptionTab[i].name) == 0)
	{
          OFLG_SET(xf86_OptionTab[i].token, &(devp->options));
	  break;
	}
	i++;
      }
      if (xf86_OptionTab[i].token == -1)
        /*xf86ConfigError("Unknown option string");*/
	devp->DCOptions = xf86DCOption(devp->DCOptions,val);
      break;

    case VIDEORAM:
      OFLG_SET(XCONFIG_VIDEORAM,&(devp->xconfigFlag));
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Video RAM size expected");
      devp->videoRam = val.num;
      break;

    case SPEEDUP:
      OFLG_SET(XCONFIG_SPEEDUP,&(devp->xconfigFlag));
      if ((token = xf86GetToken(NULL)) == STRING)
	if (!strcmp(val.str,"all"))
	  devp->speedup = SPEEDUP_ALL;
	else
	  if (!strcmp(val.str,"best"))
	    devp->speedup = SPEEDUP_BEST;
	  else
	    if (!strcmp(val.str,"none"))
	      devp->speedup = 0;
            else
	      xf86ConfigError("Unrecognised SpeedUp option");
      else
      {
        pushToken = token;
	if ((token = xf86GetToken(NULL)) == NUMBER)
	  devp->speedup = val.num;
	else
	{
	  pushToken = token;
	  devp->speedup = SPEEDUP_ALL;
	}
      }
      break;

    case NOSPEEDUP:
      OFLG_SET(XCONFIG_SPEEDUP,&(devp->xconfigFlag));
      devp->speedup = 0;
      break;

    case CLOCKPROG:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("ClockProg string expected");
      if (val.str[0] != '/')
        FatalError("Full pathname must be given for ClockProg \"%s\"\n",
                   val.str);
      if (access(val.str, X_OK) < 0)
      {
        if (access(val.str, F_OK) < 0)
          FatalError("ClockProg \"%s\" does not exist\n", val.str);
        else
          FatalError("ClockProg \"%s\" is not executable\n", val.str);
      }
      {
        struct stat stat_buf;
        stat(val.str, &stat_buf);
	if (!CHECK_TYPE(stat_buf.st_mode, S_IFREG))
          FatalError("ClockProg \"%s\" is not a regular file\n", val.str);
      }
      devp->clockprog = val.str;
      if (xf86GetToken(NULL) == NUMBER)
      {
        devp->textClockValue = (int)(val.realnum * 1000.0 + 0.5);
      }
      else
      {
        pushToken = token;
      }
      break;

    case BIOSBASE:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("BIOS base address expected");
      devp->BIOSbase = val.num;
      OFLG_SET(XCONFIG_BIOSBASE, &(devp->xconfigFlag));
      break;

    case MEMBASE:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Memory base address expected");
      devp->MemBase = val.num;
      OFLG_SET(XCONFIG_MEMBASE, &(devp->xconfigFlag));
      break;

    case IOBASE:
      if (xf86GetToken(NULL) != NUMBER)
        xf86ConfigError("Direct access register I/O base address expected");
      devp->IObase = val.num;
      OFLG_SET(XCONFIG_IOBASE, &(devp->xconfigFlag));
      break;

    case DACBASE:
      if (xf86GetToken(NULL) != NUMBER)
        xf86ConfigError("DAC base I/O address expected");
      devp->DACbase = val.num;
      OFLG_SET(XCONFIG_DACBASE, &(devp->xconfigFlag));
      break;

    case COPBASE:
      if (xf86GetToken(NULL) != NUMBER)
        xf86ConfigError("Coprocessor base memory address expected");
      devp->COPbase = val.num;
      OFLG_SET(XCONFIG_COPBASE, &(devp->xconfigFlag));
      break;

    case POSBASE:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("POS base address expected");
      devp->POSbase = val.num;
      OFLG_SET(XCONFIG_POSBASE, &(devp->xconfigFlag));
      break;

    case INSTANCE:
      if (xf86GetToken(NULL) != NUMBER)
        xf86ConfigError("Video adapter instance number expected");
      devp->instance = val.num;
      OFLG_SET(XCONFIG_INSTANCE, &(devp->xconfigFlag));
      break;

    case S3MNADJUST:
      if ((token = xf86GetToken(NULL)) == DASH) {  /* negative number */
	 token = xf86GetToken(NULL);
	 val.num = -val.num;
      }
      if (token != NUMBER || val.num<-31 || val.num>31) 
	 xf86ConfigError("M adjust (max. 31) expected");
        devp->s3Madjust = val.num;

      if ((token = xf86GetToken(NULL)) == DASH) {  /* negative number */
	 token = xf86GetToken(NULL);
	 val.num = -val.num;
      }
      if (token == NUMBER) {
	 if (val.num<-255 || val.num>255) 
	    xf86ConfigError("N adjust (max. 255) expected");
	 else
	    devp->s3Nadjust = val.num;
      }
      else pushToken = token;
      break;

    case S3MCLK:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("MCLK value in MHz expected");
      devp->s3MClk = (int)(val.realnum * 1000.0 + 0.5);
      break;

    case MEMCLOCK:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Memory Clock value in MHz expected");
      devp->MemClk = (int)(val.realnum * 1000.0 + 0.5);
      OFLG_SET(XCONFIG_MEMCLOCK,&(devp->xconfigFlag));
      break;

    case LCDCLOCK:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("LCD Clock value in MHz expected");
      devp->LCDClk = (int)(val.realnum * 1000.0 + 0.5);
      OFLG_SET(XCONFIG_LCDCLOCK,&(devp->xconfigFlag));
      break;

    case CHIPID:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("ChipID expected");
      devp->chipID = val.num;
      break;

    case CHIPREV:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("ChipRev expected");
      devp->chipRev = val.num;
      break;

    case VGABASEADDR:
      if (xf86GetToken(NULL) != NUMBER) 
         xf86ConfigError("VGA aperature base address expected");
      devp->VGAbase = val.num;
      OFLG_SET(XCONFIG_VGABASE, &(devp->xconfigFlag));
      break;

    case S3REFCLK:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("RefCLK value in MHz expected");
      devp->s3RefClk = (int)(val.realnum * 1000.0 + 0.5);
      break;

   case S3BLANKDELAY:
      if (xf86GetToken(NULL) != NUMBER || val.num>7)
	 xf86ConfigError("number(s) 0..7 expected");
      devp->s3BlankDelay = val.num;
      if ((token=xf86GetToken(NULL)) == NUMBER) {
	 if (val.num>7) xf86ConfigError("number2 0..7 expected");
	 devp->s3BlankDelay |= val.num<<4;
      }
      else pushToken = token;
      break;

    case TEXTCLOCKFRQ:
      if (xf86GetToken(NULL) != NUMBER)
         xf86ConfigError("Text clock expected");
      devp->textClockValue = (int)(val.realnum * 1000.0 + 0.5);
      break;

    case EOF:
      FatalError("Unexpected EOF (missing EndSection?)");
      break; /* :-) */
    default:
      if(DCerr)
	xf86ConfigError("Device section keyword expected");
      break;
    }
  }
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}

static CONFIG_RETURN_TYPE
configMonitorSection()
{
  int            token;
  int            i;
  MonPtr monp;
  float multiplier;
      
  /* Allocate one more monitor */
  if ( monitor_list == NULL ) {
    monitor_list = (MonPtr) xalloc(sizeof(MonRec));
  } else {
    monitor_list = (MonPtr) xrealloc(monitor_list,
				     (n_monitors+1) * sizeof(MonRec));
  }
  monp = &(monitor_list[n_monitors]); /* Point to the new monitor */
  monp->Modes = 0;
  monp->Last = 0;
  monp->n_hsync = 0;
  monp->n_vrefresh = 0;
  n_monitors++; 
  
  while ((token = xf86GetToken(MonitorTab)) != ENDSECTION) {
    switch (token) {
    case IDENTIFIER:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("identifier name expected");
      monp->id = val.str;
      break;
    case VENDOR:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("vendor name expected");
      monp->vendor = val.str;
      break;
    case MODEL:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("model name expected");
      monp->model = val.str;
      break;
    case MODE:
      readVerboseMode(monp);
      break;
    case MODELINE:
      token = xf86GetToken(NULL);
      pNew = (DisplayModePtr)xalloc(sizeof(DisplayModeRec));

      if (monp->Last) 
         monp->Last->next = pNew;
      else
        monp->Modes = pNew;
          
      if (token == STRING)
        {
          pNew->name = val.str;
          if ((token = xf86GetToken(NULL)) != NUMBER)
            FatalError("Dotclock expected");
        }
      else if (monp->Last)
        {
#if defined(MACH) || defined(AMOEBA)
          pNew->name = (char *) xalloc (strlen (monp->Last->name) + 1);
          strcpy (pNew->name, monp->Last->name);
#else
          pNew->name = (char *)strdup(monp->Last->name);
#endif
        }
      else
        xf86ConfigError("Mode name expected");

      pNew->next = NULL;
      pNew->prev = NULL;
      pNew->Flags = 0;
      pNew->Clock = (int)(val.realnum * 1000.0 + 0.5);
      pNew->CrtcHAdjusted = FALSE;
      pNew->CrtcVAdjusted = FALSE;
      pNew->CrtcHSkew = pNew->HSkew = 0;
      
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcHDisplay = pNew->HDisplay = val.num;
      else xf86ConfigError("Horizontal display expected");
          
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcHSyncStart = pNew->HSyncStart = val.num;
      else xf86ConfigError("Horizontal sync start expected");
          
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcHSyncEnd = pNew->HSyncEnd = val.num;
      else xf86ConfigError("Horizontal sync end expected");
          
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcHTotal = pNew->HTotal = val.num;
      else xf86ConfigError("Horizontal total expected");
          
          
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcVDisplay = pNew->VDisplay = val.num;
      else xf86ConfigError("Vertical display expected");
          
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcVSyncStart = pNew->VSyncStart = val.num;
      else xf86ConfigError("Vertical sync start expected");
          
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcVSyncEnd = pNew->VSyncEnd = val.num;
      else xf86ConfigError("Vertical sync end expected");
          
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcVTotal = pNew->VTotal = val.num;
      else xf86ConfigError("Vertical total expected");

      token = xf86GetToken(TimingTab);
      while ( (token == TT_INTERLACE) || (token == TT_PHSYNC) ||
              (token == TT_NHSYNC) || (token == TT_PVSYNC) ||
              (token == TT_NVSYNC) || (token == TT_CSYNC) ||
              (token == TT_PCSYNC) || (token == TT_NCSYNC) ||
              (token == TT_DBLSCAN) || (token == TT_HSKEW) )
      {
        switch(token) {
              
        case TT_INTERLACE: pNew->Flags |= V_INTERLACE;  break;
        case TT_PHSYNC:    pNew->Flags |= V_PHSYNC;     break;
        case TT_NHSYNC:    pNew->Flags |= V_NHSYNC;     break;
        case TT_PVSYNC:    pNew->Flags |= V_PVSYNC;     break;
        case TT_NVSYNC:    pNew->Flags |= V_NVSYNC;     break;
        case TT_CSYNC:     pNew->Flags |= V_CSYNC;      break;
        case TT_PCSYNC:    pNew->Flags |= V_PCSYNC;     break;
        case TT_NCSYNC:    pNew->Flags |= V_NCSYNC;     break;
        case TT_DBLSCAN:   pNew->Flags |= V_DBLSCAN;    break;
	case TT_HSKEW:
	  if (xf86GetToken(NULL) != NUMBER)
	    xf86ConfigError("Horizontal skew expected");
	  pNew->CrtcHSkew = pNew->HSkew = val.num;
	  pNew->Flags |= V_HSKEW;
	  break;
        default:
          xf86ConfigError("bug found in config reader"); break;
        }
        token = xf86GetToken(TimingTab);
      }
      pushToken = token;
      monp->Last = pNew; /* GJA */
      break;
    case BANDWIDTH:
      /* This should be completely removed at some point */
      if ((token = xf86GetToken(NULL)) != NUMBER)
        xf86ConfigError("Bandwidth number expected");
#if 0
      monp->bandwidth = val.realnum;
      /* Handle optional scaler */
      token = xf86GetToken(UnitTab);
      switch ( token ) {
      case HRZ: multiplier = 1.0e-6; break;
      case KHZ: multiplier = 1.0e-3; break;
      case MHZ: multiplier = 1.0; break;
      default: multiplier = 1.0; pushToken = token;
      }
      monp->bandwidth *= multiplier;
#endif
      break;
    case HORIZSYNC:
      if ((token = xf86GetToken(NULL)) != NUMBER)
        xf86ConfigError("Horizontal sync value expected");
      monp->hsync[monp->n_hsync].lo = val.realnum;
      if ((token = xf86GetToken(NULL)) == DASH) {
        if ((token = xf86GetToken(NULL)) != NUMBER)
           xf86ConfigError("Upperbound for horizontal sync value expected");
           monp->hsync[monp->n_hsync].hi = val.realnum;
      } else {
           pushToken = token;
           monp->hsync[monp->n_hsync].hi = monp->hsync[monp->n_hsync].lo;
      }
      monp->n_hsync++;
      while ( (token = xf86GetToken(NULL)) == COMMA ) {
        if ( monp->n_hsync == MAX_HSYNC )
           xf86ConfigError("Sorry. Too many horizontal sync intervals.");

        if ((token = xf86GetToken(NULL)) != NUMBER)
          xf86ConfigError("Horizontal sync value expected");
        monp->hsync[monp->n_hsync].lo = val.realnum;
        if ((token = xf86GetToken(NULL)) == DASH) {
          if ((token = xf86GetToken(NULL)) != NUMBER)
             xf86ConfigError("Upperbound for horizontal sync value expected");
             monp->hsync[monp->n_hsync].hi = val.realnum;
        } else {
          pushToken = token;
          monp->hsync[monp->n_hsync].hi = monp->hsync[monp->n_hsync].lo;
        }
        monp->n_hsync++;
      }
      pushToken = token;
      /* Handle optional scaler */
      token = xf86GetToken(UnitTab);
      switch ( token ) {
      case HRZ: multiplier = 1.0e-3; break;
      case KHZ: multiplier = 1.0; break;
      case MHZ: multiplier = 1.0e3; break;
      default: multiplier = 1.0; pushToken = token;
      }
      for ( i = 0 ; i < monp->n_hsync ; i++ ) {
         monp->hsync[i].hi *= multiplier;
         monp->hsync[i].lo *= multiplier;
      }
      break;
    case VERTREFRESH:
      if ((token = xf86GetToken(NULL)) != NUMBER)
        xf86ConfigError("Vertical refresh value expected");
      monp->vrefresh[monp->n_vrefresh].lo = val.realnum;
      if ((token = xf86GetToken(NULL)) == DASH) {
        if ((token = xf86GetToken(NULL)) != NUMBER)
          xf86ConfigError("Upperbound for vertical refresh value expected");
        monp->vrefresh[monp->n_vrefresh].hi = val.realnum;
      } else {
        monp->vrefresh[monp->n_vrefresh].hi =
            monp->vrefresh[monp->n_vrefresh].lo;
        pushToken = token;
      }
      monp->n_vrefresh++;
      while ( (token = xf86GetToken(NULL)) == COMMA ) {
        if ( monp->n_vrefresh == MAX_HSYNC )
          xf86ConfigError("Sorry. Too many vertical refresh intervals.");

        if ((token = xf86GetToken(NULL)) != NUMBER)
          xf86ConfigError("Vertical refresh value expected");
        monp->vrefresh[monp->n_vrefresh].lo = val.realnum;
        if ((token = xf86GetToken(NULL)) == DASH) {
          if ((token = xf86GetToken(NULL)) != NUMBER)
            xf86ConfigError("Upperbound for vertical refresh value expected");
          monp->vrefresh[monp->n_vrefresh].hi = val.realnum;
        } else {
          monp->vrefresh[monp->n_vrefresh].hi =
              monp->vrefresh[monp->n_vrefresh].lo;
          pushToken = token;
        }
        monp->n_vrefresh++;
      }
      pushToken = token;
      /* Handle optional scaler */
      token = xf86GetToken(UnitTab);
      switch ( token ) {
      case HRZ: multiplier = 1.0; break;
      case KHZ: multiplier = 1.0e3; break;
      case MHZ: multiplier = 1.0e6; break;
      default: multiplier = 1.0; pushToken = token;
      }
      for ( i = 0 ; i < monp->n_vrefresh ; i++ ) {
         monp->vrefresh[i].hi *= multiplier;
         monp->vrefresh[i].lo *= multiplier;
      }
      break;
    case GAMMA: {
       char *msg = "gamma correction value(s) expected\n either one value or three r/g/b values with 0.1 <= gamma <= 10";
       if ((token = xf86GetToken(NULL)) != NUMBER || val.realnum<0.1 || val.realnum>10)
	  xf86ConfigError(msg);
       else {
	  xf86rGamma = xf86gGamma = xf86bGamma = 1.0 / val.realnum;
	  if ((token = xf86GetToken(NULL)) == NUMBER) {
	     if (val.realnum<0.1 || val.realnum>10) xf86ConfigError(msg);
	     else {
		xf86gGamma = 1.0 / val.realnum;
		if ((token = xf86GetToken(NULL)) != NUMBER || val.realnum<0.1 || val.realnum>10)
		   xf86ConfigError(msg);
		else {
		   xf86bGamma = 1.0 / val.realnum;
		}
	     }
	  }
	  else pushToken = token;
       }
       break;
    }
    case EOF:
      FatalError("Unexpected EOF. Missing EndSection?");
      break; /* :-) */

    default:
      xf86ConfigError("Monitor section keyword expected");
      break;
    }
  }
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}

static CONFIG_RETURN_TYPE
configDynamicModuleSection()
{
    int		token;
 
    while ((token = xf86GetToken(ModuleTab)) != ENDSECTION) {
	switch (token) {
	case LOAD:
	    if (xf86GetToken(NULL) != STRING)
		xf86ConfigError("Dynamic module expected");
	    else {
#ifdef DYNAMIC_MODULE
		if (!modulePath) {
		    static Bool firstTime = TRUE;

		    modulePath = (char*)Xcalloc(strlen(DEFAULT_MODULE_PATH)+1);
		    strcpy(modulePath, DEFAULT_MODULE_PATH);
		
		    if (xf86Verbose && firstTime) {
			ErrorF("%s no ModulePath specified using default: %s\n",
			       XCONFIG_PROBED, DEFAULT_MODULE_PATH);
			firstTime = FALSE;
		    }
		}
		xf86LoadModule(val.str, modulePath);
#else
		ErrorF("Dynamic modules not supported. \"%s\" not loaded\n",
		       val.str);
#endif
	    }
	    break;

	case EOF:
	    FatalError("Unexpected EOF. Missing EndSection?");
	    break; /* :-) */
	    
	default:
	    xf86ConfigError("Module section keyword expected");
	    break;
	}    
    }
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}

static CONFIG_RETURN_TYPE
readVerboseMode(monp)
MonPtr monp;
{
  int token, token2;
  int had_dotclock = 0, had_htimings = 0, had_vtimings = 0;

  pNew = (DisplayModePtr)xalloc(sizeof(DisplayModeRec));
  pNew->next = NULL;
  pNew->prev = NULL;
  pNew->Flags = 0;
  pNew->HDisplay = pNew->VDisplay = 0; /* Uninitialized */
  pNew->CrtcHAdjusted = pNew->CrtcVAdjusted = FALSE;
  pNew->CrtcHSkew = pNew->HSkew = 0;

  if (monp->Last) 
    monp->Last->next = pNew;
  else
    monp->Modes = pNew;
  monp->Last = pNew;

  if ( xf86GetToken(NULL) != STRING ) {
    FatalError("Mode name expected");
  }
  pNew->name = val.str;
  while ((token = xf86GetToken(ModeTab)) != ENDMODE) {
    switch (token) {
    case DOTCLOCK:
      if ((token = xf86GetToken(NULL)) != NUMBER) {
        FatalError("Dotclock expected");
      }
      pNew->Clock = (int)(val.realnum * 1000.0 + 0.5);
      had_dotclock = 1;
      break;
    case HTIMINGS:
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcHDisplay = pNew->HDisplay = val.num;
      else xf86ConfigError("Horizontal display expected");
          
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcHSyncStart = pNew->HSyncStart = val.num;
      else xf86ConfigError("Horizontal sync start expected");
          
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcHSyncEnd = pNew->HSyncEnd = val.num;
      else xf86ConfigError("Horizontal sync end expected");
          
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcHTotal = pNew->HTotal = val.num;
      else xf86ConfigError("Horizontal total expected");
      had_htimings = 1;
      break;
    case VTIMINGS:
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcVDisplay = pNew->VDisplay = val.num;
      else xf86ConfigError("Vertical display expected");
          
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcVSyncStart = pNew->VSyncStart = val.num;
      else xf86ConfigError("Vertical sync start expected");
          
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcVSyncEnd = pNew->VSyncEnd = val.num;
      else xf86ConfigError("Vertical sync end expected");
          
      if (xf86GetToken(NULL) == NUMBER)
	pNew->CrtcVTotal = pNew->VTotal = val.num;
      else xf86ConfigError("Vertical total expected");
      had_vtimings = 1;
      break;
    case FLAGS:
      token = xf86GetToken(NULL);
      if (token != STRING)
        xf86ConfigError("Flag string expected.  Note: flags must be in \"\"");
      while ( token == STRING ) {
        token2 = getStringToken(TimingTab);
        switch(token2) {
        case TT_INTERLACE: pNew->Flags |= V_INTERLACE;  break;
        case TT_PHSYNC:    pNew->Flags |= V_PHSYNC;     break;
        case TT_NHSYNC:    pNew->Flags |= V_NHSYNC;     break;
        case TT_PVSYNC:    pNew->Flags |= V_PVSYNC;     break;
        case TT_NVSYNC:    pNew->Flags |= V_NVSYNC;     break;
        case TT_CSYNC:     pNew->Flags |= V_CSYNC;      break;
        case TT_PCSYNC:    pNew->Flags |= V_PCSYNC;     break;
        case TT_NCSYNC:    pNew->Flags |= V_NCSYNC;     break;
        case TT_DBLSCAN:   pNew->Flags |= V_DBLSCAN;    break;
        default:
          xf86ConfigError("Unknown flag string"); break;
        }
        token = xf86GetToken(NULL);
      }
      pushToken = token;
      break;
    case HSKEW:
      if (xf86GetToken(NULL) != NUMBER)
	xf86ConfigError("Horizontal skew expected");
      pNew->Flags |= V_HSKEW;
      pNew->CrtcHSkew = pNew->HSkew = val.num;
      break;
    }
  }
  if ( !had_dotclock ) xf86ConfigError("the dotclock is missing");
  if ( !had_htimings ) xf86ConfigError("the horizontal timings are missing");
  if ( !had_vtimings ) xf86ConfigError("the vertical timings are missing");
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}

static Bool dummy;

#ifdef XF86SETUP
int xf86setup_scrn_ndisps[8];
DispPtr xf86setup_scrn_displays[8];
#endif

static CONFIG_RETURN_TYPE
configScreenSection()
{
  int i, j;
  int driverno;
  int had_monitor = 0, had_device = 0;
  int dispIndex = 0;
  int numDisps = 0;
  DispPtr dispList = NULL;
  DispPtr dispp;
      
  int token;
  ScrnInfoPtr screen = NULL;
  int textClockValue = -1;

  token = xf86GetToken(ScreenTab);
  if ( token != DRIVER )
	xf86ConfigError("The screen section must begin with the 'driver' line");

  if (xf86GetToken(NULL) != STRING) xf86ConfigError("Driver name expected");
  driverno = getStringToken(DriverTab);
  switch ( driverno ) {
  case SVGA:
  case VGA2:
  case MONO:
  case VGA16:
  case ACCEL:
  case FBDEV:
	break;
  default:
    xf86ConfigError("Not a recognized driver name");
  }
  scr_index = getScreenIndex(driverno);

  dummy = scr_index < 0 || !xf86Screens[scr_index];
  if (dummy)
    screen = (ScrnInfoPtr)xalloc(sizeof(ScrnInfoRec));
  else
  {
    screen = xf86Screens[scr_index];
    screen->configured = TRUE;
    screen->tmpIndex = screenno++;
    screen->scrnIndex = scr_index;	/* scrnIndex must not be changed */
    screen->frameX0 = -1;
    screen->frameY0 = -1;
    screen->virtualX = -1;
    screen->virtualY = -1;
    screen->defaultVisual = -1;
    screen->modes = NULL;
    screen->width = 240;
    screen->height = 180;
    screen->bankedMono = FALSE;
    screen->textclock = -1;
    screen->blackColour.red = 0;
    screen->blackColour.green = 0;
    screen->blackColour.blue = 0;
    screen->whiteColour.red = 0x3F;
    screen->whiteColour.green = 0x3F;
    screen->whiteColour.blue = 0x3F;
  }
  screen->clocks = 0;

  while ((token = xf86GetToken(ScreenTab)) != ENDSECTION) {
    switch (token) {

    case DEFBPP:
      if (xf86GetToken(NULL) != NUMBER) 
        xf86ConfigError("Default color depth expected");
      screen->depth = val.num;
      break;

    case SCREENNO:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Screen number expected");
      screen->tmpIndex = val.num;
      break;

    case SUBSECTION:
      if ((xf86GetToken(NULL) != STRING) || (StrCaseCmp(val.str, "display") != 0)) {
        xf86ConfigError("You must say \"Display\" here");
      }
      if (dispList == NULL) {
        dispList = (DispPtr)xalloc(sizeof(DispRec));
      } else {
        dispList = (DispPtr)xrealloc(dispList,
				     (numDisps + 1) * sizeof(DispRec));
      }
      dispp = dispList + numDisps;
      numDisps++;
      dispp->depth = -1;
      dispp->weight.red = dispp->weight.green = dispp->weight.blue = 0;
      dispp->frameX0 = -1;
      dispp->frameY0 = -1;
      dispp->virtualX = -1;
      dispp->virtualY = -1;
      dispp->modes = NULL;
      dispp->whiteColour.red = dispp->whiteColour.green = 
                               dispp->whiteColour.blue = 0x3F;
      dispp->blackColour.red = dispp->blackColour.green = 
                               dispp->blackColour.blue = 0;
      dispp->defaultVisual = -1;
      OFLG_ZERO(&(dispp->options));
      OFLG_ZERO(&(dispp->xconfigFlag));
      dispp->DCOptions = NULL;

      configDisplaySubsection(dispp);
      break;

    case EOF:
      FatalError("Unexpected EOF (missing EndSection?)");
      break; /* :-) */

    case MDEVICE:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("Device name expected");
      for ( i = 0 ; i < n_devices ; i++ ) {
        if ( strcmp(device_list[i].identifier,val.str) == 0 ) {
          /* Copy back */
          if (!dummy && xf86Verbose) {
            ErrorF("%s %s: Graphics device ID: \"%s\"\n",
                   XCONFIG_GIVEN, screen->name, device_list[i].identifier);
          }
          screen->clocks = device_list[i].clocks;
          for ( j = 0 ; j < MAXCLOCKS ; j++ ) {
             screen->clock[j] = device_list[i].clock[j];
          }
          screen->chipset = device_list[i].chipset;
          screen->ramdac = device_list[i].ramdac;
	  for (j=0; j<MAXDACSPEEDS; j++)
	     screen->dacSpeeds[j] = device_list[i].dacSpeeds[j];
	  screen->dacSpeedBpp = 0;
          screen->options = device_list[i].options;
          screen->clockOptions = device_list[i].clockOptions;
          screen->xconfigFlag = device_list[i].xconfigFlag;
          screen->videoRam = device_list[i].videoRam;
          screen->speedup = device_list[i].speedup;
          screen->clockprog = device_list[i].clockprog;
          textClockValue = device_list[i].textClockValue;
          if (OFLG_ISSET(XCONFIG_BIOSBASE, &screen->xconfigFlag))
            screen->BIOSbase = device_list[i].BIOSbase;
          if (OFLG_ISSET(XCONFIG_MEMBASE, &screen->xconfigFlag))
            screen->MemBase = device_list[i].MemBase;
          if (OFLG_ISSET(XCONFIG_IOBASE, &screen->xconfigFlag))
            screen->IObase = device_list[i].IObase;
          if (OFLG_ISSET(XCONFIG_DACBASE, &screen->xconfigFlag))
            screen->DACbase = device_list[i].DACbase;
          if (OFLG_ISSET(XCONFIG_COPBASE, &screen->xconfigFlag))
            screen->COPbase = device_list[i].COPbase;
          if (OFLG_ISSET(XCONFIG_POSBASE, &screen->xconfigFlag))
            screen->POSbase = device_list[i].POSbase;
          if (OFLG_ISSET(XCONFIG_INSTANCE, &screen->xconfigFlag))
            screen->instance = device_list[i].instance;
          screen->s3Madjust = device_list[i].s3Madjust;
          screen->s3Nadjust = device_list[i].s3Nadjust;
	  screen->s3MClk = device_list[i].s3MClk;
	  screen->MemClk = device_list[i].MemClk;
	  screen->LCDClk = device_list[i].LCDClk;
	  screen->chipID = device_list[i].chipID;
	  screen->chipRev = device_list[i].chipRev;
	  screen->s3RefClk = device_list[i].s3RefClk;
	  screen->s3BlankDelay = device_list[i].s3BlankDelay;
	  screen->textClockFreq = device_list[i].textClockValue;
	  if (OFLG_ISSET(XCONFIG_VGABASE, &screen->xconfigFlag))
	    screen->VGAbase = device_list[i].VGAbase;
	  screen->DCConfig = device_list[i].DCConfig;
	  screen->DCOptions = device_list[i].DCOptions;
#ifdef XF86SETUP
	  screen->device = (void *) &device_list[i];
#endif
          break;
        }
      }
      if ( i == n_devices ) { /* Exhausted the device list */
         xf86ConfigError("Not a declared device");
      }
      had_device = 1;
      break;
      
    case MONITOR:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("Monitor name expected");
      for ( i = 0 ; i < n_monitors ; i++ ) {
        if ( strcmp(monitor_list[i].id,val.str) == 0 ) {
          if (!dummy && xf86Verbose) {
            ErrorF("%s %s: Monitor ID: \"%s\"\n",
                   XCONFIG_GIVEN, screen->name, monitor_list[i].id);
          }
	  if (!dummy) {
            monitor_list[i].Modes = xf86PruneModes(&monitor_list[i],
                                                   monitor_list[i].Modes,
                                                   screen, FALSE);
	    screen->monitor = (MonPtr)xalloc(sizeof(MonRec));
	    memcpy(screen->monitor, &monitor_list[i], sizeof(MonRec));
          }
          break;
        }
      }
      if ( i == n_monitors ) { /* Exhausted the monitor list */
         xf86ConfigError("Not a declared monitor");
      }
      had_monitor = 1;
      break;
      
    case BLANKTIME:
      if (xf86GetToken(NULL) != NUMBER)
	xf86ConfigError("Screensaver blank time expected");
      if (!dummy && !xf86sFlag)
	defaultScreenSaverTime = ScreenSaverTime = val.num * MILLI_PER_MIN;
      break;

    case STANDBYTIME:
      if (xf86GetToken(NULL) != NUMBER)
	xf86ConfigError("Screensaver standby time expected");
#ifdef DPMSExtension
      if (!dummy)
        DPMSStandbyTime = val.num * MILLI_PER_MIN;
#endif
      break;

    case SUSPENDTIME:
      if (xf86GetToken(NULL) != NUMBER)
	xf86ConfigError("Screensaver suspend time expected");
#ifdef DPMSExtension
      if (!dummy)
        DPMSSuspendTime = val.num * MILLI_PER_MIN;
#endif
      break;

    case OFFTIME:
      if (xf86GetToken(NULL) != NUMBER)
	xf86ConfigError("Screensaver off time expected");
#ifdef DPMSExtension
      if (!dummy)
        DPMSOffTime = val.num * MILLI_PER_MIN;
#endif
      break;

    default:
      if (!dummy && !validateGraphicsToken(screen->validTokens, token))
      {
        xf86ConfigError("Screen section keyword expected");
      }
      break;
    }
  }

  if (!dummy) {
    if (dispList == NULL) {
      FatalError(
         "A \"Display\" subsection is required in each \"Screen\" section\n");
    } else {
      /* Work out which if any Display subsection to use based on depth */
      if (xf86bpp < 0) { 
        /*
	 * no -bpp option given, so take depth if only one Display subsection
	 * Don't do this for VGA2 and VGA16 where it makes no sense, and only
	 * causes problems
	 */
        if (numDisps == 1) {
#ifndef XF86SETUP
          if (dispList[0].depth > 0
	      && !(driverno >= VGA2 && driverno <= VGA16)) {
            xf86bpp = dispList[0].depth;
          }
#endif
          dispIndex = 0;
        } else {
          xf86bpp = screen->depth;
          /* Look for a section which matches the driver's default depth */
          for (dispIndex = 0; dispIndex < numDisps; dispIndex++) {
            if (dispList[dispIndex].depth == screen->depth)
              break;
          }
          if (dispIndex == numDisps) {
            /* No match.  This time, allow 15/16 and 24/32 to match */
            for (dispIndex = 0; dispIndex < numDisps; dispIndex++) {
              if ((screen->depth == 15 && dispList[dispIndex].depth == 16) ||
                  (screen->depth == 16 && dispList[dispIndex].depth == 15) ||
                  (screen->depth == 24 && dispList[dispIndex].depth == 32) ||
                  (screen->depth == 32 && dispList[dispIndex].depth == 24))
                break;
            }
          }
          if (dispIndex == numDisps) {
            /* Still no match, so exit */
            FatalError("No \"Display\" subsection for default depth %d\n",
                       screen->depth);
          }
        }
      } else {
        /* xf86bpp is set */
        if (numDisps == 1 && dispList[0].depth < 0) {
          /* one Display subsection, no depth set, so use it */
          /* XXXX Maybe should only do this when xf86bpp == default depth?? */
          dispIndex = 0;
        } else {
          /* find Display subsection matching xf86bpp */
          for (dispIndex = 0; dispIndex < numDisps; dispIndex++) {
            if (dispList[dispIndex].depth == xf86bpp)
              break;
          }
          if (dispIndex == numDisps) {
#if 0
            /* No match.  This time, allow 15/16 and 24/32 to match */
            for (dispIndex = 0; dispIndex < numDisps; dispIndex++) {
              if ((xf86bpp == 15 && dispList[dispIndex].depth == 16) ||
                  (xf86bpp == 16 && dispList[dispIndex].depth == 15) ||
                  (xf86bpp == 24 && dispList[dispIndex].depth == 32) ||
                  (xf86bpp == 32 && dispList[dispIndex].depth == 24))
                break;
#else
            /* No match.  This time, allow 15/16 to match */
            for (dispIndex = 0; dispIndex < numDisps; dispIndex++) {
              if ((xf86bpp == 15 && dispList[dispIndex].depth == 16) ||
                  (xf86bpp == 16 && dispList[dispIndex].depth == 15))
                break;
#endif
            }
          }
	  if (dispIndex == numDisps) {
	    if (!(driverno >= VGA2 && driverno <= VGA16)) {
	      /* Still no match, so exit */
	      FatalError("No \"Display\" subsection for -bpp depth %d\n",
			 xf86bpp);
	    }
	    else 
	      dispIndex = 0;
	  }
        }
      }
      /* Now copy the info across to the screen rec */
      dispp = dispList + dispIndex;
      if (xf86bpp > 0) screen->depth = xf86bpp;
      else if (dispp->depth > 0) screen->depth = dispp->depth;
      if (xf86weight.red || xf86weight.green || xf86weight.blue)
	 screen->weight = xf86weight;
      else if (dispp->weight.red > 0) {
	 screen->weight = dispp->weight;
	 xf86weight = dispp->weight;
      }
      screen->frameX0 = dispp->frameX0;
      screen->frameY0 = dispp->frameY0;
      screen->virtualX = dispp->virtualX;
      screen->virtualY = dispp->virtualY;
      screen->modes = dispp->modes;
      screen->whiteColour = dispp->whiteColour;
      screen->blackColour = dispp->blackColour;
      screen->defaultVisual = dispp->defaultVisual;
      /* Add any new options that might be set */
      for (i = 0; i < MAX_OFLAGS; i++) {
        if (OFLG_ISSET(i, &(dispp->options)))
          OFLG_SET(i, &(screen->options));
        if (OFLG_ISSET(i, &(dispp->xconfigFlag)))
          OFLG_SET(i, &(screen->xconfigFlag));
      }
	screen->DCOptions = xf86DCConcatOption(screen->DCOptions,dispp->DCOptions);
#ifdef XF86SETUP
      xf86setup_scrn_ndisps[driverno-SVGA] = numDisps;
      xf86setup_scrn_displays[driverno-SVGA] = dispList;
#else
      /* Don't need them any more */
      xfree(dispList);
#endif
    }
  
    /* Maybe these should be FatalError() instead? */
    if ( !had_monitor ) {
      xf86ConfigError("A screen must specify a monitor");
    }
    if ( !had_device ) {
      xf86ConfigError("A screen must specify a device");
    }
  }

  /* Check for information that must be specified in XF86Config */
  if (scr_index >= 0 && xf86Screens[scr_index])
  {
    ScrnInfoPtr driver = xf86Screens[scr_index];

    graphFound = TRUE;

    if (driver->clockprog && !driver->clocks)
    {
       if (!OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &(screen->clockOptions))){
       ErrorF("%s: No clock line specified: assuming programmable clocks\n");
       OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &(screen->clockOptions));}
       driver->textclock = textClockValue;
    }

    /* Find the Index of the Text Clock for the ClockProg */
    if (driver->clockprog && textClockValue > 0
 	&& !OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &(screen->clockOptions)))
    {
      driver->textclock = xf86GetNearestClock(driver, textClockValue);
      if (abs(textClockValue - driver->clock[driver->textclock]) >
          CLOCK_TOLERANCE)
        FatalError(
          "There is no defined dot-clock matching the text clock\n");
      if (xf86Verbose)
        ErrorF("%s %s: text clock = %7.3f, clock used = %7.3f\n",
          XCONFIG_GIVEN,
          driver->name, textClockValue / 1000.0,
          driver->clock[driver->textclock] / 1000.0);
    }
    if (xf86Verbose && driver->defaultVisual > 0) {
      char *visualname;
      switch (driver->defaultVisual) {
      case StaticGray:
      case GrayScale:
      case StaticColor:
      case PseudoColor:
      case TrueColor:
      case DirectColor:
        visualname = xf86VisualNames[driver->defaultVisual];
        break;
      default:
        xf86ConfigError("unknown visual type");
      }
      ErrorF("%s %s: Default visual: %s\n", XCONFIG_GIVEN, driver->name,
             visualname);
    }
    if (defaultColorVisualClass < 0)
      defaultColorVisualClass = driver->defaultVisual;

    /* GJA --Moved these from the device code. Had to reorganize it
     * a bit.
     */
    if (xf86Verbose) {
       if (OFLG_ISSET(XCONFIG_IOBASE, &driver->xconfigFlag)) 
           ErrorF("%s %s: Direct Access Register I/O Base Address: %x\n",
                  XCONFIG_GIVEN, driver->name, driver->IObase);

       if (OFLG_ISSET(XCONFIG_DACBASE, &driver->xconfigFlag)) 
           ErrorF("%s %s: DAC Base I/O Address: %x\n",
                  XCONFIG_GIVEN, driver->name, driver->DACbase);

       if (OFLG_ISSET(XCONFIG_COPBASE, &driver->xconfigFlag)) 
           ErrorF("%s %s: Coprocessor Base Memory Address: %x\n",
                  XCONFIG_GIVEN, driver->name, driver->COPbase);

       if (OFLG_ISSET(XCONFIG_POSBASE, &driver->xconfigFlag)) 
           ErrorF("%s %s: POS Base Address: %x\n", XCONFIG_GIVEN, driver->name,
                driver->POSbase);

       if (OFLG_ISSET(XCONFIG_BIOSBASE, &driver->xconfigFlag)) 
          ErrorF("%s %s: BIOS Base Address: %x\n", XCONFIG_GIVEN, driver->name,
	         driver->BIOSbase);

       if (OFLG_ISSET(XCONFIG_MEMBASE, &driver->xconfigFlag)) 
          ErrorF("%s %s: Memory Base Address: %x\n", XCONFIG_GIVEN,
                 driver->name, driver->MemBase);

       if (OFLG_ISSET(XCONFIG_VGABASE, &driver->xconfigFlag)) 
          ErrorF("%s %s: VGA Aperture Base Address: %x\n", XCONFIG_GIVEN,
                 driver->name, driver->VGAbase);

      /* Print clock program */
      if ( driver->clockprog ) {
        ErrorF("%s %s: ClockProg: \"%s\"", XCONFIG_GIVEN, driver->name,
               driver->clockprog);
        if ( textClockValue )
            ErrorF(", Text Clock: %7.3f\n", textClockValue / 1000.0);
        ErrorF("\n");
       }
    }
  }
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}

static CONFIG_RETURN_TYPE
configDisplaySubsection(disp)
DispPtr disp;
{
  int token;
  int i;

  while ((token = xf86GetToken(DisplayTab)) != ENDSUBSECTION) {
    switch (token) {
    case DEPTH:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Display depth expected");
      disp->depth = val.num;
      break;

    case WEIGHT:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Display weight expected");
      if (val.num > 9) {
        disp->weight.red = (val.num / 100) % 10;
        disp->weight.green = (val.num / 10) % 10;
        disp->weight.blue = val.num % 10;
      } else {
        disp->weight.red = val.num;
        if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Display weight expected");
        disp->weight.green = val.num;
        if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Display weight expected");
        disp->weight.blue = val.num;
      }
      break;

    case VIEWPORT:
      OFLG_SET(XCONFIG_VIEWPORT,&(disp->xconfigFlag));
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Viewport X expected");
      disp->frameX0 = val.num;
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Viewport Y expected");
      disp->frameY0 = val.num;
      break;

    case VIRTUAL:
      OFLG_SET(XCONFIG_VIRTUAL,&(disp->xconfigFlag));
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Virtual X expected");
      disp->virtualX = val.num;
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Virtual Y expected");
      disp->virtualY = val.num;
      break;

    case MODES:
      for (pLast=NULL; (token = xf86GetToken(NULL)) == STRING; pLast = pNew)
	{
	  pNew = (DisplayModePtr)xalloc(sizeof(DisplayModeRec));
	  pNew->name = val.str;
	  pNew->PrivSize = 0;
	  pNew->Private = NULL;

	  if (pLast) 
	    {
	      pLast->next = pNew;
	      pNew->prev  = pLast;
	    }
	  else
	    disp->modes = pNew;
	}
      /* Make sure at least one mode was present */
      if (!pLast)
	 xf86ConfigError("Mode name expected");
      pNew->next = disp->modes;
      disp->modes->prev = pLast;
      pushToken = token;
      break;

    case BLACK:
    case WHITE:
      {
        unsigned char rgb[3];
        int ii;
        
        for (ii = 0; ii < 3; ii++)
        {
          if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("RGB value expected");
          rgb[ii] = val.num & 0x3F;
        }
        if (token == BLACK)
        {
          disp->blackColour.red = rgb[0];
          disp->blackColour.green = rgb[1];
          disp->blackColour.blue = rgb[2];
        }
        else
        {
          disp->whiteColour.red = rgb[0];
          disp->whiteColour.green = rgb[1];
          disp->whiteColour.blue = rgb[2];
        }
      }
      break;

    case VISUAL:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("Visual name expected");
      token = getStringToken(VisualTab);
      if (!dummy && disp->defaultVisual >= 0)
        xf86ConfigError("Only one default visual may be specified");
      disp->defaultVisual = token - STATICGRAY;
      break;

    case OPTION:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("Option string expected");
      i = 0;
      while (xf86_OptionTab[i].token != -1) 
      {
	if (StrCaseCmp(val.str, xf86_OptionTab[i].name) == 0)
	{
          OFLG_SET(xf86_OptionTab[i].token, &(disp->options));
	  break;
	}
	i++;
      }
      if (xf86_OptionTab[i].token == -1)
      disp->DCOptions = xf86DCOption(disp->DCOptions,val);
      break;

    /* The following should really go in the S3 server */
    case INVERTVCLK:
    case BLANKDELAY:
    case EARLYSC:
     {
       DisplayModePtr p = disp->modes;
       if (xf86GetToken(NULL) != STRING) xf86ConfigError("Mode name expected");
       if (disp->modes == NULL)
	 xf86ConfigError("This must be after the Modes line");
       {
	 Bool found = FALSE;
	 int opt;
	 INT32 value;
	 char *mode_string = (char *)xalloc(strlen(val.str)+1);
	 strcpy(mode_string,val.str);

	 switch (token) {
	 default:	/* pacify compiler (uninitialized opt, value) */
	 case INVERTVCLK:
	    if (xf86GetToken(NULL) != NUMBER || val.num < 0 || val.num > 1)
	       xf86ConfigError("0 or 1 expected");
	    opt = S3_INVERT_VCLK;
	    value = val.num;
	    break;
	    
	 case BLANKDELAY:
	    if (xf86GetToken(NULL) != NUMBER || val.num < 0 || val.num > 7)
	       xf86ConfigError("number(s) 0..7 expected");
	    opt = S3_BLANK_DELAY;
	    value = val.num;
	    if ((token=xf86GetToken(NULL)) == NUMBER) {
	       if (val.num < 0 || val.num > 7)
		 xf86ConfigError("number2 0..7 expected");
	       value |= val.num << 4;
	    }
	    else pushToken = token;
	    break;
    
	 case EARLYSC:
	    if (xf86GetToken(NULL) != NUMBER || val.num < 0 || val.num > 1)
	       xf86ConfigError("0 or 1 expected");
	    opt = S3_EARLY_SC;
	    value = val.num;
	    break;
	 }
	 
	 do {
	    if (strcmp(p->name, mode_string) == 0
		|| strcmp("*", mode_string) == 0) {
	       found = TRUE;
	       if (!p->PrivSize || !p->Private) {
		  p->PrivSize = S3_MODEPRIV_SIZE;
		  p->Private = (INT32 *)Xcalloc(sizeof(INT32) * S3_MODEPRIV_SIZE);
		  p->Private[0] = 0;
	       }
	       p->Private[0] |= (1 << opt);
	       p->Private[opt] = value;
	    }
	    p = p->next;
         } while (p != disp->modes);
         if (!found) xf86ConfigError("No mode of that name in the Modes line");
	 xfree(mode_string);
       }
     }
     break;
    
    
    case EOF:
      FatalError("Unexpected EOF (missing EndSubSection)");
      break; /* :-) */

    default:
      xf86ConfigError("Display subsection keyword expected");
      break;
    }
  }
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}

Bool 
xf86LookupMode(target, driver, flags)
     DisplayModePtr target;
     ScrnInfoPtr    driver;
     int	    flags;
{
  DisplayModePtr p;
  DisplayModePtr best_mode = NULL;
  int            i, j, k, Gap;
  int            Minimum_Gap = CLOCK_TOLERANCE + 1;
  Bool           found_mode = FALSE;
  Bool           clock_too_high = FALSE;
  static Bool	 first_time = TRUE;
  double         refresh, bestRefresh = 0.0;

  if (first_time)
  {
    ErrorF("%s %s: Maximum allowed dot-clock: %1.3f MHz\n", XCONFIG_PROBED,
	   driver->name, driver->maxClock / 1000.0);
    first_time = FALSE;
    /*
     * First time through, cull modes which are not valid for the
     * card/driver.
     */
    driver->monitor->Modes = xf86PruneModes(NULL, driver->monitor->Modes,
					    driver, TRUE);
  }

  if (xf86BestRefresh && !(flags & LOOKUP_FORCE_DEFAULT))
    flags |= LOOKUP_BEST_REFRESH;

  for (p = driver->monitor->Modes; p != NULL; p = p->next)	/* scan list */
  {
    if (!strcmp(p->name, target->name))		/* names equal ? */
    {
      /* First check if the driver objects to the mode */
      if ((driver->ValidMode)(p, xf86Verbose, MODE_USED) != MODE_OK)
      {
         ErrorF("%s %s: Mode \"%s\" rejected by driver.  Deleted.\n",
                XCONFIG_PROBED,driver->name, target->name );
	 break;
      }

      if ((flags & LOOKUP_NO_INTERLACED) && (p->Flags & V_INTERLACE))
      {
         continue;
      }

      if ((OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &(driver->clockOptions))) &&
	  !OFLG_ISSET(OPTION_NO_PROGRAM_CLOCKS, &(driver->options)))
      {
        if (driver->clocks == 0)
        {
          /* this we know */
          driver->clock[0] = 25175;		/* 25.175Mhz */
          driver->clock[1] = 28322;		/* 28.322MHz */
          driver->clocks = 2;
        }

        if ((p->Clock / 1000) > (driver->maxClock / 1000))
          clock_too_high = TRUE;
        else
        {
          /* We fill in the the programmable clocks as we go */
          for (i=0; i < driver->clocks; i++)
            if (driver->clock[i] == p->Clock)
              break;

          if (i >= MAXCLOCKS)
          {
            ErrorF("%s %s: Too many programmable clocks used (limit %d)!\n",
                   XCONFIG_PROBED, driver->name, MAXCLOCKS);
            return FALSE;
          }

          if (i == driver->clocks)
          {
            driver->clock[i] = p->Clock;
            driver->clocks++;
          }
	  

          if (flags & LOOKUP_BEST_REFRESH)
          {
             refresh = p->Clock * 1000.0 / p->HTotal / p->VTotal;
             if (p->Flags & V_INTERLACE)
             {
                refresh *= 2;
                refresh /= INTERLACE_REFRESH_WEIGHT;
             }
             else if (p->Flags & V_DBLSCAN)
             {
                refresh /= 2;
             }
             if (refresh > bestRefresh)
             {
                best_mode = p;
                bestRefresh = refresh;
                target->Clock = i;
             }
          }
          else
          {
             target->Clock = i;
             best_mode = p;
          }
        }
      }
      else
      {
        /*
         * go look if any of the clocks in the list matches the one in
         * the mode (j=1), or if a better match exists when the clocks
         * in the list are divided by 2 (j=2)
         */
        if (OFLG_ISSET(OPTION_CLKDIV2, &(driver->options)))
           k=2;
        else
           k=1;
        for (j=1 ; j<=k ; j++)
        {
          i = xf86GetNearestClock(driver, p->Clock*j);
          if (flags & LOOKUP_BEST_REFRESH)
          {
            if ( ((driver->clock[i]/j) / 1000) > (driver->maxClock / 1000) )
              clock_too_high = TRUE;
            else
            {
              refresh = p->Clock * 1000.0 / p->HTotal / p->VTotal;
              if (p->Flags & V_INTERLACE)
              {
                 refresh *= 2;
                 refresh /= INTERLACE_REFRESH_WEIGHT;
              }
              else if (p->Flags & V_DBLSCAN)
              {
                 refresh /= 2;
              }
              if (refresh > bestRefresh)
              {
                 target->Clock = i;
                 if (j==2) p->Flags |= V_CLKDIV2;
                 best_mode = p;
                 bestRefresh = refresh;
              }
            }
          }
          else
          {
            Gap = abs( p->Clock - (driver->clock[i]/j) );
            if (Gap < Minimum_Gap)
            {
              if ( ((driver->clock[i]/j) / 1000) > (driver->maxClock / 1000) )
                clock_too_high = TRUE;
              else
              {
                target->Clock = i;
                if (j==2) p->Flags |= V_CLKDIV2;
                best_mode = p;
                Minimum_Gap = Gap;
              }
            }
          }
        }
      }
      found_mode = TRUE;
    }
  }

  if (best_mode != NULL)
  {
    target->HDisplay       = best_mode->HDisplay;
    target->HSyncStart     = best_mode->HSyncStart;
    target->HSyncEnd       = best_mode->HSyncEnd;
    target->HTotal         = best_mode->HTotal;
    target->HSkew          = best_mode->HSkew;
    target->VDisplay       = best_mode->VDisplay;
    target->VSyncStart     = best_mode->VSyncStart;
    target->VSyncEnd       = best_mode->VSyncEnd;
    target->VTotal         = best_mode->VTotal;
    target->Flags          = best_mode->Flags; 
    target->CrtcHDisplay   = best_mode->CrtcHDisplay;
    target->CrtcHSyncStart = best_mode->CrtcHSyncStart;
    target->CrtcHSyncEnd   = best_mode->CrtcHSyncEnd;
    target->CrtcHTotal     = best_mode->CrtcHTotal;
    target->CrtcHSkew      = best_mode->CrtcHSkew;
    target->CrtcVDisplay   = best_mode->CrtcVDisplay;
    target->CrtcVSyncStart = best_mode->CrtcVSyncStart;
    target->CrtcVSyncEnd   = best_mode->CrtcVSyncEnd;
    target->CrtcVTotal     = best_mode->CrtcVTotal;
    target->CrtcHAdjusted  = best_mode->CrtcHAdjusted;
    target->CrtcVAdjusted  = best_mode->CrtcVAdjusted;
    if (target->Flags & V_DBLSCAN)
    {
      target->CrtcVDisplay *= 2;
      target->CrtcVSyncStart *= 2;
      target->CrtcVSyncEnd *= 2;
      target->CrtcVTotal *= 2;
      target->CrtcVAdjusted = TRUE;
    }

#if 0
    /* I'm not sure if this is the best place for this in the
     * new XF86Config organization. - SRA
     */
    if (found_mode)
      if ((driver->ValidMode)(target, xf86Verbose, MODE_USED) != MODE_OK)
        {
         ErrorF("%s %s: Unable to support mode \"%s\"\n",
              XCONFIG_GIVEN,driver->name, target->name );
         return(FALSE);
        }
#endif

    if (xf86Verbose)
    {
      ErrorF("%s %s: Mode \"%s\": mode clock = %7.3f",
             XCONFIG_GIVEN, driver->name, target->name,
             best_mode->Clock / 1000.0);
      if (!OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &(driver->clockOptions)) ||
	  OFLG_ISSET(OPTION_NO_PROGRAM_CLOCKS, &(driver->options))) {
        ErrorF(", clock used = %7.3f", driver->clock[target->Clock] / 1000.0);
        if (target->Flags & V_CLKDIV2)
          ErrorF("/2");
      }
      ErrorF("\n");
    }
  }
  else if (!found_mode)
    ErrorF("%s %s: There is no mode definition named \"%s\"\n",
	   XCONFIG_PROBED, driver->name, target->name);
  else if (clock_too_high)
    ErrorF("%s %s: Clock for mode \"%s\" %s\n\tLimit is %7.3f MHz\n",
           XCONFIG_PROBED, driver->name, target->name,
           "is too high for the configured hardware.",
           driver->maxClock / 1000.0);
  else
    ErrorF("%s %s: There is no defined dot-clock matching mode \"%s\"\n",
           XCONFIG_PROBED, driver->name, target->name);

  return (best_mode != NULL);
}

void
xf86VerifyOptions(allowedOptions, driver)
     OFlagSet      *allowedOptions;
     ScrnInfoPtr    driver;
{
  int j;

  for (j=0; xf86_OptionTab[j].token >= 0; j++)
    if ((OFLG_ISSET(xf86_OptionTab[j].token, &driver->options)))
      if (OFLG_ISSET(xf86_OptionTab[j].token, allowedOptions))
      {
	if (xf86Verbose)
	  ErrorF("%s %s: Option \"%s\"\n", XCONFIG_GIVEN,
	         driver->name, xf86_OptionTab[j].name);
      }
      else
	ErrorF("%s %s: Option flag \"%s\" is not defined for this driver\n",
	       XCONFIG_GIVEN, driver->name, xf86_OptionTab[j].name);
}

/* Note: (To keep me [GJA] from getting confused)
 * We have two mode-related datastructures:
 * 1. A doubly linked mode name list, with ends marked by self-pointers.
 * 2. A doubly linked mode structure list.
 * We are operating here on the second structure.
 * Initially this is just singly linked.
 */
static DisplayModePtr
xf86PruneModes(monp, allmodes, scrp, card)
     MonPtr monp;		/* Monitor specification */
     DisplayModePtr allmodes;	/* List to be pruned */
     ScrnInfoPtr scrp;
     Bool card;			/* TRUE => do driver validity check */
{
	DisplayModePtr dispmp;	/* To walk the list */
	DisplayModePtr olddispmp; /* The one being freed. */
	DisplayModePtr remainder; /* The first one retained. */

	dispmp = allmodes;

	/* The first modes to be deleted require that the pointer to the
	 * mode list is updated. Also, they have no predecessor in the list.
	 */
	while (dispmp &&
	       (card ?
		 ((scrp->ValidMode)(dispmp, xf86Verbose, MODE_SUGGESTED) 
		 	!= MODE_OK) :
		 (xf86CheckMode(scrp, dispmp, monp, xf86Verbose) != MODE_OK))) {
		olddispmp = dispmp;
		dispmp = dispmp->next;
		xfree(olddispmp->name);
		xfree(olddispmp);
	}
	/* Now we either have a mode that fits, or no mode at all */
	if ( ! dispmp ) { /* No mode at all */
		return NULL;
	}
	remainder = dispmp;
	while ( dispmp->next ) {
		if (card ?
		     ((scrp->ValidMode)(dispmp->next,xf86Verbose,MODE_SUGGESTED)
		     		!= MODE_OK) :
		     (xf86CheckMode(scrp, dispmp->next, monp, xf86Verbose) !=
                      MODE_OK)) {
			olddispmp = dispmp->next;
			dispmp->next = dispmp->next->next;
			xfree(olddispmp->name);
			xfree(olddispmp);
		} else {
			dispmp = dispmp->next;
		}
	}
	return remainder; /* Return pointer to {the first / the list } */
}

/*
 * Return MODE_OK if the mode pointed to by dispmp agrees with all constraints
 * we can make up for the monitor pointed to by monp.
 */
int
xf86CheckMode(scrp, dispmp, monp, verbose)
     ScrnInfoPtr scrp;
     DisplayModePtr dispmp;
     MonPtr monp;
     Bool verbose;
{
	int i;
	float dotclock, hsyncfreq, vrefreshrate;
	char *scrname = scrp->name;

	/* Sanity checks */
	if ((0 >= dispmp->HDisplay) ||
	    (dispmp->HDisplay > dispmp->HSyncStart) ||
	    (dispmp->HSyncStart >= dispmp->HSyncEnd) ||
	    (dispmp->HSyncEnd >= dispmp->HTotal))
	{
		ErrorF(
                  "%s %s: Invalid horizontal timing for mode \"%s\". Deleted.\n",
		  XCONFIG_PROBED, scrname, dispmp->name);
		return MODE_HSYNC;
	}

	if ((0 >= dispmp->VDisplay) ||
	    (dispmp->VDisplay > dispmp->VSyncStart) ||
	    (dispmp->VSyncStart >= dispmp->VSyncEnd) ||
	    (dispmp->VSyncEnd >= dispmp->VTotal))
	{
		ErrorF(
		  "%s %s: Invalid vertical timing for mode \"%s\". Deleted.\n",
		  XCONFIG_PROBED, scrname, dispmp->name);
		return MODE_VSYNC;
	}

	/* Deal with the dispmp->Clock being a frequency or index */
	if (dispmp->Clock > MAXCLOCKS) {
		dotclock = (float)dispmp->Clock;
	} else {
		dotclock = (float)scrp->clock[dispmp->Clock];
	}
	hsyncfreq = dotclock / (float)(dispmp->HTotal);
	for ( i = 0 ; i < monp->n_hsync ; i++ )
		if ( (hsyncfreq > 0.999 * monp->hsync[i].lo) &&
		     (hsyncfreq < 1.001 * monp->hsync[i].hi) )
			break; /* In range. */

	/* Now see whether we ran out of sync frequencies */
	if ( i == monp->n_hsync ) {
	    if (verbose) {
		ErrorF(
		  "%s %s: Mode \"%s\" needs hsync freq of %.2f kHz. Deleted.\n",
		  XCONFIG_PROBED, scrname, dispmp->name, hsyncfreq);
	    }
	    return MODE_HSYNC;
	}
			
	vrefreshrate = dotclock * 1000.0 /
			((float)(dispmp->HTotal) * (float)(dispmp->VTotal)) ;
	if ( dispmp->Flags & V_INTERLACE ) vrefreshrate *= 2.0;
	if ( dispmp->Flags & V_DBLSCAN ) vrefreshrate /= 2.0;
	for ( i = 0 ; i < monp->n_vrefresh ; i++ )
		if ( (vrefreshrate > 0.999 * monp->vrefresh[i].lo) &&
		     (vrefreshrate < 1.001 * monp->vrefresh[i].hi) )
			break; /* In range. */

	/* Now see whether we ran out of refresh rates */
	if ( i == monp->n_vrefresh ) {
	    if (verbose) {
		ErrorF(
		  "%s %s: Mode \"%s\" needs vert refresh rate of %.2f Hz. Deleted.\n",
		  XCONFIG_PROBED, scrname, dispmp->name, vrefreshrate);
	    }
	    return MODE_VSYNC;
	}

	/* Interlaced modes should have an odd VTotal */
	if (dispmp->Flags & V_INTERLACE)
	    dispmp->CrtcVTotal = dispmp->VTotal |= 1;

	/* Passed every test. */
	return MODE_OK;
}

/*
 * Save entire line from config file in memory area, if memory area
 * does not exist allocate it. Set DCerr according to value of token.
 * Return address of memory area.
 */
static char *xf86DCSaveLine(DCPointer,token)
     char *DCPointer;
     int token;
{
  static int len = 0; /* length of memory area where to store strings */
  static int pos = 0; /* current position */
  char *currpointer;  /* pointer to current position in memory area */
  static int currline; /* lineno of line currently interpreted */
  int addlen;         /* len to add to pos */

  if(DCPointer == NULL){   /* initialize */
    DCPointer = (char *)xalloc(4096);  /* initial size 4kB */
    len = 4096;  
    strcpy(DCPointer,configPath);
    pos = strlen(DCPointer) + 1;
    currline = -1;  /* no line yet */
  }

    if(configLineNo != currline)  /* new line */
      {
	currline = configLineNo; 
	addlen = strlen(configBuf) + 1 + sizeof(int); /* string + lineno */
	while ( (pos + addlen) >= len ){  /* not enough space? */
	  DCPointer = (char *)xrealloc(DCPointer, (len + 4096));
	  len += 4096;
	}
	currpointer = DCPointer + pos;  /* find current position */
	memcpy(currpointer, &currline, sizeof(int)); /* Grrr unaligned ints.. */
	strcpy((currpointer + sizeof(int)),configBuf); /* store complete line*/
	pos += addlen;                      /* goto end */
	currpointer += addlen;
	*(currpointer) = EOF;               /* mark end */
      }
  switch(token){
  case STRING:
  case DASH:
  case NUMBER:
  case COMMA:
    break;
  case ERROR_TOKEN:    /* if unknown token unset DCerr to ignore it  */
    DCerr = 0;         /* and subsequent STRING, DASH, NUMBER, COMMA */
    break;
  default:             /* set to complain if a valid token is        */
    DCerr = 1;         /* followed by an unwanted STRING etc.        */
  }
  return(DCPointer);      
}

/* 
 * Store any unknown Option strings (contained in val.str)
 * in a  memory are pointed to by pointer. If it doesn't 
 * exist allocate it and return a pointer pointing to it
 */

static char *
xf86DCOption(DCPointer, val)
     char *DCPointer;
     LexRec val;
{
  static int len = 0;
  static int pos = 0;
  int addlen;
  char *currpointer;                   /* current position */

  if (DCPointer == NULL){              /* First time: initialize */
    DCPointer = (char *)xalloc(4096);       /* allocate enough space  */
    strcpy(DCPointer,configPath);
    pos = strlen(DCPointer) + 1;
    len = 4096;                      /* and total length       */
  } 

  addlen = sizeof(int) + strlen(val.str) + 1;  /* token, lineno */  
  while( (pos + addlen) >= len ){              /* reallocate if not enough */
    DCPointer = (char *)xrealloc(DCPointer, (len + 4096));
    len += 4096;
  }
  currpointer = DCPointer + pos;        
  *(int *)currpointer=configLineNo;
  strcpy(currpointer + sizeof(int),val.str);         /* store string */
  pos += addlen;
  *(currpointer + addlen) = EOF;                     /* mark end     */
  return(DCPointer);
}

static char 
* xf86DCConcatOption(Pointer1, Pointer2)
char *Pointer1;
char *Pointer2;
{
  int s1 = 0;
  int s2 = 0;
  int s3;
  char *ptmp;

  if(Pointer1)
    while(*(Pointer1 + s1) != EOF){s1++;}
  else if (Pointer2)
    return Pointer2;
  else return NULL;
  if(Pointer2)
    while(*(Pointer2 + s2) != EOF){s2++;}
  else if (Pointer1)
    return Pointer1;
  else return NULL;
  s3 = strlen(Pointer2) + 1;
  s2 -= s3;

  Pointer1 = (char *)xrealloc(Pointer1,s1+s2+1); 
  ptmp = Pointer1 + s1;
  Pointer2 += s3;
  do{
    *ptmp = *Pointer2;
    *ptmp++;
    *Pointer2++;
  } while(s2--);
  return Pointer1;
}
    

/* $XFree86: xc/programs/Xserver/hw/xfree86/xf86Version.h,v 3.236.2.50 1998/03/02 09:58:21 dawes Exp $ */

#define XF86_VERSION " 3.3.2 "

/* The finer points in versions... */
#define XF86_VERSION_MAJOR	3
#define XF86_VERSION_MINOR	3
#define XF86_VERSION_SUBMINOR	2
#define XF86_VERSION_BETA	0	/* 0="", 1="A", 2="B", etc... */
#define XF86_VERSION_ALPHA	0	/* 0="", 1="a", 2="b", etc... */

#define XF86_VERSION_NUMERIC(major,minor,subminor,beta,alpha)	\
   ((((((((major << 7) | minor) << 7) | subminor) << 5) | beta) << 5) | alpha)
#define XF86_VERSION_CURRENT					\
   XF86_VERSION_NUMERIC(XF86_VERSION_MAJOR,			\
			XF86_VERSION_MINOR,			\
			XF86_VERSION_SUBMINOR,			\
			XF86_VERSION_BETA,			\
			XF86_VERSION_ALPHA)

#define XF86_DATE	"March 2 1998"

/* $Xorg: xf86Version.h,v 1.3 2000/08/17 19:48:48 cpqbld Exp $ */

/*
 * Wrapper to add missing symbol to externally supplied code
 */

#ifdef Lynx
extern int optind;
extern char *optarg;
#endif

#include "ttf2pt1.c"

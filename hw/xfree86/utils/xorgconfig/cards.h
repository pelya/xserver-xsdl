/* $XFree86: xc/programs/Xserver/hw/xfree86/xf86config/cards.h,v 3.3 1996/12/23 07:04:44 dawes Exp $ */





/* $Xorg: cards.h,v 1.3 2000/08/17 19:53:05 cpqbld Exp $ */

#ifndef CARD_DATABASE_FILE
#define CARD_DATABASE_FILE "Cards"
#endif

#define MAX_CARDS 1000

typedef struct {
	char *name;		/* Name of the card. */
	char *chipset;		/* Chipset (decriptive). */
	char *server;		/* Server identifier. */
	char *ramdac;		/* Ramdac identifier. */
	char *clockchip;	/* Clockchip identifier. */
	char *dacspeed;		/* DAC speed rating. */
	int flags;
	char *lines;		/* Additional Device section lines. */
} Card;

/* Flags: */
#define NOCLOCKPROBE	0x1	/* Never probe clocks of the card. */
#define UNSUPPORTED	0x2	/* Card is not supported (only VGA). */

extern int lastcard;

extern Card card[MAX_CARDS];


int parse_database();


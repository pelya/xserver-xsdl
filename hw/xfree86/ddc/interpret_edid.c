/* $XFree86: xc/programs/Xserver/hw/xfree86/ddc/interpret_edid.c,v 1.8 2000/07/13 21:31:37 tsi Exp $ */

/* interpret_edid.c: interpret a primary EDID block
 * 
 * Copyright 1998 by Egbert Eich <Egbert.Eich@Physik.TU-Darmstadt.DE>
 */
#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"
#define _PARSE_EDID_
#include "xf86DDC.h"

static void get_vendor_section(Uchar*, struct vendor *);
static void get_version_section(Uchar*, struct edid_version *);
static void get_display_section(Uchar*, struct disp_features *);
static void get_established_timing_section(Uchar*, struct established_timings *);
static void get_std_timing_section(Uchar*, struct std_timings *);
static void get_dt_md_section(Uchar *, struct edid_version *,
			      struct detailed_monitor_section *det_mon);
static void copy_string(Uchar *, Uchar *);
static void get_dst_timing_section(Uchar *, struct std_timings *);
static void get_monitor_ranges(Uchar *, struct monitor_ranges *);
static void get_whitepoint_section(Uchar *, struct whitePoints *);
static void get_detailed_timing_section(Uchar*, struct 	detailed_timings *);

xf86MonPtr
xf86InterpretEDID(int scrnIndex, Uchar *block)
{
    xf86MonPtr m;

    if (!block) return NULL;
    if (! (m = xnfcalloc(sizeof(xf86Monitor),1))) return NULL;
    m->scrnIndex = scrnIndex;
    m->rawData = block;
    get_vendor_section(SECTION(VENDOR_SECTION,block),&m->vendor);
    get_version_section(SECTION(VERSION_SECTION,block),&m->ver);
    get_display_section(SECTION(DISPLAY_SECTION,block),&m->features);
    get_established_timing_section(SECTION(ESTABLISHED_TIMING_SECTION,block),
				   &m->timings1);
    get_std_timing_section(SECTION(STD_TIMING_SECTION,block),m->timings2);
    get_dt_md_section(SECTION(DET_TIMING_SECTION,block),&m->ver, m->det_mon);
    m->no_sections = (int)*(char *)SECTION(NO_EDID,block);
    return (m);
}

static void
get_vendor_section(Uchar *c, struct vendor *r)
{
    r->name[0] = L1;
    r->name[1] = L2;
    r->name[2] = L3;
    r->name[3] = '\0';
  
    r->prod_id = PROD_ID;
    r->serial  = SERIAL_NO;
    r->week    = WEEK;
    r->year    = YEAR;
}

static void 
get_version_section(Uchar *c, struct edid_version *r)
{
    r->version  = VERSION;
    r->revision = REVISION;
}

static void 
get_display_section(Uchar *c, struct disp_features *r)
{
    r->input_type = INPUT_TYPE;
    r->input_voltage = INPUT_VOLTAGE;
    r->input_setup = SETUP;
    r->input_sync = SYNC;
    r->hsize = HSIZE_MAX;
    r->vsize = VSIZE_MAX;
    r->gamma = GAMMA;
    r->dpms =  DPMS;
    r->display_type = DISPLAY_TYPE;
    r->msc = MSC;
    r->redx = REDX;
    r->redy = REDY;
    r->greenx = GREENX;
    r->greeny = GREENY;
    r->bluex = BLUEX;
    r->bluey = BLUEY;
    r->whitex = WHITEX;
    r->whitey = WHITEY;
}

static void 
get_established_timing_section(Uchar *c, struct established_timings *r)
{
    r->t1 = T1;
    r->t2 = T2;
    r->t_manu = T_MANU;
}

static void
get_std_timing_section(Uchar *c, struct std_timings *r)
{
    int i;

    for (i=0;i<STD_TIMINGS;i++){
	r[i].hsize = HSIZE1;
	VSIZE1(r[i].vsize);
	r[i].refresh = REFRESH_R;
	r[i].id = STD_TIMING_ID;
	NEXT_STD_TIMING;
    }
}

static void
get_dt_md_section(Uchar *c, struct edid_version *ver, 
		  struct detailed_monitor_section *det_mon)
{
  int i;
 
  for (i=0;i<DET_TIMINGS;i++) {  
    if (ver->version == 1 && ver->revision >= 1 && IS_MONITOR_DESC) {

      switch (MONITOR_DESC_TYPE) {
      case SERIAL_NUMBER:
	det_mon[i].type = DS_SERIAL;
	copy_string(c,det_mon[i].section.serial);
	break;
      case ASCII_STR:
	det_mon[i].type = DS_ASCII_STR;
	copy_string(c,det_mon[i].section.ascii_data);
	break;
      case MONITOR_RANGES:
	det_mon[i].type = DS_RANGES;
	get_monitor_ranges(c,&det_mon[i].section.ranges);
	break;
      case MONITOR_NAME:
	det_mon[i].type = DS_NAME;
	copy_string(c,det_mon[i].section.name);
	break;
      case ADD_COLOR_POINT:
	det_mon[i].type = DS_WHITE_P;
	get_whitepoint_section(c,det_mon[i].section.wp);
	break;
      case ADD_STD_TIMINGS:
	det_mon[i].type = DS_STD_TIMINGS;
	get_dst_timing_section(c,det_mon[i].section.std_t);
	break;
      }
    } else { 
      det_mon[i].type = DT;
      get_detailed_timing_section(c,&det_mon[i].section.d_timings);
    }
    NEXT_DT_MD_SECTION;
  }
}

static void
copy_string(Uchar *c, Uchar *s)
{
  int i;
  c = c + 5;
  for (i = 0; (i < 13 && *c != 0x0A); i++) 
    *(s++) = *(c++);
  *s = 0;
  while (i-- && (*--s == 0x20)) *s = 0;
}

static void
get_dst_timing_section(Uchar *c, struct std_timings *t)
{
  int j;
    c = c + 5;
    for (j = 0; j < 5; j++) {
	t[j].hsize = HSIZE1;
	VSIZE1(t[j].vsize);
	t[j].refresh = REFRESH_R;
	t[j].id = STD_TIMING_ID;
	NEXT_STD_TIMING;
    }
}

static void
get_monitor_ranges(Uchar *c, struct monitor_ranges *r)
{
    r->min_v = MIN_V;
    r->max_v = MAX_V;
    r->min_h = MIN_H;
    r->max_h = MAX_H;
    r->max_clock = MAX_CLOCK * 10;
}

static void
get_whitepoint_section(Uchar *c, struct whitePoints *wp)
{
    wp[1].white_x = WHITEX1;
    wp[1].white_y = WHITEY1;
    wp[2].white_x = WHITEX2;
    wp[2].white_y = WHITEY2;
    wp[1].index  = WHITE_INDEX1;
    wp[2].index  = WHITE_INDEX2;
    wp[1].white_gamma  = WHITE_GAMMA1;
    wp[2].white_gamma  = WHITE_GAMMA2;
}

static void
get_detailed_timing_section(Uchar *c, struct detailed_timings *r)
{
  r->clock = PIXEL_CLOCK;
  r->h_active = H_ACTIVE;
  r->h_blanking = H_BLANK;
  r->v_active = V_ACTIVE;
  r->v_blanking = V_BLANK;
  r->h_sync_off = H_SYNC_OFF;
  r->h_sync_width = H_SYNC_WIDTH;
  r->v_sync_off = V_SYNC_OFF;
  r->v_sync_width = V_SYNC_WIDTH;
  r->h_size = H_SIZE;
  r->v_size = V_SIZE;
  r->h_border = H_BORDER;
  r->v_border = V_BORDER;
  r->interlaced = INTERLACED;
  r->stereo = STEREO;
  r->sync = SYNC_T;
  r->misc = MISC;
}




/* print_edid.c: print out all information retrieved from display device 
 * 
 * Copyright 1998 by Egbert Eich <Egbert.Eich@Physik.TU-Darmstadt.DE>
 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "misc.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86DDC.h"
  
static void print_vendor(int scrnIndex, struct vendor *);
static void print_version(int scrnIndex, struct edid_version *);
static void print_display(int scrnIndex, struct disp_features *,
			  struct edid_version *);
static void print_established_timings(int scrnIndex,
				      struct established_timings *);
static void print_std_timings(int scrnIndex, struct std_timings *);
static void print_detailed_monitor_section(int scrnIndex,
					   struct detailed_monitor_section *);
static void print_detailed_timings(int scrnIndex, struct detailed_timings *);

static void print_input_features(int scrnIndex, struct disp_features *);
static void print_dpms_features(int scrnIndex, struct disp_features *,
				struct edid_version *v);
static void print_whitepoint(int scrnIndex, struct disp_features *);
static void print_number_sections(int scrnIndex, int);

#define EDID_WIDTH	16

xf86MonPtr
xf86PrintEDID(xf86MonPtr m)
{
    CARD16 i, j;
    char buf[EDID_WIDTH * 2 + 1];

    if (!(m)) return NULL;

    print_vendor(m->scrnIndex,&m->vendor);
    print_version(m->scrnIndex,&m->ver);
    print_display(m->scrnIndex,&m->features, &m->ver);
    print_established_timings(m->scrnIndex,&m->timings1);
    print_std_timings(m->scrnIndex,m->timings2);
    print_detailed_monitor_section(m->scrnIndex,m->det_mon);
    print_number_sections(m->scrnIndex,m->no_sections);

    xf86DrvMsg(m->scrnIndex, X_INFO, "EDID (in hex):\n");
 
    for (i = 0; i < 128; i += j) {
	for (j = 0; j < EDID_WIDTH; ++j) {
	    sprintf(&buf[j * 2], "%02x", m->rawData[i + j]);
	}
	xf86DrvMsg(m->scrnIndex, X_INFO, "\t%s\n", buf);
    }
    
    return m;
}
  
static void
print_vendor(int scrnIndex, struct vendor *c)
{
    xf86DrvMsg(scrnIndex, X_INFO, "Manufacturer: %s  Model: %x  Serial#: %u\n",
	(char *)&c->name, c->prod_id, c->serial);
    xf86DrvMsg(scrnIndex, X_INFO, "Year: %u  Week: %u\n", c->year, c->week);
}
  
static void
print_version(int scrnIndex, struct edid_version *c)
{
    xf86DrvMsg(scrnIndex,X_INFO,"EDID Version: %u.%u\n",c->version,
	       c->revision);  
}
  
static void
print_display(int scrnIndex, struct disp_features *disp,
	      struct edid_version *version)
{
    print_input_features(scrnIndex,disp);
    xf86DrvMsg(scrnIndex,X_INFO,"Max H-Image Size [cm]: ");
    if (disp->hsize)
	xf86ErrorF("horiz.: %i  ",disp->hsize);
    else
	xf86ErrorF("H-Size may change,  ");
    if (disp->vsize)
	xf86ErrorF("vert.: %i\n",disp->vsize);
      else
	xf86ErrorF("V-Size may change\n");
    xf86DrvMsg(scrnIndex,X_INFO,"Gamma: %.2f\n", disp->gamma);
    print_dpms_features(scrnIndex,disp,version);
    print_whitepoint(scrnIndex,disp);
}
  
static void 
print_input_features(int scrnIndex, struct disp_features *c)
{
    if (DIGITAL(c->input_type)) {
	xf86DrvMsg(scrnIndex,X_INFO,"Digital Display Input\n");
	if (DFP1(c->input_dfp))
	    xf86DrvMsg(scrnIndex,X_INFO,"DFP 1.x compatible TMDS\n");
    } else {
	xf86DrvMsg(scrnIndex,X_INFO,"Analog Display Input,  ");
	xf86ErrorF("Input Voltage Level: ");
	switch (c->input_voltage){
	case V070:
	    xf86ErrorF("0.700/0.300 V\n");
	    break;
	case V071:
	    xf86ErrorF("0.714/0.286 V\n");
	    break;
	case V100:
	    xf86ErrorF("1.000/0.400 V\n");
	    break;
	case V007:
            xf86ErrorF("0.700/0.700 V\n");
	    break;
	default:
	    xf86ErrorF("undefined\n");
	}
	if (SIG_SETUP(c->input_setup))
	    xf86DrvMsg(scrnIndex,X_INFO,"Signal levels configurable\n");
	xf86DrvMsg(scrnIndex,X_INFO,"Sync:");
	if (SEP_SYNC(c->input_sync))
	    xf86ErrorF("  Separate");
	if (COMP_SYNC(c->input_sync))
	    xf86ErrorF("  Composite");
	if (SYNC_O_GREEN(c->input_sync))
	    xf86ErrorF("  SyncOnGreen");
	if (SYNC_SERR(c->input_sync)) 
	    xf86ErrorF("Serration on. "
		       "V.Sync Pulse req. if CompSync or SyncOnGreen\n");
	else xf86ErrorF("\n");
    }
}
  
static void 
print_dpms_features(int scrnIndex, struct disp_features *c,
		    struct edid_version *v)
{
     if (c->dpms) {
	 xf86DrvMsg(scrnIndex,X_INFO,"DPMS capabilities:");
	 if (DPMS_STANDBY(c->dpms)) xf86ErrorF(" StandBy");
	 if (DPMS_SUSPEND(c->dpms)) xf86ErrorF(" Suspend");
	 if (DPMS_OFF(c->dpms)) xf86ErrorF(" Off");
     } else 
	 xf86DrvMsg(scrnIndex,X_INFO,"No DPMS capabilities specified");
    switch (c->display_type){
    case DISP_MONO:
	xf86ErrorF("; Monochorome/GrayScale Display\n");
	break;
    case DISP_RGB:
	xf86ErrorF("; RGB/Color Display\n");
	break;
    case DISP_MULTCOLOR:
	xf86ErrorF("; Non RGB Multicolor Display\n");
	break;
    default:
	xf86ErrorF("\n");
	break;
    }
    if (STD_COLOR_SPACE(c->msc))
	xf86DrvMsg(scrnIndex,X_INFO,
		   "Default color space is primary color space\n"); 
    if (PREFERRED_TIMING_MODE(c->msc))
	xf86DrvMsg(scrnIndex,X_INFO,
		   "First detailed timing is preferred mode\n"); 
    else if (v->version == 1 && v->revision >= 3)
	xf86DrvMsg(scrnIndex,X_INFO,
		   "First detailed timing not preferred "
		   "mode in violation of standard!");
    if (GFT_SUPPORTED(c->msc))
	xf86DrvMsg(scrnIndex,X_INFO,
		   "GTF timings supported\n"); 
}
  
static void 
print_whitepoint(int scrnIndex, struct disp_features *disp)
{
    xf86DrvMsg(scrnIndex,X_INFO,"redX: %.3f redY: %.3f   ",
	       disp->redx,disp->redy);
    xf86ErrorF("greenX: %.3f greenY: %.3f\n",
	       disp->greenx,disp->greeny);
    xf86DrvMsg(scrnIndex,X_INFO,"blueX: %.3f blueY: %.3f   ",
	       disp->bluex,disp->bluey);
    xf86ErrorF("whiteX: %.3f whiteY: %.3f\n",
	       disp->whitex,disp->whitey);
}
  
static void 
print_established_timings(int scrnIndex, struct established_timings *t)
{
    unsigned char c;

    if (t->t1 || t->t2 || t->t_manu)
	xf86DrvMsg(scrnIndex,X_INFO,"Supported VESA Video Modes:\n");
    c=t->t1;
    if (c&0x80) xf86DrvMsg(scrnIndex,X_INFO,"720x400@70Hz\n");
    if (c&0x40) xf86DrvMsg(scrnIndex,X_INFO,"720x400@88Hz\n");
    if (c&0x20) xf86DrvMsg(scrnIndex,X_INFO,"640x480@60Hz\n");
    if (c&0x10) xf86DrvMsg(scrnIndex,X_INFO,"640x480@67Hz\n");
    if (c&0x08) xf86DrvMsg(scrnIndex,X_INFO,"640x480@72Hz\n");
    if (c&0x04) xf86DrvMsg(scrnIndex,X_INFO,"640x480@75Hz\n");
    if (c&0x02) xf86DrvMsg(scrnIndex,X_INFO,"800x600@56Hz\n");
    if (c&0x01) xf86DrvMsg(scrnIndex,X_INFO,"800x600@60Hz\n");
    c=t->t2;
    if (c&0x80) xf86DrvMsg(scrnIndex,X_INFO,"800x600@72Hz\n");
    if (c&0x40) xf86DrvMsg(scrnIndex,X_INFO,"800x600@75Hz\n");
    if (c&0x20) xf86DrvMsg(scrnIndex,X_INFO,"832x624@75Hz\n");
    if (c&0x10) xf86DrvMsg(scrnIndex,X_INFO,"1024x768@87Hz (interlaced)\n");
    if (c&0x08) xf86DrvMsg(scrnIndex,X_INFO,"1024x768@60Hz\n");
    if (c&0x04) xf86DrvMsg(scrnIndex,X_INFO,"1024x768@70Hz\n");
    if (c&0x02) xf86DrvMsg(scrnIndex,X_INFO,"1024x768@75Hz\n");
    if (c&0x01) xf86DrvMsg(scrnIndex,X_INFO,"1280x1024@75Hz\n");
    c=t->t_manu;
    if (c&0x80) xf86DrvMsg(scrnIndex,X_INFO,"1152x870@75Hz\n");
    xf86DrvMsg(scrnIndex,X_INFO,"Manufacturer's mask: %X\n",c&0x7F);
}
  
static void
print_std_timings(int scrnIndex, struct std_timings *t)
{
    int i;
    char done = 0;
    for (i=0;i<STD_TIMINGS;i++) {
	if (t[i].hsize > 256) {  /* sanity check */
	    if (!done) {
		xf86DrvMsg(scrnIndex,X_INFO,"Supported Future Video Modes:\n");
		done = 1;
	    }
	    xf86DrvMsg(scrnIndex,X_INFO,
		       "#%i: hsize: %i  vsize %i  refresh: %i  vid: %i\n",
		       i, t[i].hsize, t[i].vsize, t[i].refresh, t[i].id);
	}
    }
}
  
static void
print_detailed_monitor_section(int scrnIndex,
			       struct detailed_monitor_section *m)
{
    int i,j;
  
    for (i=0;i<DET_TIMINGS;i++) {
	switch (m[i].type) {
	case DT:
	    print_detailed_timings(scrnIndex,&m[i].section.d_timings);
	    break;
	case DS_SERIAL:
	    xf86DrvMsg(scrnIndex,X_INFO,"Serial No: %s\n",m[i].section.serial);
	    break;
	case DS_ASCII_STR:
	    xf86DrvMsg(scrnIndex,X_INFO," %s\n",m[i].section.ascii_data);
	    break;
	case DS_NAME:
	    xf86DrvMsg(scrnIndex,X_INFO,"Monitor name: %s\n",m[i].section.name);
	    break;
	case DS_RANGES:
	    xf86DrvMsg(scrnIndex,X_INFO,
		       "Ranges: V min: %i  V max: %i Hz, H min: %i  H max: %i kHz,",
		       m[i].section.ranges.min_v, m[i].section.ranges.max_v, 
		       m[i].section.ranges.min_h, m[i].section.ranges.max_h);
	    if (m[i].section.ranges.max_clock != 0)
		xf86ErrorF(" PixClock max %i MHz\n",m[i].section.ranges.max_clock);
	    else
		xf86ErrorF("\n");
	    if (m[i].section.ranges.gtf_2nd_f > 0)
		xf86DrvMsg(scrnIndex,X_INFO," 2nd GTF parameters: f: %i kHz "
			   "c: %i m: %i k %i j %i\n",
			   m[i].section.ranges.gtf_2nd_f,
			   m[i].section.ranges.gtf_2nd_c,
			   m[i].section.ranges.gtf_2nd_m,
			   m[i].section.ranges.gtf_2nd_k,
			   m[i].section.ranges.gtf_2nd_j);
	    break;
	case DS_STD_TIMINGS:
	    for (j = 0; j<5; j++) 
		xf86DrvMsg(scrnIndex,X_INFO,"#%i: hsize: %i  vsize %i  refresh: %i  "
			   "vid: %i\n",i,m[i].section.std_t[i].hsize,
			   m[i].section.std_t[j].vsize,m[i].section.std_t[j].refresh,
			   m[i].section.std_t[j].id);
	    break;
	case DS_WHITE_P:
	    for (j = 0; j<2; j++)
		if (m[i].section.wp[j].index != 0)
		    xf86DrvMsg(scrnIndex,X_INFO,
			       "White point %i: whiteX: %f, whiteY: %f; gamma: %f\n",
			       m[i].section.wp[j].index,m[i].section.wp[j].white_x,
			       m[i].section.wp[j].white_y,
			       m[i].section.wp[j].white_gamma);
	    break;
	case DS_DUMMY:
	default:
	    break;
	}
    }
}
  
static void
print_detailed_timings(int scrnIndex, struct detailed_timings *t)
{

    if (t->clock > 15000000) {  /* sanity check */
	xf86DrvMsg(scrnIndex,X_INFO,"Supported additional Video Mode:\n");
	xf86DrvMsg(scrnIndex,X_INFO,"clock: %.1f MHz   ",t->clock/1000000.0);
	xf86ErrorF("Image Size:  %i x %i mm\n",t->h_size,t->v_size); 
	xf86DrvMsg(scrnIndex,X_INFO,
		   "h_active: %i  h_sync: %i  h_sync_end %i h_blank_end %i ",
		   t->h_active, t->h_sync_off + t->h_active,
		   t->h_sync_off + t->h_sync_width + t->h_active,
		   t->h_active + t->h_blanking);
	xf86ErrorF("h_border: %i\n",t->h_border);
	xf86DrvMsg(scrnIndex,X_INFO,
		   "v_active: %i  v_sync: %i  v_sync_end %i v_blanking: %i ",
		   t->v_active, t->v_sync_off + t->v_active,
		   t->v_sync_off + t->v_sync_width + t->v_active,
		   t->v_active + t->v_blanking);
	xf86ErrorF("v_border: %i\n",t->v_border);
	if (IS_STEREO(t->stereo)) {
	    xf86DrvMsg(scrnIndex,X_INFO,"Stereo: ");
	    if (IS_RIGHT_STEREO(t->stereo)) {
		if (!t->stereo_1)
		    xf86ErrorF("right channel on sync\n");
		else
		    xf86ErrorF("left channel on sync\n");
	    } else if (IS_LEFT_STEREO(t->stereo)) {
		if (!t->stereo_1)
		    xf86ErrorF("right channel on even line\n");
		else 
		    xf86ErrorF("left channel on evel line\n");
	    }
	    if (IS_4WAY_STEREO(t->stereo)) {
		if (!t->stereo_1)
		    xf86ErrorF("4-way interleaved\n");
		else
		    xf86ErrorF("side-by-side interleaved");
	    }
	}
    }
}

static void
print_number_sections(int scrnIndex, int num)
{
    if (num)
	xf86DrvMsg(scrnIndex,X_INFO,"Number of EDID sections to follow: %i\n",
		   num);
}


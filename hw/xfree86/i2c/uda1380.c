/*************************************************************************************
 * $Id$
 * 
 * Created by Bogdan D. bogdand@users.sourceforge.net 
 * License: GPL
 *
 * $Log$
 * Revision 1.1  2005/04/17 22:58:03  bogdand
 * This is the UDA1380 sound coder-decoder module
 *
 *
 ************************************************************************************/

#include "xf86.h"
#include "xf86i2c.h"
#include "uda1380.h"
#include "i2c_def.h"

UDA1380Ptr Detect_uda1380(I2CBusPtr b, I2CSlaveAddr addr)
{
	UDA1380Ptr t;
	I2CByte a;
  
	t = xcalloc(1, sizeof(UDA1380Rec));
	if(t == NULL) return NULL;
	switch(addr)
	{
		case UDA1380_ADDR_1:
		case UDA1380_ADDR_2:
			t->d.DevName = "UDA1380 Stereo audion coder-decoder";
			break;
		default:
			t->d.DevName = "Generic UDAxxxx";
		break;
	}
	t->d.SlaveAddr = addr;
	t->d.pI2CBus = b;
	t->d.NextDev = NULL;
	t->d.StartTimeout = b->StartTimeout;
	t->d.BitTimeout = b->BitTimeout;
	t->d.AcknTimeout = b->AcknTimeout;
	t->d.ByteTimeout = b->ByteTimeout;
  
	if(!I2C_WriteRead(&(t->d), NULL, 0, &a, 1))
	{
		xfree(t);
		return NULL;
	}
  
	/* set default parameters */
	if(!I2CDevInit(&(t->d)))
	{
		xfree(t);
		return NULL;
	}
  
	xf86DrvMsg(t->d.pI2CBus->scrnIndex,X_INFO,"UDA1380 stereo coder-decoder detected\n");

	return t;  
}

Bool uda1380_init(UDA1380Ptr t)
{
	CARD8 data[3];
	CARD16 tmp;
	Bool ret;

	/* Power control */
	data[0] = 0x02;
	tmp = (1 << 13) | (1 << 10) | ( 1 << 8) | (1 << 7) | (1 << 6) | (1 << 3) | (1 << 1);
	data[1] = (CARD8)((tmp >> 8) & 0xff);
	data[2] = (CARD8)(tmp & 0xff);
	ret = I2C_WriteRead(&(t->d), data, 3, NULL, 0);
	if (ret == FALSE)
	{
		xf86DrvMsg(t->d.pI2CBus->scrnIndex,X_INFO,"UDA1380 failed to initialize\n");
		return FALSE;
	}

	/* Analog mixer  (AVC) */
	data[0] = 0x03;
	/* the analog mixer is muted initially */
	data[1] = 0x3f;
	data[2] = 0x3f;
	ret = I2C_WriteRead(&(t->d), data, 3, NULL, 0);
	if (ret == FALSE)
	{
		xf86DrvMsg(t->d.pI2CBus->scrnIndex,X_INFO,"UDA1380 failed to initialize\n");
		return FALSE;
	}
		  
	xf86DrvMsg(t->d.pI2CBus->scrnIndex,X_INFO,"UDA1380 initialized\n");

	return TRUE;
}

void uda1380_shutdown(UDA1380Ptr t)
{
	CARD8 data[3];
	Bool ret;

	/* Power control */
	data[0] = 0x02;
	data[1] = 0;
	data[2] = 0;
	ret = I2C_WriteRead(&(t->d), data, 3, NULL, 0);
	if (ret == FALSE)
		xf86DrvMsg(t->d.pI2CBus->scrnIndex,X_INFO,"UDA1380 failed to shutdown\n");
}

void uda1380_setvolume(UDA1380Ptr t, INT32 value)
{
	CARD8 data[3];
	/*
	 * We have to scale the value ranging from -1000 to 1000 to 0x2c to 0
	 */
	CARD8 volume = 47 - (CARD8)((value + 1000) * 47 / 2000);
	Bool ret;
		  
	t->analog_mixer_settings = ((volume << 8) & 0x3f00) | (volume & 0x3f);
					 
	/* Analog mixer  (AVC) */
	data[0] = 0x03;
	data[1] = volume & 0x3f;
	data[2] = volume & 0x3f;
	ret = I2C_WriteRead(&(t->d), data, 3, NULL, 0);
	if (ret == FALSE)
		xf86DrvMsg(t->d.pI2CBus->scrnIndex,X_INFO,"UDA1380 failed to set volume\n");
}

void uda1380_mute(UDA1380Ptr t, Bool mute)
{
	CARD8 data[3];
	Bool ret;
		  
	if (mute == TRUE)
	{
		/* Analog mixer  (AVC) */
		data[0] = 0x03;
		data[1] = 0xff;
		data[2] = 0xff;
		ret = I2C_WriteRead(&(t->d), data, 3, NULL, 0);
		if (ret == FALSE)
			xf86DrvMsg(t->d.pI2CBus->scrnIndex,X_INFO,"UDA1380 failed to mute\n");
	}
	else
	{
		/* Analog mixer  (AVC) */
		data[0] = 0x03;
		data[1] = (CARD8)((t->analog_mixer_settings >> 8) & 0x3f);
		data[2] = (CARD8)(t->analog_mixer_settings & 0x3f);
		ret = I2C_WriteRead(&(t->d), data, 3, NULL, 0);
		if (ret == FALSE)
			xf86DrvMsg(t->d.pI2CBus->scrnIndex,X_INFO,"UDA1380 failed to unmute\n");
	}
}

void uda1380_getstatus(UDA1380Ptr t)
{
}

void uda1380_setparameters(UDA1380Ptr t)
{
}

void uda1380_dumpstatus(UDA1380Ptr t)
{
}

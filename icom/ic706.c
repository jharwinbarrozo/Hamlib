/*
 * hamlib - (C) Frank Singleton 2000 (vk3fcs@ix.netcom.com)
 *
 * ic706.c - Copyright (C) 2000 Stephane Fillod
 * This shared library provides an API for communicating
 * via serial interface to an IC-706,IC-706MKII,IC706-MKIIG
 * using the "CI-V" interface.
 *
 *
 * $Id: ic706.c,v 1.14 2001-02-27 23:03:45 f4cfe Exp $  
 *
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */

#include <stdlib.h>
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <sys/ioctl.h>

#include <hamlib/rig.h>
#include <hamlib/riglist.h>
#include "icom.h"


#define IC706_ALL_RX_MODES (RIG_MODE_AM|RIG_MODE_CW|RIG_MODE_USB|RIG_MODE_LSB|RIG_MODE_RTTY|RIG_MODE_FM)
#define IC706_1MHZ_TS_MODES (RIG_MODE_AM|RIG_MODE_FM)
#define IC706_1HZ_TS_MODES (RIG_MODE_CW|RIG_MODE_USB|RIG_MODE_LSB|RIG_MODE_RTTY)

/* tx doesn't have WFM.
 * 100W in all modes but AM (40W)
 */ 
#define IC706_OTHER_TX_MODES (RIG_MODE_AM|RIG_MODE_CW|RIG_MODE_USB|RIG_MODE_LSB|RIG_MODE_RTTY|RIG_MODE_FM)
#define IC706_AM_TX_MODES (RIG_MODE_AM)

#define IC706_FUNC_ALL (RIG_FUNC_FAGC|RIG_FUNC_NB|RIG_FUNC_COMP|RIG_FUNC_VOX|RIG_FUNC_TONE|RIG_FUNC_TSQL|RIG_FUNC_SBKIN|RIG_FUNC_FBKIN)

#define IC706_LEVEL_ALL (RIG_LEVEL_PREAMP|RIG_LEVEL_ATT|RIG_LEVEL_AGC|RIG_LEVEL_SQL|RIG_LEVEL_SQLSTAT|RIG_LEVEL_STRENGTH)

/*
 * ic706 rigs capabilities.
 * Notice that some rigs share the same functions.
 * Also this struct is READONLY!
 */
const struct rig_caps ic706_caps = {
  RIG_MODEL_IC706, "IC-706", "Icom", "0.2", "GPL",
  RIG_STATUS_UNTESTED, RIG_TYPE_MOBILE,
  RIG_PTT_NONE, RIG_DCD_NONE, RIG_PORT_SERIAL,
  300, 19200, 8, 1, RIG_PARITY_NONE, RIG_HANDSHAKE_NONE, 
  0, 0, 200, 3, 
  RIG_FUNC_NONE, IC706_FUNC_ALL, RIG_FUNC_NONE, IC706_LEVEL_ALL,
  { 20, RIG_DBLST_END, },
  { 20, RIG_DBLST_END, },
  0,			/* TBC: RIT controllable? */
  0,			/* FIXME: VFO list */
  0, RIG_TRN_ON, 
  101, 0, 0,

  { RIG_FRNG_END, },	/* FIXME: enter region 1 setting */
  { RIG_FRNG_END, },
  { {kHz(30),199999999,IC706_ALL_RX_MODES,-1,-1},RIG_FRNG_END, }, /* rx range */
  { {kHz(1800),1999999,IC706_OTHER_TX_MODES,5000,100000},	/* 100W class */
    {kHz(1800),1999999,IC706_AM_TX_MODES,2000,40000},	/* 40W class */
    {kHz(3500),3999999,IC706_OTHER_TX_MODES,5000,100000},
    {kHz(3500),3999999,IC706_AM_TX_MODES,2000,40000},
    {MHz(7),kHz(7300),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(7),kHz(7300),IC706_AM_TX_MODES,2000,40000},
    {kHz(10100),kHz(10150),IC706_OTHER_TX_MODES,5000,100000},
    {kHz(10100),kHz(10150),IC706_AM_TX_MODES,2000,40000},
    {MHz(14),kHz(14350),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(14),kHz(14350),IC706_AM_TX_MODES,2000,40000},
    {kHz(18068),kHz(18168),IC706_OTHER_TX_MODES,5000,100000},
    {kHz(18068),kHz(18168),IC706_AM_TX_MODES,2000,40000},
    {MHz(21),kHz(21450),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(21),kHz(21450),IC706_AM_TX_MODES,2000,40000},
    {kHz(24890),kHz(24990),IC706_OTHER_TX_MODES,5000,100000},
    {kHz(24890),kHz(24990),IC706_AM_TX_MODES,2000,40000},
    {MHz(28),kHz(29700),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(28),kHz(29700),IC706_AM_TX_MODES,2000,40000},
    {MHz(50),MHz(54),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(50),MHz(54),IC706_AM_TX_MODES,2000,40000},
    {MHz(144),MHz(148),IC706_OTHER_TX_MODES,5000,20000}, /* not sure.. */
    {MHz(144),MHz(148),IC706_AM_TX_MODES,2000,8000}, /* anyone? */
	RIG_FRNG_END, },

	{{IC706_1HZ_TS_MODES,1},
	 {IC706_ALL_RX_MODES,10},
	 {IC706_ALL_RX_MODES,100},
	 {IC706_ALL_RX_MODES,kHz(1)},
	 {IC706_ALL_RX_MODES,kHz(5)},
	 {IC706_ALL_RX_MODES,kHz(9)},
	 {IC706_ALL_RX_MODES,kHz(10)},
	 {IC706_ALL_RX_MODES,12500},
	 {IC706_ALL_RX_MODES,kHz(20)},
	 {IC706_ALL_RX_MODES,kHz(25)},
	 {IC706_ALL_RX_MODES,kHz(100)},
	 {IC706_1MHZ_TS_MODES,MHz(1)},
	 RIG_TS_END,
	},

	/* mode/filter list, remember: order matters! */
	{
		{RIG_MODE_SSB|RIG_MODE_CW|RIG_MODE_RTTY, kHz(2.4)},	/* bultin FL-272 */
		{RIG_MODE_AM, kHz(8)},		/* mid w/ bultin FL-94 */
		{RIG_MODE_AM, kHz(2.4)},	/* narrow w/ bultin FL-272 */
		{RIG_MODE_FM, kHz(15)},		/* ?? TBC, mid w/ bultin FL-23+SFPC455E */
		{RIG_MODE_FM, kHz(8)},		/* narrow w/ bultin FL-94 */
		{RIG_MODE_WFM, kHz(230)},	/* WideFM, filter FL?? */
		RIG_FLT_END,
	},
  NULL,	/* priv */
  icom_init, icom_cleanup, NULL, NULL, NULL /* probe not supported yet */,
  icom_set_freq, icom_get_freq, icom_set_mode, icom_get_mode, icom_set_vfo,
  NULL,
};

const struct rig_caps ic706mkii_caps = {
  RIG_MODEL_IC706MKII, "IC-706MKII", "Icom", "0.2", "GPL",
  RIG_STATUS_UNTESTED, RIG_TYPE_MOBILE, 
  RIG_PTT_NONE, RIG_DCD_NONE, RIG_PORT_SERIAL,
  300, 19200, 8, 1, RIG_PARITY_NONE, RIG_HANDSHAKE_NONE, 
  0, 0, 200, 3, 
  RIG_FUNC_NONE, IC706_FUNC_ALL, RIG_FUNC_NONE, IC706_LEVEL_ALL,
  { 20, RIG_DBLST_END, },
  { 20, RIG_DBLST_END, },
  0,			/* TBC: RIT controllable? */
  0,			/* FIXME: VFO list */
  0, RIG_TRN_ON, 
  101, 0, 0,

  { RIG_FRNG_END, },	/* FIXME: enter region 1 setting */
  { RIG_FRNG_END, },
  { {kHz(30),199999999,IC706_ALL_RX_MODES,-1,-1},RIG_FRNG_END, }, /* rx range */
  { {kHz(1800),1999999,IC706_OTHER_TX_MODES,5000,100000},	/* 100W class */
    {kHz(1800),1999999,IC706_AM_TX_MODES,2000,40000},	/* 40W class */
    {kHz(3500),3999999,IC706_OTHER_TX_MODES,5000,100000},
    {kHz(3500),3999999,IC706_AM_TX_MODES,2000,40000},
    {MHz(7),kHz(7300),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(7),kHz(7300),IC706_AM_TX_MODES,2000,40000},
    {kHz(10100),kHz(10150),IC706_OTHER_TX_MODES,5000,100000},
    {kHz(10100),kHz(10150),IC706_AM_TX_MODES,2000,40000},
    {MHz(14),kHz(14350),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(14),kHz(14350),IC706_AM_TX_MODES,2000,40000},
    {kHz(18068),kHz(18168),IC706_OTHER_TX_MODES,5000,100000},
    {kHz(18068),kHz(18168),IC706_AM_TX_MODES,2000,40000},
    {MHz(21),kHz(21450),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(21),kHz(21450),IC706_AM_TX_MODES,2000,40000},
    {kHz(24890),kHz(24990),IC706_OTHER_TX_MODES,5000,100000},
    {kHz(24890),kHz(24990),IC706_AM_TX_MODES,2000,40000},
    {MHz(28),kHz(29700),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(28),kHz(29700),IC706_AM_TX_MODES,2000,40000},
    {MHz(50),MHz(54),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(50),MHz(54),IC706_AM_TX_MODES,2000,40000},
    {MHz(144),MHz(148),IC706_OTHER_TX_MODES,5000,20000}, /* not sure.. */
    {MHz(144),MHz(148),IC706_AM_TX_MODES,2000,8000}, /* anyone? */
	RIG_FRNG_END, },

	{{IC706_1HZ_TS_MODES,1},
	 {IC706_ALL_RX_MODES,10},
	 {IC706_ALL_RX_MODES,100},
	 {IC706_ALL_RX_MODES,kHz(1)},
	 {IC706_ALL_RX_MODES,kHz(5)},
	 {IC706_ALL_RX_MODES,kHz(9)},
	 {IC706_ALL_RX_MODES,kHz(10)},
	 {IC706_ALL_RX_MODES,12500},
	 {IC706_ALL_RX_MODES,kHz(20)},
	 {IC706_ALL_RX_MODES,kHz(25)},
	 {IC706_ALL_RX_MODES,kHz(100)},
	 {IC706_1MHZ_TS_MODES,MHz(1)},
	 RIG_TS_END,
	},

	/* mode/filter list, remember: order matters! */
	{
		{RIG_MODE_SSB|RIG_MODE_CW|RIG_MODE_RTTY, kHz(2.4)},	/* bultin FL-272 */
		{RIG_MODE_AM, kHz(8)},		/* mid w/ bultin FL-94 */
		{RIG_MODE_AM, kHz(2.4)},	/* narrow w/ bultin FL-272 */
		{RIG_MODE_FM, kHz(15)},		/* ?? TBC, mid w/ bultin FL-23+SFPC455E */
		{RIG_MODE_FM, kHz(8)},		/* narrow w/ bultin FL-94 */
		{RIG_MODE_WFM, kHz(230)},	/* WideFM, filter FL?? */
		RIG_FLT_END,
	},
  NULL,	/* priv */
  icom_init, icom_cleanup, NULL, NULL, NULL /* probe not supported yet */,
  icom_set_freq, icom_get_freq, icom_set_mode, icom_get_mode, icom_set_vfo,
  NULL,
};

/*
 * Basically, the IC706MKIIG is an IC706MKII plus UHF, a DSP
 * and 50W VHF
 */
const struct rig_caps ic706mkiig_caps = {
  RIG_MODEL_IC706MKIIG, "IC-706MKIIG", "Icom", "0.2", "GPL", 
  RIG_STATUS_ALPHA, RIG_TYPE_MOBILE, 
  RIG_PTT_NONE, RIG_DCD_RIG, RIG_PORT_SERIAL,
  300, 19200, 8, 1, RIG_PARITY_NONE, RIG_HANDSHAKE_NONE, 
  0, 0, 200, 3, 
  RIG_FUNC_NONE, IC706_FUNC_ALL|RIG_FUNC_NR|RIG_FUNC_ANF, IC706_LEVEL_ALL, IC706_LEVEL_ALL,
  { 20, RIG_DBLST_END, },
  { 20, RIG_DBLST_END, },
  9999,			/* RIT */
  0,			/* FIXME: VFO list */
  0, RIG_TRN_ON, 
  101, 0, 0,

  { RIG_FRNG_END, },	/* FIXME: enter region 1 setting */
  { RIG_FRNG_END, },
  { {kHz(30),MHz(200)-1,IC706_ALL_RX_MODES,-1,-1},	/* this trx also has UHF */
 	{MHz(400),MHz(470),IC706_ALL_RX_MODES,-1,-1},
	RIG_FRNG_END, },
  { {kHz(1800),MHz(2)-1,IC706_OTHER_TX_MODES,5000,100000},	/* 100W class */
    {kHz(1800),MHz(2)-1,IC706_AM_TX_MODES,2000,40000},	/* 40W class */
    {kHz(3500),MHz(4)-1,IC706_OTHER_TX_MODES,5000,100000},
    {kHz(3500),MHz(4)-1,IC706_AM_TX_MODES,2000,40000},
	{MHz(7),kHz(7300),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(7),kHz(7300),IC706_AM_TX_MODES,2000,40000},
    {kHz(10100),kHz(10150),IC706_OTHER_TX_MODES,5000,100000},
    {kHz(10100),kHz(10150),IC706_AM_TX_MODES,2000,40000},
    {MHz(14),kHz(14350),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(14),kHz(14350),IC706_AM_TX_MODES,2000,40000},
    {kHz(18068),kHz(18168),IC706_OTHER_TX_MODES,5000,100000},
    {kHz(18068),kHz(18168),IC706_AM_TX_MODES,2000,40000},
    {MHz(21),kHz(21450),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(21),kHz(21450),IC706_AM_TX_MODES,2000,40000},
    {kHz(24890),kHz(24990),IC706_OTHER_TX_MODES,5000,100000},
    {kHz(24890),kHz(24990),IC706_AM_TX_MODES,2000,40000},
    {MHz(28),kHz(29700),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(28),kHz(29700),IC706_AM_TX_MODES,2000,40000},
    {MHz(50),MHz(54),IC706_OTHER_TX_MODES,5000,100000},
    {MHz(50),MHz(54),IC706_AM_TX_MODES,2000,40000},
    {MHz(144),MHz(148),IC706_OTHER_TX_MODES,5000,50000}, /* 50W */
    {MHz(144),MHz(148),IC706_AM_TX_MODES,2000,20000}, /* AM VHF is 20W */
    {MHz(430),MHz(450),IC706_OTHER_TX_MODES,5000,20000},
    {MHz(430),MHz(450),IC706_AM_TX_MODES,2000,8000},
	RIG_FRNG_END, },
	{{IC706_1HZ_TS_MODES,1},
	 {IC706_ALL_RX_MODES,10},
	 {IC706_ALL_RX_MODES,100},
	 {IC706_ALL_RX_MODES,kHz(1)},
	 {IC706_ALL_RX_MODES,kHz(5)},
	 {IC706_ALL_RX_MODES,kHz(9)},
	 {IC706_ALL_RX_MODES,kHz(10)},
	 {IC706_ALL_RX_MODES,12500},
	 {IC706_ALL_RX_MODES,kHz(20)},
	 {IC706_ALL_RX_MODES,kHz(25)},
	 {IC706_ALL_RX_MODES,kHz(100)},
	 {IC706_1MHZ_TS_MODES,MHz(1)},
	 RIG_TS_END,
	},
	/* mode/filter list, remember: order matters! */
	{
		{RIG_MODE_SSB|RIG_MODE_CW|RIG_MODE_RTTY, kHz(2.4)},	/* bultin FL-272 */
		{RIG_MODE_AM, kHz(8)},		/* mid w/ bultin FL-94 */
		{RIG_MODE_AM, kHz(2.4)},	/* narrow w/ bultin FL-272 */
		{RIG_MODE_FM, kHz(15)},		/* ?? TBC, mid w/ bultin FL-23+SFPC455E */
		{RIG_MODE_FM, kHz(8)},		/* narrow w/ bultin FL-94 */
		{RIG_MODE_WFM, kHz(230)},	/* WideFM, filter FL?? */
		RIG_FLT_END,
	},
  NULL,	/* priv */
  icom_init, icom_cleanup, NULL, NULL, NULL /* probe not supported yet */,
  icom_set_freq, icom_get_freq, icom_set_mode, icom_get_mode, icom_set_vfo,
  NULL, 
  /*
   * FIXME:
   * the use of the following GNU extension (field: value)
   * is bad manner in portable code but admit it, quite handy
   * when testing stuff. --SF
   */
decode_event: icom_decode_event,
set_level: icom_set_level,
get_level: icom_get_level,
set_func: icom_set_func,
set_channel: icom_set_channel,
get_channel: icom_get_channel,
set_mem: icom_set_mem,
mv_ctl: icom_mv_ctl,
set_ptt: icom_set_ptt,
get_ptt: icom_get_ptt,
get_dcd: icom_get_dcd,
set_ts: icom_set_ts,
get_ts: icom_get_ts,
set_rptr_shift: icom_set_rptr_shift,
get_rptr_shift: icom_get_rptr_shift,
set_rptr_offs: icom_set_rptr_offs,
get_rptr_offs: icom_get_rptr_offs,
set_split_freq: icom_set_split_freq,
get_split_freq: icom_get_split_freq,
set_split: icom_set_split,
get_split: icom_get_split,
};


/*
 * Function definitions below
 */

static const int mkiig_raw[STR_CAL_LENGTH] = {
100, 104, 108, 111, 114, 118, 121, 125, 129, 133, 137, 142, 146, 151, 156, 161
};
static const int mkiig_db[STR_CAL_LENGTH] = {
-18, -16, -14, -12, -10,  -8,  -6,  -4,  -2,   0,   2,   4,   6,   8,  10,  12
};

/*
 * called by icom_init
 * assume rig!=NULL, rig->state.priv!=NULL
 */
int ic706mkiig_str_cal_init(RIG *rig)
{
	int i;
	struct icom_priv_data *p = (struct icom_priv_data *)rig->state.priv;

	/*
	 * initialize the S Meter calibration table
	 */
	for (i=0; i<STR_CAL_LENGTH; i++) {
			p->str_cal_raw[i] = mkiig_raw[i];
			p->str_cal_db[i] = mkiig_db[i];
	}
	return RIG_OK;
}


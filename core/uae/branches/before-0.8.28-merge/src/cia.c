 /*
  * UAE - The Un*x Amiga Emulator
  *
  * CIA chip support
  *
  * Copyright 1995 Bernd Schmidt, Alessandro Bissacco
  * Copyright 1996, 1997 Stefan Reinauer, Christian Schmitt
  */


#include "sysconfig.h"
#include "sysdeps.h"
#include <assert.h>

#include "config.h"
#include "options.h"
#include "threaddep/thread.h"
#include "events.h"
#include "memory.h"
#include "custom.h"
#include "cia.h"
#include "serial.h"
#include "disk.h"
#include "xwin.h"
#include "keybuf.h"
#include "gui.h"
#include "savestate.h"
#include "inputdevice.h"
#include "zfile.h"
#include "ar.h"
#ifdef CD32
#include "akiko.h"
#endif
#include "debug.h"

//#define CIA_DEBUG
//#define DONGLE_DEBUG

#define TOD_HACK

#define DIV10 (10 * CYCLE_UNIT / 2) /* Yes, a bad identifier. */

/* battclock stuff */
#define RTC_D_ADJ      8
#define RTC_D_IRQ      4
#define RTC_D_BUSY     2
#define RTC_D_HOLD     1
#define RTC_E_t1       8
#define RTC_E_t0       4
#define RTC_E_INTR     2
#define RTC_E_MASK     1
#define RTC_F_TEST     8
#define RTC_F_24_12    4
#define RTC_F_STOP     2
#define RTC_F_RSET     1

static unsigned int clock_control_d = RTC_D_ADJ + RTC_D_HOLD;
static unsigned int clock_control_e = 0;
static unsigned int clock_control_f = RTC_F_24_12;

static unsigned int ciaaicr, ciaaimask, ciabicr, ciabimask;
static unsigned int ciaacra, ciaacrb, ciabcra, ciabcrb;

/* Values of the CIA timers.  */
static unsigned long ciaata, ciaatb, ciabta, ciabtb;
/* Computed by compute_passed_time.  */
static unsigned long ciaata_passed, ciaatb_passed, ciabta_passed, ciabtb_passed;

static unsigned long ciaatod, ciabtod, ciaatol, ciabtol, ciaaalarm, ciabalarm;
static int ciaatlatch, ciabtlatch;
static int oldled, oldovl;

unsigned int ciabpra;

unsigned int gui_ledstate;

static unsigned long ciaala, ciaalb, ciabla, ciablb;
static int ciaatodon, ciabtodon;
static unsigned int ciaapra, ciaaprb, ciaadra, ciaadrb, ciaasdr, ciaasdr_cnt;
static unsigned int ciabprb, ciabdra, ciabdrb, ciabsdr, ciabsdr_cnt;
static int div10;
static int kbstate, kback, ciaasdr_unread;
#ifdef TOD_HACK
static int tod_hack, tod_hack_delay;
#endif

static uae_u8 serbits;

static void setclr (unsigned int *p, unsigned int val)
{
    if (val & 0x80) {
	*p |= val & 0x7F;
    } else {
	*p &= ~val;
    }
}

#include "newcpu.h"

static void RethinkICRA (void)
{
    if (ciaaimask & ciaaicr) {
	ciaaicr |= 0x80;
	INTREQ_0 (0x8000 | 0x0008);
    } else {
	ciaaicr &= 0x7F;
    }
}

static void RethinkICRB (void)
{
    if (ciabimask & ciabicr) {
	ciabicr |= 0x80;
	INTREQ_0 (0x8000 | 0x2000);
    } else {
	ciabicr &= 0x7F;
    }
}

void rethink_cias (void)
{
    RethinkICRA ();
    RethinkICRB ();
}

/* Figure out how many CIA timer cycles have passed for each timer since the
   last call of CIA_calctimers.  */

static void compute_passed_time (void)
{
    unsigned long int ccount = (get_cycles () - eventtab[ev_cia].oldcycles + div10);
    unsigned long int ciaclocks = ccount / DIV10;

    ciaata_passed = ciaatb_passed = ciabta_passed = ciabtb_passed = 0;

    /* CIA A timers */
    if ((ciaacra & 0x21) == 0x01) {
	assert ((ciaata+1) >= ciaclocks);
	ciaata_passed = ciaclocks;
    }
    if ((ciaacrb & 0x61) == 0x01) {
	assert ((ciaatb+1) >= ciaclocks);
	ciaatb_passed = ciaclocks;
    }

    /* CIA B timers */
    if ((ciabcra & 0x21) == 0x01) {
	assert ((ciabta+1) >= ciaclocks);
	ciabta_passed = ciaclocks;
    }
    if ((ciabcrb & 0x61) == 0x01) {
	assert ((ciabtb+1) >= ciaclocks);
	ciabtb_passed = ciaclocks;
    }
}

/* Called to advance all CIA timers to the current time.  This expects that
   one of the timer values will be modified, and CIA_calctimers will be called
   in the same cycle.  */

static void CIA_update (void)
{
    unsigned long int ccount = (get_cycles () - eventtab[ev_cia].oldcycles + div10);
    unsigned long int ciaclocks = ccount / DIV10;

    int aovfla = 0, aovflb = 0, asp = 0, bovfla = 0, bovflb = 0, bsp = 0;

    div10 = ccount % DIV10;

    /* CIA A timers */
    if ((ciaacra & 0x21) == 0x01) {
	assert ((ciaata+1) >= ciaclocks);
	if ((ciaata+1) == ciaclocks) {
	    if ((ciaacra & 0x48) == 0x40 && ciaasdr_cnt > 0 && --ciaasdr_cnt == 0)
		asp = 1;
	    aovfla = 1;
	    if ((ciaacrb & 0x61) == 0x41) {
		if (ciaatb-- == 0)
		    aovflb = 1;
	    }
	}
	ciaata -= ciaclocks;
    }
    if ((ciaacrb & 0x61) == 0x01) {
	assert ((ciaatb+1) >= ciaclocks);
	if ((ciaatb+1) == ciaclocks) aovflb = 1;
	ciaatb -= ciaclocks;
    }

    /* CIA B timers */
    if ((ciabcra & 0x21) == 0x01) {
	assert ((ciabta+1) >= ciaclocks);
	if ((ciabta+1) == ciaclocks) {
	    if ((ciabcra & 0x48) == 0x40 && ciabsdr_cnt > 0 && --ciabsdr_cnt == 0)
		bsp = 1;
	    bovfla = 1;
	    if ((ciabcrb & 0x61) == 0x41) {
		if (ciabtb-- == 0)
		    bovflb = 1;
	    }
	}
	ciabta -= ciaclocks;
    }
    if ((ciabcrb & 0x61) == 0x01) {
	assert ((ciabtb+1) >= ciaclocks);
	if ((ciabtb+1) == ciaclocks) bovflb = 1;
	ciabtb -= ciaclocks;
    }

    if (aovfla) {
	ciaaicr |= 1; RethinkICRA();
	ciaata = ciaala;
	if (ciaacra & 0x8) ciaacra &= ~1;
    }
    if (aovflb) {
	ciaaicr |= 2; RethinkICRA();
	ciaatb = ciaalb;
	if (ciaacrb & 0x8) ciaacrb &= ~1;
    }
    if (asp) {
	ciaaicr |= 8; RethinkICRA();
    }
    if (bovfla) {
	ciabicr |= 1; RethinkICRB();
	ciabta = ciabla;
	if (ciabcra & 0x8) ciabcra &= ~1;
    }
    if (bovflb) {
	ciabicr |= 2; RethinkICRB();
	ciabtb = ciablb;
	if (ciabcrb & 0x8) ciabcrb &= ~1;
    }
    if (bsp) {
	ciabicr |= 8; RethinkICRB();
    }
}

/* Call this only after CIA_update has been called in the same cycle.  */

static void CIA_calctimers (void)
{
    long int ciaatimea = -1, ciaatimeb = -1, ciabtimea = -1, ciabtimeb = -1;

    eventtab[ev_cia].oldcycles = get_cycles ();
    if ((ciaacra & 0x21) == 0x01) {
	ciaatimea = (DIV10 - div10) + DIV10 * ciaata;
    }
    if ((ciaacrb & 0x61) == 0x41) {
	/* Timer B will not get any pulses if Timer A is off. */
	if (ciaatimea >= 0) {
	    /* If Timer A is in one-shot mode, and Timer B needs more than
	     * one pulse, it will not underflow. */
	    if (ciaatb == 0 || (ciaacra & 0x8) == 0) {
		/* Otherwise, we can determine the time of the underflow. */
 		/* This may overflow, however.  So just ignore this timer and
 		   use the fact that we'll call CIA_handler for the A timer.  */
#if 0
  		ciaatimeb = ciaatimea + ciaala * DIV10 * ciaatb;
#endif
	    }
	}
    }
    if ((ciaacrb & 0x61) == 0x01) {
	ciaatimeb = (DIV10 - div10) + DIV10 * ciaatb;
    }

    if ((ciabcra & 0x21) == 0x01) {
	ciabtimea = (DIV10 - div10) + DIV10 * ciabta;
    }
    if ((ciabcrb & 0x61) == 0x41) {
	/* Timer B will not get any pulses if Timer A is off. */
	if (ciabtimea >= 0) {
	    /* If Timer A is in one-shot mode, and Timer B needs more than
	     * one pulse, it will not underflow. */
	    if (ciabtb == 0 || (ciabcra & 0x8) == 0) {
		/* Otherwise, we can determine the time of the underflow. */
#if 0
 		ciabtimeb = ciabtimea + ciabla * DIV10 * ciabtb;
#endif
	    }
	}
    }
    if ((ciabcrb & 0x61) == 0x01) {
	ciabtimeb = (DIV10 - div10) + DIV10 * ciabtb;
    }
    eventtab[ev_cia].active = (ciaatimea != -1 || ciaatimeb != -1
			       || ciabtimea != -1 || ciabtimeb != -1);
    if (eventtab[ev_cia].active) {
	unsigned long int ciatime = ~0L;
	if (ciaatimea != -1) ciatime = ciaatimea;
	if (ciaatimeb != -1 && ciaatimeb < ciatime) ciatime = ciaatimeb;
	if (ciabtimea != -1 && ciabtimea < ciatime) ciatime = ciabtimea;
	if (ciabtimeb != -1 && ciabtimeb < ciatime) ciatime = ciabtimeb;
	eventtab[ev_cia].evtime = ciatime + get_cycles ();
    }
    events_schedule();
}

void CIA_handler (void)
{
    CIA_update ();
    CIA_calctimers ();
}

void cia_diskindex (void)
{
    ciabicr |= 0x10;
    RethinkICRB();
}

static void ciab_checkalarm (void)
{
    if (ciabtod != ciabalarm)
	return;
    ciabicr |= 4; RethinkICRB();
}

static void ciaa_checkalarm (void)
{
    if (ciaatod != ciaaalarm)
	return;
    ciaaicr |= 4; RethinkICRA();
}

void CIA_hsync_handler (void)
{
    static unsigned int keytime = 0, sleepyhead = 0;

    if (ciabtodon) {
	ciabtod++;
	ciabtod &= 0xFFFFFF;
	ciab_checkalarm ();
    }

    if (keys_available() && kback && (ciaacra & 0x40) == 0 && (++keytime & 15) == 0) {
	/*
	 * This hack lets one possible ciaaicr cycle go by without any key
	 * being read, for every cycle in which a key is pulled out of the
	 * queue.  If no hack is used, a lot of key events just get lost
	 * when you type fast.  With a simple hack that waits for ciaasdr
	 * to be read before feeding it another, it will keep up until the
	 * queue gets about 14 characters ahead and then lose events, and
	 * the mouse pointer will freeze while typing is being taken in.
	 * With this hack, you can type 30 or 40 characters ahead with little
	 * or no lossage, and the mouse doesn't get stuck.  The tradeoff is
	 * that the total slowness of typing appearing on screen is worse.
	 */
	if (ciaasdr_unread == 2) {
	    ciaasdr_unread = 0;
	} else if (ciaasdr_unread == 0) {
	    switch (kbstate) {
	     case 0:
		ciaasdr = (uae_s8)~0xFB; /* aaarghh... stupid compiler */
		kbstate++;
		break;
	     case 1:
		kbstate++;
		ciaasdr = (uae_s8)~0xFD;
		break;
	     case 2:
		ciaasdr = ~get_next_key();
		ciaasdr_unread = 1;      /* interlock to prevent lost keystrokes */
		break;
	    }
	    ciaaicr |= 8;
	    RethinkICRA();
	    sleepyhead = 0;
	} else if (!(++sleepyhead & 15)) {
	    ciaasdr_unread = 0;          /* give up on this key event after unread for a long time */
	}
    }
}

#ifdef TOD_HACK
static void tod_hack_reset (void)
{
    struct timeval tv;
    uae_u32 rate = currprefs.ntscmode ? 60 : 50;
    gettimeofday (&tv, NULL);
    tod_hack = (uae_u32)(((uae_u64)tv.tv_sec) * rate  + tv.tv_usec / (1000000 / rate));
    tod_hack -= ciaatod;
    tod_hack_delay = 10 * 50;
}
#endif

void CIA_vsync_handler ()
{
#ifdef TOD_HACK
    if (currprefs.tod_hack && ciaatodon) {
        struct timeval tv;
	uae_u32 t, nt, rate = currprefs.ntscmode ? 60 : 50;

	if (tod_hack_delay > 0) {
	    tod_hack_delay--;
	    if (tod_hack_delay == 0) {
		tod_hack_reset ();
		tod_hack_delay = 0;
		write_log ("TOD_HACK re-initialized CIATOD=%06.6X\n", ciaatod);
	    }
	}
	if (tod_hack_delay == 0) {
	    gettimeofday (&tv, NULL);
	    t = (uae_u32)(((uae_u64)tv.tv_sec) * rate + tv.tv_usec / (1000000 / rate));
	    nt = t - tod_hack;
	    if ((nt < ciaatod && ciaatod - nt < 10) || nt == ciaatod)
		return; /* try not to count backwards */
	    ciaatod = nt;
	    ciaatod &= 0xffffff;
	    ciaa_checkalarm ();
	    return;
	}
    }
#endif
    if (ciaatodon) {
	ciaatod++;
	ciaatod &= 0xFFFFFF;
	ciaa_checkalarm ();
    }
}

static void bfe001_change (void)
{
    uae_u8 v = ciaapra;

    v |= ~ciaadra; /* output is high when pin's direction is input */
    if ((v & 2) != oldled) {
	int led = (v & 2) ? 0 : 1;
        oldled = v & 2;
        gui_led (0, led);
        gui_ledstate &= ~1;
        gui_data.powerled = led;
	gui_ledstate |= led;
    }
    if ((v & 1) != oldovl) {
	oldovl = v & 1;
        if (!oldovl || ersatzkickfile) {
	    map_overlay (1);
	} else if (!(currprefs.chipset_mask & CSMASK_AGA)) {
	    /* pin disconnected in AGA chipset, CD audio mute on/off on CD32 */
 	    map_overlay (0);
        }
    }
}

#ifdef PARALLEL_PORT
extern int isprinter (void);
extern int doprinter (uae_u8);
#endif

static uae_u8 ReadCIAA (unsigned int addr)
{
    unsigned int tmp;

    compute_passed_time ();

#ifdef CIA_DEBUG
    write_log("R_CIAA: %02.2X %08.8X\n", addr, m68k_getpc());
#endif

    switch (addr & 0xf) {
    case 0:
#ifdef ACTION_REPLAY
	action_replay_ciaread();
#endif
	tmp = DISK_status() & 0x3c;
	tmp |= handle_joystick_buttons (ciaadra);
	tmp |= (ciaapra | (ciaadra ^ 3)) & 0x03;
	if (ciaadra & 0x40)
	    tmp = (tmp & ~0x40) | (ciaapra & 0x40);
	if (ciaadra & 0x80)
	    tmp = (tmp & ~0x80) | (ciaapra & 0x80);
#ifdef DONGLE_DEBUG
	if (notinrom())
	    write_log ("BFE001 R %02.2X %s\n", tmp, debuginfo(0));
#endif
	return tmp;
    case 1:
#ifdef PARALLEL_PORT
	if (isprinter () > 0) {
	    tmp = ciaaprb;
	} else if (isprinter () < 0) {
	    uae_u8 v;
	    parallel_direct_read_data (&v);
	    tmp = v;
	} else {
	    tmp = handle_parport_joystick (0, ciaaprb, ciaadrb);
	}
#else
	tmp = handle_parport_joystick (0, ciaaprb, ciaadrb);
#ifdef DONGLE_DEBUG
	if (notinrom())
	    write_log ("BFE101 R %02.2X %s\n", tmp, debuginfo(0));
#endif
#endif
	return tmp;
    case 2:
#ifdef DONGLE_DEBUG
	if (notinrom ())
	    write_log ("BFE201 R %02.2X %s\n", ciaadra, debuginfo(0));
#endif
	return ciaadra;
    case 3:
#ifdef DONGLE_DEBUG
	if (notinrom ())
	    write_log ("BFE301 R %02.2X %s\n", ciaadrb, debuginfo(0));
#endif
	return ciaadrb;
    case 4:
	return (uae_u8)((ciaata - ciaata_passed) & 0xff);
    case 5:
	return (uae_u8)((ciaata - ciaata_passed) >> 8);
    case 6:
	return (uae_u8)((ciaatb - ciaatb_passed) & 0xff);
    case 7:
	return (uae_u8)((ciaatb - ciaatb_passed) >> 8);
    case 8:
	if (ciaatlatch) {
	    ciaatlatch = 0;
	    return (uae_u8)ciaatol;
	} else
	    return (uae_u8)ciaatod;
    case 9:
	if (ciaatlatch)
	    return (uae_u8)(ciaatol >> 8);
	else
	    return (uae_u8)(ciaatod >> 8);
    case 10:
	ciaatlatch = 1;
	ciaatol = ciaatod; /* ??? only if not already latched? */
	return (uae_u8)(ciaatol >> 16);
    case 12:
	if (ciaasdr_unread == 1)
	    ciaasdr_unread = 2;
	return ciaasdr;
    case 13:
	tmp = ciaaicr; ciaaicr = 0; RethinkICRA();
	return tmp;
    case 14:
	return ciaacra;
    case 15:
	return ciaacrb;
    }
    return 0;
}

static uae_u8 ReadCIAB (unsigned int addr)
{
    unsigned int tmp;

#ifdef CIA_DEBUG
    write_log("R_CIAB: %02.2X %08.8X\n", addr, m68k_getpc());
#endif

    compute_passed_time ();

    switch (addr & 0xf) {
    case 0:
	tmp = 0;
#ifdef SERIAL_PORT
	if (currprefs.use_serial)
	    tmp = serial_readstatus(ciabdra);
#endif
#ifdef PARALLEL_PORT
	if (isprinter () > 0) {
	    tmp |= ciabpra & (0x04 | 0x02 | 0x01);
	} else if (isprinter () < 0) {
	    uae_u8 v;
	    parallel_direct_read_status (&v);
	    tmp |= v & 7;
	} else {
	    tmp |= handle_parport_joystick (1, ciabpra, ciabdra);
	}
#endif
#ifdef DONGLE_DEBUG
	if (notinrom ())
	    write_log ("BFD000 R %02.2X %s\n", tmp, debuginfo(0));
#endif
	return tmp;
    case 1:
#ifdef DONGLE_DEBUG
	if (notinrom ())
	    write_log ("BFD100 R %02.2X %s\n", ciabprb, debuginfo(0));
#endif
	return ciabprb;
    case 2:
	return ciabdra;
    case 3:
	return ciabdrb;
    case 4:
	return (uae_u8)((ciabta - ciabta_passed) & 0xff);
    case 5:
	return (uae_u8)((ciabta - ciabta_passed) >> 8);
    case 6:
	return (uae_u8)((ciabtb - ciabtb_passed) & 0xff);
    case 7:
	return (uae_u8)((ciabtb - ciabtb_passed) >> 8);
    case 8:
	if (ciabtlatch) {
	    ciabtlatch = 0;
	    return (uae_u8)ciabtol;
	} else
	    return (uae_u8)ciabtod;
    case 9:
	if (ciabtlatch)
	    return (uae_u8)(ciabtol >> 8);
	else
	    return (uae_u8)(ciabtod >> 8);
    case 10:
	ciabtlatch = 1;
	ciabtol = ciabtod;
	return (uae_u8)(ciabtol >> 16);
    case 12:
	return ciabsdr;
    case 13:
	tmp = ciabicr; ciabicr = 0; RethinkICRB();
	return tmp;
    case 14:
	return ciabcra;
    case 15:
	return ciabcrb;
    }
    return 0;
}

static void WriteCIAA (uae_u16 addr,uae_u8 val)
{
#ifdef CIA_DEBUG
    write_log("W_CIAA: %02.2X %02.2X %08.8X\n", addr, val, m68k_getpc());
#endif
    switch (addr & 0xf) {
    case 0:
#ifdef DONGLE_DEBUG
	if (notinrom ())
	    write_log ("BFE001 W %02.2X %s\n", val, debuginfo(0));
#endif
	ciaapra = (ciaapra & ~0xc3) | (val & 0xc3);
	bfe001_change ();
	handle_cd32_joystick_cia (ciaapra, ciaadra);
	break;
    case 1:
#ifdef DONGLE_DEBUG
	if (notinrom ())
	    write_log ("BFE101 W %02.2X %s\n", val, debuginfo(0));
#endif
	ciaaprb = val;
#ifdef PARALLEL_PORT
	if (isprinter() > 0) {
	    doprinter (val);
	    ciaaicr |= 0x10;
	} else if (isprinter() < 0) {
	    parallel_direct_write_data (val, ciaadrb);
	    ciaaicr |= 0x10;
	}
#endif
	break;
    case 2:
#ifdef DONGLE_DEBUG
	if (notinrom ())
	    write_log ("BFE201 W %02.2X %s\n", val, debuginfo(0));
#endif
	ciaadra = val; bfe001_change (); break;
    case 3:
#ifdef DONGLE_DEBUG
	if (notinrom ())
	    write_log ("BFE301 W %02.2X %s\n", val, debuginfo(0));
#endif
	ciaadrb = val; break;
    case 4:
	CIA_update ();
	ciaala = (ciaala & 0xff00) | val;
	CIA_calctimers ();
	break;
    case 5:
	CIA_update ();
	ciaala = (ciaala & 0xff) | (val << 8);
	if ((ciaacra & 1) == 0)
	    ciaata = ciaala;
	if (ciaacra & 8) {
	    ciaata = ciaala;
	    ciaacra |= 1;
	}
	CIA_calctimers ();
	break;
    case 6:
	CIA_update ();
	ciaalb = (ciaalb & 0xff00) | val;
	CIA_calctimers ();
	break;
    case 7:
	CIA_update ();
	ciaalb = (ciaalb & 0xff) | (val << 8);
	if ((ciaacrb & 1) == 0)
	    ciaatb = ciaalb;
	if (ciaacrb & 8) {
	    ciaatb = ciaalb;
	    ciaacrb |= 1;
	}
	CIA_calctimers ();
	break;
    case 8:
	if (ciaacrb & 0x80) {
	    ciaaalarm = (ciaaalarm & ~0xff) | val;
	} else {
	    ciaatod = (ciaatod & ~0xff) | val;
	    ciaatodon = 1;
	    ciaa_checkalarm ();

#ifdef TOD_HACK
	if (currprefs.tod_hack)
	    tod_hack_reset ();
#endif
	}
	break;
    case 9:
	if (ciaacrb & 0x80) {
	    ciaaalarm = (ciaaalarm & ~0xff00) | (val << 8);
	} else {
	    ciaatod = (ciaatod & ~0xff00) | (val << 8);
	    ciaatodon = 0;
	}
	break;
    case 10:
	if (ciaacrb & 0x80) {
	    ciaaalarm = (ciaaalarm & ~0xff0000) | (val << 16);
	} else {
	    ciaatod = (ciaatod & ~0xff0000) | (val << 16);
	    ciaatodon = 0;
	}
	break;
    case 12:
	CIA_update ();
        ciaasdr = val;
        if (ciaacra & 0x40) {
	    kback = 1;
	} else {
	    ciaasdr_cnt = 0;
        }
        if ((ciaacra & 0x41) == 0x41)
	    ciaasdr_cnt = 8 * 2;
	CIA_calctimers ();
	break;
    case 13:
	setclr(&ciaaimask,val);
	break;
    case 14:
	CIA_update ();
	val &= 0x7f; /* bit 7 is unused */
	ciaacra = val;
	if (ciaacra & 0x10) {
	    ciaacra &= ~0x10;
	    ciaata = ciaala;
	}
	if (ciaacra & 0x40)
	    kback = 1;
	CIA_calctimers ();
	break;
    case 15:
	CIA_update ();
	ciaacrb = val;
	if (ciaacrb & 0x10) {
	    ciaacrb &= ~0x10;
	    ciaatb = ciaalb;
	}
	CIA_calctimers ();
	break;
    }
}

static void WriteCIAB (uae_u16 addr,uae_u8 val)
{
#ifdef CIA_DEBUG
    write_log("W_CIAB: %02.2X %02.2X %08.8X\n", addr, val, m68k_getpc());
#endif
    switch (addr & 0xf) {
    case 0:
#ifdef DONGLE_DEBUG
	if (notinrom ())
	    write_log ("BFD000 W %02.2X %s\n", val, debuginfo(0));
#endif
	ciabpra = val;
#ifdef SERIAL_PORT
	if (currprefs.use_serial)
	    serial_writestatus(ciabpra, ciabdra);
#endif
#ifdef PARALLEL_PORT
	if (isprinter () < 0)
    	    parallel_direct_write_status (val, ciabdra);
#endif
	break;
    case 1:
#ifdef DONGLE_DEBUG
	if (notinrom ())
	    write_log ("BFD100 W %02.2X %s\n", val, debuginfo(0));
#endif
	ciabprb = val; DISK_select(val); break;
    case 2:
#ifdef DONGLE_DEBUG
	if (notinrom ())
	    write_log ("BFD200 W %02.2X %s\n", val, debuginfo(0));
#endif
	ciabdra = val;
#ifdef SERIAL_PORT
	if (currprefs.use_serial)
	    serial_writestatus (ciabpra, ciabdra);
#endif
	break;
    case 3:
#ifdef DONGLE_DEBUG
	if (notinrom ())
	    write_log ("BFD300 W %02.2X %s\n", val, debuginfo(0));
#endif
	ciabdrb = val; break;
    case 4:
	CIA_update ();
	ciabla = (ciabla & 0xff00) | val;
	CIA_calctimers ();
	break;
    case 5:
	CIA_update ();
	ciabla = (ciabla & 0xff) | (val << 8);
	if ((ciabcra & 1) == 0)
	    ciabta = ciabla;
	if (ciabcra & 8) {
	    ciabta = ciabla;
	    ciabcra |= 1;
	}
	CIA_calctimers ();
	break;
    case 6:
	CIA_update ();
	ciablb = (ciablb & 0xff00) | val;
	CIA_calctimers ();
	break;
    case 7:
	CIA_update ();
	ciablb = (ciablb & 0xff) | (val << 8);
	if ((ciabcrb & 1) == 0)
	    ciabtb = ciablb;
	if (ciabcrb & 8) {
	    ciabtb = ciablb;
	    ciabcrb |= 1;
	}
	CIA_calctimers ();
	break;
    case 8:
	if (ciabcrb & 0x80) {
	    ciabalarm = (ciabalarm & ~0xff) | val;
	} else {
	    ciabtod = (ciabtod & ~0xff) | val;
	    ciabtodon = 1;
	    ciab_checkalarm ();
	}
	break;
    case 9:
	if (ciabcrb & 0x80) {
	    ciabalarm = (ciabalarm & ~0xff00) | (val << 8);
	} else {
	    ciabtod = (ciabtod & ~0xff00) | (val << 8);
	    ciabtodon = 0;
	}
	break;
    case 10:
	if (ciabcrb & 0x80) {
	    ciabalarm = (ciabalarm & ~0xff0000) | (val << 16);
	} else {
	    ciabtod = (ciabtod & ~0xff0000) | (val << 16);
	    ciabtodon = 0;
	}
	break;
    case 12:
	CIA_update ();
	ciabsdr = val;
	if ((ciabcra & 0x41) == 0x41)
	    ciabsdr_cnt = 8 * 2;
	if ((ciabcra & 0x40) == 0)
	    ciabsdr_cnt = 0;
	CIA_calctimers ();
	break;
    case 13:
	setclr(&ciabimask,val);
	break;
    case 14:
	CIA_update ();
	val &= 0x7f; /* bit 7 is unused */
	ciabcra = val;
	if (ciabcra & 0x10) {
	    ciabcra &= ~0x10;
	    ciabta = ciabla;
	}
	CIA_calctimers ();
	break;
    case 15:
	CIA_update ();
	ciabcrb = val;
	if (ciabcrb & 0x10) {
	    ciabcrb &= ~0x10;
	    ciabtb = ciablb;
	}
	CIA_calctimers ();
	break;
    }
}

void CIA_reset (void)
{
#ifdef TOD_HACK
    tod_hack = 0;
    if (currprefs.tod_hack)
	tod_hack_reset ();
#endif
    kback = 1;
    kbstate = 0;
    ciaasdr_unread = 0;
    serbits = 0;
    oldovl = -1;
    oldled = -1;

    if (!savestate_state) {
	ciaatlatch = ciabtlatch = 0;
	ciaapra = 0; ciaadra = 0;
	ciaatod = ciabtod = 0; ciaatodon = ciabtodon = 0;
	ciaaicr = ciabicr = ciaaimask = ciabimask = 0;
	ciaacra = ciaacrb = ciabcra = ciabcrb = 0x4; /* outmode = toggle; */
	ciaala = ciaalb = ciabla = ciablb = ciaata = ciaatb = ciabta = ciabtb = 0xFFFF;
	ciaaalarm = ciabalarm = 0;
	ciabpra = 0x8C; ciabdra = 0;
	div10 = 0;
	ciaasdr_cnt = 0; ciaasdr = 0;
	ciabsdr_cnt = 0; ciabsdr = 0;
	ciaata_passed = ciaatb_passed = ciabta_passed = ciabtb_passed = 0;
    }
    CIA_calctimers ();
    if (! ersatzkickfile)
	map_overlay (0);
#ifdef SERIAL_PORT
    if (currprefs.use_serial && !savestate_state)
	serial_dtr_off (); /* Drop DTR at reset */
#endif
    if (savestate_state) {
	bfe001_change ();
	/* select drives */
	DISK_select (ciabprb);
    }
#ifdef CD32
    if (cd32_enabled) {
	akiko_reset ();
	if (!akiko_init ())
	    cd32_enabled = 0;
    }
#endif
}

void dumpcia (void)
{
    write_log("A: CRA %02x CRB %02x ICR %02x IM %02x TOD %06x %c%c TA %04x (%04x) TB %04x (%04x)\n",
	   ciaacra, ciaacrb, ciaaicr, ciaaimask, ciaatod,
	   ciaatlatch ? 'L' : ' ', ciaatodon ? ' ' : 'S', ciaata, ciaala, ciaatb, ciaalb);
    write_log("B: CRA %02x CRB %02x ICR %02x IM %02x TOD %06x %c%c TA %04x (%04x) TB %04x (%04x)\n",
	   ciabcra, ciabcrb, ciaaicr, ciabimask, ciabtod,
	   ciabtlatch ? 'L' : ' ', ciabtodon ? ' ' : 'S', ciabta, ciabla, ciabtb, ciablb);
}

/* CIA memory access */

static uae_u32 cia_lget (uaecptr) REGPARAM;
static uae_u32 cia_wget (uaecptr) REGPARAM;
static uae_u32 cia_bget (uaecptr) REGPARAM;
static void cia_lput (uaecptr, uae_u32) REGPARAM;
static void cia_wput (uaecptr, uae_u32) REGPARAM;
static void cia_bput (uaecptr, uae_u32) REGPARAM;

addrbank cia_bank = {
    cia_lget, cia_wget, cia_bget,
    cia_lput, cia_wput, cia_bput,
    default_xlate, default_check, NULL
};


#ifdef CD32
extern uae_u32 akiko_lget (uaecptr addr) REGPARAM;
extern uae_u32 akiko_wget (uaecptr addr) REGPARAM;
extern uae_u32 akiko_bget (uaecptr addr) REGPARAM;
extern void akiko_bput (uaecptr addr, uae_u32 value) REGPARAM;
extern void akiko_wput (uaecptr addr, uae_u32 value) REGPARAM;
extern void akiko_lput (uaecptr addr, uae_u32 value) REGPARAM;
#endif

/* e-clock is 10 CPU cycles, 6 cycles low, 4 high
 * data transfer happens during 4 high cycles
 */

#define ECLOCK_DATA_CYCLE 4

static void cia_wait_pre (void)
{
#ifndef CUSTOM_SIMPLE
    int div10 = (get_cycles () - eventtab[ev_cia].oldcycles) % DIV10;
    int cycles;

    cycles = 2 * CYCLE_UNIT / 2;
    if (div10 > DIV10 * ECLOCK_DATA_CYCLE / 10) {
	cycles += DIV10 - div10;
	cycles += DIV10 * ECLOCK_DATA_CYCLE / 10;
    } else {
	cycles += DIV10 * ECLOCK_DATA_CYCLE / 10 - div10;
    }
    do_cycles (cycles);
#endif
}

static void cia_wait_post (void)
{
    do_cycles (2 * CYCLE_UNIT / 2);
}

uae_u32 REGPARAM2 cia_bget (uaecptr addr)
{
    int r = (addr & 0xf00) >> 8;
    uae_u8 v;

#ifdef JIT
    special_mem |= S_READ;
#endif
#ifdef CD32
    if (cd32_enabled && addr >= AKIKO_BASE && addr < AKIKO_BASE_END)
	return akiko_bget (addr);
#endif
    cia_wait_pre ();
    v = 0xff;
    switch ((addr >> 12) & 3)
    {
	case 0:
	v = (addr & 1) ? ReadCIAA (r) : ReadCIAB (r);
	break;
	case 1:
	v = (addr & 1) ? 0xff : ReadCIAB (r);
	break;
	case 2:
	v = (addr & 1) ? ReadCIAA (r) : 0xff;
	break;
    }
    cia_wait_post ();
    return v;
}

uae_u32 REGPARAM2 cia_wget (uaecptr addr)
{
    int r = (addr & 0xf00) >> 8;
    uae_u16 v;

#ifdef JIT
    special_mem |= S_READ;
#endif
#ifdef CD32
    if (cd32_enabled && addr >= AKIKO_BASE && addr < AKIKO_BASE_END)
	return akiko_wget (addr);
#endif
    cia_wait_pre ();
    v = 0xffff;
    switch ((addr >> 12) & 3)
    {
	case 0:
	v = (ReadCIAB (r) << 8) | ReadCIAA (r);
	break;
	case 1:
	v = (ReadCIAB (r) << 8) | 0xff;
	break;
	case 2:
	v = (0xff << 8) | ReadCIAA (r);
	break;
    }
    cia_wait_post ();
    return v;
}

uae_u32 REGPARAM2 cia_lget (uaecptr addr)
{
    uae_u32 v;
#ifdef JIT
    special_mem |= S_READ;
#endif
#ifdef CD32
    if (cd32_enabled && addr >= AKIKO_BASE && addr < AKIKO_BASE_END)
	return akiko_lget (addr);
#endif
    v = cia_wget (addr) << 16;
    v |= cia_wget (addr + 2);
    return v;
}

void REGPARAM2 cia_bput (uaecptr addr, uae_u32 value)
{
    int r = (addr & 0xf00) >> 8;
#ifdef JIT
    special_mem |= S_WRITE;
#endif
#ifdef CD32
    if (cd32_enabled && addr >= AKIKO_BASE && addr < AKIKO_BASE_END) {
	akiko_bput (addr, value);
	return;
    }
#endif
    cia_wait_pre ();
    if ((addr & 0x2000) == 0)
	WriteCIAB (r, value);
    if ((addr & 0x1000) == 0)
	WriteCIAA (r, value);
    cia_wait_post ();
}

void REGPARAM2 cia_wput (uaecptr addr, uae_u32 value)
{
    int r = (addr & 0xf00) >> 8;
#ifdef JIT
    special_mem |= S_WRITE;
#endif
#ifdef CD32
    if (cd32_enabled && addr >= AKIKO_BASE && addr < AKIKO_BASE_END) {
	akiko_wput (addr, value);
	return;
    }
#endif
    cia_wait_pre ();
    if ((addr & 0x2000) == 0)
	WriteCIAB (r, value >> 8);
    if ((addr & 0x1000) == 0)
	WriteCIAA (r, value & 0xff);
    cia_wait_post ();
}

void REGPARAM2 cia_lput (uaecptr addr, uae_u32 value)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
#ifdef CD32
    if (cd32_enabled && addr >= AKIKO_BASE && addr < AKIKO_BASE_END) {
	akiko_lput (addr, value);
	return;
    }
#endif
    cia_wput (addr, value >> 16);
    cia_wput (addr + 2, value & 0xffff);
}

#ifdef CDTV
/* CDTV batterybacked RAM emulation */

static uae_u8 cdtv_battram[4096];
#define CDTV_NVRAM_MASK 4095
#define CDTV_NVRAM_SIZE 2048
int cdtv_enabled;

static void cdtv_battram_reset (void)
{
    struct zfile *f = zfile_fopen (currprefs.flashfile,"rb");
    if (!f)
        return;
    zfile_fread (cdtv_battram, CDTV_NVRAM_SIZE,1 ,f);
    zfile_fclose (f);
}

static void cdtv_battram_write (int addr, int v)
{
    struct zfile *f;
    int offset = addr & CDTV_NVRAM_MASK;
    if (offset >= CDTV_NVRAM_SIZE) return;
    if (cdtv_battram[offset] == v) return;
    cdtv_battram[offset] = v;
    f = zfile_fopen (currprefs.flashfile,"rb+");
    if (!f)
	return;
    zfile_fseek (f, offset, SEEK_SET);
    zfile_fwrite (cdtv_battram + offset, 1, 1, f);
    zfile_fclose (f);
}

static uae_u8 cdtv_battram_read (int addr)
{
    uae_u8 v;
    int offset;
    offset = addr & CDTV_NVRAM_MASK;
    if (offset >= CDTV_NVRAM_SIZE) return 0;
    v =  cdtv_battram[offset];
    return v;
}
#endif

/* battclock memory access */

static uae_u32 clock_lget (uaecptr) REGPARAM;
static uae_u32 clock_wget (uaecptr) REGPARAM;
static uae_u32 clock_bget (uaecptr) REGPARAM;
static void clock_lput (uaecptr, uae_u32) REGPARAM;
static void clock_wput (uaecptr, uae_u32) REGPARAM;
static void clock_bput (uaecptr, uae_u32) REGPARAM;

addrbank clock_bank = {
    clock_lget, clock_wget, clock_bget,
    clock_lput, clock_wput, clock_bput,
    default_xlate, default_check, NULL
};

uae_u32 REGPARAM2 clock_lget (uaecptr addr)
{
    return (clock_wget (addr) << 16) | clock_wget (addr + 2);
}

uae_u32 REGPARAM2 clock_wget (uaecptr addr)
{
    return (clock_bget (addr) << 8) | clock_bget (addr + 1);
}

uae_u32 REGPARAM2 clock_bget (uaecptr addr)
{
    time_t t = time(0);
    struct tm *ct;

#ifdef CDTV
    if (cdtv_enabled && addr >= 0xdc8000)
	return cdtv_battram_read (addr);
#endif
    ct = localtime (&t);
#ifdef JIT
    special_mem |= S_READ;
#endif
    switch (addr & 0x3f) {
    case 0x03: return ct->tm_sec % 10;
    case 0x07: return ct->tm_sec / 10;
    case 0x0b: return ct->tm_min % 10;
    case 0x0f: return ct->tm_min / 10;
    case 0x13: return ct->tm_hour % 10;
    case 0x17: return ct->tm_hour / 10;
    case 0x1b: return ct->tm_mday % 10;
    case 0x1f: return ct->tm_mday / 10;
    case 0x23: return (ct->tm_mon+1) % 10;
    case 0x27: return (ct->tm_mon+1) / 10;
    case 0x2b: return ct->tm_year % 10;
    case 0x2f: return ct->tm_year / 10;
    case 0x33: return ct->tm_wday;  /*Hack by -=SR=- */
    case 0x37: return clock_control_d;
    case 0x3b: return clock_control_e;
    case 0x3f: return clock_control_f;
    }
    return 0;
}

void REGPARAM2 clock_lput (uaecptr addr, uae_u32 value)
{
    clock_wput (addr, value >> 16);
    clock_wput (addr + 2, value);
}

void REGPARAM2 clock_wput (uaecptr addr, uae_u32 value)
{
    clock_bput (addr, value >> 8);
    clock_bput (addr + 1, value);
}

void REGPARAM2 clock_bput (uaecptr addr, uae_u32 value)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
#ifdef CDTV
    if (cdtv_enabled && addr >= 0xdc8000) {
	cdtv_battram_write (addr, value);
	return;
    }
#endif
    switch (addr & 0x3f) {
    case 0x37: clock_control_d = value; break;
    case 0x3b: clock_control_e = value; break;
    case 0x3f: clock_control_f = value; break;
    }
}

/* CIA-A and CIA-B save/restore code */

uae_u8 *restore_cia (int num, uae_u8 *src)
{
    uae_u8 b;
    uae_u16 w;
    uae_u32 l;

    /* CIA registers */
    b = restore_u8 ();					/* 0 PRA */
    if (num) ciabpra = b; else ciaapra = b;
    b = restore_u8 ();					/* 1 PRB */
    if (num) ciabprb = b; else ciaaprb = b;
    b = restore_u8 ();					/* 2 DDRA */
    if (num) ciabdra = b; else ciaadra = b;
    b = restore_u8 ();					/* 3 DDRB */
    if (num) ciabdrb = b; else ciaadrb = b;
    w = restore_u16 ();					/* 4 TA */
    if (num) ciabta = w; else ciaata = w;
    w = restore_u16 ();					/* 6 TB */
    if (num) ciabtb = w; else ciaatb = w;
    l = restore_u8 ();					/* 8/9/A TOD */
    l |= restore_u8 () << 8;
    l |= restore_u8 () << 16;
    if (num) ciabtod = l; else ciaatod = l;
    restore_u8 ();					/* B unused */
    b = restore_u8 ();					/* C SDR */
    if (num) ciabsdr = b; else ciaasdr = b;
    b = restore_u8 ();					/* D ICR INFORMATION (not mask!) */
    if (num) ciabicr = b; else ciaaicr = b;
    b = restore_u8 ();					/* E CRA */
    if (num) ciabcra = b; else ciaacra = b;
    b = restore_u8 ();					/* F CRB */
    if (num) ciabcrb = b; else ciaacrb = b;

/* CIA internal data */

    b = restore_u8 ();					/* ICR MASK */
    if (num) ciabimask = b; else ciaaimask = b;
    w = restore_u8 ();					/* timer A latch */
    w |= restore_u8 () << 8;
    if (num) ciabla = w; else ciaala = w;
    w = restore_u8 ();					/* timer B latch */
    w |= restore_u8 () << 8;
    if (num) ciablb = w; else ciaalb = w;
    w = restore_u8 ();					/* TOD latched value */
    w |= restore_u8 () << 8;
    w |= restore_u8 () << 16;
    if (num) ciabtol = w; else ciaatol = w;
    l = restore_u8 ();					/* alarm */
    l |= restore_u8 () << 8;
    l |= restore_u8 () << 16;
    if (num) ciabalarm = l; else ciaaalarm = l;
    b = restore_u8 ();
    if (num) ciabtlatch = b & 1; else ciaatlatch = b & 1;	/* is TOD latched? */
    if (num) ciabtodon = b & 2; else ciaatodon = b & 2;		/* is TOD stopped? */
    b = restore_u8 ();
    if (num)
	div10 = CYCLE_UNIT * b;
    b = restore_u8 ();
    if (num) ciabsdr_cnt = b; else ciaasdr_cnt = b;
    return src;
}

uae_u8 *save_cia (int num, int *len, uae_u8 *dstptr)
{
    uae_u8 *dstbak,*dst, b;
    uae_u16 t;

    if (dstptr)
	dstbak = dst = dstptr;
    else
	dstbak = dst = malloc (16 + 12 + 1 + 1);

    compute_passed_time ();

    /* CIA registers */

    b = num ? ciabpra : ciaapra;				/* 0 PRA */
    save_u8 (b);
    b = num ? ciabprb : ciaaprb;				/* 1 PRB */
    save_u8 (b);
    b = num ? ciabdra : ciaadra;				/* 2 DDRA */
    save_u8 (b);
    b = num ? ciabdrb : ciaadrb;				/* 3 DDRB */
    save_u8 (b);
    t = (num ? ciabta - ciabta_passed : ciaata - ciaata_passed);/* 4 TA */
    save_u16 (t);
    t = (num ? ciabtb - ciabtb_passed : ciaatb - ciaatb_passed);/* 8 TB */
    save_u16 (t);
    b = (num ? ciabtod : ciaatod);			/* 8 TODL */
    save_u8 (b);
    b = (num ? ciabtod >> 8 : ciaatod >> 8);		/* 9 TODM */
    save_u8 (b);
    b = (num ? ciabtod >> 16 : ciaatod >> 16);		/* A TODH */
    save_u8 (b);
    save_u8 (0);						/* B unused */
    b = num ? ciabsdr : ciaasdr;				/* C SDR */
    save_u8 (b);
    b = num ? ciabicr : ciaaicr;				/* D ICR INFORMATION (not mask!) */
    save_u8 (b);
    b = num ? ciabcra : ciaacra;				/* E CRA */
    save_u8 (b);
    b = num ? ciabcrb : ciaacrb;				/* F CRB */
    save_u8 (b);

    /* CIA internal data */

    save_u8 (num ? ciabimask : ciaaimask);			/* ICR */
    b = (num ? ciabla : ciaala);			/* timer A latch LO */
    save_u8 (b);
    b = (num ? ciabla >> 8 : ciaala >> 8);		/* timer A latch HI */
    save_u8 (b);
    b = (num ? ciablb : ciaalb);			/* timer B latch LO */
    save_u8 (b);
    b = (num ? ciablb >> 8 : ciaalb >> 8);		/* timer B latch HI */
    save_u8 (b);
    b = (num ? ciabtol : ciaatol);			/* latched TOD LO */
    save_u8 (b);
    b = (num ? ciabtol >> 8 : ciaatol >> 8);		/* latched TOD MED */
    save_u8 (b);
    b = (num ? ciabtol >> 16 : ciaatol >> 16);		/* latched TOD HI */
    save_u8 (b);
    b = (num ? ciabalarm : ciaaalarm);			/* alarm LO */
    save_u8 (b);
    b = (num ? ciabalarm >> 8 : ciaaalarm >>8 );	/* alarm MED */
    save_u8 (b);
    b = (num ? ciabalarm >> 16 : ciaaalarm >> 16);	/* alarm HI */
    save_u8 (b);
    b = 0;
    if (num)
	b |= ciabtlatch ? 1 : 0;
    else
	b |= ciaatlatch ? 1 : 0; /* is TOD latched? */
    if (num)
	b |= ciabtodon ? 2 : 0;
    else
	b |= ciaatodon ? 2 : 0;   /* TOD stopped? */
    save_u8 (b);
    save_u8 (div10 / CYCLE_UNIT);
    save_u8 (num ? ciabsdr_cnt : ciaasdr_cnt);
    *len = dst - dstbak;
    return dstbak;
}

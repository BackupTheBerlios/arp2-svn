/*
 * UAE - The Un*x Amiga Emulator
 *
 * Support for IPF/CAPS disk images
 *
 * Copyright 2004 Richard Drummond
 *
 * Based on Win32 CAPS code by Toni Wilen
 */

#include "sysconfig.h"
#include "sysdeps.h"

#ifdef CAPS

#include <caps/capsimage.h>
#include "zfile.h"
#include "caps.h"

/* Stuff not defined in the old CAPS API */
#ifndef DI_LOCK_UPDATEFD
#define DI_LOCK_UPDATEFD (1L<<7)
#endif
#ifndef CTIT_FLAG_FLAKEY
#define CTIT_FLAG_FLAKEY (1L<<31)
#endif
#ifndef CTIT_MASK_TYPE
#define CTIT_MASK_TYPE 0xff
#endif

static CapsLong caps_cont[4]= {-1, -1, -1, -1};
static int caps_locked[4];
static int caps_flags = DI_LOCK_DENVAR|DI_LOCK_DENNOISE|DI_LOCK_NOISE|DI_LOCK_UPDATEFD;


#ifndef AMIGA

/*
 * Repository for function pointers to the CAPSLib routines
 * which gets filled when we link at run-time
 *
 * We don't symbolically link on the Amiga, so don't need
 * this there
 */
struct {
  void *handle;
  CapsLong (*CAPSInit)(void);
  CapsLong (*CAPSExit)(void);
  CapsLong (*CAPSAddImage)(void);
  CapsLong (*CAPSRemImage)(CapsLong id);
  CapsLong (*CAPSLockImage)(CapsLong id, char *name);
  CapsLong (*CAPSLockImageMemory)(CapsLong id, CapsUByte *buffer, CapsULong length, CapsULong flag);
  CapsLong (*CAPSUnlockImage)(CapsLong id);
  CapsLong (*CAPSLoadImage)(CapsLong id, CapsULong flag);
  CapsLong (*CAPSGetImageInfo)(struct CapsImageInfo *pi, CapsLong id);
  CapsLong (*CAPSLockTrack)(struct CapsTrackInfo *pi, CapsLong id, CapsULong cylinder, CapsULong head, CapsULong flag);
  CapsLong (*CAPSUnlockTrack)(CapsLong id, CapsULong cylinder, CapsULong head);
  CapsLong (*CAPSUnlockAllTracks)(CapsLong id);
  char *(*CAPSGetPlatformName)(CapsULong pid);
} capslib;

#endif


#ifdef HAVE_DLOPEN

#include <dlfcn.h>

#define CAPSLIB_NAME    "libcapsimage.so.2"

/*
 * The Unix/dlopen method for loading and linking the CAPSLib plug-in
 */
static int load_capslib (void)
{
    /* This could be done more elegantly ;-) */
    if ((capslib.handle = dlopen(CAPSLIB_NAME, RTLD_LAZY))) {
	capslib.CAPSInit            = dlsym (capslib.handle, "CAPSInit");            if (dlerror () != 0) return 0;
	capslib.CAPSExit            = dlsym (capslib.handle, "CAPSExit");            if (dlerror () != 0) return 0;
	capslib.CAPSAddImage        = dlsym (capslib.handle, "CAPSAddImage");        if (dlerror () != 0) return 0;
	capslib.CAPSRemImage        = dlsym (capslib.handle, "CAPSRemImage");        if (dlerror () != 0) return 0;
	capslib.CAPSLockImage       = dlsym (capslib.handle, "CAPSLockImage");       if (dlerror () != 0) return 0;
	capslib.CAPSLockImageMemory = dlsym (capslib.handle, "CAPSLockImageMemory"); if (dlerror () != 0) return 0;
	capslib.CAPSUnlockImage     = dlsym (capslib.handle, "CAPSUnlockImage");     if (dlerror () != 0) return 0;
	capslib.CAPSLoadImage       = dlsym (capslib.handle, "CAPSLoadImage");       if (dlerror () != 0) return 0;
	capslib.CAPSGetImageInfo    = dlsym (capslib.handle, "CAPSGetImageInfo");    if (dlerror () != 0) return 0;
	capslib.CAPSLockTrack       = dlsym (capslib.handle, "CAPSLockTrack");       if (dlerror () != 0) return 0;
	capslib.CAPSUnlockTrack     = dlsym (capslib.handle, "CAPSUnlockTrack");     if (dlerror () != 0) return 0;
	capslib.CAPSUnlockAllTracks = dlsym (capslib.handle, "CAPSUnlockAllTracks"); if (dlerror () != 0) return 0;
	capslib.CAPSGetPlatformName = dlsym (capslib.handle, "CAPSGetPlatformName"); if (dlerror () != 0) return 0;
	if (capslib.CAPSInit() == imgeOk)
	    return 1;
    }
    write_log ("Unable to open " CAPSLIB_NAME "\n.");
    return 0;
}

#else

#ifdef __amigaos4__
#define __USE_BASETYPE__
#include <exec/emulation.h>
#include <proto/exec.h>

extern struct Device *CapsImageBase;

/* Emulation stubs for AmigaOS4. Ideally this should be in a separate
 * link library (until a native CAPS plug-in becomes available), but I
 * haven't been able to get that to work.
 *
 * This stuff is adapated from the machine-generated output from fdtrans.
 */
LONG CAPSInit (void)
{
    struct Library *LibBase = (struct Library *)CapsImageBase;
    LONG retval;
    ULONG *regs   = (ULONG *)(SysBase->EmuWS);
    ULONG save_A6 = regs[14];

    retval = (LONG)EmulateTags ((APTR)LibBase,
				ET_Offset,      -42, /* Hex: -0x2A */
				ET_RegisterA6,  LibBase,
				TAG_DONE);
    regs[14] = save_A6;
    return retval;
}

LONG CAPSExit (void)
{
    struct Library *LibBase = (struct Library *)CapsImageBase;
    LONG retval;
    ULONG *regs   = (ULONG *)(SysBase->EmuWS);
    ULONG save_A6 = regs[14];

    retval = (LONG)EmulateTags ((APTR)LibBase,
				ET_Offset,      -48, /* Hex: -0x30 */
				ET_RegisterA6,  LibBase,
				TAG_DONE);
    regs[14] = save_A6;
    return retval;
}

LONG CAPSAddImage (void)
{
    struct Library *LibBase = (struct Library *)CapsImageBase;
    LONG retval;
    ULONG *regs   = (ULONG *)(SysBase->EmuWS);
    ULONG save_A6 = regs[14];

    retval = (LONG)EmulateTags ((APTR)LibBase,
				ET_Offset,      -54, /* Hex: -0x36 */
				ET_RegisterA6,  LibBase,
				TAG_DONE);
    regs[14] = save_A6;
    return retval;
}

LONG CAPSRemImage (LONG id)
{
    struct Library *LibBase = (struct Library*) CapsImageBase;
    LONG retval;
    ULONG *regs   = (ULONG *)(SysBase->EmuWS);
    ULONG save_A6 = regs[14];

    retval = (LONG)EmulateTags ((APTR)LibBase,
				ET_Offset,      -60, /* Hex: -0x3C */
				ET_RegisterD0,  id,
				ET_RegisterA6,  LibBase,
				TAG_DONE);
    regs[14] = save_A6;
    return retval;
}

CapsLong CAPSLockImage (CapsLong id, char *name)
{
    struct Library *LibBase = (struct Library*)CapsImageBase;
    LONG retval;
    ULONG *regs   = (ULONG *)(SysBase->EmuWS);
    ULONG save_a0 = regs[8];
    ULONG save_A6 = regs[14];

    retval = (LONG)EmulateTags ((APTR)LibBase,
				ET_Offset,      -66, /* Hex: -0x42 */
				ET_RegisterD0,  id,
				ET_RegisterA0,  name,
				ET_RegisterA6,  LibBase,
				TAG_DONE);
    regs[8]  = save_a0;
    regs[14] = save_A6;
    return retval;
}

LONG CAPSLockImageMemory (LONG id,
			  UBYTE * buffer,
			  ULONG length,
			  ULONG flag)
{
    struct Library *LibBase = (struct Library*)CapsImageBase;
    LONG retval;
    ULONG *regs   = (ULONG *)(SysBase->EmuWS);
    ULONG save_a0 = regs[8];
    ULONG save_d2 = regs[2];
    ULONG save_A6 = regs[14];

    retval = (LONG)EmulateTags ((APTR)LibBase,
				ET_Offset,      -72, /* Hex: -0x48 */
				ET_RegisterD0,  id,
				ET_RegisterA0,  buffer,
				ET_RegisterD1,  length,
				ET_RegisterD2,  flag,
				ET_RegisterA6,  LibBase,
				TAG_DONE);
    regs[8]  = save_a0;
    regs[2]  = save_d2;
    regs[14] = save_A6;
    return retval;
}

LONG CAPSUnlockImage (LONG id)
{
    struct Library *LibBase = (struct Library*)CapsImageBase;
    LONG retval;
    ULONG *regs   = (ULONG *)(SysBase->EmuWS);
    ULONG save_A6 = regs[14];

    retval = (LONG)EmulateTags ((APTR)LibBase,
				ET_Offset,      -78, /* Hex: -0x4E */
				ET_RegisterD0,  id,
				ET_RegisterA6,  LibBase,
				TAG_DONE);
    regs[14] = save_A6;
    return retval;
}

LONG CAPSLoadImage (LONG id, ULONG flag)
{
    struct Library *LibBase = (struct Library *)CapsImageBase;
    LONG retval;
    ULONG *regs   = (ULONG *)(SysBase->EmuWS);
    ULONG save_A6 = regs[14];

    retval = (LONG)EmulateTags ((APTR)LibBase,
				ET_Offset,      -84, /* Hex: -0x54 */
				ET_RegisterD0,  id,
				ET_RegisterD1,  flag,
				ET_RegisterA6,  LibBase,
				TAG_DONE);
    regs[14] = save_A6;
    return retval;
}

LONG CAPSGetImageInfo (struct CapsImageInfo * pi, LONG id)
{
    struct Library *LibBase = (struct Library*)CapsImageBase;
    LONG retval;
    ULONG *regs   = (ULONG *)(SysBase->EmuWS);
    ULONG save_a0 = regs[8];
    ULONG save_A6 = regs[14];

    retval = (LONG)EmulateTags ((APTR)LibBase,
				ET_Offset,      -90, /* Hex: -0x5A */
				ET_RegisterA0,  pi,
				ET_RegisterD0,  id,
				ET_RegisterA6,  LibBase,
				TAG_DONE);
    regs[8]  = save_a0;
    regs[14] = save_A6;
    return retval;
}

LONG CAPSLockTrack (struct CapsTrackInfo * pi,
		    LONG id,
		    ULONG cylinder,
		    ULONG head,
		    ULONG flag)
{
    struct Library *LibBase = (struct Library*)CapsImageBase;
    LONG retval;
    ULONG *regs   = (ULONG *)(SysBase->EmuWS);
    ULONG save_a0 = regs[8];
    ULONG save_d2 = regs[2];
    ULONG save_d3 = regs[3];
    ULONG save_A6 = regs[14];

    retval = (LONG) EmulateTags((APTR)LibBase,
				ET_Offset,      -96, /* Hex: -0x60 */
				ET_RegisterA0,  pi,
				ET_RegisterD0,  id,
				ET_RegisterD1,  cylinder,
				ET_RegisterD2,  head,
				ET_RegisterD3,  flag,
				ET_RegisterA6,  LibBase,
				TAG_DONE);
    regs[8]  = save_a0;
    regs[2]  = save_d2;
    regs[3]  = save_d3;
    regs[14] = save_A6;
    return retval;
}

LONG CAPSUnlockAllTracks (LONG id)
{
    struct Library *LibBase = (struct Library*)CapsImageBase;
    LONG retval;
    ULONG *regs   = (ULONG *)(SysBase->EmuWS);
    ULONG save_A6 = regs[14];

    retval = (LONG) EmulateTags ((APTR)LibBase,
				 ET_Offset,      -108, /* Hex: -0x6C */
				 ET_RegisterD0,  id,
				 ET_RegisterA6,  LibBase,
				 TAG_DONE);
    regs[14] = save_A6;
    return retval;
}

#endif


#ifdef AMIGA

#if 0
/* proto file is broken in current CAPS API */
#include <proto/capsimage.h>
#else
#if defined __GNUC__ && !defined __amigaos4__
#include <inline/capsimage.h>
#endif
#endif

#include <proto/exec.h>

static struct Device *CapsImageBase;

static struct MsgPort   *CAPS_MsgPort;
static struct IORequest *CAPS_IOReq;

static void unload_capslib (void)
{
    CloseDevice (CAPS_IOReq);
    DeleteIORequest (CAPS_IOReq);
    DeleteMsgPort (CAPS_MsgPort);
}

static int load_capslib (void)
{
    if ((CAPS_MsgPort = CreateMsgPort ())) {
	if ((CAPS_IOReq = CreateIORequest (CAPS_MsgPort, sizeof(struct IORequest)))) {
	    if (!OpenDevice(CAPS_NAME, 0, CAPS_IOReq, 0)) {
		CapsImageBase = CAPS_IOReq->io_Device;
		atexit (unload_capslib);
		if (CAPSInit () == imgeOk)
		    return 1;
		else
		    CloseDevice (CAPS_IOReq);
	    }
	    DeleteIORequest (CAPS_IOReq);
	}
	DeleteMsgPort (CAPS_MsgPort);
    }

    return 0;
}

#else

/*
 * Sorry, we don't know how to load the CAPSLib plug-in
 * on other systems yet . ..
 */
static int load_capslib (void)
{
    return 0;
}

#endif
#endif


#ifndef AMIGA

/*
 * Some defines so that we don't care that CAPSLib
 * isn't statically linked
 */
#define CAPSInit            capslib.CAPSInit
#define CAPSExit            capslib.CAPSExit
#define CAPSAddImage        capslib.CAPSAddImage
#define CAPSRemImage        capslib.CAPSRemImage
#define CAPSLockImage       capslib.CAPSLockImage
#define CAPSLockImageMemory capslib.CAPSLockImageMemory
#define CAPSUnlockImage     capslib.CAPSUnlockImage
#define CAPSLoadImage       capslib.CAPSLoadImage
#define CAPSGetImageInfo    capslib.CAPSGetImageInfo
#define CAPSLockTrack       capslib.CAPSLockTrack
#define CAPSUnlockTrack     capslib.CAPSUnlockTrack
#define CAPSUnlockAllTracks capslib.CAPSUnlockAllTracks
#define CAPSGetPlatformName capslib.CAPSGetPlatformName

#endif


/*
 * CAPS support proper starts here
 *
 * This is more or less a straight copy of Toni's Win32 code
 */
int caps_init (void)
{
    static int init, noticed;
    int i;

    if (init)
	return 1;

    if (!load_capslib ()) {
	write_log ("Failed to load CAPS plug-in.\n");
	if (noticed)
	    return 0;
	gui_message ("This disk image needs the C.A.P.S. plugin\n"
	             "which is available from\n"
	             "http//www.caps-project.org/download.shtml\n");
	noticed = 1;
	return 0;
    }
    init = 1;
    for (i = 0; i < 4; i++)
	caps_cont[i] = CAPSAddImage ();

    return 1;
}

void caps_unloadimage (int drv)
{
    if (!caps_locked[drv])
	return;
    CAPSUnlockAllTracks (caps_cont[drv]);
    CAPSUnlockImage (caps_cont[drv]);
    caps_locked[drv] = 0;
}

int caps_loadimage (struct zfile *zf, int drv, int *num_tracks)
{
    struct CapsImageInfo ci;
    int len, ret ;
    uae_u8 *buf;
    char s1[100];
    struct CapsDateTimeExt *cdt;

    if (!caps_init ())
	return 0;
    caps_unloadimage (drv);
    zfile_fseek (zf, 0, SEEK_END);
    len = zfile_ftell (zf);
    zfile_fseek (zf, 0, SEEK_SET);
    buf = xmalloc (len);
    if (!buf)
	return 0;
    if (zfile_fread (buf, len, 1, zf) == 0)
	return 0;
    ret = CAPSLockImageMemory(caps_cont[drv], buf, len, 0);
    free (buf);
    if (ret != imgeOk) {
	free (buf);
	return 0;
    }
    caps_locked[drv] = 1;
    CAPSGetImageInfo (&ci, caps_cont[drv]);
    *num_tracks = (ci.maxcylinder - ci.mincylinder + 1) * (ci.maxhead - ci.minhead + 1);
    CAPSLoadImage(caps_cont[drv], caps_flags);
    cdt = &ci.crdt;
    sprintf (s1, "%d.%d.%d %d:%d:%d", cdt->day, cdt->month, cdt->year, cdt->hour, cdt->min, cdt->sec);
    write_log ("caps: type:%d date:%s rel:%d rev:%d\n",
	       ci.type, s1, ci.release, ci.revision);
    return 1;
}

int caps_loadrevolution (uae_u16 *mfmbuf, int drv, int track, int *tracklength)
{
    static int revcnt;
    int rev, len, i;
    uae_u16 *mfm;
    struct CapsTrackInfo ci;

    revcnt++;
    CAPSLockTrack(&ci, caps_cont[drv], track / 2, track & 1, caps_flags);
    rev = revcnt % ci.trackcnt;
    len = ci.tracksize[rev];
    *tracklength = len * 8;
    mfm = mfmbuf;
    for (i = 0; i < (len + 1) / 2; i++) {
        uae_u8 *data = ci.trackdata[rev]+ i * 2;
        *mfm++ = 256 * *data + *(data + 1);
    }
    return 1;
}

int caps_loadtrack (uae_u16 *mfmbuf, uae_u16 *tracktiming, int drv, int track, int *tracklength, int *multirev)
{
    unsigned int i, len, type;
    uae_u16 *mfm;
    struct CapsTrackInfo ci;

    *tracktiming = 0;
    CAPSLockTrack(&ci, caps_cont[drv], track / 2, track & 1, caps_flags);
    mfm = mfmbuf;
    *multirev = (ci.type & CTIT_FLAG_FLAKEY) ? 1 : 0;
    if (ci.trackcnt > 1)
	*multirev = 1;
    type = ci.type & CTIT_MASK_TYPE;
    len = ci.tracksize[0];
    *tracklength = len * 8;
    for (i = 0; i < (len + 1) / 2; i++) {
        uae_u8 *data = ci.trackdata[0]+ i * 2;
        *mfm++ = 256 * *data + *(data + 1);
    }
#if 0
    {
	FILE *f=fopen("c:\\1.txt","wb");
	fwrite (ci.trackdata[0], len, 1, f);
	fclose (f);
    }
#endif
    if (ci.timelen > 0) {
	for (i = 0; i < ci.timelen; i++)
	    tracktiming[i] = (uae_u16)ci.timebuf[i];
    }
#if 0
    write_log ("caps: drive:%d track:%d flakey:%d multi:%d timing:%d type:%d\n",
	drv, track, *multirev, ci.trackcnt, ci.timelen, type);
#endif
    return 1;
}

#else /* #ifdef CAPS */

/* Stop OS X linker complaining about empty link library */

void caps_dummy (void);
void caps_dummy (void)
{
}

#endif

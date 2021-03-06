 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Support for Linux/USS sound
  * 
  * Copyright 1997 Bernd Schmidt
  */

extern int sound_fd;
extern uae_u16 sndbuffer[];
extern uae_u16 *sndbufpt;
extern int sndbufsize;

static __inline__ void check_sound_buffers (void)
{
    int delay;
    
    int size=(char *)sndbufpt - (char *)sndbuffer;
    if (size >= sndbufsize) {
#if 0
	ioctl(sound_fd,SNDCTL_DSP_GETODELAY,&delay);
	if (delay<sndbufsize*1+size) {
	    /* We are still OK */
	}
	else {
	    unsigned long long x=sndbufsize;
	    x*=syncbase;
	    x/=currprefs.sound_freq;
	    x/=(currprefs.sound_bits/8);
	    x/=(currprefs.stereo+1);
	    x/=16;  
	    /* Skip one 1/16 buffer worth of time. That's 6% we can slow
	       things down --- that should be enough! */
	    vsyncmintime+=x;
	    // fprintf(stderr,"Adding %llu ticks in sound\n",x);
	}
#endif
	write (sound_fd, sndbuffer, size);
	sndbufpt = sndbuffer;
    }
}

#define PUT_SOUND_BYTE(b) do { *(uae_u8 *)sndbufpt = b; sndbufpt = (uae_u16 *)(((uae_u8 *)sndbufpt) + 1); } while (0)
#define PUT_SOUND_WORD(b) do { *(uae_u16 *)sndbufpt = b; sndbufpt = (uae_u16 *)(((uae_u8 *)sndbufpt) + 2); } while (0)
#define PUT_SOUND_BYTE_LEFT(b) PUT_SOUND_BYTE(b)
#define PUT_SOUND_WORD_LEFT(b) PUT_SOUND_WORD(b)
#define PUT_SOUND_BYTE_RIGHT(b) PUT_SOUND_BYTE(b)
#define PUT_SOUND_WORD_RIGHT(b) PUT_SOUND_WORD(b)
#define SOUND16_BASE_VAL 0
#define SOUND8_BASE_VAL 128

#define DEFAULT_SOUND_MAXB 8192
#define DEFAULT_SOUND_MINB 8192
#define DEFAULT_SOUND_BITS 16
#define DEFAULT_SOUND_FREQ 44100
#define HAVE_STEREO_SUPPORT
#define HAVE_8BIT_AUDIO_SUPPORT


#define _GNU_SOURCE  // for PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#include "glgfx-config.h"
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

#include <GL/glu.h>

#include "glgfx.h"
#include "glgfx_intern.h"

/** Signal used for various glgfx timers, like the vsync emulation in
    glgfx_monitor */
int glgfx_signum = 0;

/** A copy of the old signal handler */
static struct sigaction old_sa;

pthread_mutex_t glgfx_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;


static void init_halffloat(void);


/** The global signal handler, used for vsync emulation */
static void glgfx_sighandler(int sig, siginfo_t * si, void* extra) {
  if (si->si_signo == glgfx_signum &&
      si->si_code == SI_TIMER && 
      si->si_ptr != NULL) {
    struct glgfx_monitor* monitor = (struct glgfx_monitor*) si->si_ptr;

    // Re-arm timer
    long ns = (long) (1e9 / (monitor->dotclock * 1000.0 /
			     (monitor->mode.htotal * monitor->mode.vtotal)));

    monitor->vsync_itimerspec.it_value.tv_nsec += ns;

    while (monitor->vsync_itimerspec.it_value.tv_nsec > 1e9) {
      monitor->vsync_itimerspec.it_value.tv_nsec -= 1e9;
      ++monitor->vsync_itimerspec.it_value.tv_sec;
    }
    
    timer_settime(monitor->vsync_timer, TIMER_ABSTIME, 
		  &monitor->vsync_itimerspec, NULL);

    // Wake up render thread
    pthread_cond_signal(&monitor->vsync_cond);
  }
}


bool glgfx_init_a(struct glgfx_tagitem const* tags) {
  struct glgfx_tagitem const* tag;
  bool rc = false;

  if (!XInitThreads()) {
    BUG("Failed to make Xlib thread safe!\n");
    return false;
  }

  glgfx_signum = SIGRTMIN + 0;

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_init_tag) tag->tag) {
      case glgfx_init_signal:
	glgfx_signum = tag->data;
	break;

      case glgfx_init_unknown:
      case glgfx_init_max:
	/* Make compiler happy */
	break;
    }
  }

  // Install our signal handler
  struct sigaction sa;
  sa.sa_sigaction = glgfx_sighandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_ONSTACK | SA_RESTART | SA_SIGINFO;

  if (sigaction(glgfx_signum, &sa, &old_sa) == 0) {
    rc = true;
  }

  // Init half-float tables
  init_halffloat();

  return rc;
}


void glgfx_cleanup() {
  // Restore signal handler to default
  sigaction(glgfx_signum, &old_sa, NULL);
}


int glgfx_getattrs_a(void* object, 
		     glgfx_getattr_proto* fn, 
		     struct glgfx_tagitem const* tags) {
  struct glgfx_tagitem const* tag;
  int count = 0;

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    if (fn(object, tag->tag, (intptr_t*) tag->data)) {
      ++count;
    }
  }

  return count;
}


void glgfx_checkerror(char const* func, char const* file, int line) {
  GLenum error = glGetError();

  if (error != 0) {
    char const* msg = (char const*) gluErrorString(error);
    
    BUG("OpenGL error %d %s:%d (%s): %s\n", error, file, line, func, msg);
    abort();
  }
}

struct glgfx_tagitem const* glgfx_nexttagitem(struct glgfx_tagitem const** taglist_ptr) {
  if (taglist_ptr == NULL || *taglist_ptr == NULL) {
    return NULL;
  }

  while (true) {
    switch ((*taglist_ptr)->tag) {
      case glgfx_tag_end:
	*taglist_ptr = NULL;
	return NULL;

      case glgfx_tag_more:
	*taglist_ptr = (struct glgfx_tagitem const*) (*taglist_ptr)->data;
	if (*taglist_ptr == NULL) {
	  return NULL;
	}
	break;

      case glgfx_tag_ignore:
	++(*taglist_ptr);
	break;

      case glgfx_tag_skip:
	*taglist_ptr += (*taglist_ptr)->data + 1;
	break;

      default: {
	struct glgfx_tagitem const* res = *taglist_ptr;

	++(*taglist_ptr);
	return res;
      }
    }
  }
}


static void _initELut(uint16_t eLut[]);
static uint32_t _halfToFloat (uint16_t y);

static uint16_t _eLut[1 << 9];
static union { float f; uint32_t i; } _toFloat[1 << 16];

static void init_halffloat(void) {
  int i;

  _initELut(_eLut);

  for (i = 0; i < (1 << 16); ++i) {
    _toFloat[i].i = _halfToFloat(i);
  }
}


///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

// Primary authors:
//     Florian Kainz <kainz@ilm.com>
//     Rod Bogart <rgb@ilm.com>


static void
_initELut (uint16_t eLut[])
{
  int i;

  for (i = 0; i < 0x100; i++)
  {
    int e = (i & 0x0ff) - (127 - 15);

    if (e <= 0 || e >= 30)
    {
      //
      // Special case
      //

      eLut[i]         = 0;
      eLut[i | 0x100] = 0;
    }
    else
    {
      //
      // Common case - normalized half, no exponent overflow possible
      //

      eLut[i]         =  (e << 10);
      eLut[i | 0x100] = ((e << 10) | 0x8000);
    }
  }
}

//---------------------------------------------------
// Interpret an unsigned short bit pattern as a half,
// and convert that half to the corresponding float's
// bit pattern.
//---------------------------------------------------

static uint32_t
_halfToFloat (uint16_t y)
{
  int s = (y >> 15) & 0x00000001;
  int e = (y >> 10) & 0x0000001f;
  int m =  y        & 0x000003ff;

  if (e == 0)
  {
    if (m == 0)
    {
      //
      // Plus or minus zero
      //

      return s << 31;
    }
    else
    {
      //
      // Denormalized number -- renormalize it
      //

      while (!(m & 0x00000400))
      {
	m <<= 1;
	e -=  1;
      }

      e += 1;
      m &= ~0x00000400;
    }
  }
  else if (e == 31)
  {
    if (m == 0)
    {
      //
      // Positive or negative infinity
      //

      return (s << 31) | 0x7f800000;
    }
    else
    {
      //
      // Nan -- preserve sign and significand bits
      //

      return (s << 31) | 0x7f800000 | (m << 13);
    }
  }

  //
  // Normalized number
  //

  e = e + (127 - 15);
  m = m << 13;

  //
  // Assemble s, e and m.
  //

  return (s << 31) | (e << 23) | m;
}


//------------------------------------------
// Half-to-float conversion via table lookup
//------------------------------------------

float glgfx_half2float(uint16_t h) {
  return _toFloat[h].f;
}


//-----------------------------------------------------
// Float-to-half conversion -- general case, including
// zeroes, denormalized numbers and exponent overflows.
//-----------------------------------------------------

uint16_t glgfx_float2half(float f) {
  union { float f; uint32_t i; } u;
  
  if (f == 0)
  {
    //
    // Common special case - zero.
    // For speed, we don't preserve the zero's sign.
    //

    return 0;
  }

  u.f = f;

  //
  // We extract the combined sign and exponent, e, from our
  // floating-point number, f.  Then we convert e to the sign
  // and exponent of the half number via a table lookup.
  //
  // For the most common case, where a normalized half is produced,
  // the table lookup returns a non-zero value; in this case, all
  // we have to do, is round f's significand to 10 bits and combine
  // the result with e.
  //
  // For all other cases (overflow, zeroes, denormalized numbers
  // resulting from underflow, infinities and NANs), the table
  // lookup returns zero, and we call a longer, non-inline function
  // to do the float-to-half conversion.
  //

  register int se = (u.i >> 23) & 0x000001ff;

  se = _eLut[se];

  if (se)
  {
    //
    // Simple case - round the significand and
    // combine it with the sign and exponent.
    //

    return se + (((u.i & 0x007fffff) + 0x00001000) >> 13);
  }

  //
  // Our floating point number, f, is represented by the bit
  // pattern in integer i.  Disassemble that bit pattern into
  // the sign, s, the exponent, e, and the significand, m.
  // Shift s into the position where it will go in in the
  // resulting half number.
  // Adjust e, accounting for the different exponent bias
  // of float and half (127 versus 15).
  //

  register int s =  (u.i >> 16) & 0x00008000;
  register int e = ((u.i >> 23) & 0x000000ff) - (127 - 15);
  register int m =   u.i        & 0x007fffff;

  //
  // Now reassemble s, e and m into a half:
  //

  if (e <= 0)
  {
    if (e < -10)
    {
      //
      // E is less than -10.  The absolute value of f is
      // less than HALF_MIN (f may be a small normalized
      // float, a denormalized float or a zero).
      //
      // We convert f to a half zero.
      //

      return 0;
    }

    //
    // E is between -10 and 0.  F is a normalized float,
    // whose magnitude is less than HALF_NRM_MIN.
    //
    // We convert f to a denormalized half.
    // 

    m = (m | 0x00800000) >> (1 - e);

    //
    // Round to nearest, round "0.5" up.
    //
    // Rounding may cause the significand to overflow and make
    // our number normalized.  Because of the way a half's bits
    // are laid out, we don't have to treat this case separately;
    // the code below will handle it correctly.
    // 

    if (m &  0x00001000)
      m += 0x00002000;

    //
    // Assemble the half from s, e (zero) and m.
    //

    return s | (m >> 13);
  }
  else if (e == 0xff - (127 - 15))
  {
    if (m == 0)
    {
      //
      // F is an infinity; convert f to a half
      // infinity with the same sign as f.
      //

      return s | 0x7c00;
    }
    else
    {
      //
      // F is a NAN; we produce a half NAN that preserves
      // the sign bit and the 10 leftmost bits of the
      // significand of f, with one exception: If the 10
      // leftmost bits are all zero, the NAN would turn 
      // into an infinity, so we have to set at least one
      // bit in the significand.
      //

      m >>= 13;
      return s | 0x7c00 | m | (m == 0);
    }
  }
  else
  {
    //
    // E is greater than zero.  F is a normalized float.
    // We try to convert f to a normalized half.
    //

    //
    // Round to nearest, round "0.5" up
    //

    if (m &  0x00001000)
    {
      m += 0x00002000;

      if (m & 0x00800000)
      {
	m =  0;		// overflow in significand,
	e += 1;		// adjust exponent
      }
    }

    //
    // Handle exponent overflow
    //

    if (e > 30)
    {
//      overflow ();	// Cause a hardware floating point overflow;
      return s | 0x7c00;	// if this returns, the half becomes an
    }   			// infinity with the same sign as f.

    //
    // Assemble the half from s, e and m.
    //

    return s | (e << 10) | (m >> 13);
  }
}

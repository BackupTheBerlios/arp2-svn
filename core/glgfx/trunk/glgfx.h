#ifndef ARP2_glgfx_glgfx_h
#define ARP2_glgfx_glgfx_h

#include <glgfx-config.h>

#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#else
# define bool  int
# define true  1
# define false 0
# define __bool_true_false_are_defined 1
#endif

#include <stdio.h>
#define BUG(...) printf(__VA_ARGS__)

#if defined(DEBUG)
# define D(x) (x)
#else
# define D(x)
#endif

enum glgfx_pixel_format {
  glgfx_pixel_unknown,
  glgfx_pixel_r8g8b8,		// RGB,  3 * UBYTE
  glgfx_pixel_r8g8b8a8,		// RGBA, 4 * UBYTE
  glgfx_pixel_b8g8r8,		// BGR,  3 * UBYTE
  glgfx_pixel_b8g8r8a8,		// BGRA, 4 * UBYTE
  glgfx_pixel_max
};

bool glgfx_create_monitors(void);
void glgfx_destroy_monitors(void);
bool glgfx_waitblit(void);
bool glgfx_waittof(void);

bool glopen(void);
void glclose(void);

#define max_monitors  8               // Four cards, two outputs/card max
extern int                   num_monitors;
extern struct glgfx_monitor* monitors[max_monitors];


#define check_error() glgfx_check_error(__PRETTY_FUNCTION__, __FILE__, __LINE__);
void glgfx_check_error(char const* func, char const* file, int line);

#endif /* ARP2_glgfx_glgfx_h */


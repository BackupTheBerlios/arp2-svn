#ifndef ARP2_glgfx_glgfx_h
#define ARP2_glgfx_glgfx_h

#include <stdbool.h>
#include <stdio.h>
#define BUG(...) printf(__VA_ARGS__)

#if defined(DEBUG)
# define D(x) (x)
#else
# define D(x)
#endif

enum glgfx_pixel_format {
  glgfx_pixel_unknown,
  glgfx_pixel_a4r4g4b4,
  glgfx_pixel_r5g6b5,
  glgfx_pixel_a1r5g5b5,
//  glgfx_pixel_r8g8b8,
  glgfx_pixel_a8b8g8r8,
//  glgfx_pixel_b8g8r8,
  glgfx_pixel_a8r8g8b8,
  glgfx_pixel_max
};

bool glgfx_create_monitors(void);
void glgfx_destroy_monitors(void);
bool glgfx_waitblit(void);
bool glgfx_waittof(void);

struct glgfx_monitor;
#define max_monitors  8               // Four cards, two outputs/card max
extern int                   glgfx_num_monitors;
extern struct glgfx_monitor* glgfx_monitors[max_monitors];


#define check_error() glgfx_check_error(__PRETTY_FUNCTION__, __FILE__, __LINE__);
void glgfx_check_error(char const* func, char const* file, int line);

#endif /* ARP2_glgfx_glgfx_h */


#ifndef ARP2_glgfx_glgfx_h
#define ARP2_glgfx_glgfx_h

#define HAVE_STDBOOL_H

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

bool glgfx_create_monitors(void);
void glgfx_destroy_monitors(void);

bool glopen(void);
void glclose(void);

#endif /* ARP2_glgfx_glgfx_h */


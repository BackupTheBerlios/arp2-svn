
#include <glib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "glgfx_bitmap.h"
#include "glgfx_input.h"
#include "glgfx_monitor.h"
#include "glgfx_sprite.h"

#include "pointer.c"
#include "selected.c"
#include "unselected.c"
#include "Prairie Wind.c"

int main(int argc, char** argv) {
  int rc = 0;
  struct glgfx_monitor* monitor;

  (void) argc;
  (void) argv;

  if (!glgfx_init(glgfx_tag_end)) {
    printf("Unable to initialize glgfx\n");
    return 20;
  }

  monitor = glgfx_monitor_create(getenv("DISPLAY"),
				 glgfx_monitor_attr_fullscreen, true,
				 glgfx_tag_end);

  if (monitor == NULL) {
    printf("Unable to open display\n");
  }
  else {
    intptr_t width, height;

    if (glgfx_getattrs(monitor,
		       (glgfx_getattr_proto*) glgfx_monitor_getattr,
		       glgfx_monitor_attr_width,  (intptr_t) &width,
		       glgfx_monitor_attr_height, (intptr_t) &height,
		       glgfx_tag_end) != 2) {
      printf("Unable to get display dimensions\n");
      rc = 20;
    }
    else {
      struct glgfx_bitmap* background;

      struct glgfx_bitmap* pointer;
      struct glgfx_bitmap* tile;
      struct glgfx_bitmap* selected;
      struct glgfx_bitmap* unselected;

      pointer = glgfx_bitmap_create(glgfx_bitmap_attr_width,  sprite_pointer.width,
				    glgfx_bitmap_attr_height, sprite_pointer.height,
				    glgfx_bitmap_attr_format, glgfx_pixel_format_a8b8g8r8,
				    glgfx_tag_end);

      background = glgfx_bitmap_create(glgfx_bitmap_attr_width,  width,
				       glgfx_bitmap_attr_height, height,
				       glgfx_bitmap_attr_format, glgfx_pixel_format_a8b8g8r8,
				       glgfx_tag_end);

      tile = glgfx_bitmap_create(glgfx_bitmap_attr_width,  tile_background.width,
				 glgfx_bitmap_attr_height, tile_background.height,
				 glgfx_bitmap_attr_format, glgfx_pixel_format_a8b8g8r8,
				 glgfx_tag_end);

      selected = glgfx_bitmap_create(glgfx_bitmap_attr_width,  window_selected.width,
				     glgfx_bitmap_attr_height, window_selected.height,
				     glgfx_bitmap_attr_format, glgfx_pixel_format_a8b8g8r8,
				     glgfx_tag_end);

      unselected = glgfx_bitmap_create(glgfx_bitmap_attr_width,  window_unselected.width,
				       glgfx_bitmap_attr_height, window_unselected.height,
				       glgfx_bitmap_attr_format, glgfx_pixel_format_a8b8g8r8,
				       glgfx_tag_end);

      if (background == NULL || pointer == NULL ||
	  tile == NULL || selected == NULL || unselected == NULL) {
	printf("Unable to allocate bitmaps\n");
	rc = 20;
      }
      else {
	struct glgfx_sprite* sp = glgfx_sprite_create(
	  glgfx_sprite_attr_width,  sprite_pointer.width*2,
	  glgfx_sprite_attr_height, sprite_pointer.height*2,
	  glgfx_sprite_attr_bitmap, (intptr_t) pointer,
	  glgfx_tag_end);

	struct glgfx_viewport* vp =
	  glgfx_viewport_create(glgfx_viewport_attr_width,  width,
				glgfx_viewport_attr_height, height,
				glgfx_tag_end);

	
	// Add background
	struct glgfx_rasinfo* ri_background =
	  glgfx_viewport_addbitmap(vp, background,
				   glgfx_tag_end);

	// Add "windows"
	struct glgfx_rasinfo* ri_unselected2 =
	  glgfx_viewport_addbitmap(vp, unselected,
				   glgfx_rasinfo_attr_x,      -500,
				   glgfx_rasinfo_attr_y,      -120,
				   glgfx_rasinfo_attr_blend_equation, glgfx_blend_equation_func_add,
				   glgfx_tag_end);

	struct glgfx_rasinfo* ri_selected =
	  glgfx_viewport_addbitmap(vp, selected,
				   glgfx_rasinfo_attr_x,      -400,
				   glgfx_rasinfo_attr_y,      -100,
				   glgfx_rasinfo_attr_blend_equation, glgfx_blend_equation_func_add,
				   glgfx_tag_end);

	struct glgfx_rasinfo* ri_unselected1 =
	  glgfx_viewport_addbitmap(vp, unselected,
				   glgfx_rasinfo_attr_x,      -450,
				   glgfx_rasinfo_attr_y,      -110,
				   glgfx_rasinfo_attr_height, window_unselected.height * 2,
				   glgfx_rasinfo_attr_blend_equation, glgfx_blend_equation_func_add,
				   glgfx_tag_end);


	struct glgfx_view* v = glgfx_view_create();

	if (vp == NULL || ri_background == NULL || ri_selected == NULL ||
	    ri_unselected1 == NULL ||  ri_unselected2 == NULL ||
	    v == NULL ||
	    !glgfx_view_addsprite(v, sp) ||
	    !glgfx_view_addviewport(v, vp) ||
	    !glgfx_monitor_addview(monitor, v) ||
	    !glgfx_monitor_loadview(monitor, v)) {
	  printf("Unable to create view/viewport\n");
	  rc = 20;
	}
	else {
	  // Load and blit tile to background
	  glgfx_bitmap_write(tile,
			     glgfx_bitmap_copy_width,   tile_background.width,
			     glgfx_bitmap_copy_height,  tile_background.height,

			     glgfx_bitmap_copy_format,  glgfx_pixel_format_a8b8g8r8,
			     glgfx_bitmap_copy_data,    (uintptr_t) tile_background.pixel_data,
			     glgfx_tag_end);

	  int x, y;

	  for (y = 0; y < height; y += tile_background.height) {
	    for (x = 0; x < width; x += tile_background.width) {
	      glgfx_bitmap_blit(background,
				glgfx_bitmap_blit_x,          x,
				glgfx_bitmap_blit_y,          y,
				glgfx_bitmap_blit_width,      tile_background.width,
				glgfx_bitmap_blit_height,     tile_background.height,
				glgfx_bitmap_blit_src_bitmap, (uintptr_t) tile,
				glgfx_bitmap_blit_src_x,      0, 
				glgfx_bitmap_blit_src_y,      0, 
				// "Destroy" alpha channels so we can
				// verify that non-blended rasports work too
				glgfx_bitmap_blit_mod_a,      0x4000, 
				glgfx_tag_end);
	    }
	  }


	  // Load windows
	  glgfx_bitmap_write(selected,
			     glgfx_bitmap_copy_width,   window_selected.width,
			     glgfx_bitmap_copy_height,  window_selected.height,

			     glgfx_bitmap_copy_format,  glgfx_pixel_format_a8b8g8r8,
			     glgfx_bitmap_copy_data,    (uintptr_t) window_selected.pixel_data,
			     glgfx_tag_end);

	  glgfx_bitmap_write(unselected,
			     glgfx_bitmap_copy_width,   window_unselected.width,
			     glgfx_bitmap_copy_height,  window_unselected.height,

			     glgfx_bitmap_copy_format,  glgfx_pixel_format_a8b8g8r8,
			     glgfx_bitmap_copy_data,    (uintptr_t) window_unselected.pixel_data,
			     glgfx_tag_end);

	  // Load cursor
	  glgfx_bitmap_write(pointer,
			     glgfx_bitmap_copy_width,   sprite_pointer.width,
			     glgfx_bitmap_copy_height,  sprite_pointer.height,

			     glgfx_bitmap_copy_format,  glgfx_pixel_format_a8b8g8r8,
			     glgfx_bitmap_copy_data,    (uintptr_t) sprite_pointer.pixel_data,
			     glgfx_tag_end);

	  // Test one rendering before locking input, in case we crash ...
	  glgfx_monitor_render(monitor);

	  // Handle input
	  glgfx_input_acquire(monitor);
	  
	  bool quit = false;
	  bool drag = false;
	  intptr_t win_x = 0, win_y = 0;
	  int mouse_x = 0, mouse_y = 0;
	  double phase1 = 0, phase2 = 0;

	  glgfx_rasinfo_getattr(ri_selected, glgfx_rasinfo_attr_x, &win_x);
	  glgfx_rasinfo_getattr(ri_selected, glgfx_rasinfo_attr_y, &win_y);

	  while (!quit) {
	    enum glgfx_input_code code;

	    glgfx_monitor_render(monitor);

	    while ((code = glgfx_input_getcode()) != glgfx_input_none) {
	      if ((code & glgfx_input_typemask) == glgfx_input_mouse_xyz) {
		int8_t dx = ((code & glgfx_input_valuemask) >> 0) & 0xff;
		int8_t dy = ((code & glgfx_input_valuemask) >> 8) & 0xff;
		
		int old_mouse_x = mouse_x;
		int old_mouse_y = mouse_y;

		mouse_x += dx;
		mouse_y += dy;

		if (mouse_x < 0) mouse_x = 0;
		if (mouse_y < 0) mouse_y = 0;
		if (mouse_x >= width)  mouse_x = width - 1;
		if (mouse_y >= height) mouse_y = height -1;

		if (drag) {
		  win_x -= mouse_x - old_mouse_x;
		  win_y -= mouse_y - old_mouse_y;
		  
		  glgfx_rasinfo_setattrs(ri_selected,
					 glgfx_rasinfo_attr_x, win_x,
					 glgfx_rasinfo_attr_y, win_y,
					 glgfx_tag_end);
		}
	      }
	      else if ((code & glgfx_input_typemask) == glgfx_input_mouse_button) {
		if ((code & glgfx_input_valuemask) == 0) {
		  if (code & glgfx_input_releasemask) {
		    drag = false;
		  }
		  else if (mouse_x >= -win_x && mouse_x < -win_x + window_selected.width &&
			   mouse_y >= -win_y && mouse_y < -win_y + 24) {
		    drag = true;
		  }
		}
		else if ((code & glgfx_input_valuemask) == 1) {
		  quit = true;
		}
	      }
	    }

	    glgfx_sprite_setattrs(sp, 
				  glgfx_sprite_attr_x, mouse_x,
				  glgfx_sprite_attr_y, mouse_y,
/* 				  glgfx_sprite_attr_width, 500, */
/* 				  glgfx_sprite_attr_height, 500, */
				  glgfx_tag_end);

	    phase1 += 0.01;
	    phase2 += 0.0377;

	    if (phase1 > 2 * M_PI) { 
	      phase1 -= 2 * M_PI;
	    }

	    if (phase2 > 2 * M_PI) { 
	      phase2 -= 2 * M_PI;
	    }

	    glgfx_rasinfo_setattrs(ri_unselected1,
				   glgfx_rasinfo_attr_x, -500 + 50 * sin(phase1),
				   glgfx_rasinfo_attr_y, -110 + 20 * cos(phase2),
				   glgfx_tag_end);
	    
	  }

	  glgfx_input_release(monitor);
	}

	glgfx_view_destroy(v);
	glgfx_viewport_destroy(vp);
	glgfx_sprite_destroy(sp);
      }

      glgfx_bitmap_destroy(background);
      glgfx_bitmap_destroy(tile);
      glgfx_bitmap_destroy(selected);
      glgfx_bitmap_destroy(unselected);
    }

    glgfx_monitor_destroy(monitor);
  }

  glgfx_cleanup();

  return rc;
}


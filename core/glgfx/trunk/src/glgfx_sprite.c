
#include "glgfx-config.h"
#include <stdlib.h>
#include <GL/gl.h>

#include "glgfx.h"
#include "glgfx_sprite.h"
#include "glgfx_intern.h"

#ifndef GL_TEXTURE_RECTANGLE_ARB
# define GL_TEXTURE_RECTANGLE_ARB GL_TEXTURE_RECTANGLE_EXT
#endif

struct glgfx_sprite* glgfx_sprite_create_a(struct glgfx_tagitem const* tags) {
  struct glgfx_sprite* sprite;

  sprite = calloc(1, sizeof (*sprite));

  if (sprite == NULL) {
    return NULL;
  }
  
  if (!glgfx_sprite_setattrs_a(sprite, tags)) {
    glgfx_sprite_destroy(sprite);
    return NULL;
  }

  return sprite;
}


void glgfx_sprite_destroy(struct glgfx_sprite* sprite) {
  if (sprite == NULL) {
    return;
  }

  free(sprite);
}


bool glgfx_sprite_setattrs_a(struct glgfx_sprite* sprite,
			     struct glgfx_tagitem const* tags) {
  struct glgfx_tagitem const* tag;

  if (sprite == NULL) {
    return false;
  }

  pthread_mutex_lock(&glgfx_mutex);

  while ((tag = glgfx_nexttagitem(&tags)) != NULL) {
    switch ((enum glgfx_sprite_attr) tag->tag) {
      case glgfx_sprite_attr_width:
	sprite->width = tag->data;
	break;

      case glgfx_sprite_attr_height:
	sprite->height = tag->data;
	break;

      case glgfx_sprite_attr_x:
	sprite->x = tag->data;
	break;

      case glgfx_sprite_attr_y:
	sprite->y = tag->data;
	break;

      case glgfx_sprite_attr_bitmap:
	sprite->bitmap = (struct glgfx_bitmap*) tag->data;
	break;

      case glgfx_sprite_attr_unknown:
      case glgfx_sprite_attr_max:
	/* Make compiler happy */
	break;
    }
  }

  bool rc = (sprite->width > 0 && 
	     sprite->height > 0 && 
	     sprite->bitmap != NULL);

  pthread_mutex_unlock(&glgfx_mutex);
  return rc;
}


bool glgfx_sprite_getattr(struct glgfx_sprite* sprite,
			    enum glgfx_sprite_attr attr,
			    intptr_t* storage) {
  if (sprite == NULL || storage == NULL ) {
    return false;
  }

  switch (attr) {
    case glgfx_sprite_attr_width:
      *storage = sprite->width;
      break;

    case glgfx_sprite_attr_height:
      *storage = sprite->height;
      break;

    case glgfx_sprite_attr_x:
      *storage = sprite->x;
      break;

    case glgfx_sprite_attr_y:
      *storage = sprite->y;
      break;

    case glgfx_sprite_attr_bitmap:
      *storage = (intptr_t) sprite->bitmap;
      break;

    default:
      return false;
  }

  return true;
}

bool glgfx_sprite_haschanged(struct glgfx_sprite* sprite) {
  bool has_changed = sprite->has_changed;

  sprite->has_changed = false;

  // Always call glgfx_bitmap_haschanged, since we want to reset its
  // has_changed flag.
  return glgfx_bitmap_haschanged(sprite->bitmap) || sprite->has_changed;
}

bool glgfx_sprite_render(struct glgfx_sprite* sprite) {
  glColor4f(1,1,1,1);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);

  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, sprite->bitmap->texture);

  glBegin(GL_QUADS);
  glTexCoord2i(0, 0);
  glVertex3f(sprite->x,
	     sprite->y, 0);
  glTexCoord2i(sprite->bitmap->width,
	       0);
  glVertex3f(sprite->x + sprite->width,
	     sprite->y, 0);
  glTexCoord2i(sprite->bitmap->width,
	       sprite->bitmap->height);
  glVertex3f(sprite->x + sprite->width,
	     sprite->y + sprite->height, 0);
  glTexCoord2i(0,
	       sprite->bitmap->height);
  glVertex3f(sprite->x,
	     sprite->y + sprite->height, 0);
  glEnd();
  GLGFX_CHECKERROR();

  return true;
}

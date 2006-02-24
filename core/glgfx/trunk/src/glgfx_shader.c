
#include "glgfx-config.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h> // get memcmp
#include <GL/gl.h>
#include <GL/glext.h>

#include "glgfx.h"
#include "glgfx_intern.h"

/** A hash table with all compiled shader programs */
static GHashTable* shader_hash;

enum read_functions {
  readRGBPixel,
  readRGBPixelFP32,
  num_read_functions
};

static char const* read_shader_source[num_read_functions] = {
  "vec4 readPixel(uniform samplerRect tex, vec2 pos) {"
  "  return textureRect(tex, pos);"
  "}",

  "vec4 readPixel(uniform samplerRect tex, vec2 pos) {"
  "  return textureRect(tex, pos);" // We could read four pixels and
				    // interpolate here if we really
				    // want
  "}",
};

static int read_shader_funcs[glgfx_pixel_format_max] = {
  -1,

  readRGBPixel,		// glgfx_pixel_a4r4g4b4
  readRGBPixel,		// glgfx_pixel_r5g6b5
  readRGBPixel,		// glgfx_pixel_a1r5g5b5
  readRGBPixel,		// glgfx_pixel_a8b8g8r8
  readRGBPixel,		// glgfx_pixel_a8r8g8b8

  readRGBPixel,		// glgfx_pixel_r16g16b16a16f
  readRGBPixelFP32	// glgfx_pixel_r32g32b32a32f
};


enum write_functions {
  writeRGBPixel,
  num_write_functions
};

static char const* write_shader_source[num_write_functions] = {
  "void writePixel(int mrt, vec4 color) {"
  "  gl_FragData[mrt] = color;"
  "}",
};

static int write_shader_funcs[glgfx_pixel_format_max] = {
  -1,

  writeRGBPixel,	// glgfx_pixel_a4r4g4b4
  writeRGBPixel,	// glgfx_pixel_r5g6b5
  writeRGBPixel,	// glgfx_pixel_a1r5g5b5
  writeRGBPixel,	// glgfx_pixel_a8b8g8r8
  writeRGBPixel,	// glgfx_pixel_a8r8g8b8

  writeRGBPixel,	// glgfx_pixel_r16g16b16a16f
  writeRGBPixel		// glgfx_pixel_r32g32b32a32f
};


struct key {
    enum read_functions src_func;
    enum write_functions dst_func;
    char const* source;
};


static guint hashfunc(gconstpointer v) {
  struct key* key = (struct key*) v;
  
  return (g_direct_hash((gconstpointer) (intptr_t) 
			((key->src_func << 16) | key->dst_func)) +
	  g_direct_hash(key->source));
}


static gint comparefunc(gconstpointer v1, gconstpointer v2) {
  struct key const* k1 = (struct key*) v1;
  struct key const* k2 = (struct key*) v2;

  return memcmp(k1, k2, sizeof (*k1)) == 0;
}



bool glgfx_shader_init() {
  shader_hash = g_hash_table_new(hashfunc, comparefunc);

  if (shader_hash == NULL) {
    BUG("Failed to create shader hash table!\n");
    return false;
  }

  return true;
}

void glgfx_shader_cleanup() {
  glgfx_shader_removeprogram(NULL);
  g_hash_table_destroy(shader_hash);
}


GLuint glgfx_shader_getprogram(enum glgfx_pixel_format src, 
			       enum glgfx_pixel_format dst,
			       char const* shader) {
  struct key const key = {
    read_shader_funcs[src],
    write_shader_funcs[dst],
    shader
  };

  GLuint program = GPOINTER_TO_UINT(g_hash_table_lookup(shader_hash, &key));

  if (program == 0) {
    // Compile and link program 

    // Insert program into hash table
    struct key* k = malloc(sizeof (*k));
    *k = key;
    g_hash_table_insert(shader_hash, (gpointer) k, GUINT_TO_POINTER(program));
  }

  return program;
}


void glgfx_shader_removeprogram(char const* shader) {
  gboolean remove(gpointer _key, gpointer value, gpointer userdata) {
    (void) userdata;

    struct key* key = _key;

    if (key->source == shader || shader == NULL) {
      free(key);
      // TODO: Delete GLSL program and shader objects here!!
      return true;
    }
    else {
      return false;
    }
  }

  g_hash_table_foreach_remove(shader_hash, remove, NULL);
}



/*     GLcharARB const* source =  */
/*       "void main(void) {\n" */
/*       "  gl_FragColor = vec4(1.0, 1.0, 1.0, 0.5);\n" */
/*       "}"; */

/*     GLhandleARB program = glCreateProgramObjectARB(); */
/*     GLhandleARB shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB); */
    
/*     glShaderSourceARB(shader, 1, &source, NULL); */
/*     glCompileShaderARB(shader); */
/*     glAttachObjectARB(program, shader); */
/*     glLinkProgramARB(program); */

/*     void printInfoLog(GLhandleARB obj) { */
/*       int infologLength = 0; */
/*       int charsWritten  = 0; */
/*       char *infoLog; */

/*       glGetObjectParameterivARB(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB, */
/* 				&infologLength); */

/*       if (infologLength > 0) { */
/* 	infoLog = (char*) malloc(infologLength); */
/* 	glGetInfoLogARB(obj, infologLength, &charsWritten, infoLog); */
/* 	printf("%s\n",infoLog); */
/* 	free(infoLog); */
/*       } */
/*     } */

/*     printInfoLog(program); */



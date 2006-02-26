
#include "glgfx-config.h"
#include <glib.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include "glgfx.h"
#include "glgfx_glext.h"
#include "glgfx_intern.h"

struct shader raw_texture_blitter = {
  .fragment = 
  "uniform samplerRect tex;"
  ""
  "void main() {"
  "   gl_FragColor = textureRect(tex, gl_TexCoord[0].xy);"
  "}"
};

struct shader color_blitter = {
  .fragment = 
  "void main() {"
  "   writePixel(0, gl_Color);"
  "}"
};


struct shader plain_texture_blitter = {
  .fragment = 
  "uniform samplerRect tex;"
  ""
  "void main() {"
  "   writePixel(0, readPixel(tex, gl_TexCoord[0].xy));"
  "}"
};

struct shader modulated_texture_blitter = {
  .fragment = 
  "uniform samplerRect tex;"
  ""
  "void main() {"
  "   writePixel(0, gl_Color * readPixel(tex, gl_TexCoord[0].xy));"
  "}"
};

static char const* read_shader_source[shader_function_read_max] = {
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

  shader_function_read_rgba,		// glgfx_pixel_a4r4g4b4
  shader_function_read_rgba,		// glgfx_pixel_r5g6b5
  shader_function_read_rgba,		// glgfx_pixel_a1r5g5b5
  shader_function_read_rgba,		// glgfx_pixel_a8b8g8r8
  shader_function_read_rgba,		// glgfx_pixel_a8r8g8b8

  shader_function_read_rgba,		// glgfx_pixel_r16g16b16a16f
  shader_function_read_fp32rgba		// glgfx_pixel_r32g32b32a32f
};



static char const* write_shader_source[shader_function_write_max] = {
  "void writePixel(int mrt, vec4 color) {"
//  "  gl_FragData[mrt] = color;"
  "  gl_FragColor = color;"
  "}",
};

static int write_shader_funcs[glgfx_pixel_format_max] = {
  -1,

  shader_function_write_rgba,	// glgfx_pixel_a4r4g4b4
  shader_function_write_rgba,	// glgfx_pixel_r5g6b5
  shader_function_write_rgba,	// glgfx_pixel_a1r5g5b5
  shader_function_write_rgba,	// glgfx_pixel_a8b8g8r8
  shader_function_write_rgba,	// glgfx_pixel_a8r8g8b8

  shader_function_write_rgba,	// glgfx_pixel_r16g16b16a16f
  shader_function_write_rgba	// glgfx_pixel_r32g32b32a32f
};


static void bug_infolog(GLuint obj) {
  GLint len = 0;
  char* log = NULL;

  if (glIsShader(obj)) {
    glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &len); 
    log = malloc(len);
    glGetShaderInfoLog(obj, len, NULL, log);
  }
  else if (glIsProgram(obj)) {
    glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &len); 
    log = malloc(len);
    glGetProgramInfoLog(obj, len, NULL, log);
  }
  else {
    abort();
  }

  if (len > 0) {
    BUG("Info log: %s\n", log);
  }

  free(log);
}

static bool init_table(struct shader* shader) {
  int s, d;

  for (s = 0; s < shader_function_read_max; ++s) {
    for (d = 0; d < shader_function_write_max; ++d) {
      GLuint program = glCreateProgram();

      if (shader->fragment != NULL) {
	char const* source[3] = {
	  read_shader_source[s],
	  write_shader_source[d],
	  shader->fragment
	};

	GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(fragment, 3, source, NULL);
	glCompileShader(fragment);
	GLGFX_CHECKERROR();

	GLint res = GL_FALSE;
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &res);

	if (!res) {
	  BUG("Failed to compile this shader:\n%s\n%s\n%s\n",
	      source[0], source[1], source[2]);
	  bug_infolog(fragment);
	}

	glAttachShader(program, fragment);
	GLGFX_CHECKERROR();
      }

      if (shader->vertex != NULL) {
	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);

	glShaderSource(vertex, 1, &shader->vertex, NULL);
	glCompileShader(vertex);
	GLGFX_CHECKERROR();

	GLint res = GL_FALSE;
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &res);

	if (!res) {
	  BUG("Failed to compile this vertex shader:\n%s\n\n",
	      shader->vertex);
	  bug_infolog(vertex);
	}

	glAttachShader(program, vertex);
	GLGFX_CHECKERROR();
      }

      glLinkProgram(program);      

      GLint res = GL_FALSE;
      glGetProgramiv(program, GL_LINK_STATUS, &res);

      if (!res) {
	BUG("Failed to link shader %p [vertex: %s; fragment: %s]\n", 
	    shader, shader->vertex, shader->fragment);
	bug_infolog(program);
      }

      GLGFX_CHECKERROR();
      shader->programs[s * shader_function_write_max + d] = program;
    }
  }
  
  return true;
}

static void destroy_table(struct shader* shader) {
  // TODO: Destroy table here
}


bool glgfx_shader_init() {
  static bool initialized = false;

  if (initialized) {
    return true;
  }

  initialized = (init_table(&raw_texture_blitter) &&
		 init_table(&color_blitter) &&
		 init_table(&plain_texture_blitter) &&
		 init_table(&modulated_texture_blitter));

  return initialized;
}


void glgfx_shader_cleanup() {
  destroy_table(&raw_texture_blitter);
  destroy_table(&color_blitter);
  destroy_table(&plain_texture_blitter);
  destroy_table(&modulated_texture_blitter);
}


GLuint glgfx_shader_getprogram(enum glgfx_pixel_format src, 
			       enum glgfx_pixel_format dst,
			       struct shader* shader) {
  return shader->programs[read_shader_funcs[src] * shader_function_write_max +
			  write_shader_funcs[dst]];
}

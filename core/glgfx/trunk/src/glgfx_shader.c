
#include "glgfx-config.h"
#include <glib.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include "glgfx.h"
#include "glgfx_glext.h"
#include "glgfx_intern.h"

struct shader color_blitter = {
  .channels = 1,

  .fragment = 
  "void main() {"
  "   writePixel0(gl_Color);"
  "}"
};


struct shader raw_texture_blitter = {
/*   .vertex =  */
/*   "void main() {" */
/*   "  gl_Position = gl_Vertex;" // No transformation */
/*   "}", */

  .channels = 0,

  .fragment = 
  "uniform samplerRect tex;"
  ""
  "void main() {"
  "   gl_FragColor = textureRect(tex, gl_TexCoord[0].xy);"
  "}"
};


struct shader plain_texture_blitter = {
  .channels = 2,

  .fragment = 
  "void main() {"
  "   writePixel0(readPixel0(gl_TexCoord[0].xy));"
  "}"
};

struct shader color_texture_blitter = {
  .channels = 2,

  .fragment = 
  "void main() {"
  "   writePixel0(gl_Color * readPixel0(gl_TexCoord[0].xy));"
  "}"
};


struct shader modulated_texture_blitter = {
  .channels = 3,

  .fragment = 
  "void main() {"
  "   writePixel0(gl_Color *"
  "               readPixel0(gl_TexCoord[0].xy) *"
  "               readPixel1(gl_TexCoord[1].xy));"
  "}"
};

static char const* read_shader_source[shader_function_read_max] = {
  "uniform samplerRect tex%d;"
  "vec4 readPixel%d(vec2 pos) {"
  "  return textureRect(tex%d, pos);"
  "}",

  "uniform samplerRect tex%d;"
  "vec4 readPixel%d(vec2 pos) {"
  "  return textureRect(tex%d, pos);" // We could read four pixels and
				         // interpolate here if we really
				         // want
  "}",
};

static GLuint read0_shader_objects[shader_function_read_max];
static GLuint read1_shader_objects[shader_function_read_max];


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
  "void writePixel0(vec4 color) {"
  "  gl_FragColor = color;" // gl_FragData[0] is not supported on NV3x
  "}",
};

static GLuint write_shader_objects[shader_function_write_max];

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


static GLuint compile_fragment_shader(char const* main_src) {
  char const* source[] = {
    "void writePixel0(vec4 rgba);\n"
    "vec4 readPixel0(vec2 pos);\n"
    "vec4 readPixel1(vec2 pos);\n"
    "\n",
    main_src
  };

  GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);

  glShaderSource(fragment, 2, source, NULL);
  glCompileShader(fragment);
  GLGFX_CHECKERROR();

  GLint res = GL_FALSE;
  glGetShaderiv(fragment, GL_COMPILE_STATUS, &res);

  if (!res) {
    BUG("Failed to compile this shader:\n%s\n", main_src);
    bug_infolog(fragment);
    glDeleteShader(fragment);
  }

  return fragment;
}


static GLuint link(struct shader* shader, 
		   GLuint vertex, GLuint fragment, 
		   GLuint src0, GLuint src1, GLuint dst) {
  GLuint program = glCreateProgram(); 

  if (vertex != 0) {
    glAttachShader(program, vertex);
    GLGFX_CHECKERROR();
  }

  if (fragment != 0) {
    glAttachShader(program, fragment);
    GLGFX_CHECKERROR();
  }

  if (src0 != 0) {
    glAttachShader(program, src0);
    GLGFX_CHECKERROR();
  }

  if (src1 != 0) {
    glAttachShader(program, src1);
    GLGFX_CHECKERROR();
  }

  if (dst != 0) {
    glAttachShader(program, dst);
    GLGFX_CHECKERROR();
  }

  glLinkProgram(program);      

  GLint res = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &res);

  if (!res) {
    BUG("Failed to link shader %p [vertex: %s; fragment: %s]\n", 
	shader, shader->vertex, shader->fragment);
    bug_infolog(program);
    glDeleteProgram(program);
    return 0;
  }

  GLGFX_CHECKERROR();
  return program;
}



static bool init_table(struct shader* shader) {
  switch (shader->channels) {
    case 0:
      shader->programs = malloc(sizeof (*shader->programs));
      break;

    case 1:
      shader->programs = malloc(sizeof (*shader->programs) * 
				shader_function_write_max);
      break;

    case 2:
      shader->programs = malloc(sizeof (*shader->programs) * 
				shader_function_write_max *
				shader_function_read_max);
      break;

    case 3:
      shader->programs = malloc(sizeof (*shader->programs) * 
				shader_function_write_max *
				shader_function_read_max *
				shader_function_read_max);
      break;

    default:
      abort();
  }

  if (shader->programs == NULL) {
    BUG("Failed to allocate memory for shader program table\n");
    return false;
  }


  GLuint fragment = 0;
  GLuint vertex   = 0;

  if (shader->fragment != NULL) {
    fragment = compile_fragment_shader(shader->fragment);
  }

  if (shader->vertex != NULL) {
    vertex = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertex, 1, &shader->vertex, NULL);
    glCompileShader(vertex);
    GLGFX_CHECKERROR();

    GLint res = GL_FALSE;
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &res);

    if (!res) {
      BUG("Failed to compile this vertex shader:\n%s\n",
	  shader->vertex);
      bug_infolog(vertex);
      glDeleteShader(vertex);
    }
  }


  switch (shader->channels) {
    case 0:
      shader->programs[0] = link(shader, vertex, fragment, 0, 0, 0);

      if (shader->programs[0] == 0) {
	return false;
      }
      break;

    case 1: {
      int d;

      for (d = 0; d < shader_function_write_max; ++d) {
	shader->programs[d] = link(shader, vertex, fragment, 
				   0, 
				   0, 
				   write_shader_objects[d]);
	if (shader->programs[d] == 0) {
	  return false;
	}
      }
      break;
    }

    case 2: {
      int s, d;

      for (s = 0; s < shader_function_read_max; ++s) {
	for (d = 0; d < shader_function_write_max; ++d) {
	  int idx = (s * shader_function_write_max +
		     d);

	  shader->programs[idx] = 
	    link(shader, vertex, fragment, 
		 read0_shader_objects[s], 
		 0, 
		 write_shader_objects[d]);

	  if (shader->programs[idx] == 0) {
	    return false;
	  }
	}
      }
      break;
    }

    case 3: {
      int s1, s2, d;

      for (s1 = 0; s1 < shader_function_read_max; ++s1) {
	for (s2 = 0; s2 < shader_function_read_max; ++s2) {
	  for (d = 0; d < shader_function_write_max; ++d) {
	    int idx = (s2 * shader_function_write_max * shader_function_read_max +
		       s1 * shader_function_write_max +
		       d);

	    shader->programs[idx] = 
	      link(shader, vertex, fragment, 
		   read0_shader_objects[s1], 
		   read1_shader_objects[s2], 
		   write_shader_objects[d]);

	    if (shader->programs[idx] == 0) {
	      return false;
	    }
	  }
	}
      }
      break;
    }

    default:
      abort();
  }

  return true;
}

static void destroy_table(struct shader* shader) {
  // TODO: Destroy table here
}


bool glgfx_shader_init() {
  int i;
  static bool initialized = false;

  if (initialized) {
    return true;
  }

  char main_src[4096];

  for (i = 0; i < shader_function_read_max; ++i) {
    snprintf(main_src, sizeof (main_src), read_shader_source[i], 0, 0, 0);
    read0_shader_objects[i] = compile_fragment_shader(main_src);

    if (read0_shader_objects[i] == 0) {
      return false;
    }
  }

  for (i = 0; i < shader_function_read_max; ++i) {
    snprintf(main_src, sizeof (main_src), read_shader_source[i], 1, 1, 1);
    read1_shader_objects[i] = compile_fragment_shader(main_src);

    if (read1_shader_objects[i] == 0) {
      return false;
    }
  }

  for (i = 0; i < shader_function_write_max; ++i) {
    write_shader_objects[i] = compile_fragment_shader(write_shader_source[i]);

    if (write_shader_objects[i] == 0) {
      return false;
    }
  }

  initialized = (init_table(&raw_texture_blitter) &&
		 init_table(&color_blitter) &&
		 init_table(&plain_texture_blitter) &&
		 init_table(&color_texture_blitter) &&
		 init_table(&modulated_texture_blitter));

  return initialized;
}


void glgfx_shader_cleanup() {
  destroy_table(&raw_texture_blitter);
  destroy_table(&color_blitter);
  destroy_table(&plain_texture_blitter);
  destroy_table(&color_texture_blitter);
  destroy_table(&modulated_texture_blitter);
}


GLuint glgfx_shader_getprogram(enum glgfx_pixel_format src0, 
			       enum glgfx_pixel_format src1,
			       enum glgfx_pixel_format dst,
			       struct shader* shader) {
  int s0 = read_shader_funcs[src0];
  int s1 = read_shader_funcs[src1];
  int d = write_shader_funcs[dst];

  if (shader->channels <= 2) {
    s1 = 0;
  }

  if (shader->channels <= 1) {
    s0 = 0;
  }

  int idx = (s1 * shader_function_write_max * shader_function_read_max +
	     s0 * shader_function_write_max +
	     d);

  return shader->programs[idx];
}

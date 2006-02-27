
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
  "   writePixel(0, gl_Color);"
  "}"
};


struct shader raw_texture_blitter = {
/*   .vertex =  */
/*   "void main() {" */
/*   "  gl_Position = gl_Vertex;" // No transformation */
/*   "}", */

  .channels = 2,

  .fragment = 
  "uniform samplerRect tex[2];"
  ""
  "void main() {"
  "   gl_FragColor = textureRect(tex[0], gl_TexCoord[0].xy);"
  "}"
};


struct shader plain_texture_blitter = {
  .channels = 2,

  .fragment = 
  "uniform samplerRect tex[2];"
  ""
  "void main() {"
  "   writePixel(0, readPixel(tex, gl_TexCoord[0].xy));"
  "}"
};

struct shader color_texture_blitter = {
  .channels = 2,

  .fragment = 
  "uniform samplerRect tex[2];"
  ""
  "void main() {"
  "   writePixel(0, gl_Color * readPixel(tex, gl_TexCoord[0].xy));"
  "}"
};


struct shader modulated_texture_blitter = {
  .channels = 3,

  .fragment = 
  "uniform samplerRect tex[2];"
  "uniform samplerRect mod[2];"
  ""
  "void main() {"
  "   writePixel(0, gl_Color *"
  "                 readPixel(tex, gl_TexCoord[0].xy) *"
  "                 readPixel(mod, gl_TexCoord[1].xy));"
  "}"
};

static char const* read_shader_source[shader_function_read_max] = {
  "vec4 readPixel(uniform samplerRect tex[2], vec2 pos) {"
  "  return textureRect(tex[0], pos);"
  "}",

  "vec4 readPixel(uniform samplerRect tex[2], vec2 pos) {"
  "  return textureRect(tex[0], pos);" // We could read four pixels and
				    // interpolate here if we really
				    // want
  "}",
};

static GLuint read_shader_objects[shader_function_read_max];


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

static GLuint compile_fragment_shader(char const* source) {
  GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);

  glShaderSource(fragment, 1, &source, NULL);
  glCompileShader(fragment);
  GLGFX_CHECKERROR();

  GLint res = GL_FALSE;
  glGetShaderiv(fragment, GL_COMPILE_STATUS, &res);

  if (!res) {
    BUG("Failed to compile this shader:\n%s\n", source);
    bug_infolog(fragment);
    glDeleteShader(fragment);
  }

  return fragment;
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


  GLuint link(GLuint vertex, GLuint fragment, 
	      GLuint src1, GLuint src2, GLuint dst) {
    GLuint program = glCreateProgram(); 
    
    if (vertex != 0) {
      glAttachShader(program, vertex);
      GLGFX_CHECKERROR();
    }

    if (fragment != 0) {
      glAttachShader(program, fragment);
      GLGFX_CHECKERROR();
    }

    if (src1 != 0) {
      glAttachShader(program, src1);
      GLGFX_CHECKERROR();
    }

    if (src1 != 0) {
      glAttachShader(program, src1);
      GLGFX_CHECKERROR();
    }

    if (src2 != 0) {
      glAttachShader(program, src2);
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

  
  printf("shader %p %s %s %d\n", shader, shader->vertex, shader->fragment, shader->channels);

  switch (shader->channels) {
    case 0:
      shader->programs[0] = link(vertex, fragment, 0, 0, 0);

      if (shader->programs[0] == 0) {
	return false;
      }
      break;

    case 1: {
      int d;

      for (d = 0; d < shader_function_write_max; ++d) {
	shader->programs[d] = link(vertex, fragment, 
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
	  shader->programs[s * shader_function_write_max + d] = 
	    link(vertex, fragment, 
		 read_shader_objects[s], 
		 0, 
		 write_shader_objects[d]);

	  if (shader->programs[s * shader_function_write_max + d] == 0) {
	    return false;
	  }
	}
      }
      break;
      }

    case 3:
      break;

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

  for (i = 0; i < shader_function_read_max; ++i) {
    read_shader_objects[i] = compile_fragment_shader(read_shader_source[i]);

    if (read_shader_objects[i] == 0) {
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

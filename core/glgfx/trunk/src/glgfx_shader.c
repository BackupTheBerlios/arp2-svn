
#include "glgfx-config.h"
#include <glib.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include "glgfx.h"
#include "glgfx_glext.h"
#include "glgfx_intern.h"


enum shader_function_read {
  shader_function_read_rgba,
  shader_function_read_max
};

enum shader_function_write {
  shader_function_write_rgba,
  shader_function_write_max
};


struct shader {
    GLuint* programs;
    GLint   tex0a, tex0b;
    GLint   tex1a, tex1b;
    GLint   tex_scale0, tex_scale1;
    int channels;
    char const* vertex;
    char const* fragment;
};


struct shader color_blitter = {
  .channels = 1,

  .vertex =
  "varying vec4 color;\n" // gl_FrontColor is clamped!
  "\n"
  "void main() {\n"
  "  color = gl_Color;\n"
  "  gl_FrontColor = gl_Color;\n" // FIXME: NVIDIA driver bug?
  "  gl_Position = positionTransform();\n"
  "}\n",

  .fragment =
  "varying vec4 color;\n"
  "\n"
  "void main() {"
  "   writePixel0(gl_Color);" // FIXME: NVIDIA driver bug?
  "}",

};


struct shader raw_texture_blitter = {
  .channels = 0,

  .vertex =
  "void main() {\n"
  "  gl_TexCoord[0] = textureTransform0(gl_MultiTexCoord0);\n"
  "  gl_Position = positionTransform();\n"
  "}\n",

  .fragment = 
  "uniform SAMPLER tex0a, tex0b;\n"
  "\n"
  "void main() {\n"
  "   gl_FragColor = TEXTURE(tex0a, gl_TexCoord[0].xy);\n"
  "}\n"
};


struct shader plain_texture_blitter = {
  .channels = 2,

  .vertex =
  "void main() {\n"
  "  gl_TexCoord[0] = textureTransform0(gl_MultiTexCoord0);\n"
  "  gl_Position = positionTransform();\n"
  "}\n",

  .fragment = 
  "void main() {\n"
  "   writePixel0(readPixel0(gl_TexCoord[0].xy));\n"
  "}\n"
};

struct shader color_texture_blitter = {
  .channels = 2,

  .vertex = 
  "varying vec4 color;\n" // gl_FrontColor is clamped!
  "\n"
  "void main() {\n"
  "  color = gl_Color;\n"
  "  gl_TexCoord[0] = textureTransform0(gl_MultiTexCoord0);\n"
  "  gl_Position = positionTransform();\n"
  "}\n",

  .fragment = 
  "varying vec4 color;\n"
  "\n"
  "void main() {\n"
  "   writePixel0(color * readPixel0(gl_TexCoord[0].xy));\n"
  "}\n"
};


struct shader modulated_texture_blitter = {
  .channels = 3,

  .vertex = 
  "varying vec4 color;\n" // gl_FrontColor is clamped!
  "\n"
  "void main() {\n"
  "  color = gl_Color;\n"
  "  gl_TexCoord[0] = textureTransform0(gl_MultiTexCoord0);\n"
  "  gl_TexCoord[2] = textureTransform1(gl_MultiTexCoord2);\n"
  "  gl_Position = positionTransform();\n"
  "}\n",

  .fragment = 
  "varying vec4 color;\n"
  "\n"
  "void main() {\n"
  "   writePixel0(color *\n"
  "               readPixel0(gl_TexCoord[0].xy) *\n"
  "               readPixel1(gl_TexCoord[2].xy));\n"
  "}\n"
};

static char const* read_shader_source[shader_function_read_max] = {
  "uniform SAMPLER tex%da, tex%db;\n"
  "\n"
  "vec4 readPixel%d(vec2 pos) {\n"
  "  return TEXTURE(tex%da, pos);\n"
  "}\n"
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
  shader_function_read_rgba		// glgfx_pixel_r32g32b32a32f
};



static char const* write_shader_source[shader_function_write_max] = {
  "void writePixel0(vec4 color) {\n"
  "  gl_FragColor = color;\n" // gl_FragData[0] is not supported on NV3x
  "}\n",
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


static GLuint compile_fragment_shader(char const* main_src, 
				      struct glgfx_monitor* monitor) {
  char texture_rectangle[128];
  char const* source[] = {
    "void writePixel0(vec4 rgba);\n"
    "vec4 readPixel0(vec2 pos);\n"
    "vec4 readPixel1(vec2 pos);\n",
    texture_rectangle,
    main_src
  };

  snprintf(texture_rectangle, sizeof (texture_rectangle),
	   "#define HAVE_GL_ARB_TEXTURE_RECTANGLE %d\n"
	   "#define SAMPLER %s\n"
	   "#define TEXTURE %s\n",
	   monitor->have_GL_ARB_texture_rectangle,
	   monitor->have_GL_ARB_texture_rectangle ? "samplerRect" : "sampler2D",
	   monitor->have_GL_ARB_texture_rectangle ? "textureRect" : "texture2D");

  GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);

  glShaderSource(fragment, sizeof (source) / sizeof (source[0]), source, NULL);
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


static GLuint link(struct shader* shader, struct glgfx_monitor* monitor,
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

  if (src0 != 0) {
    shader->tex0a = glGetUniformLocation(program, "tex0a");
    shader->tex0b = glGetUniformLocation(program, "tex0b");
  }
  else {
    shader->tex0a = shader->tex0b = -2;
  }

  if (src1 != 0) {
    shader->tex1a = glGetUniformLocation(program, "tex1a");
    shader->tex1b = glGetUniformLocation(program, "tex1b");
  }
  else {
    shader->tex1a = shader->tex1b = -2;
  }

  if (!monitor->have_GL_ARB_texture_rectangle) {
    shader->tex_scale0 = glGetUniformLocation(program, "texScale0");
    shader->tex_scale1 = glGetUniformLocation(program, "texScale1");
  }
  else {
    shader->tex_scale0 = -2;
    shader->tex_scale1 = -2;
  }


  printf("shader %p: %d %d %d %d (scale at %d %d)\n", 
	 shader,
	 shader->tex0a, shader->tex0b,
	 shader->tex1a, shader->tex1b,
	 shader->tex_scale0, shader->tex_scale1);
  GLGFX_CHECKERROR();

  return program;
}



static bool init_table(struct shader* shader, struct glgfx_monitor* monitor) {
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
    fragment = compile_fragment_shader(shader->fragment, monitor);
  }

  if (shader->vertex != NULL) {
    char texture_rectangle[64];
    char const* source[] = {
      // Define HAVE_GL_ARB_TEXTURE_RECTANGLE
      texture_rectangle,

      // Standard projection transform
      "vec4 positionTransform() {\n"
      "  return ftransform();\n"
      "}\n",

      // Texture coordinate transform
      "#if HAVE_GL_ARB_TEXTURE_RECTANGLE\n"

      "vec4 textureTransform0(vec4 tc) {\n"
      "  return tc;\n"
      "}\n"
      "vec4 textureTransform1(vec4 tc) {\n"
      "  return tc;\n"
      "}\n"

      "#else\n"

      "uniform vec4 texScale0, texScale1;\n"
      "\n"
      "vec4 textureTransform0(vec4 tc) {\n"
      "  return tc * texScale0;"
      "}\n"
      "vec4 textureTransform1(vec4 tc) {\n"
      "  return tc * texScale1;"
      "}\n"

      "#endif\n",

      // Main source file
      shader->vertex
    };

    snprintf(texture_rectangle, sizeof (texture_rectangle),
	     "#define HAVE_GL_ARB_TEXTURE_RECTANGLE %d\n", 
	     monitor->have_GL_ARB_texture_rectangle);

    vertex = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertex, sizeof (source) / sizeof (source[0]), source, NULL);
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
      shader->programs[0] = link(shader, monitor, vertex, fragment, 0, 0, 0);

      if (shader->programs[0] == 0) {
	return false;
      }
      break;

    case 1: {
      int d;

      for (d = 0; d < shader_function_write_max; ++d) {
	shader->programs[d] = link(shader, monitor, vertex, fragment, 
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
	    link(shader, monitor, vertex, fragment, 
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
	      link(shader, monitor, vertex, fragment, 
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


bool glgfx_shader_init(struct glgfx_monitor* monitor) {
  int i;
  static bool initialized = false;

  if (initialized) {
    return true;
  }

  char main_src[4096];

  for (i = 0; i < shader_function_read_max; ++i) {
    snprintf(main_src, sizeof (main_src), read_shader_source[i], 0, 0, 0, 0);
    read0_shader_objects[i] = compile_fragment_shader(main_src, monitor);

    if (read0_shader_objects[i] == 0) {
      return false;
    }
  }

  for (i = 0; i < shader_function_read_max; ++i) {
    snprintf(main_src, sizeof (main_src), read_shader_source[i], 1, 1, 1, 1);
    read1_shader_objects[i] = compile_fragment_shader(main_src, monitor);

    if (read1_shader_objects[i] == 0) {
      return false;
    }
  }

  for (i = 0; i < shader_function_write_max; ++i) {
    write_shader_objects[i] = compile_fragment_shader(write_shader_source[i], monitor);

    if (write_shader_objects[i] == 0) {
      return false;
    }
  }

  initialized = (init_table(&raw_texture_blitter, monitor) &&
		 init_table(&color_blitter, monitor) &&
		 init_table(&plain_texture_blitter, monitor) &&
		 init_table(&color_texture_blitter, monitor) &&
		 init_table(&modulated_texture_blitter, monitor));

  return initialized;
}


void glgfx_shader_cleanup() {
  destroy_table(&raw_texture_blitter);
  destroy_table(&color_blitter);
  destroy_table(&plain_texture_blitter);
  destroy_table(&color_texture_blitter);
  destroy_table(&modulated_texture_blitter);
}


GLuint glgfx_shader_load(struct glgfx_bitmap* src_bm0,
			 struct glgfx_bitmap* src_bm1,
			 enum glgfx_pixel_format dst,
			 struct shader* shader,
			 struct glgfx_monitor* monitor) {
  enum glgfx_pixel_format src0 = glgfx_pixel_format_a8r8g8b8;
  enum glgfx_pixel_format src1 = glgfx_pixel_format_a8r8g8b8;

  if (src_bm0 != NULL) {
    src0 = src_bm0->format;
  }

  if (src_bm1 != NULL) {
    src1 = src_bm1->format;
  }

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

  if (shader->programs[idx] != 0) {
    glUseProgram(shader->programs[idx]);

    if (src_bm0 != NULL) {
      if (shader->tex0a >= 0) {
	glUniform1i(shader->tex0a, 0);
	GLGFX_CHECKERROR();
      }

      if (shader->tex0b >= 0) {
	glUniform1i(shader->tex0b, 1);
	GLGFX_CHECKERROR();
      }

      if (!monitor->have_GL_ARB_texture_rectangle) {
	glUniform4f(shader->tex_scale0, 
		    1.0f / src_bm0->width, 1.0f / src_bm0->height, 1, 1);
	GLGFX_CHECKERROR();
      }
    }

    if (src_bm1 != NULL) {
      if (shader->tex1a >= 0) {
	glUniform1i(shader->tex1a, 2);
	GLGFX_CHECKERROR();
      }

      if (shader->tex1b >= 0) {
	glUniform1i(shader->tex1b, 3);
	GLGFX_CHECKERROR();
      }

      if (!monitor->have_GL_ARB_texture_rectangle) {
	glUniform4f(shader->tex_scale1, 
		    1.0f / src_bm1->width, 1.0f / src_bm1->height, 1, 1);
	GLGFX_CHECKERROR();
      }
    }

    GLGFX_CHECKERROR();
  }
  
  return shader->programs[idx];
}

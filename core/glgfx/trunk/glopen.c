
#include <stdlib.h>
#include <dlfcn.h>

#include "glgfx.h"

#pragma pack(1)
struct tramp {
    unsigned char jmp;
    void*         addr;
};
#pragma pack()

struct symdef {
    char* const   name;
    struct tramp* tramp;
};

#define GLFUNC(name) extern struct tramp name; \
                     __asm__(".section \"glopen_tramp\", \"awx\"; \
			      .globl " #name ";" \
			     #name ": jmp glopen_error");

#include "glfuncs.h"

#undef GLFUNC
#define GLFUNC(name) { #name, &name },

static const struct symdef symdefs[] = {
#include "glfuncs.h"
  { NULL, NULL }
};





static void* libgl = NULL;

void glopen_error() {
  printf("You forgot to call glopen!\n");
  abort();
}

bool glopen() {
  int i;

  if (libgl != NULL) {
    return 1;
  }

  printf("opening\n");
  libgl = dlopen("libGL.so", RTLD_LAZY | RTLD_GLOBAL);

  if (libgl == NULL) {
    BUG("Unable to open 'libGL.so'.\n");
    return false;
  }

  printf("opened\n");

  for (i = 0; symdefs[i].name != NULL; ++i) {
    void* addr = dlsym(libgl, symdefs[i].name);

    if (addr == NULL) {
      BUG("Unable to find symbol '%s' in 'libGL.so'.\n");
//      return false;
    }
    
    symdefs[i].tramp->addr = ((char*) addr -
			      (char*) symdefs[i].tramp - 5);
  }
//  glclose();

  char cmd[128];
  sprintf(cmd,"cat /proc/%d/maps", getpid());
  system(cmd);
  return true;
}

void glclose(void) {
  int i;

  for (i = 0; symdefs[i].name != NULL; ++i) {
    symdefs[i].tramp->addr = ((char*) &glopen_error -
			      (char*) symdefs[i].tramp - 5);
  }

  if (libgl != NULL) {
    dlclose(libgl);
  }
}

#include <stdio.h>

int
unlink(const char *path)
{
  return remove( path );
}

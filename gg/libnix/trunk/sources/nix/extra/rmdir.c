#include <stdio.h>

int
rmdir(const char *path)
{
  return remove( path );
}

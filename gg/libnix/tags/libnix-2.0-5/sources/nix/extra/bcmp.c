#include <string.h>

int
bcmp(const void *b1, const void *b2, size_t len)
{
  return memcmp( b1, b2, len );
}

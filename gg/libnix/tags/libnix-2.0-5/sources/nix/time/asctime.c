#include <time.h>

static char buf[26];

char *asctime(const struct tm *t)
{ strftime(buf,sizeof(buf),"%C\n",t);
  return buf;
}

/*
char *ctime(const time_t *t)
{ return asctime(localtime(t)); }
*/

char *ctime(const time_t *t)
{ strftime(buf,sizeof(buf),"%C\n",localtime(t));
  return buf;
}

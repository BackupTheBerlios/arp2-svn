/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$Id: localeconv.c,v 1.1.1.1 2000/05/07 19:36:27 emm Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/localedef.h>
#include <locale.h>

/*
 * Return the current locale conversion.
 */
struct lconv *
localeconv()
{
    static struct lconv ret = {
	".",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX
    };

    return (&ret);
}

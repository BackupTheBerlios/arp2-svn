/* ngettext - retrieve plural form strings from message catalog and print them.
   Copyright (C) 1995-1997, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>

#include "error.h"
#include "system.h"

#define HAVE_SETLOCALE 1
/* Make sure we use the included libintl, not the system's one. */
#define textdomain textdomain__
#define bindtextdomain bindtextdomain__
#define gettext gettext__
#define dngettext dngettext__
#undef _LIBINTL_H
#include "libgnuintl.h"

#define _(str) gettext (str)

/* Name the program is called with.  */
char *program_name;

/* Long options.  */
static const struct option long_options[] =
{
  { "domain", required_argument, NULL, 'd' },
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'V' },
  { NULL, 0, NULL, 0 }
};

/* Prototypes for local functions.  */
static void usage PARAMS ((int __status))
#if defined __GNUC__ && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 5) || __GNUC__ > 2)
     __attribute__ ((noreturn))
#endif
;

int
main (argc, argv)
     int argc;
     char *argv[];
{
  int optchar;
  const char *msgid;
  const char *msgid_plural;
  const char *count;
  unsigned long n;

  /* Default values for command line options.  */
  int do_help = 0;
  int do_version = 0;
  const char *domain = getenv ("TEXTDOMAIN");
  const char *domaindir = getenv ("TEXTDOMAINDIR");

  /* Set program name for message texts.  */
  program_name = argv[0];

#ifdef HAVE_SETLOCALE
  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");
#endif

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  /* Parse command line options.  */
  while ((optchar = getopt_long (argc, argv, "+d:hV", long_options, NULL))
	 != EOF)
    switch (optchar)
    {
    case '\0':		/* Long option.  */
      break;
    case 'd':
      domain = optarg;
      break;
    case 'h':
      do_help = 1;
      break;
    case 'V':
      do_version = 1;
      break;
    default:
      usage (EXIT_FAILURE);
    }

  /* Version information is requested.  */
  if (do_version)
    {
      printf ("%s (GNU %s) %s\n", basename (program_name), PACKAGE, VERSION);
      /* xgettext: no-wrap */
      printf (_("Copyright (C) %s Free Software Foundation, Inc.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"),
	      "1995-1997, 2000, 2001");
      printf (_("Written by %s.\n"), "Ulrich Drepper");
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  /* More optional command line options.  */
  if (argc - optind <= 2)
    error (EXIT_FAILURE, 0, _("missing arguments"));

  /* Now the mandatory command line options.  */
  msgid = argv[optind++];
  msgid_plural = argv[optind++];

  /* If no domain name is given we print the original string.
     We mark this assigning NULL to domain.  */
  if (domain == NULL || domain[0] == '\0')
    domain = NULL;
  else
    /* Bind domain to appropriate directory.  */
    if (domaindir != NULL && domaindir[0] != '\0')
      bindtextdomain (domain, domaindir);

  /* To speed up the plural-2 test, we accept more than one COUNT in one
     call.  */
  while (optind < argc)
    {
      count = argv[optind++];

      {
	char *endp;
	unsigned long tmp_val;

	errno = 0;
	tmp_val = strtoul (count, &endp, 10);
	if (errno == 0 && count[0] != '\0' && endp[0] == '\0')
	  n = tmp_val;
	else
	  /* When COUNT is not valid, use plural.  */
	  n = 99;
      }

      /* If no domain name is given we don't translate, and we use English
	 plural form handling.  */
      if (domain == NULL)
	fputs (n == 1 ? msgid : msgid_plural, stdout);
      else
	/* Write out the result.  */
	fputs (dngettext (domain, msgid, msgid_plural, n), stdout);
    }

  exit (EXIT_SUCCESS);
}


/* Display usage information and exit.  */
static void
usage (status)
     int status;
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      /* xgettext: no-wrap */
      printf (_("\
Usage: %s [OPTION] MSGID MSGID-PLURAL COUNT...\n\
  -d, --domain=TEXTDOMAIN   retrieve translated message from TEXTDOMAIN\n\
  -h, --help                display this help and exit\n\
  -V, --version             display version information and exit\n\
  MSGID MSGID-PLURAL        translate MSGID (singular) / MSGID-PLURAL (plural)\n\
  COUNT                     choose singular/plural form based on this value\n"),
	      program_name);
      /* xgettext: no-wrap */
      printf (_("\
\n\
If the TEXTDOMAIN parameter is not given, the domain is determined from the\n\
environment variable TEXTDOMAIN.  If the message catalog is not found in the\n\
regular directory, another location can be specified with the environment\n\
variable TEXTDOMAINDIR.\n\
Standard search directory: %s\n"), LOCALEDIR);
      fputs (_("Report bugs to <bug-gnu-utils@gnu.org>.\n"), stdout);
    }

  exit (status);
}

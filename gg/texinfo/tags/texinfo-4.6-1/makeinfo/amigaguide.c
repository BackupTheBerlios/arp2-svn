
#include "system.h"
#include "amigaguide.h"
#include "cmds.h"
#include "makeinfo.h"

/* **************************************************************** */
/*                                                                  */
/*                  Amiga extras                                    */
/*                                                                  */
/* **************************************************************** */

/* Non-zero indicates that we're converting to AmigaGuide hypertext,
   instead of plain info. The numerical value indicates what specific
   version we're targeting. Currently, the accepted values are 34, 39
   and 40.  */
int amiga_guide = 0;

/* Non-zero indicates we're writing an AmigaGuide button.  */
int amiga_guide_writing_button = 0;

/* Count nonprinting characters in titles.  */
int amiga_guide_hide_title_chars = 0;

/* Count nonprinting characters, that may affect linebreaks.  */
int amiga_guide_nonprinting_chars = 0;

/* When converting menus, output the gadget text in accordance with
   this format string.  */
char amiga_guide_menu_gadgets[12] = "\" %-25s \"\0";

#ifdef ENABLE_AMIGAGUIDE

/* Tweak the menu into a series of AmigaGuide buttons. Ideally, the entire
   menu would be read and queued before output, so the exact width of the
   link buttons can be calculated to the correct (largest) size. But for
   now, we use a fixed width, which hopefully isn't too bad.  */

#ifndef MENU_STARTER
#define MENU_STARTER "* "
#endif

int
amiga_guide_convert_menu ()
{
  int orig_offset, ccpos, dotpos, lfpos;
  char *gadgetname = NULL;	/* Presented text */
  char *nodename = NULL;	/* Node that's linked to */
  char *filename = NULL;	/* .. in this file */

  if (strncmp (&input_text[input_text_offset + 1], MENU_STARTER, strlen
	       (MENU_STARTER)) != 0)
    return (0);
  else
    input_text_offset += strlen (MENU_STARTER) + 1;

  orig_offset = input_text_offset;

  /* Remove leading whitespace before entry.  */
  while (whitespace (curchar()))
    input_text_offset++;

  ccpos = search_forward ("::", input_text_offset);
  dotpos = search_forward (".", input_text_offset);
  lfpos = search_forward ("\n", input_text_offset);

  if ((ccpos > 0) && (ccpos < lfpos))
    /* Node and entry are identical.  */
    {
      if (curchar() == '(')	/* This is a link to another file.  */
	{
	  input_text_offset++;
	  get_until_in_line (0, ")", &filename);

	  if (curchar() != ')')
	    {
	      /* This doesn't look good. Chicken out.  */
	      input_text_offset = orig_offset;
	      free (filename);
	      return (0);
	    }

	  input_text_offset++;
	}

      get_until_in_line (0, "::", &nodename);
      input_text_offset++ ;
    }
  else if ((dotpos > 0) && (dotpos < lfpos)) /* Distinct names.  */
    {
      get_until_in_line (0, ":", &gadgetname);

      input_text_offset++;
      while (whitespace (curchar()))
	input_text_offset++;

      if (curchar() == '(')	/* This is a link to another file.  */
	{
	  input_text_offset++;
	  get_until_in_line (0, ")", &filename);

	  if (curchar() != ')')
	    {
	      /* This doesn't look good. Chicken out.  */
	      input_text_offset = orig_offset;
	      free (filename);
	      free (gadgetname);
	      return (0);
	    }

	  input_text_offset++;
	}

      get_until_in_line (0, ".", &nodename);
      input_text_offset++;
    }
  else
    {
      /* This doesn't look like a valid menu entry. Back up, and enter
	 it as plain text.  */

      input_text_offset = orig_offset;
      return (0);
    }

  /* Skip trailing whitespace.  */
  while (whitespace (curchar()))
    input_text_offset++;

  if (filename) {
      canon_white (filename);
      if (!filename[0]) {
	free (filename);
	filename = NULL;
      }
  }

  if (nodename) {
    canon_white (nodename);
    if (!nodename[0]) {
      free (nodename);
      nodename = NULL;
    }
  }

  if ((nodename == NULL) && (filename == NULL))
    {
      /* Nothing to link to.  */
      input_text_offset = orig_offset;
      free (gadgetname);
      return (0);
    }

  normalize_node_name(nodename);

  add_word (" ");
  amiga_guide_add_link (amiga_guide_menu_gadgets, gadgetname, nodename, filename);
  add_word ("  ");

  if (filename)
    free (filename);
  if (nodename)
    free (nodename);
  if (gadgetname)
    free (gadgetname);

  return (1);
}

 /* Add an AmigaGuide link, with the entire load of bells, whistles
    and gongs. This includes adding the extension '.guide' to any
    filenames, using 'Main' instead of 'Top' and removing any nasty
    characters from the presented text.  */

void
amiga_guide_add_link (butfmt, buttxt, linknode, linkfile)
     char *butfmt;
     char *buttxt;
     char *linknode;
     char *linkfile;
{
  char *newlinkfile = NULL;
  int newbuttxt = 0;
  int old_fill_enable = filling_enabled;
  int adjust;

  filling_enabled = 0;
  amiga_guide_writing_button++;
  non_splitting_words++;

  /* In AmigaGuide, Top is Main.  */
  if (!linknode || !strcmp(linknode, "Top"))
    linknode = "Main";

  /* If we link to another file, ensure that its name ends on .guide.  */

  if (linkfile)
    {
      char *strp;
      newlinkfile = (char *)xmalloc(strlen(linkfile) + 7);
      strcpy(newlinkfile, linkfile);
      if ((strp = strrchr(newlinkfile, '.')))
        {
          if (!strcmp(strp, ".info"))
            {
              strcpy(strp, ".guide");
            }
        }
      else
        /* Assume we have a clean basename.  */
        strcat(newlinkfile, ".guide");

      if (buttxt == NULL)
        {
          buttxt = xmalloc (strlen (newlinkfile) + strlen (linknode) + 3);
          sprintf (buttxt, "(%s)%s", newlinkfile, linknode);
          newbuttxt = 1;
        }
      else if (!strp || !strcmp(strp, ".guide"))
        {
          char *tmp = xmalloc (strlen(newlinkfile) + strlen(buttxt) + 2);
          sprintf (tmp, "%s/%s", newlinkfile, buttxt);
          buttxt = tmp;
          newbuttxt = 1;
        }
    }

  if (buttxt == NULL)
    buttxt = linknode;

  add_word ("@{");
  execute_string (butfmt, buttxt);

  add_word(" link \"");
  if (newlinkfile)
    add_word_args ("%s/", newlinkfile);
  execute_string ("%s", linknode);
  add_word ("\"}");

  /* Compute adjustment.  */

  adjust = strlen ("@{\"\" link \"\"}") + strlen (linknode);
  if (newlinkfile)
    adjust += (strlen (newlinkfile) + 1);

  /* Adjust output_column.  */

  output_column -= adjust;
  output_column += 1; /* Fudge, ends of button.  */

  /* Remember nonprinting chars.  */

  amiga_guide_nonprinting_chars += adjust;
  amiga_guide_nonprinting_chars -= 2;

  non_splitting_words--;
  amiga_guide_writing_button--;
  filling_enabled = old_fill_enable;

  if (newlinkfile)
    free (newlinkfile);

  if (newbuttxt)
    free (buttxt);
}

int
amiga_set_attributes (attr, arg)
     enum amiga_guide_attrs attr;
     int arg;
{
  extern int printing_index;

  /* This array contains the sequences of characters to add, for any of
     the attributes above. The ordering are: AmigaGuide enable,
     AmigaGuide disable, console.device enable, console.device
     disable.  */
  static const char *amiga_attr_cmds[] = {
    /* AG(on), AG(off), CON:(on), CON:(off) */
    "@{b}", "@{ub}", "\x9B" "1m", "\x9B" "22m", /* AG_BOLD */
    "@{i}", "@{ui}", "\x9B" "3m", "\x9B" "23m", /* AG_ITALICS */
    "@{u}", "@{uu}", "\x9B" "4m", "\x9B" "24m"  /* AG_UNDERLINE */
  };
  static int active[AG_COUNT] = {0,0,0};

  int attr_width, attr_column_select;
  const char *cmd;

  /* Some sanity checks.  */

  if (amiga_guide <= 34 || amiga_guide_writing_button || printing_index)
    return 0;

  if (arg == START)
    {
      if (++active[attr] > 1) return 0;
    }
  else
    {
      if (--active[attr]) return 0;
    }

  /* Emit the attribute.  */

  if (input_text[input_text_offset - 1] == '\\')
    add_word (" ");		/* Avoid escaping of attr.  */

  attr_column_select = 4 * attr;
  if (arg != START)
    attr_column_select++;
  if (no_headers)
    attr_column_select += 2;

  attr_width = strlen (cmd=amiga_attr_cmds[attr_column_select]);

  amiga_guide_hide_title_chars += attr_width;
  amiga_guide_nonprinting_chars += attr_width;

  if (paragraph_is_open)
    insert_string (cmd);
  else
    add_word (cmd), output_column -= attr_width;

  return 1;
}

#endif /* ENABLE_AMIGAGUIDE */

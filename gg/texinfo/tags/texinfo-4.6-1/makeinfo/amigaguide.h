/* The various attributes supported by AmigaGuide.  */
enum amiga_guide_attrs { AG_BOLD=0, AG_ITALICS, AG_UNDERLINE, AG_COUNT };

/* AmigaGuide messages and commands.  */
#define AG_WRITING_MESG   "writing-amigaguide %d"
#define AG_CONVERT_NODES  "amigaguide_convert_nodes"
#define AG_VERBOSE_HEADER "amigaguide_verbose_header"

/* Non-zero indicates that we're converting to AmigaGuide hypertext,
   instead of plain info. The numerical value indicates what specific
   version we're targeting. Currently, the accepted values are 34, 39
   and 40.  */
extern int amiga_guide;

/* Non-zero indicates we're writing an AmigaGuide button.  */
extern int amiga_guide_writing_button;

/* Count nonprinting characters in titles.  */
extern int amiga_guide_hide_title_chars;

/* Count nonprinting characters, that may affect linebreaks.  */
extern int amiga_guide_nonprinting_chars;

/* When converting menus, output the gadget text in accordance with
   this format string.  */
extern char amiga_guide_menu_gadgets[12];

/* Prototypes for the various AmigaGuide functions.  */
extern int amiga_guide_convert_menu();	/* Tweak menus */
extern void amiga_guide_add_link();	/* Canonize AmigaGuide links */
extern int amiga_set_attributes ();	/* Handle Amiga attributes */

/* Enable or disable AmigaGuide support.  */
#if !defined (ENABLE_AMIGAGUIDE)
#define have_amigaguide 0
#else
#define have_amigaguide 1
#endif

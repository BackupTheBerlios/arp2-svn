/* BFD back-end for Intel 386 PE IMAGE COFF files.
   Copyright 1995 Free Software Foundation, Inc.

This file is part of BFD, the Binary File Descriptor library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "bfd.h"
#include "sysdep.h"

#define TARGET_SYM i386pei_vec
#define TARGET_NAME "pei-i386"
#define IMAGE_BASE NT_IMAGE_BASE
#define COFF_IMAGE_WITH_PE
#define COFF_WITH_PE
#define PCRELOFFSET true
#define TARGET_UNDERSCORE '_'
#define COFF_LONG_SECTION_NAMES

#include "coff-i386.c"
#include <ctype.h>

/* Routine to get the comdat symbol or associated section
     for a section, used by the linker to determine if sections are
     duplicates. needs the asection passed to it */

void pei_get_comdat_info PARAMS ((asection *));

void pei_get_comdat_info(sec)
asection *sec;
{
const char *nosym = "none";
bfd_size_type symesz;
bfd_byte *esym;
bfd_byte *esym_end;

comdat_info *comdat;

comdat = (comdat_info *) bfd_alloc (sec->owner, sizeof(comdat_info));

memcpy (comdat->name_buf, nosym, 5);
comdat->asoc_sec = (int)NULL;
symesz = bfd_coff_symesz (sec->owner);
esym = (bfd_byte *) obj_coff_external_syms (sec->owner);
esym_end = esym + obj_raw_syment_count (sec->owner) * symesz;


/* walk the native table looking for the section symbol */
  while (esym < esym_end)
    {
      struct internal_syment sym;
      bfd_coff_swap_sym_in (sec->owner, (PTR) esym, (PTR) &sym);
	/* FIXME add ISSYM() ISWEAK()? to function symbols ISFCN( sym.n_type )) would make things MUCH easier*/
	/* every comdat has either another comdat that it is addociated with
	   or a comdat symbol of it's own. if a symbol it isn't associated */
      if ((sym.n_scnum == sec->index + 1) && !isalpha(sym.n_name[0]) && !sym.n_numaux )
         {
	  comdat->comdat_sym = _bfd_coff_internal_syment_name(sec->owner, &sym, comdat->name_buf);
		break;
	 }
      if (!isalpha(sym.n_name[0]) && (sym.n_scnum == sec->index + 1) && sym.n_numaux)
         {
          union internal_auxent aux;
          bfd_coff_swap_aux_in(sec->owner, (PTR) (esym + symesz)
		,sym.n_type, sym.n_sclass, 0, sym.n_numaux, (PTR) &aux);
          comdat->asoc_sec = aux.x_scn.x_associated;
	  if (comdat->asoc_sec)
            break;
         }
      esym += (sym.n_numaux + 1) * symesz;
    }
sec->pe_comdat_info = comdat;
}

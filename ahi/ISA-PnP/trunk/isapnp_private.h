/* $Id$ */

/*
     ISA-PnP -- A Plug And Play ISA software layer for AmigaOS.
     Copyright (C) 2001 Martin Blom <martin@blom.org>
     
     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.
     
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.
     
     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef	ISA_PNP_isapnp_private_h
#define ISA_PNP_isapnp_private_h

#include <exec/libraries.h>
#include <libraries/configvars.h>

struct ISAPnPResource
{
  struct Library        m_Library;

  UWORD                 m_RegAddress;
  UWORD                 m_RegWriteData;
  UWORD                 m_RegReadData;

//  UWORD                 m_Pad;            /* Align to longword */

  APTR                  m_Base;



  struct CurrentBinding m_CurrentBinding;
};

#endif /* ISA_PNP_isapnp_private_h */

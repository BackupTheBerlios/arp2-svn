/* $Id$ */

#ifndef	ISA_PNP_isapnp_private_h
#define ISA_PNP_isapnp_private_h

#include <exec/libraries.h>
#include <libraries/configvars.h>

struct ISAPnPResource
{
  struct Library        m_Library;
  UWORD                 m_Pad;            /* Align to longword */
  APTR                  m_Base;
  struct CurrentBinding m_CurrentBinding;
};

#endif /* ISA_PNP_isapnp_private_h */

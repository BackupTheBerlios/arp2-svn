/* $Id$ */

#ifndef	ISAPNP_ISAPNP_PRIVATE_H
#define ISAPNP_ISAPNP_PRIVATE_H

#include <exec/libraries.h>

struct ISAPnPResource
{
  struct Library        m_Library;
  UWORD                 m_Pad;            /* Align to longword */
  APTR                  m_Base;
  struct CurrentBinding m_CurrentBinding;
};

#endif /* ISAPNP_ISAPNP_PRIVATE_H */

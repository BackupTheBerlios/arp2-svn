/* $Id$ */

#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <libraries/configvars.h>
#include <libraries/expansion.h>

#include <proto/expansion.h>

#include "isapnp_private.h"


extern struct Custom custom;


/******************************************************************************
*** Some local functions used to make memory accesses to hardware register ****
******************************************************************************/

static UBYTE
ReadByte( const volatile UBYTE* address )
{
  return *address;
}


static UWORD
ReadWord( const volatile UWORD* address )
{
  return *address;
}


static ULONG
ReadLong( const volatile ULONG* address )
{
  return *address;
}


static void
WriteByte( volatile UBYTE* address,
           UBYTE           value )
{
  *address = value;
}


static void
WriteWord( volatile UWORD* address,
           UWORD           value )
{
  *address = value;
}


static void
WriteLong( volatile ULONG* address,
           ULONG           value )
{
  *address = value;
}


/******************************************************************************
*** Controller functions ******************************************************
******************************************************************************/

void
ISAC_SetMasterInt( BOOL                   on,
                   struct ISAPnPResource* res )
{
  UWORD* reg2 = (UWORD*)( res->m_Base + 0x18002 );

  if( on )
  {
    WriteWord( reg2, 0 );
  }
  else
  {
    ReadWord( reg2 );
  }
}


BOOL
ISAC_GetMasterInt( struct ISAPnPResource* res )

{
  UWORD* reg1 = (UWORD*)( res->m_Base + 0x18000 );

  return ( ReadWord( reg1 ) & 1 ) != 0;
}


void
ISAC_SetWaitState( BOOL                   on,
                   struct ISAPnPResource* res )

{
#if 0
  UBYTE* reg3 = (UBYTE*) ( isa_Base + 0x18007 );

  if( (  on && !WaitState() ) ||
      ( !on &&  WaitState() ) )
  {
    *reg3 = 0;             // Toggle
  }
#endif
}


BOOL
ISAC_GetWaitState( struct ISAPnPResource* res )

{
  UWORD* reg1 = (UWORD*)( res->m_Base + 0x18000 );

  return ( ReadWord( reg1 ) & 2 ) != 0;
}


BOOL
ISAC_GetInterruptStatus( UBYTE                  interrupt,
                         struct ISAPnPResource* res )
{
  UWORD* reg1 = (UWORD*)( res->m_Base + 0x18000 );

  switch( interrupt )
  {
    case 3:
      return ( ReadWord( reg1 ) & 4 ) != 0;

    case 4:
      return ( ReadWord( reg1 ) & 8 ) != 0;

    case 5:
      return ( ReadWord( reg1 ) & 16 ) != 0;

    case 6:
      return ( ReadWord( reg1 ) & 32 ) != 0;

    case 7:
      return ( ReadWord( reg1 ) & 64 ) != 0;

    case 9:
      return ( ReadWord( reg1 ) & 128 ) != 0;

    case 10:
      return ( ReadWord( reg1 ) & 256 ) != 0;

    case 11:
      return ( ReadWord( reg1 ) & 512 ) != 0;

    case 12:
      return ( ReadWord( reg1 ) & 1024 ) != 0;

    case 14:
      return ( ReadWord( reg1 ) & 2048 ) != 0;

    case 15:
      return ( ReadWord( reg1 ) & 4096 ) != 0;
  }
  
  return FALSE;
}


UBYTE
ISAC_GetRegByte( UWORD                  reg,
                 struct ISAPnPResource* res )

{
  return ReadByte( res->m_Base + 2 * reg + 1 );
}


void
ISAC_SetRegByte( UWORD                  reg, 
                 UBYTE                  value,
                 struct ISAPnPResource* res )
{
  WriteByte( res->m_Base + 2 * reg + 1, value );
}


UWORD
ISAC_GetRegWord( UWORD                  reg,
                 struct ISAPnPResource* res )
{
  ULONG value;

  value = ReadLong( (ULONG*)( res->m_Base + 2 * reg ) );

  return ( value << 8 ) | ( ( value & 0x00ff0000 ) >> 16 );
}


void
ISAC_SetRegWord( UWORD                  reg, 
                 UWORD                  value,
                 struct ISAPnPResource* res )

{
  WriteLong( (ULONG*)( res->m_Base + 2 * reg ),
             ( (ULONG)( value & 0xff ) << 16 ) | ( value >> 8 ) );
}


UBYTE
ISAC_ReadByte( ULONG                  address,
               struct ISAPnPResource* res )

{
  return ReadByte( res->m_Base + ( ( 2 * address ) & 0xfffff ) + 1 );
}


void
ISAC_WriteByte( ULONG                  address, 
                UBYTE                  value,
                struct ISAPnPResource* res )
{
  WriteByte( res->m_Base + ( ( 2 * address ) & 0xfffff ) + 1, value );
}


UWORD
ISAC_ReadWord( ULONG                  address,
               struct ISAPnPResource* res )
{
  return ReadWord( (UWORD*)( res->m_Base + ( ( 2 * address ) & 0xfffff ) ) );
}


void
ISAC_WriteWord( ULONG                  address, 
                UWORD                  value,
                struct ISAPnPResource* res )
{
  WriteWord( (UWORD*)( res->m_Base + ( ( 2 * address ) & 0xfffff ) ),
             value );
}

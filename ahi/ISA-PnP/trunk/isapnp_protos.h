#ifndef CLIB_ISAPNP_PROTOS_H
#define CLIB_ISAPNP_PROTOS_H

/*
**      $VER: isapnp_protos.h 1.0 (2.5.2001)
**
**      C prototypes. For use with 32 bit integers only.
**
**      (C) Copyright 2001 Martin Blom
**      All Rights Reserved.
**
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

// Controller functions

void ISAC_SetMasterInt( BOOL on );
BOOL ISAC_GetMasterInt( void );

void ISAC_SetWaitState( BOOL on );
BOOL ISAC_GetWaitState( void );

BOOL ISAC_GetInterruptStatus( UBYTE interrupt );

UBYTE ISAC_GetRegByte( UWORD reg );
void ISAC_SetRegByte( UWORD reg, UBYTE value );

UWORD ISAC_GetRegWord( UWORD reg );
void ISAC_SetRegWord( UWORD reg, UWORD value );

UBYTE ISAC_ReadByte( ULONG address );
void ISAC_WriteByte( ULONG address, UBYTE value );

UWORD ISAC_ReadWord( ULONG address );
void ISAC_WriteWord( ULONG address, UWORD value );

#endif /* CLIB_PNPISA_PROTOS_H */

#ifndef CLIB_ISAPNP_PROTOS_H
#define CLIB_ISAPNP_PROTOS_H

/*
**      $VER: isapnp_protos.h 1.0 (7.5.2001)
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

struct ISAPNP_Card;
struct ISAPNP_Device;
struct ISAPNP_ResourceGroup;
struct ISAPNP_Resource;


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


// Structure allocation and deallocation

struct ISAPNP_Card* ISAPNP_AllocCard( struct ISAPNPBase* res );
void ISAPNP_FreeCard( struct ISAPNP_Card* card );


struct ISAPNP_Device* ISAPNP_AllocDevice( void );
void ISAPNP_FreeDevice( struct ISAPNP_Device* dev );


struct ISAPNP_ResourceGroup* ISAPNP_AllocResourceGroup( UBYTE pri );
void ISAPNP_FreeResourceGroup( struct ISAPNP_ResourceGroup* rg );


struct ISAPNP_Resource* ISAPNP_AllocResource( UBYTE type );
void ISAPNP_FreeResource( struct ISAPNP_Resource* r );


// Card and device handling

BOOL ISAPNP_ScanCards( void );
BOOL ISAPNP_ConfigureCards( void );


struct ISAPNP_Card* ISAPNP_FindCard( struct ISAPNP_Card* last_card, LONG manufacturer, WORD product, BYTE revision, LONG serial );
struct ISAPNP_Device* ISAPNP_FindDevice( struct ISAPNP_Device* last_device, LONG manufacturer, WORD product, BYTE revision );

#endif /* CLIB_PNPISA_PROTOS_H */

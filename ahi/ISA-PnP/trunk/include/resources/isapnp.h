#ifndef	RESOURCES_ISAPNP_H
#define RESOURCES_ISAPNP_H

/*
**	$VER: isapnp.h 1.0 (2.5.2001)
**
**	isapnp.resource definitions.
**
**	(C) Copyright 2001 Martin Blom
**	All Rights Reserved.
**
*/

#include <exec/lists.h>
#include <exec/nodes.h>

#define ISAPNPNAME "isapnp.resource"

/* Node types */

#define ISAPNP_NT_CARD            ( NT_USER - 1 )
#define ISAPNP_NT_DEVICE          ( NT_USER - 2 )
#define ISAPNP_NT_RESOURCE_GROUP  ( NT_USER - 3 )
#define ISAPNP_NT_IRQ_RESOURCE    ( NT_USER - 4 )
#define ISAPNP_NT_IO_RESOURCE     ( NT_USER - 5 )
#define ISAPNP_NT_DMA_RESOURCE    ( NT_USER - 6 )
#define ISAPNP_NT_MEMORY_RESOURCE ( NT_USER - 7 )

/* A unique identifier for a card or logical device */

struct ISAPNP_Identifier
{
  struct MinNode m_MinNode;
  char           m_Vendor[ 4 ];
  UWORD          m_ProductID;
  UBYTE	         m_Revision;
  UBYTE          m_Pad;
};

/* A PNP ISA card */

struct ISAPNP_Card
{
  struct Node              m_Node;
  UWORD                    m_Pad1;

  struct List              m_Devices;
  UWORD                    m_Pad2;

  struct ISAPNP_Identifier m_ID;
  ULONG                    m_SerialNumber;

  UBYTE                    m_CSN;

  UBYTE                    m_MajorPnPVersion;
  UBYTE                    m_MinorPnPVersion;
  UBYTE                    m_VendorPnPVersion;

  STRPTR                   m_Identifier;
};


/* A logical device on an ISA card */

struct ISAPNP_Device
{
  struct Node    m_Node;
  UWORD       	 m_Pad1;
  
  struct MinList m_IDs;

  UWORD          m_SupportedCommands;
};

/* Flags for m_SupportedCommands */

#define ISAPNP_DEVICE_SCF_BOOTABLE 0x01
#define ISAPNP_DEVICE_SCB_BOOTABLE 0


/* A resource (IRQ, DMA, IO, memory */

//struct ISAPNP_IRQResource

#endif /* RESOURCES_ISAPNP_H */

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
#define ISAPNP_NT_DMA_RESOURCE    ( NT_USER - 5 )
#define ISAPNP_NT_IO_RESOURCE     ( NT_USER - 6 )
#define ISAPNP_NT_MEMORY_RESOURCE ( NT_USER - 7 )

/* Priorities for PNPISA_AllocResourceGroup() */

#define ISAPNP_RG_PRI_GOOD        64
#define ISAPNP_RG_PRI_ACCEPTABLE  0
#define ISAPNP_RG_PRI_SUBOPTIMAL  -64

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
};

struct ISAPNP_ResourceGroup;

/* A logical device on an ISA card */

struct ISAPNP_Device
{
  struct Node                  m_Node;
  UWORD       	               m_Pad1;
  
  struct MinList               m_IDs;
  struct ISAPNP_ResourceGroup* m_Options;
  struct MinList               m_Resources;

  UWORD                        m_SupportedCommands;
  UWORD                        m_DeviceNumber;
};

/* Flags for m_SupportedCommands */

#define ISAPNP_DEVICE_SCF_BOOTABLE 0x01
#define ISAPNP_DEVICE_SCB_BOOTABLE 0


/* A resource group */

struct ISAPNP_ResourceGroup
{
  struct MinNode m_MinNode;
  UBYTE          m_Type;
  UBYTE          m_Pri;

  UWORD          m_Pad;

  struct MinList m_Resources;
  struct MinList m_ResourceGroups;
};


/* An resource (the "base class") */

struct ISAPNP_Resource
{
  struct MinNode m_MinNode;
  UBYTE          m_Type;
};


/* An IRQ resource */

struct ISAPNP_IRQResource
{
  struct MinNode m_MinNode;
  UBYTE          m_Type;
  
  UBYTE          m_IRQType;
  UWORD          m_IRQMask;
};

/* Flags for m_IRQType */

#define ISAPNP_IRQRESOURCE_ITF_HIGH_EDGE  0x01
#define ISAPNP_IRQRESOURCE_ITF_LOW_EDGE   0x02
#define ISAPNP_IRQRESOURCE_ITF_HIGH_LEVEL 0x04
#define ISAPNP_IRQRESOURCE_ITF_LOW_LEVEL  0x08
#define ISAPNP_IRQRESOURCE_ITB_HIGH_EDGE  0
#define ISAPNP_IRQRESOURCE_ITB_LOW_EDGE   1
#define ISAPNP_IRQRESOURCE_ITB_HIGH_LEVEL 2
#define ISAPNP_IRQRESOURCE_ITB_LOW_LEVEL  3

/* A DMA resource */

struct ISAPNP_DMAResource
{
  struct MinNode m_MinNode;
  UBYTE          m_Type;

  UBYTE          m_ChannelMask;
  UBYTE          m_Flags;
};

/* Flags for m_Flags */

#define ISAPNP_DMARESOURCE_F_TRANSFER_MASK  0x03
#define ISAPNP_DMARESOURCE_F_TRANSFER_8BIT  0x00
#define ISAPNP_DMARESOURCE_F_TRANSFER_BOTH  0x01
#define ISAPNP_DMARESOURCE_F_TRANSFER_16BIT 0x02

#define ISAPNP_DMARESOURCE_FF_BUS_MASTER    0x04
#define ISAPNP_DMARESOURCE_FF_BYTE_MODE     0x08
#define ISAPNP_DMARESOURCE_FF_WORD_MODE     0x10
#define ISAPNP_DMARESOURCE_FB_BUS_MASTER    2
#define ISAPNP_DMARESOURCE_FB_BYTE_MODE     3
#define ISAPNP_DMARESOURCE_FB_WORD_MODE     4

#define ISAPNP_DMARESOURCE_F_SPEED_MASK       0x60
#define ISAPNP_DMARESOURCE_F_SPEED_COMPATIBLE 0x00
#define ISAPNP_DMARESOURCE_F_SPEED_TYPE_A     0x20
#define ISAPNP_DMARESOURCE_F_SPEED_TYPE_B     0x40
#define ISAPNP_DMARESOURCE_F_SPEED_TYPE_F     0x60


/* An IO resource */

struct ISAPNP_IOResource
{
  struct MinNode m_MinNode;
  UBYTE          m_Type;

  UBYTE          m_Flags;

  UBYTE          m_Alignment;
  UBYTE          m_Length;

  UWORD          m_MinBase;
  UWORD          m_MaxBase;
};

/* Flags for m_Flags */

#define ISAPNP_IORESOURCE_FF_FULL_DECODE 0x01
#define ISAPNP_IORESOURCE_FB_FULL_DECODE 0

#endif /* RESOURCES_ISAPNP_H */

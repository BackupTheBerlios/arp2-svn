/* $Id$ */

/*
     ISA-PnP -- A Plug And Play ISA software layer for AmigaOS.
     Copyright (C) 2001 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#ifndef	ISA_PNP_isapnp_h
#define ISA_PNP_isapnp_h

/* Definitions from Plug and Play ISA Specification Version 1.0a 
   (May 5, 1994). 32 bit memory space configuration registers omitted. */


/* The ISA ports */

#define ISAPNP_ADDRESS    0x0279
#define ISAPNP_WRITE_DATA 0x0a79


/* ISAPNP_ADDRESS initiation key */

#define ISAPNP_INITIATION_KEY \
  0x6A, 0xB5, 0xDA, 0xED, 0xF6, 0xFB, 0x7D, 0xBE, \
  0xDF, 0x6F, 0x37, 0x1B, 0x0D, 0x86, 0xC3, 0x61, \
  0xB0, 0x58, 0x2C, 0x16, 0x8B, 0x45, 0xA2, 0xD1, \
  0xE8, 0x74, 0x3A, 0x9D, 0xCE, 0xE7, 0x73, 0x39


/* ISAPNP_ADDRESS values */

#define ISAPNP_REG_SET_RD_DATA_PORT                  0x00
#define ISAPNP_REG_SERIAL_ISOLATION                  0x01
#define ISAPNP_REG_CONFIG_CONTROL                    0x02
#define ISAPNP_REG_WAKE                              0x03
#define ISAPNP_REG_RESOURCE_DATA                     0x04
#define ISAPNP_REG_STATUS                            0x05
#define ISAPNP_REG_CARD_SELECT_NUMBER                0x06
#define ISAPNP_REG_LOGICAL_DEVICE_NUMBER             0x07


#define ISAPNP_REG_ACTIVATE                          0x30
#define ISAPNP_REG_IO_RANGE_CHECK                    0x31


#define ISAPNP_REG_MEMORY_BASE_ADDRESS_HIGH_0        0x40
#define ISAPNP_REG_MEMORY_BASE_ADDRESS_LOW_0         0x41
#define ISAPNP_REG_MEMORY_CONTROL_0                  0x42
#define ISAPNP_REG_MEMORY_UPPER_HIGH_0               0x43
#define ISAPNP_REG_MEMORY_UPPER_LOW_0                0x44

#define ISAPNP_REG_MEMORY_BASE_ADDRESS_HIGH_1        0x48
#define ISAPNP_REG_MEMORY_BASE_ADDRESS_LOW_1         0x49
#define ISAPNP_REG_MEMORY_CONTROL_1                  0x4a
#define ISAPNP_REG_MEMORY_UPPER_HIGH_1               0x4b
#define ISAPNP_REG_MEMORY_UPPER_LOW_1                0x4c

#define ISAPNP_REG_MEMORY_BASE_ADDRESS_HIGH_2        0x50
#define ISAPNP_REG_MEMORY_BASE_ADDRESS_LOW_2         0x51
#define ISAPNP_REG_MEMORY_CONTROL_2                  0x52
#define ISAPNP_REG_MEMORY_UPPER_HIGH_2               0x53
#define ISAPNP_REG_MEMORY_UPPER_LOW_2                0x54

#define ISAPNP_REG_MEMORY_BASE_ADDRESS_HIGH_3        0x58
#define ISAPNP_REG_MEMORY_BASE_ADDRESS_LOW_3         0x59
#define ISAPNP_REG_MEMORY_CONTROL_3                  0x5a
#define ISAPNP_REG_MEMORY_UPPER_HIGH_3               0x5b
#define ISAPNP_REG_MEMORY_UPPER_LOW_3                0x5c


#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_HIGH_0       0x60
#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_LOW_0        0x61

#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_HIGH_1       0x62
#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_LOW_1        0x63

#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_HIGH_2       0x64
#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_LOW_2        0x65

#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_HIGH_3       0x66
#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_LOW_3        0x67

#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_HIGH_4       0x68
#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_LOW_4        0x69

#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_HIGH_5       0x6a
#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_LOW_5        0x6b

#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_HIGH_6       0x6c
#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_LOW_6        0x6d

#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_HIGH_7       0x6e
#define ISAPNP_REG_IO_PORT_BASE_ADDRESS_LOW_7        0x6f


#define ISAPNP_REG_INTERRUPT_REQUEST_LEVEL_SELECT_0  0x70
#define ISAPNP_REG_INTERRUPT_REQUEST_TYPE_SELECT_0   0x71

#define ISAPNP_REG_INTERRUPT_REQUEST_LEVEL_SELECT_1  0x72
#define ISAPNP_REG_INTERRUPT_REQUEST_TYPE_SELECT_1   0x73


#define ISAPNP_REG_DMA_CHANNEL_SELECT_0              0x74
#define ISAPNP_REG_DMA_CHANNEL_SELECT_1              0x75


/* Bit masks for ISAPNP_REG_CONFIG_CONTROL */

#define ISAPNP_CCF_RESET        0x01
#define ISAPNP_CCF_WAIT_FOR_KEY 0x02
#define ISAPNP_CCF_RESET_CSN    0x04

#define ISAPNP_CCB_RESET        0
#define ISAPNP_CCB_WAIT_FOR_KEY 1
#define ISAPNP_CCB_RESET_CSN    2


#endif /* ISA_PNP_isapnp_private_h */

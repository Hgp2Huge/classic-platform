/* -------------------------------- Arctic Core ------------------------------
 * Arctic Core - the open source AUTOSAR platform http://arccore.com
 *
 * Copyright (C) 2009  ArcCore AB <contact@arccore.com>
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation; See <http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * -------------------------------- Arctic Core ------------------------------*/

#ifndef PORT_CFG_H_
#define PORT_CFG_H_

#include "Std_Types.h"

/** Build version info API */
#define PORT_VERSION_INFO_API				STD_ON  /** @req PORT100 PORT101 */
/** Enable Development Error Trace */
#define PORT_DEV_ERROR_DETECT				STD_ON
/** Build change pin direction API */
#define PORT_SET_PIN_DIRECTION_API	        STD_ON
/** Allow Pin mode changes during runtime (not avail on this CPU) */
#define PORT_SET_PIN_MODE_API               STD_ON

/** Parameter to enable/disable configuration on a port */
#define PORTA_CONFIGURABLE  STD_OFF
#define PORTB_CONFIGURABLE  STD_OFF
#define PORTE_CONFIGURABLE  STD_OFF
#define PORTK_CONFIGURABLE  STD_OFF
#define PORTH_CONFIGURABLE  STD_ON
#define PORTJ_CONFIGURABLE  STD_OFF
#define PORTM_CONFIGURABLE  STD_OFF
#define PORTP_CONFIGURABLE  STD_OFF
#define PORTS_CONFIGURABLE  STD_OFF
#define PORTT_CONFIGURABLE  STD_OFF

#define PORT_A_BASE 0x0100
#define PORT_B_BASE 0x0200
#define PORT_E_BASE 0x0300
#define PORT_K_BASE 0x0400
#define PORT_H_BASE 0x0500
#define PORT_J_BASE 0x0600
#define PORT_M_BASE 0x0700
#define PORT_P_BASE 0x0800
#define PORT_S_BASE 0x0900
#define PORT_T_BASE 0x0A00
#define PORT_BITMASK  0x00FF
#define PORT_BASEMASK 0xFF00;

/** HW specific symbolic names of pins */
/** @req PORT013 */
typedef enum
{
  PORT_PA0 = PORT_A_BASE,
  PORT_PA1,
  PORT_PA2,
  PORT_PA3,
  PORT_PA4,
  PORT_PA5,
  PORT_PA6,
  PORT_PA7,
  PORT_PB0 = PORT_B_BASE,
  PORT_PB1,
  PORT_PB2,
  PORT_PB3,
  PORT_PB4,
  PORT_PB5,
  PORT_PB6,
  PORT_PB7,
  PORT_PE0 = PORT_E_BASE,
  PORT_PE1,
  PORT_PE2,
  PORT_PE3,
  PORT_PE4,
  PORT_PE5,
  PORT_PE6,
  PORT_PE7,
  PORT_PH0 = PORT_H_BASE,
  PORT_PH1,
  PORT_PH2,
  PORT_PH3,
  PORT_PH4,
  PORT_PH5,
  PORT_PH6,
  PORT_PH7,
  PORT_PJ0 = PORT_J_BASE,
  PORT_PJ1,
  PORT_PJ2,
  PORT_PJ3,
  PORT_PJ4,
  PORT_PJ5,
  PORT_PJ6,
  PORT_PJ7,
  PORT_PK0 = PORT_K_BASE,
  PORT_PK1,
  PORT_PK2,
  PORT_PK3,
  PORT_PK4,
  PORT_PK5,
  PORT_PK6,
  PORT_PK7,
  PORT_PM0 = PORT_M_BASE,
  PORT_PM1,
  PORT_PM2,
  PORT_PM3,
  PORT_PM4,
  PORT_PM5,
  PORT_PM6,
  PORT_PM7,
  PORT_PP0 = PORT_P_BASE,
  PORT_PP1,
  PORT_PP2,
  PORT_PP3,
  PORT_PP4,
  PORT_PP5,
  PORT_PP6,
  PORT_PP7,
  PORT_PS0 = PORT_S_BASE,
  PORT_PS1,
  PORT_PS2,
  PORT_PS3,
  PORT_PS4,
  PORT_PS5,
  PORT_PS6,
  PORT_PS7,
  PORT_PT0 = PORT_T_BASE,
  PORT_PT1,
  PORT_PT2,
  PORT_PT3,
  PORT_PT4,
  PORT_PT5,
  PORT_PT6,
  PORT_PT7,
} Port_PinType;

/** @req PORT124 */
//typedef uint8 Port_PinModeType;

/** Top level configuration container */
/** @req PORT073 */
typedef struct
{
  const uint8_t corePullUpRegister; // PUCR
  const uint8_t coreReducedDriveRegister; // RDRIV
  const uint8_t modeRoutingRegister; // MODRR

  /**
   * Every port has:
   *  a direction (input or output)
   *  a default out value
   *  a mask that is 0 if the direction is allowed to change during runtime
   *
   * Some ports have:
   *  a reduced drive conf (power saving)
   *  a pull enable conf (enable pull-up/pull-down
   *  a pull direction conf (pull-up or pull-down)
   *  a wired mode (enable open drain outputs)
   */
#if ( PORTA_CONFIGURABLE == STD_ON )
  const uint8_t portADirection;
  const uint8_t portAOutValue;
  const uint8_t portAMask;
#endif

#if ( PORTB_CONFIGURABLE == STD_ON )
  const uint8_t portBDirection;
  const uint8_t portBOutValue;
  const uint8_t portBMask;
#endif

#if ( PORTE_CONFIGURABLE == STD_ON )
  const uint8_t portEDirection;
  const uint8_t portEOutValue;
  const uint8_t portEMask;
#endif

#if ( PORTK_CONFIGURABLE == STD_ON )
  const uint8_t portKDirection;
  const uint8_t portKOutValue;
  const uint8_t portKMask;
#endif

#if ( PORTH_CONFIGURABLE == STD_ON )
  const uint8_t portHDirection;
  const uint8_t portHOutValue;
  const uint8_t portHMask;
  const uint8_t portHPullEnableRegister;
  const uint8_t portHPullPolarityRegsiter;
  const uint8_t portHReducedDriveRegister;
#endif

#if ( PORTJ_CONFIGURABLE == STD_ON )
  const uint8_t portJDirection;
  const uint8_t portJOutValue;
  const uint8_t portJMask;
  const uint8_t portJPullEnableRegister;
  const uint8_t portJPullPolarityRegsiter;
  const uint8_t portJReducedDriveRegister;
#endif

#if ( PORTM_CONFIGURABLE == STD_ON )
  const uint8_t portMDirection;
  const uint8_t portMOutValue;
  const uint8_t portMMask;
  const uint8_t portMPullEnableRegister;
  const uint8_t portMPullPolarityRegsiter;
  const uint8_t portMWiredModeRegsiter;
  const uint8_t portMReducedDriveRegister;
#endif

#if ( PORTP_CONFIGURABLE == STD_ON )
  const uint8_t portPDirection;
  const uint8_t portPOutValue;
  const uint8_t portPMask;
  const uint8_t portPPullEnableRegister;
  const uint8_t portPPullPolarityRegsiter;
  const uint8_t portPReducedDriveRegister;
#endif

#if ( PORTS_CONFIGURABLE == STD_ON )
  const uint8_t portSDirection;
  const uint8_t portSOutValue;
  const uint8_t portSMask;
  const uint8_t portSPullEnableRegister;
  const uint8_t portSPullPolarityRegsiter;
  const uint8_t portSWiredModeRegsiter;
  const uint8_t portSReducedDriveRegister;
#endif

#if ( PORTT_CONFIGURABLE == STD_ON )
  const uint8_t portTDirection;
  const uint8_t portTOutValue;
  const uint8_t portTMask;
  const uint8_t portTPullEnableRegister;
  const uint8_t portTPullPolarityRegsiter;
  const uint8_t portTReducedDriveRegister;
#endif
} Port_ConfigType;

/** Instance of the top level configuration container */
extern const Port_ConfigType PortConfigData;

#endif /*PORT_CFG_H_*/

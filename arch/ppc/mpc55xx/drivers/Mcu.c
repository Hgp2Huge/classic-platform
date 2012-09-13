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


#include <assert.h>
#include <string.h>
#include "Std_Types.h"
#include "Mcu.h"
#include "Det.h"
#if defined(USE_DEM)
#include "Dem.h"
#endif
#include "mpc55xx.h"
#include "Cpu.h"
#include "Ramlog.h"
#include "Os.h"
#include "isr.h"

//#define USE_LDEBUG_PRINTF 1
#include "debug.h"

#define SYSCLOCK_SELECT_PLL	0x2

#if defined(CFG_MPC5567)
#define CALC_SYSTEM_CLOCK(_extal,_emfd,_eprediv,_erfd)  \
            ( (_extal) * ((_emfd)+4) / (((_eprediv)+1)*(1<<(_erfd))) )
#elif defined(CFG_MPC560X)
#define CALC_SYSTEM_CLOCK(_extal,_emfd,_eprediv,_erfd)  \
	        ( (_extal)*(_emfd) / ((_eprediv+1)*(2<<(_erfd))) )
#else
#define CALC_SYSTEM_CLOCK(_extal,_emfd,_eprediv,_erfd)  \
            ( (_extal) * ((_emfd)+16) / (((_eprediv)+1)*((_erfd)+1)) )
#endif

typedef void (*vfunc_t)();

/* Function declarations. */
static void Mcu_ConfigureFlash(void);

typedef struct{
	uint32 lossOfLockCnt;
	uint32 lossOfClockCnt;
} Mcu_Stats;

/**
 * Type that holds all global data for Mcu
 */
typedef struct
{
    // Set if Mcu_Init() have been called
    boolean initRun;
    // Our config
    const Mcu_ConfigType *config;
    Mcu_ClockType clockSetting;
    Mcu_Stats stats;
} Mcu_GlobalType;

/* Development error macros. */
#if ( MCU_DEV_ERROR_DETECT == STD_ON )
#define VALIDATE(_exp,_api,_err ) \
        if( !(_exp) ) { \
          Det_ReportError(MODULE_ID_MCU,0,_api,_err); \
          return; \
        }

#define VALIDATE_W_RV(_exp,_api,_err,_rv ) \
        if( !(_exp) ) { \
          Det_ReportError(MODULE_ID_MCU,0,_api,_err); \
          return (_rv); \
        }
#else
#define VALIDATE(_exp,_api,_err )
#define VALIDATE_W_RV(_exp,_api,_err,_rv )
#endif

// Global config
Mcu_GlobalType Mcu_Global =
{
	.initRun = 0,
	.config = &McuConfigData[0],
};

//-------------------------------------------------------------------

void Mcu_LossOfLock( void  ){
#if defined(USE_DEM)
	Dem_ReportErrorStatus(MCU_E_CLOCK_FAILURE, DEM_EVENT_STATUS_FAILED);
#endif

  /*
   * NOTE!!!
   * This interrupt may be triggered more than expected.
   * If you are going to use this interrupt, see [Freescale Device Errata MPC5510ACE, Rev. 10 APR 2009, errata ID: 6764].
   *
   */
#if defined(CFG_MPC560X)
	/*not support*/
#else
	Mcu_Global.stats.lossOfLockCnt++;
	// Clear interrupt
	FMPLL.SYNSR.B.LOLF = 1;
#endif
}

//-------------------------------------------------------------------

void Mcu_LossOfClock( void  ){
	/* Should report MCU_E_CLOCK_FAILURE with DEM here */
#if defined(CFG_MPC560X)
	/*not support*/
#else
	Mcu_Global.stats.lossOfClockCnt++;
	// Clear interrupt
	FMPLL.SYNSR.B.LOCF = 1;
#endif
}

#define SPR_PIR 286
#define SPR_PVR 287

#define CORE_PVR_E200Z1   	0x81440000UL
#define CORE_PVR_E200Z0   	0x81710000UL
#define CORE_PVR_E200Z3 	0x81120000UL
#define CORE_PVR_E200Z6   	0x81170000UL
#define CORE_PVR_E200Z65   	0x81150000UL	/* Is actually a 5668 */
#define CORE_PVR_E200Z0H   	0x817F0000UL

typedef struct{
	char *name;
	uint32 pvr;
} core_info_t;

typedef struct{
	char *name;
	uint32 pvr;
} cpu_info_t;

const cpu_info_t cpu_info_list[] = {
#if defined(CFG_MPC5516)
    {
    	.name = "MPC5516",
    	.pvr = CORE_PVR_E200Z1,
    },
    {
    	.name = "MPC5516",
    	.pvr = CORE_PVR_E200Z0,
    },
#elif defined(CFG_MPC5567)
    {
    	.name = "MPC5567",
    	.pvr = CORE_PVR_E200Z6,
    }
#elif defined(CFG_MPC5633)
    {
    	.name = "MPC563X",
    	.pvr = CORE_PVR_E200Z3,
    },
#elif defined(CFG_MPC5604B)
    {
    	.name = "MPC5604B",
    	.pvr = CORE_PVR_E200Z0H,
    },
#elif defined(CFG_MPC5604P)
    {
    	.name = "MPC5604P",
    	.pvr = CORE_PVR_E200Z0H,
    },
#elif defined(CFG_MPC5606B)
    {
    	.name = "MPC5606B",
    	.pvr = CORE_PVR_E200Z0H,
    },
#elif defined(CFG_MPC5606S)
    {
    	.name = "MPC5606S",
    	.pvr = CORE_PVR_E200Z0H,
    },
#elif defined(CFG_MPC5668)
	{
		.name = "MPC5668",
		.pvr = CORE_PVR_E200Z65,
	},
	{
		.name = "MPC5668",
		.pvr = CORE_PVR_E200Z0,
	},
#endif
};

const core_info_t core_info_list[] = {
#if defined(CFG_MPC5516)
	{
		.name = "CORE_E200Z1",
		.pvr = CORE_PVR_E200Z1,
    },
    {
    	.name = "CORE_E200Z1",
    	.pvr = CORE_PVR_E200Z1,
    },
#elif defined(CFG_MPC5567)
    {
    	.name = "CORE_E200Z6",
    	.pvr = CORE_PVR_E200Z6,
    }
#elif defined(CFG_MPC5633)
    {
		.name = "CORE_E200Z3",
		.pvr = CORE_PVR_E200Z3,
    },
#elif defined(CFG_MPC5604B)
    {
    	.name = "MPC5604B",
    	.pvr = CORE_PVR_E200Z0H,
    },
#elif defined(CFG_MPC5604P)
    {
    	.name = "MPC5604P",
    	.pvr = CORE_PVR_E200Z0H,
    },
#elif defined(CFG_MPC5606B)
    {
    	.name = "MPC5606B",
    	.pvr = CORE_PVR_E200Z0H,
    },
#elif defined(CFG_MPC5606S)
    {
    	.name = "MPC5606S",
    	.pvr = CORE_PVR_E200Z0H,
    },
#elif defined(CFG_MPC5668)
    {
    	.name = "CORE_E200Z65",
    	.pvr = CORE_PVR_E200Z65,
    },
    {
    	.name = "CORE_E200Z0",
    	.pvr = CORE_PVR_E200Z1,
    },
#endif
};

// TODO: move
#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(_x)  (sizeof(_x)/sizeof((_x)[0]))
#endif

static const cpu_info_t *Mcu_IdentifyCpu(uint32 pvr)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(cpu_info_list); i++) {
    	if (cpu_info_list[i].pvr == pvr) {
    		return &cpu_info_list[i];
        }
    }

    return NULL;
}

static const core_info_t *Mcu_IdentifyCore(uint32 pvr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(core_info_list); i++) {
		if (core_info_list[i].pvr == pvr) {
			return &core_info_list[i];
		}
    }

  return NULL;
}

static uint32 Mcu_CheckCpu( void ) {
	uint32 pvr;
	// uint32 pir;
	const cpu_info_t *cpuType;
	const core_info_t *coreType;

    // We have to registers to read here, PIR and PVR
    // pir = get_spr(SPR_PIR);
    pvr = get_spr(SPR_PVR);

    cpuType = Mcu_IdentifyCpu(pvr);
    coreType = Mcu_IdentifyCore(pvr);

    if( (cpuType == NULL) || (coreType == NULL) ) {
    	// Just hang
    	while(1) ;
    }

    //DEBUG(DEBUG_HIGH,"/drivers/mcu: Cpu:  %s( 0x%08x )\n",cpuType->name,pvr);
    //DEBUG(DEBUG_HIGH,"/drivers/mcu: Core: %s( 0x%08x )\n",coreType->name,pvr);

    return 0;
}

//-------------------------------------------------------------------

void Mcu_Init(const Mcu_ConfigType *configPtr)
{
	VALIDATE( ( NULL != configPtr ), MCU_INIT_SERVICE_ID, MCU_E_PARAM_CONFIG );

#if defined(CFG_MPC560X)
	/* Disable watchdog. Watchdog is enabled default after reset.*/
 	SWT.SR.R = 0x0000c520;     /* Write keys to clear soft lock bit */
 	SWT.SR.R = 0x0000d928;
 	SWT.CR.R = 0x8000010A;     /* Disable watchdog */
#if defined(USE_WDG)
#if !defined(CFG_MPC5604P)
	SWT.TO.R = 0xfa00;         	/* set the timout to 500ms, 128khz clock */
#else
	SWT.TO.R = 0x7d000;         	/* set the timout to 500ms, , 16khz clock */
#endif
	SWT.CR.R = 0x8000011B;      /* enable watchdog */
#endif
#endif

    if( !SIMULATOR() ) {
    	Mcu_CheckCpu();
    }

    memset(&Mcu_Global.stats,0,sizeof(Mcu_Global.stats));

    // Setup memories
    Mcu_ConfigureFlash();

    Mcu_Global.config = configPtr;

#if defined(CFG_MPC560X)
    /* Enable DRUN, RUN0, SAFE, RESET modes */
    ME.MER.R = 0x0000001D;
	/*	MPC5604P: CMU_0 must be initialized differently from the default value
                in case of 8 MHz crystal. */
#if defined (CFG_MPC5604P)
	CGM.CMU_0_CSR.R = 0x00000004;
#endif
#endif

    Mcu_Global.initRun = 1;

    if( Mcu_Global.config->McuClockSrcFailureNotification == TRUE  ) {
#if defined(CFG_MPC560X)
    	/*not support*/
#else
    	ISR_INSTALL_ISR1("LossOfLock", Mcu_LossOfLock, PLL_SYNSR_LOLF, 10 , 0 );
#if defined(CFG_MPC5516)  || defined(CFG_MPC5668)
    	FMPLL.ESYNCR2.B.LOLIRQ = 1;
#elif defined(CFG_MPC5554) || defined(CFG_MPC5567)
    	FMPLL.SYNCR.B.LOLIRQ = 1;
#endif
    	ISR_INSTALL_ISR1("LossOfClock", Mcu_LossOfClock, PLL_SYNSR_LOLF, 10 , 0 );
#if defined(CFG_MPC5516) || defined(CFG_MPC5668)
    	FMPLL.ESYNCR2.B.LOCIRQ = 1;
#elif defined(CFG_MPC5554) || defined(CFG_MPC5567)
    	FMPLL.SYNCR.B.LOCIRQ = 1;
#endif
#endif
    }
}

//-------------------------------------------------------------------

void Mcu_DeInit()
{
	Mcu_Global.initRun = FALSE; // Very simple Deinit. Should we do more?
}

//-------------------------------------------------------------------

Std_ReturnType Mcu_InitRamSection(const Mcu_RamSectionType RamSection)
{
	VALIDATE_W_RV( ( 1 == Mcu_Global.initRun ), MCU_INITRAMSECTION_SERVICE_ID, MCU_E_UNINIT, E_NOT_OK );
	VALIDATE_W_RV( ( RamSection <= Mcu_Global.config->McuRamSectors ), MCU_INITRAMSECTION_SERVICE_ID, MCU_E_PARAM_RAMSECTION, E_NOT_OK );

    /* NOT SUPPORTED, reason: no support for external RAM */

    return E_OK;
}

//-------------------------------------------------------------------

Std_ReturnType Mcu_InitClock(const Mcu_ClockType ClockSetting)
{
    Mcu_ClockSettingConfigType *clockSettingsPtr;
    VALIDATE_W_RV( ( 1 == Mcu_Global.initRun ), MCU_INITCLOCK_SERVICE_ID, MCU_E_UNINIT, E_NOT_OK );
    VALIDATE_W_RV( ( ClockSetting < Mcu_Global.config->McuClockSettings ), MCU_INITCLOCK_SERVICE_ID, MCU_E_PARAM_CLOCK, E_NOT_OK );

    Mcu_Global.clockSetting = ClockSetting;
    clockSettingsPtr = &Mcu_Global.config->McuClockSettingConfig[Mcu_Global.clockSetting];

    // TODO: find out if the 5554 really works like the 5516 here
    // All three (16, 54, 67) used to run the same code here though, so i'm sticking it with 5516
#if defined(CFG_MPC5516) || defined(CFG_MPC5554) || defined(CFG_MPC5668)
    /* 5516clock info:
     * Fsys - System frequency ( CPU + all periperals? )
     *
     *  Fsys = EXTAL_FREQ *(  (emfd+16) / ( (eprediv+1) * ( erfd+1 )) ) )
     */
    // Check ranges...
    assert((clockSettingsPtr->Pll2>=32) && (clockSettingsPtr->Pll2<=132));
    assert( (clockSettingsPtr->Pll1 != 6) &&
            (clockSettingsPtr->Pll1 != 8) &&
            (clockSettingsPtr->Pll1 < 10) );
    assert( clockSettingsPtr->Pll3 & 1); // Must be odd
#elif defined(CFG_MPC5567)
    /* 5567 clock info:
     *  Fsys = EXTAL_FREQ *(  (emfd+4) / ( (eprediv+1) * ( 2^erfd )) ) )
     */
    // Check ranges...
    assert(clockSettingsPtr->Pll2 < 16);
    assert(clockSettingsPtr->Pll1 <= 4);
    assert(clockSettingsPtr->Pll3 < 8);
#endif

#if defined(USE_LDEBUG_PRINTF)
    {
    	uint32  extal = Mcu_Global.config->McuClockSettingConfig[Mcu_Global.clockSetting].McuClockReferencePointFrequency;
    	uint32  f_sys;

    	f_sys = CALC_SYSTEM_CLOCK( extal,
    		clockSettingsPtr->Pll2,
    		clockSettingsPtr->Pll1,
    		clockSettingsPtr->Pll3 );

        //DEBUG(DEBUG_HIGH,"/drivers/mcu: F_sys will be:%08d Hz\n",f_sys);
    }
#endif

#if defined(CFG_MPC5516) || defined(CFG_MPC5668)

    // set post divider to next valid value to ensure that an overshoot during lock phase
    // won't result in a too high freq
    FMPLL.ESYNCR2.B.ERFD = (clockSettingsPtr->Pll3 + 1) | 1;

    // External crystal PLL mode.
    FMPLL.ESYNCR1.B.CLKCFG = 7; //TODO: Hur st�lla detta f�r 5567?

    // Write pll parameters.
    FMPLL.ESYNCR1.B.EPREDIV = clockSettingsPtr->Pll1;
    FMPLL.ESYNCR1.B.EMFD    = clockSettingsPtr->Pll2;

    while(FMPLL.SYNSR.B.LOCK != 1) {};

    FMPLL.ESYNCR2.B.ERFD    = clockSettingsPtr->Pll3;
    // Connect SYSCLK to FMPLL
    SIU.SYSCLK.B.SYSCLKSEL = SYSCLOCK_SELECT_PLL;
#elif defined(CFG_MPC5604B) || defined(CFG_MPC5606B)
    // Write pll parameters.
    CGM.FMPLL_CR.B.IDF = clockSettingsPtr->Pll1;
    CGM.FMPLL_CR.B.NDIV = clockSettingsPtr->Pll2;
    CGM.FMPLL_CR.B.ODF = clockSettingsPtr->Pll3;

    /* RUN0 cfg: 16MHzIRCON,OSC0ON,PLL0ON,syclk=PLL0 */
    ME.RUN[0].R = 0x001F0074;
    /* Peri. Cfg. 1 settings: only run in RUN0 mode */
    ME.RUNPC[1].R = 0x00000010;
    /* MPC56xxB/S: select ME.RUNPC[1] */
    ME.PCTL[68].R = 0x01; //SIUL control
    ME.PCTL[91].R = 0x01; //RTC/API control
    ME.PCTL[92].R = 0x01; //PIT_RTI control
    ME.PCTL[72].R = 0x01; //eMIOS0 control
    ME.PCTL[73].R = 0x01; //eMIOS1 control
    ME.PCTL[16].R = 0x01; //FlexCAN0 control
    ME.PCTL[17].R = 0x01; //FlexCAN1 control
    ME.PCTL[4].R = 0x01;  /* MPC56xxB/P/S DSPI0  */
    ME.PCTL[5].R = 0x01;  /* MPC56xxB/P/S DSPI1:  */
    ME.PCTL[32].R = 0x01; //ADC0 control
#if defined(CFG_MPC5606B)
    ME.PCTL[33].R = 0x01; //ADC1 control
#endif
    ME.PCTL[23].R = 0x01; //DMAMUX control
    ME.PCTL[48].R = 0x01; /* MPC56xxB/P/S LINFlex  */
    ME.PCTL[49].R = 0x01; /* MPC56xxB/P/S LINFlex  */
    /* Mode Transition to enter RUN0 mode: */
    /* Enter RUN0 Mode & Key */
    ME.MCTL.R = 0x40005AF0;
    /* Enter RUN0 Mode & Inverted Key */
    ME.MCTL.R = 0x4000A50F;

    /* Wait for mode transition to complete */
    while (ME.GS.B.S_MTRANS) {}
    /* Verify RUN0 is the current mode */
    while(ME.GS.B.S_CURRENTMODE != 4) {}

    CGM.SC_DC[0].R = 0x80; /* MPC56xxB/S: Enable peri set 1 sysclk divided by 1 */
    CGM.SC_DC[1].R = 0x80; /* MPC56xxB/S: Enable peri set 2 sysclk divided by 1 */
    CGM.SC_DC[2].R = 0x80; /* MPC56xxB/S: Enable peri set 3 sysclk divided by 1 */

    SIU.PSMI[0].R = 0x01; /* CAN1RX on PCR43 */
    SIU.PSMI[6].R = 0x01; /* CS0/DSPI_0 on PCR15 */

#elif defined(CFG_MPC5606S) || defined(CFG_MPC5604P)
    // Write pll parameters.
    CGM.FMPLL[0].CR.B.IDF = clockSettingsPtr->Pll1;
    CGM.FMPLL[0].CR.B.NDIV = clockSettingsPtr->Pll2;
    CGM.FMPLL[0].CR.B.ODF = clockSettingsPtr->Pll3;

    /* RUN0 cfg: 16MHzIRCON,OSC0ON,PLL0ON,syclk=PLL0 */
    ME.RUN[0].R = 0x001F0074;
    /* Peri. Cfg. 1 settings: only run in RUN0 mode */
    ME.RUNPC[1].R = 0x00000010;
    /* MPC56xxB/S: select ME.RUNPC[1] */
    ME.PCTL[68].R = 0x01; //SIUL control
    ME.PCTL[91].R = 0x01; //RTC/API control
    ME.PCTL[92].R = 0x01; //PIT_RTI control
    ME.PCTL[72].R = 0x01; //eMIOS0 control
    ME.PCTL[73].R = 0x01; //eMIOS1 control
    ME.PCTL[16].R = 0x01; //FlexCAN0 control
    ME.PCTL[17].R = 0x01; //FlexCAN1 control
    ME.PCTL[4].R = 0x01;  /* MPC56xxB/P/S DSPI0  */
    ME.PCTL[5].R = 0x01;  /* MPC56xxB/P/S DSPI1:  */
    ME.PCTL[32].R = 0x01; //ADC0 control
#if defined(CFG_MPC5604P)
    ME.PCTL[33].R = 0x01; //ADC1 control
#endif
    ME.PCTL[23].R = 0x01; //DMAMUX control
    ME.PCTL[48].R = 0x01; /* MPC56xxB/P/S LINFlex  */
    ME.PCTL[49].R = 0x01; /* MPC56xxB/P/S LINFlex  */
    /* Mode Transition to enter RUN0 mode: */
    /* Enter RUN0 Mode & Key */
    ME.MCTL.R = 0x40005AF0;
    /* Enter RUN0 Mode & Inverted Key */
    ME.MCTL.R = 0x4000A50F;

    /* Wait for mode transition to complete */
    while (ME.GS.B.S_MTRANS) {}
    /* Verify RUN0 is the current mode */
    while(ME.GS.B.S_CURRENTMODE != 4) {}

    CGM.SC_DC[0].R = 0x80; /* MPC56xxB/S: Enable peri set 1 sysclk divided by 1 */
    CGM.SC_DC[1].R = 0x80; /* MPC56xxB/S: Enable peri set 2 sysclk divided by 1 */
    CGM.SC_DC[2].R = 0x80; /* MPC56xxB/S: Enable peri set 3 sysclk divided by 1 */
    CGM.SC_DC[3].R = 0x80; /* MPC56xxB/S: Enable peri set 3 sysclk divided by 1 */

 #elif defined(CFG_MPC5554) || defined(CFG_MPC5567)
    // Partially following the steps in MPC5567 RM..
    FMPLL.SYNCR.B.DEPTH	= 0;
    FMPLL.SYNCR.B.LOLRE	= 0;
    FMPLL.SYNCR.B.LOLIRQ = 0;

    FMPLL.SYNCR.B.PREDIV 	= clockSettingsPtr->Pll1;
    FMPLL.SYNCR.B.MFD		= clockSettingsPtr->Pll2;
    FMPLL.SYNCR.B.RFD    	= clockSettingsPtr->Pll3;

	// Wait for PLL to sync.
    while (Mcu_GetPllStatus() != MCU_PLL_LOCKED) ;

    FMPLL.SYNCR.B.LOLIRQ	= 1;
#endif

    return E_OK;
}

//-------------------------------------------------------------------

void Mcu_DistributePllClock(void)
{
    VALIDATE( ( 1 == Mcu_Global.initRun ), MCU_DISTRIBUTEPLLCLOCK_SERVICE_ID, MCU_E_UNINIT );
#if defined(CFG_MPC560XB)
    VALIDATE( ( CGM.FMPLL_CR.B.S_LOCK == 1 ), MCU_DISTRIBUTEPLLCLOCK_SERVICE_ID, MCU_E_PLL_NOT_LOCKED );
#elif defined(CFG_MPC5606S) || defined(CFG_MPC5604P)
    VALIDATE( ( CGM.FMPLL[0].CR.B.S_LOCK == 1 ), MCU_DISTRIBUTEPLLCLOCK_SERVICE_ID, MCU_E_PLL_NOT_LOCKED );
#else
    VALIDATE( ( FMPLL.SYNSR.B.LOCK == 1 ), MCU_DISTRIBUTEPLLCLOCK_SERVICE_ID, MCU_E_PLL_NOT_LOCKED );
#endif
    /* NOT IMPLEMENTED due to pointless function on this hardware */

}

//-------------------------------------------------------------------

Mcu_PllStatusType Mcu_GetPllStatus(void)
{
    VALIDATE_W_RV( ( 1 == Mcu_Global.initRun ), MCU_GETPLLSTATUS_SERVICE_ID, MCU_E_UNINIT, MCU_PLL_STATUS_UNDEFINED );
    Mcu_PllStatusType rv;

    if( !SIMULATOR() )
    {
#if defined(CFG_MPC560XB)
    	if ( !CGM.FMPLL_CR.B.S_LOCK )
    	{
    		rv = MCU_PLL_UNLOCKED;
    	} else
    	{
    		rv = MCU_PLL_LOCKED;
    	}
#elif defined(CFG_MPC5606S) || defined(CFG_MPC5604P)
    	if ( !CGM.FMPLL[0].CR.B.S_LOCK )
    	{
    		rv = MCU_PLL_UNLOCKED;
    	} else
    	{
    		rv = MCU_PLL_LOCKED;
    	}
#else
    	if ( !FMPLL.SYNSR.B.LOCK )
    	{
    		rv = MCU_PLL_UNLOCKED;
    	} else
    	{
    		rv = MCU_PLL_LOCKED;
    	}
#endif
    }
    else
    {
    	/* We are running on instruction set simulator. PLL is then always in sync... */
    	rv = MCU_PLL_LOCKED;
    }

  return rv;
}

//-------------------------------------------------------------------

Mcu_ResetType Mcu_GetResetReason(void)
{
	Mcu_ResetType rv;

	VALIDATE_W_RV( ( 1 == Mcu_Global.initRun ), MCU_GETRESETREASON_SERVICE_ID, MCU_E_UNINIT, MCU_RESET_UNDEFINED );

#if defined(CFG_MPC560X)
	if( RGM.FES.B.F_SOFT ) {
		rv = MCU_SW_RESET;
	} else if( RGM.DES.B.F_SWT ) {
		rv = MCU_WATCHDOG_RESET;
	} else if( RGM.DES.B.F_POR ) {
		rv = MCU_POWER_ON_RESET;
	} else {
		rv = MCU_RESET_UNDEFINED;
	}
#else
	if( SIU.RSR.B.SSRS ) {
		rv = MCU_SW_RESET;
	} else if( SIU.RSR.B.WDRS ) {
		rv = MCU_WATCHDOG_RESET;
	} else if( SIU.RSR.B.PORS || SIU.RSR.B.ERS ) {
		rv = MCU_POWER_ON_RESET;
	} else {
		rv = MCU_RESET_UNDEFINED;
	}
#endif

	return rv;
}

//-------------------------------------------------------------------

Mcu_RawResetType Mcu_GetResetRawValue(void)
{
	VALIDATE_W_RV( ( 1 == Mcu_Global.initRun ), MCU_GETRESETREASON_SERVICE_ID, MCU_E_UNINIT, MCU_GETRESETRAWVALUE_UNINIT_RV );

	if( !Mcu_Global.initRun ) {
		return MCU_GETRESETRAWVALUE_UNINIT_RV;
	}

#if defined(CFG_MPC560X)
	if( RGM.DES.R )
		return RGM.DES.R;
	else
		return RGM.FES.R;
#else
	return SIU.RSR.R;
#endif

}

//-------------------------------------------------------------------

#if ( MCU_PERFORM_RESET_API == STD_ON )
void Mcu_PerformReset(void)
{
	VALIDATE( ( 1 == Mcu_Global.initRun ), MCU_PERFORMRESET_SERVICE_ID, MCU_E_UNINIT );

	// Reset
#if defined(CFG_MPC560X)
    ME.MCTL.R = 0x00005AF0;
    ME.MCTL.R = 0x0000A50F;

    while (ME.GS.B.S_MTRANS) {}
    while(ME.GS.B.S_CURRENTMODE != 0) {}
#else
	SIU.SRCR.B.SSR = 1;
#endif

}
#endif

//-------------------------------------------------------------------

void Mcu_SetMode(const Mcu_ModeType McuMode)
{
	VALIDATE( ( 1 == Mcu_Global.initRun ), MCU_SETMODE_SERVICE_ID, MCU_E_UNINIT );
	// VALIDATE( ( McuMode <= Mcu_Global.config->McuNumberOfMcuModes ), MCU_SETMODE_SERVICE_ID, MCU_E_PARAM_MODE );
	(void) McuMode;

	/* NOT SUPPORTED */
}

//-------------------------------------------------------------------

/**
 * Get the system clock in Hz. It calculates the clock from the
 * different register settings in HW.
 */
uint32_t McuE_GetSystemClock(void)
{
	/*
	 * System clock calculation
	 *
	 * 5516 -  f_sys = extal * (emfd+16) / ( (eprediv+1) * ( erfd+1 ));
	 * 5567 -  f_sys = extal * (emfd+4) / ( (eprediv+1) * ( 2^erfd ));
	 * 563x -  We run in legacy mode = 5567
	 * 5606s - f_sys = extal * emfd / ((eprediv+1)*(2<<(erfd)));
	 */
#if defined(CFG_MPC5516) || defined(CFG_MPC5668)
	uint32_t eprediv = FMPLL.ESYNCR1.B.EPREDIV;
	uint32_t emfd = FMPLL.ESYNCR1.B.EMFD;
	uint32_t erfd = FMPLL.ESYNCR2.B.ERFD;
#elif defined(CFG_MPC5554) || defined(CFG_MPC5567) || defined(CFG_MPC5633)
	uint32_t eprediv = FMPLL.SYNCR.B.PREDIV;
	uint32_t emfd = FMPLL.SYNCR.B.MFD;
	uint32_t erfd = FMPLL.SYNCR.B.RFD;
#elif defined(CFG_MPC560XB)
    uint32_t eprediv = CGM.FMPLL_CR.B.IDF;
    uint32_t emfd = CGM.FMPLL_CR.B.NDIV;
    uint32_t erfd = CGM.FMPLL_CR.B.ODF;
#elif defined(CFG_MPC5606S) || defined(CFG_MPC5604P)
    uint32_t eprediv = CGM.FMPLL[0].CR.B.IDF;
    uint32_t emfd = CGM.FMPLL[0].CR.B.NDIV;
    uint32_t erfd = CGM.FMPLL[0].CR.B.ODF;
#endif

    uint32_t f_sys;
    uint32  extal = Mcu_Global.config->McuClockSettingConfig[Mcu_Global.clockSetting].McuClockReferencePointFrequency;

    f_sys = CALC_SYSTEM_CLOCK(extal,emfd,eprediv,erfd);

    return f_sys;
}

#if defined(CFG_MPC5668)
uint32_t McuE_GetPeripheralClock(McuE_PeriperalClock_t type) {
	uint32_t sysClock = McuE_GetSystemClock();
	vuint32_t prescaler;

	switch (type)
	{
		case PERIPHERAL_CLOCK_FLEXCAN_A:
		case PERIPHERAL_CLOCK_FLEXCAN_B:
		case PERIPHERAL_CLOCK_FLEXCAN_C:
		case PERIPHERAL_CLOCK_FLEXCAN_D:
		case PERIPHERAL_CLOCK_FLEXCAN_E:
		case PERIPHERAL_CLOCK_FLEXCAN_F:
		case PERIPHERAL_CLOCK_DSPI_A:
		case PERIPHERAL_CLOCK_DSPI_B:
		case PERIPHERAL_CLOCK_DSPI_C:
		case PERIPHERAL_CLOCK_DSPI_D:
			prescaler = SIU.SYSCLK.B.LPCLKDIV1;
			break;
		case PERIPHERAL_CLOCK_ESCI_A:
		case PERIPHERAL_CLOCK_ESCI_B:
		case PERIPHERAL_CLOCK_ESCI_C:
		case PERIPHERAL_CLOCK_ESCI_D:
		case PERIPHERAL_CLOCK_ESCI_E:
		case PERIPHERAL_CLOCK_ESCI_F:
		case PERIPHERAL_CLOCK_IIC_A:
		case PERIPHERAL_CLOCK_IIC_B:
			prescaler = SIU.SYSCLK.B.LPCLKDIV0;
			break;
		case PERIPHERAL_CLOCK_ADC_A:
			prescaler = SIU.SYSCLK.B.LPCLKDIV2;
			break;
		case PERIPHERAL_CLOCK_EMIOS:
			prescaler = SIU.SYSCLK.B.LPCLKDIV3;
			break;
		default:
			assert(0);
			break;
	}

	return sysClock/(1<<prescaler);

}

#else

/**
 * Get the peripheral clock in Hz for a specific device
 */
uint32_t McuE_GetPeripheralClock(McuE_PeriperalClock_t type)
{
#if defined(CFG_MPC5567)
	// No peripheral dividers on 5567.
	return McuE_GetSystemClock();
#else
	uint32_t sysClock = McuE_GetSystemClock();
	vuint32_t prescaler;

  // See table 3.1, section 3.4.5 Peripheral Clock dividers
	switch (type)
	{
		case PERIPHERAL_CLOCK_FLEXCAN_A:
		case PERIPHERAL_CLOCK_DSPI_A:
#if defined(CFG_MPC5516)
			prescaler = SIU.SYSCLK.B.LPCLKDIV0;
			break;
#elif defined(CFG_MPC560X)
			prescaler = CGM.SC_DC[1].B.DIV;
			break;
#endif

		case PERIPHERAL_CLOCK_PIT:
		case PERIPHERAL_CLOCK_ESCI_A:
		case PERIPHERAL_CLOCK_IIC_A:
#if defined(CFG_MPC5516)
			prescaler = SIU.SYSCLK.B.LPCLKDIV1;
			break;
#endif

		case PERIPHERAL_CLOCK_FLEXCAN_B:
		case PERIPHERAL_CLOCK_FLEXCAN_C:
		case PERIPHERAL_CLOCK_FLEXCAN_D:
		case PERIPHERAL_CLOCK_FLEXCAN_E:
		case PERIPHERAL_CLOCK_FLEXCAN_F:
#if defined(CFG_MPC5516)
			prescaler = SIU.SYSCLK.B.LPCLKDIV2;
			break;
#elif defined(CFG_MPC560X)
			prescaler = CGM.SC_DC[1].B.DIV;
			break;
#endif

		case PERIPHERAL_CLOCK_DSPI_B:
		case PERIPHERAL_CLOCK_DSPI_C:
		case PERIPHERAL_CLOCK_DSPI_D:
		case PERIPHERAL_CLOCK_DSPI_E:
		case PERIPHERAL_CLOCK_DSPI_F:
#if defined(CFG_MPC5516)
		prescaler = SIU.SYSCLK.B.LPCLKDIV3;
			break;
#endif

		case PERIPHERAL_CLOCK_ESCI_B:
		case PERIPHERAL_CLOCK_ESCI_C:
		case PERIPHERAL_CLOCK_ESCI_D:
		case PERIPHERAL_CLOCK_ESCI_E:
		case PERIPHERAL_CLOCK_ESCI_F:
		case PERIPHERAL_CLOCK_ESCI_G:
		case PERIPHERAL_CLOCK_ESCI_H:
#if defined(CFG_MPC5516)
			prescaler = SIU.SYSCLK.B.LPCLKDIV4;
			break;
#endif

#if defined(CFG_MPC560X)
		case PERIPHERAL_CLOCK_LIN_A:
		case PERIPHERAL_CLOCK_LIN_B:
#if defined(CFG_MPC560XB) || defined(CFG_MPC5604P)
		case PERIPHERAL_CLOCK_LIN_C:
		case PERIPHERAL_CLOCK_LIN_D:
#endif
			prescaler = CGM.SC_DC[0].B.DIV;
			break;
		case PERIPHERAL_CLOCK_EMIOS_0:
			prescaler = CGM.SC_DC[2].B.DIV;
			break;
		case PERIPHERAL_CLOCK_EMIOS_1:
			prescaler = CGM.SC_DC[2].B.DIV;
			break;
#else
		case PERIPHERAL_CLOCK_EMIOS:
#if defined(CFG_MPC5516)
			prescaler = SIU.SYSCLK.B.LPCLKDIV5;
			break;
#endif
#endif

		case PERIPHERAL_CLOCK_MLB:
#if defined(CFG_MPC5516)
			prescaler = SIU.SYSCLK.B.LPCLKDIV6;
			break;
#endif

		default:
			assert(0);
			break;
	}

	return sysClock/(1<<prescaler);
#endif
}
#endif

/**
 * Function to setup the internal flash for optimal performance
 */

static void Mcu_ConfigureFlash(void)
{
	/* These flash settings increases the CPU performance of 7 times compared
   	   to reset default settings!! */

#if defined(CFG_MPC5516)
	/* Disable pipelined reads when flash options are changed. */
	FLASH.MCR.B.PRD = 1;

	/* Enable master prefetch for e200z1 and eDMA. */
	FLASH.PFCRP0.B.M0PFE = 1;
	FLASH.PFCRP0.B.M2PFE = 1;

	/* Address pipelining control. Must be set to the same value as RWSC. */
	FLASH.PFCRP0.B.APC = 2;
	FLASH.PFCRP0.B.RWSC = 2;

	/* Write wait states. */
	FLASH.PFCRP0.B.WWSC = 1;

	/* Enable data prefetch. */
	FLASH.PFCRP0.B.DPFEN = 1;

	/* Enable instruction prefetch. */
	FLASH.PFCRP0.B.IPFEN = 1;

	/* Prefetch algorithm. */
	/* TODO: Ask Freescale about this option. */
	FLASH.PFCRP0.B.PFLIM = 2;

	/* Enable line read buffers. */
	FLASH.PFCRP0.B.BFEN = 1;

	/* Enable pipelined reads again. */
	FLASH.MCR.B.PRD = 0;
#elif defined(CFG_MPC5668)
	/* Check values from cookbook and MPC5668x Microcontroller Data Sheet */

	/* Should probably trim this values */
	const typeof(FLASH.PFCRP0.B) val = {.M0PFE = 1, .M2PFE=1, .APC=3,
								 .RWSC=3, .WWSC =1, .DPFEN =1, .IPFEN = 1, .PFLIM =2,
								 .BFEN  = 1 };
	FLASH.PFCRP0.B = val;

	/* Enable pipelined reads again. */
#elif defined(CFG_MPC5554) || defined(CFG_MPC5567)
	//TODO: L�gg till flash f�r mpc5554 &67
#endif
}


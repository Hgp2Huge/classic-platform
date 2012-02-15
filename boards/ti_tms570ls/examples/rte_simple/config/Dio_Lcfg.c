/*
* Configuration of module: Dio (Dio_Lcfg.c)
*
* Created by:              
* Copyright:               
*
* Configured for (MCU):    TMS570
*
* Module vendor:           ArcCore
* Generator version:       2.0.0
*
* Generated by Arctic Studio (http://arccore.com) 
*/


	
#include "Dio.h"
#include "Dio_Cfg.h"

const Dio_ChannelType DioChannelConfigData[] = { 
	DIO_CHANNEL_NAME_LED_CHANNEL,
	DIO_END_OF_LIST
};

const Dio_PortType DioPortConfigData[] = { 
	DIO_PORT_NAME_LED_PORT,  
	DIO_END_OF_LIST
};

const Dio_ChannelGroupType DioConfigData[] = {
	{ 
	  .port = DIO_END_OF_LIST, 
	  .offset = DIO_END_OF_LIST, 
	  .mask = DIO_END_OF_LIST, 
	}
};


uint32 Dio_GetPortConfigSize(void)
{
  return sizeof(DioConfigData);
}

/* ###################################################################
**     This component module is generated by Processor Expert. Do not modify it.
**     Filename    : McuDS18B20.h
**     CDE edition : Community
**     Project     : FRDM-K64F_Generator
**     Processor   : MK64FN1M0VLL12
**     Component   : DS18B20
**     Version     : Component 01.017, Driver 01.00, CPU db: 3.00.000
**     Compiler    : GNU C Compiler
**     Date/Time   : 2018-07-03, 08:21, # CodeGen: 331
**     Abstract    :
**
This is a component for the Maxim DS18B20 1-Wire temperature sensor.
**     Settings    :
**          Component Name                                 : McuDS18B20
**          Temperature Reading                            : Auto
**          Connection mode                                : One slave
**          One Wire                                       : McuOneWire
**          SDK                                            : McuLib
**          Shell                                          : Enabled
**            Shell                                        : McuShell
**            Utility                                      : McuUtility
**     Contents    :
**         ReadRom                 - uint8_t McuDS18B20_ReadRom(uint8_t sensor_index);
**         GetRomCode              - uint8_t McuDS18B20_GetRomCode(uint8_t sensor_index, uint8_t **romCodePtr);
**         isBusy                  - bool McuDS18B20_isBusy(void);
**         StartConversion         - uint8_t McuDS18B20_StartConversion(uint8_t sensor_index);
**         ReadTemperature         - uint8_t McuDS18B20_ReadTemperature(uint8_t sensor_index);
**         GetTemperatureRaw       - uint8_t McuDS18B20_GetTemperatureRaw(uint16_t sensor_index, uint32_t *raw);
**         GetTemperatureFloat     - uint8_t McuDS18B20_GetTemperatureFloat(uint8_t sensor_index, float...
**         ReadResolution          - uint8_t McuDS18B20_ReadResolution(uint8_t sensor_index);
**         SetResolution           - uint8_t McuDS18B20_SetResolution(uint8_t sensor_index, DS18B20_ResolutionBits...
**         SearchAndAssignROMCodes - uint8_t McuDS18B20_SearchAndAssignROMCodes(void);
**         ParseCommand            - uint8_t McuDS18B20_ParseCommand(const unsigned char* cmd, bool *handled,...
**         Init                    - void McuDS18B20_Init(void);
**         Deinit                  - void McuDS18B20_Deinit(void);
**
** * Copyright (c) Original implementation: Omar Isa� Pinales Ayala, 2014, all rights reserved.
**  * Updated and maintained by Erich Styger, 2014-2017
**  * Web:         https://mcuoneclipse.com
**  * SourceForge: https://sourceforge.net/projects/mcuoneclipse
**  * Git:         https://github.com/ErichStyger/McuOnEclipse_PEx
**  * All rights reserved.
**  *
**  * Redistribution and use in source and binary forms, with or without modification,
**  * are permitted provided that the following conditions are met:
**  *
**  * - Redistributions of source code must retain the above copyright notice, this list
**  *   of conditions and the following disclaimer.
**  *
**  * - Redistributions in binary form must reproduce the above copyright notice, this
**  *   list of conditions and the following disclaimer in the documentation and/or
**  *   other materials provided with the distribution.
**  *
**  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
**  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
**  * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
**  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
**  * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
**  * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
**  * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
**  * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
**  * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
**  * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
** ###################################################################*/
/*!
** @file McuDS18B20.h
** @version 01.00
** @brief
**
This is a component for the Maxim DS18B20 1-Wire temperature sensor.
*/         
/*!
**  @addtogroup McuDS18B20_module McuDS18B20 module documentation
**  @{
*/         

/* MODULE McuDS18B20. */

#include "McuDS18B20.h"
#include <string.h>
#include "McuOneWire.h" /* interface to 1-Wire */

/* Events */
enum {
  EV_NOTHING,
  EV_INIT,
  EV_NO_BUSY,
  EV_READ_ROM,
  EV_READ_TEMP,
  EV_READ_TEMP_ALL
};

/* Rom commands */
#define RC_READ_ROM          0x33
#define RC_MATCH_ROM         0x55
#define RC_SKIP_ROM          0xCC
#define RC_RELEASE           0xFF

/* Function commands */
#define FC_CONVERT_T         0x44
#define FC_WRITE_SCRATCHPAD  0x4E
#define FC_READ_SCRATCHPAD   0xBE
#define FC_COPY_SCRATCHPAD   0x48

/* configuration byte */
#define DS18B20_CONFIG_SHIFT_R0   (5)  /* R0 in configuration byte starts at bit 0 */
#define DS18B20_CONFIG_ONE_MASK   (0x1F) /* bit 0 to bit 4 are one in configuration byte */

#define DS18B20_NOF_SCRATCHPAD_BYTES  (9) /* number of bytes in scratchpad */

typedef struct {
  int32_t Temperature;
  uint8_t Rom[DS18B20_ROM_CODE_SIZE]; /* family code (0x28), ROM code (6 bytes), CRC (1 byte) */
  DS18B20_ResolutionBits resolution; /* DS18B20_ResolutionBits */
} Sensor_t;

struct {
  int32 Value;
  uint8 Data[10];
#if McuDS18B20_CONFIG_MULTIPLE_BUS_DEVICES
  uint8 WorkSensor;
  unsigned MaxResolution:2;
#endif
  unsigned Busy:1;
} Device;

/* conversion time based on resolution */
static const uint16 ConvTime[4] = {
  94,  /* DS18B20_RESOLUTION_BITS_9: 93.75 ms */
  188, /* DS18B20_RESOLUTION_BITS_10: 187.5 ms */
  375, /* DS18B20_RESOLUTION_BITS_11: 375 ms */
  750  /* DS18B20_RESOLUTION_BITS_12: 750 ms */
};

static Sensor_t Sensor[McuDS18B20_CONFIG_NUMBER_OF_SENSORS];

static uint8_t McuDS18B20_ReadScratchpad(uint8_t sensor_index, uint8_t data[DS18B20_NOF_SCRATCHPAD_BYTES]) {
  uint8_t res;

  if(Device.Busy) {
    return ERR_BUSY;
  }
#if McuDS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=McuDS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#else
  (void)sensor_index; /* not used */
#endif
  Device.Busy = TRUE;
  McuOneWire_SendReset();
#if McuDS18B20_CONFIG_MULTIPLE_BUS_DEVICES
  McuOneWire_SendByte(RC_MATCH_ROM);
  McuOneWire_SendBytes(Sensor[sensor_index].Rom, sizeof(Sensor[sensor_index].Rom));
#else /* single device on the bus, can skip ROM code */
  McuOneWire_SendByte(RC_SKIP_ROM);
#endif
  McuOneWire_SendByte(FC_READ_SCRATCHPAD);
  McuOneWire_Receive(DS18B20_NOF_SCRATCHPAD_BYTES);
  McuOneWire_SendByte(0xFF);
  Device.Busy = FALSE;

  /* extract data */
  res = McuOneWire_GetBytes(&data[0], DS18B20_NOF_SCRATCHPAD_BYTES); /* scratchpad with 9 bytes */
  if (res!=ERR_OK) {
    return res; /* failed getting data */
  }
  /* check CRC */
  if (McuOneWire_CalcCRC(&data[0], 8)!=data[8]) { /* last byte is CRC */
    return ERR_CRC; /* CRC error */
  }
  return ERR_OK;
}

#if McuDS18B20_CONFIG_PARSE_COMMAND_ENABLED
static uint8_t PrintStatus(const McuShell_StdIOType *io) {
  uint8_t buf[48];
  uint8_t res;
  float temp;
  int i;

  McuShell_SendStatusStr((unsigned char*)"McuDS18B20", (unsigned char*)"\r\n", io->stdOut);
#if McuDS18B20_CONFIG_READ_AUTO
  McuShell_SendStatusStr((unsigned char*)"  mode", (unsigned char*)"auto: conversion reads temperature\r\n", io->stdOut);
#else
  McuShell_SendStatusStr((unsigned char*)"  mode", (unsigned char*)"read: start conversion, then read temperature\r\n", io->stdOut);
#endif
#if McuDS18B20_CONFIG_MULTIPLE_BUS_DEVICES
  McuShell_SendStatusStr((unsigned char*)"  multiple", (unsigned char*)"yes\r\n", io->stdOut);
#else
  McuShell_SendStatusStr((unsigned char*)"  multiple", (unsigned char*)"no\r\n", io->stdOut);
#endif
  McuUtility_Num8uToStr(buf, sizeof(buf), McuDS18B20_CONFIG_NUMBER_OF_SENSORS);
  McuUtility_strcat(buf, sizeof(buf), (unsigned char*)"\r\n");
  McuShell_SendStatusStr((unsigned char*)"  sensors", buf, io->stdOut);

  for(i=0;i<McuDS18B20_CONFIG_NUMBER_OF_SENSORS;i++) {
    /* ROM Code */
    McuUtility_Num8uToStr(buf, sizeof(buf), i); /* sensor number */
    McuUtility_strcat(buf, sizeof(buf), (unsigned char*)": ");
    if (Sensor[i].Rom[0]==0) { /* unknown Rom code? Try to read it from the device */
      res = McuDS18B20_ReadRom(i); /* only one device on the bus: issue READROM command */
    }  else {
      res = ERR_OK; /* do not read the ROM code from the bus, use the stored ROM values */
    }
    if (res==ERR_OK) {
      buf[0] = '\0';
      McuOneWire_strcatRomCode(buf, sizeof(buf), Sensor[i].Rom);
    } else {
      McuUtility_strcat(buf, sizeof(buf), (unsigned char*)"ERROR (");
      McuUtility_strcatNum8u(buf, sizeof(buf), res);
      McuUtility_strcat(buf, sizeof(buf), (unsigned char*)")");
    }
    McuUtility_strcat(buf, sizeof(buf), (unsigned char*)"\r\n");
    McuShell_SendStatusStr((unsigned char*)"  ROM code", buf, io->stdOut);

    /* resolution */
    McuUtility_Num8uToStr(buf, sizeof(buf), i); /* sensor number */
    McuUtility_strcat(buf, sizeof(buf), (unsigned char*)": ");
    switch(Sensor[i].resolution) {
      case DS18B20_RESOLUTION_BITS_9:  McuUtility_strcat(buf, sizeof(buf), (unsigned char*)"9 bits (93.75 ms conversion time)\r\n"); break;
      case DS18B20_RESOLUTION_BITS_10: McuUtility_strcat(buf, sizeof(buf), (unsigned char*)"10 bits (187.5 ms conversion time)\r\n"); break;
      case DS18B20_RESOLUTION_BITS_11: McuUtility_strcat(buf, sizeof(buf), (unsigned char*)"11 bits (375 ms conversion time)\r\n"); break;
      case DS18B20_RESOLUTION_BITS_12: McuUtility_strcat(buf, sizeof(buf), (unsigned char*)"12 bits (750 ms conversion time)\r\n"); break;
      default: McuUtility_strcat(buf, sizeof(buf), (unsigned char*)"UNKNOWN bits\r\n"); break;
    } /* switch */
    McuShell_SendStatusStr((unsigned char*)"  resolution", buf, io->stdOut);

    /* Temperature */
    res = McuDS18B20_StartConversion(i);
    if (res==ERR_OK) {
      McuUtility_Num8uToStr(buf, sizeof(buf), i); /* sensor number */
      McuUtility_strcat(buf, sizeof(buf), (unsigned char*)": ");
#if McuDS18B20_CONFIG_READ_AUTO
      res = ERR_OK; /* in auto mode, temperature already has been read from sensor during StartConversion() */
#else
      McuWait_WaitOSms(ConvTime[Sensor[i].resolution]);
      res = McuDS18B20_ReadTemperature(i);
#endif
      if (res==ERR_OK) {
        res = McuDS18B20_GetTemperatureFloat(i, &temp);
        if (res==ERR_OK) {
          McuUtility_strcatNumFloat(buf, sizeof(buf), temp, 4);
          McuUtility_strcat(buf, sizeof(buf), (unsigned char*)"�C");
        } else {
          McuUtility_strcat(buf, sizeof(buf), (unsigned char*)"ERROR (");
          McuUtility_strcatNum8u(buf, sizeof(buf), res);
          McuUtility_strcat(buf, sizeof(buf), (unsigned char*)")");
        }
      } else {
        McuUtility_strcat(buf, sizeof(buf), (unsigned char*)"ERROR (");
        McuUtility_strcatNum8u(buf, sizeof(buf), res);
        McuUtility_strcat(buf, sizeof(buf), (unsigned char*)")");
      }
    } else {
      McuUtility_strcpy(buf, sizeof(buf), (unsigned char*)"ERROR (");
      McuUtility_strcatNum8u(buf, sizeof(buf), res);
      McuUtility_strcat(buf, sizeof(buf), (unsigned char*)")");
    }
    McuUtility_strcat(buf, sizeof(buf), (unsigned char*)"\r\n");
    McuShell_SendStatusStr((unsigned char*)"  temperature", buf, io->stdOut);
  } /* for */
  return ERR_OK;
}

static uint8_t PrintHelp(const McuShell_StdIOType *io) {
  McuShell_SendHelpStr((unsigned char*)"McuDS18B20", (unsigned char*)"Group of McuDS18B20 commands\r\n", io->stdOut);
  McuShell_SendHelpStr((unsigned char*)"  help|status", (unsigned char*)"Print help or status information\r\n", io->stdOut);
  McuShell_SendHelpStr((unsigned char*)"  search", (unsigned char*)"Search for DS18B20 devices on the bus and assign ROM code\r\n", io->stdOut);
  McuShell_SendHelpStr((unsigned char*)"  set res <idx> <bits>", (unsigned char*)"Set sensor resolution to (9, 10, 11, 12) bits\r\n", io->stdOut);
  McuShell_SendHelpStr((unsigned char*)"  read res <idx>", (unsigned char*)"Read sensor resolution\r\n", io->stdOut);
  McuShell_SendHelpStr((unsigned char*)"  start conv <idx>", (unsigned char*)"Start temperature conversion\r\n", io->stdOut);
  McuShell_SendHelpStr((unsigned char*)"  read temp <idx>", (unsigned char*)"Read sensor temperature\r\n", io->stdOut);
  return ERR_OK;
}
#endif /* McuDS18B20_CONFIG_PARSE_COMMAND_ENABLED */

/*
** ===================================================================
**     Method      :  ParseCommand (component DS18B20)
**
**     Description :
**         Shell Command Line parser. Method is only available if Shell
**         is enabled in the component properties.
**     Parameters  :
**         NAME            - DESCRIPTION
**         cmd             - command string
**       * handled         - Pointer to variable which tells if
**                           the command has been handled or not
**         io              - Pointer to I/O structure
**     Returns     :
**         ---             - Error code
** ===================================================================
*/
uint8_t McuDS18B20_ParseCommand(const unsigned char* cmd, bool *handled, const McuShell_StdIOType *io)
{
#if McuDS18B20_CONFIG_PARSE_COMMAND_ENABLED
  uint8_t res = ERR_OK;
  const unsigned char *p;
  uint8_t sensorNr;
  float temperature;
  uint8_t buf[32];

  if (McuUtility_strcmp((char*)cmd, McuShell_CMD_HELP) == 0
    || McuUtility_strcmp((char*)cmd, "McuDS18B20 help") == 0)
  {
    *handled = TRUE;
    return PrintHelp(io);
  } else if (   (McuUtility_strcmp((char*)cmd, McuShell_CMD_STATUS)==0)
             || (McuUtility_strcmp((char*)cmd, "McuDS18B20 status") == 0)
            )
  {
    *handled = TRUE;
    res = PrintStatus(io);
  } else if (McuUtility_strncmp((char*)cmd, "McuDS18B20 start conv ", sizeof("McuDS18B20 start conv ")-1) == 0) {
    *handled = TRUE;
    p = cmd + sizeof("McuDS18B20 start conv ")-1;
    if (McuUtility_ScanDecimal8uNumber(&p, &sensorNr)==ERR_OK) {
      if (McuDS18B20_StartConversion(sensorNr)!=ERR_OK) {
        McuShell_SendStr((unsigned char*)"Failed starting conversion!\r\n", io->stdErr);
        res = ERR_FAILED;
      }
    } else {
      McuShell_SendStr((unsigned char*)"Wrong sensor index\r\n", io->stdErr);
      res = ERR_FAILED;
    }
  } else if (McuUtility_strncmp((char*)cmd, "McuDS18B20 set res ", sizeof("McuDS18B20 set res ")-1) == 0) {
    DS18B20_ResolutionBits resolution;

    *handled = TRUE;
    p = cmd + sizeof("McuDS18B20 set res ")-1;
    if (   McuUtility_ScanDecimal8uNumber(&p, &sensorNr)==ERR_OK
        && McuUtility_ScanDecimal8uNumber(&p, &resolution)==ERR_OK
        && resolution>=9 && resolution<=12
        )
    {
      switch((int)resolution) {
        case 9:  resolution = DS18B20_RESOLUTION_BITS_9; break;
        case 10: resolution = DS18B20_RESOLUTION_BITS_10; break;
        case 11: resolution = DS18B20_RESOLUTION_BITS_11; break;
        case 12: resolution = DS18B20_RESOLUTION_BITS_12; break;
        default: resolution = DS18B20_RESOLUTION_BITS_12; break;
      }
      if (McuDS18B20_SetResolution(sensorNr, resolution)!=ERR_OK) {
        McuShell_SendStr((unsigned char*)"Failed starting conversion!\r\n", io->stdErr);
        res = ERR_FAILED;
      }
    } else {
      McuShell_SendStr((unsigned char*)"Wrong sensor index or resolution!\r\n", io->stdErr);
      res = ERR_FAILED;
    }
  } else if (McuUtility_strncmp((char*)cmd, "McuDS18B20 read res ", sizeof("McuDS18B20 read res ")-1) == 0) {
    *handled = TRUE;
    p = cmd + sizeof("McuDS18B20 read res ")-1;
    if (McuUtility_ScanDecimal8uNumber(&p, &sensorNr)==ERR_OK) {
      if (McuDS18B20_ReadResolution(sensorNr)!=ERR_OK) {
        McuShell_SendStr((unsigned char*)"Failed reading resolution\r\n", io->stdErr);
        res = ERR_FAILED;
      } else {
        switch(Sensor[sensorNr].resolution) {
          case DS18B20_RESOLUTION_BITS_9: McuUtility_strcpy(buf, sizeof(buf), (unsigned char*)"9 bits\r\n"); break;
          case DS18B20_RESOLUTION_BITS_10: McuUtility_strcpy(buf, sizeof(buf), (unsigned char*)"10 bits\r\n"); break;
          case DS18B20_RESOLUTION_BITS_11: McuUtility_strcpy(buf, sizeof(buf), (unsigned char*)"11 bits\r\n"); break;
          case DS18B20_RESOLUTION_BITS_12: McuUtility_strcpy(buf, sizeof(buf), (unsigned char*)"12 bits\r\n"); break;
          default: McuUtility_strcpy(buf, sizeof(buf), (unsigned char*)"UNKNOWN bits\r\n"); break;
        } /* switch */
        McuShell_SendStr(buf, io->stdOut);
        res = ERR_OK;
      }
    } else {
      McuShell_SendStr((unsigned char*)"Wrong sensor index\r\n", io->stdErr);
      res = ERR_FAILED;
    }
  } else if (McuUtility_strncmp((char*)cmd, "McuDS18B20 read temp ", sizeof("McuDS18B20 read temp ")-1) == 0) {
    *handled = TRUE;
    p = cmd + sizeof("McuDS18B20 read temp ")-1;
    if (McuUtility_ScanDecimal8uNumber(&p, &sensorNr)==ERR_OK) {
      if (McuDS18B20_ReadTemperature(sensorNr)!=ERR_OK) {
        McuShell_SendStr((unsigned char*)"Failed reading temperature!\r\n", io->stdErr);
        res = ERR_FAILED;
      } else {
        if (McuDS18B20_GetTemperatureFloat(sensorNr, &temperature)!=ERR_OK) {
          McuShell_SendStr((unsigned char*)"Failed getting temperature!\r\n", io->stdErr);
          res = ERR_FAILED;
        } else {
          buf[0] = '\0';
          McuUtility_strcatNumFloat(buf, sizeof(buf), temperature, 4);
          McuUtility_strcat(buf, sizeof(buf), (unsigned char*)"�C\r\n");
          McuShell_SendStr(buf, io->stdOut);
          res = ERR_OK;
        }
      }
    } else {
      McuShell_SendStr((unsigned char*)"Wrong sensor index\r\n", io->stdErr);
      res = ERR_FAILED;
    }
  } else if (McuUtility_strcmp((char*)cmd, "McuDS18B20 search")==0) {
    uint8_t rom[McuOneWire_ROM_CODE_SIZE];
    bool found;
    int i, nofFound = 0;

    *handled = TRUE;
    McuOneWire_ResetSearch(); /* reset search fields */
    McuOneWire_TargetSearch(DS18B20_FAMILY_CODE); /* only search for DS18B20 */
    do {
      found = McuOneWire_Search(&rom[0], TRUE);
      if (found) {
        buf[0] = '\0';
        (void)McuOneWire_strcatRomCode(buf, sizeof(buf), &rom[0]);
        McuUtility_strcat(buf, sizeof(buf), (unsigned char*)"\r\n");
        McuShell_SendStr(buf, io->stdOut);
        /* store ROM code in device list */
        for(i=0;i<McuOneWire_ROM_CODE_SIZE;i++) {
          Sensor[nofFound].Rom[i] = rom[i];
        }
        nofFound++;
      }
    } while(found && nofFound<McuDS18B20_CONFIG_NUMBER_OF_SENSORS);
    if (nofFound==0) {
      McuShell_SendStr((unsigned char*)"No device found!\r\n", io->stdErr);
    }
    return ERR_OK;
  }
  return res;
#else
  (void)cmd;
  (void)handled;
  (void)io;
  return ERR_OK;
#endif
}

/*
** ===================================================================
**     Method      :  StartConversion (component DS18B20)
**
**     Description :
**         Starts the conversion of temperature in the sensor.
**     Parameters  :
**         NAME            - DESCRIPTION
**         sensor_index    - Sensor index, use zero
**                           if only using one sensor
**     Returns     :
**         ---             - error code
** ===================================================================
*/
uint8_t McuDS18B20_StartConversion(uint8_t sensor_index)
{
#if McuDS18B20_CONFIG_READ_AUTO
  uint8_t res;
#endif

  if(Device.Busy) {
    return ERR_BUSY;
  }
#if McuDS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=McuDS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#endif
  Device.Busy = TRUE;
  McuOneWire_SendReset();
#if McuDS18B20_CONFIG_MULTIPLE_BUS_DEVICES
  McuOneWire_SendByte(RC_MATCH_ROM);
  McuOneWire_SendBytes(Sensor[sensor_index].Rom, DS18B20_ROM_CODE_SIZE);
  McuOneWire_SendByte(FC_CONVERT_T);
#else /* only single device on the bus, can skip ROM code */
  McuOneWire_SendByte(RC_SKIP_ROM);
  McuOneWire_SendByte(FC_CONVERT_T);
#endif
  Device.Busy = FALSE;
#if McuDS18B20_CONFIG_READ_AUTO
  McuWait_WaitOSms(ConvTime[Sensor[sensor_index].resolution]);
  res = McuDS18B20_ReadTemperature(sensor_index);
  if (res!=ERR_OK) {
    return res;
  }
#endif
  return ERR_OK;
}

/*
** ===================================================================
**     Method      :  SetResolution (component DS18B20)
**
**     Description :
**         Sets the resolution
**     Parameters  :
**         NAME            - DESCRIPTION
**         sensor_index    - Index of the sensor to
**                           set the resolution.
**         config_bits     - Two bits resolution config
**                           value: 
**                           [0b00] - 9 bits.
**                           [0b01] - 10 bits.
**                           [0b10] - 11 bits.
**                           [0b11] - 12 bits.
**     Returns     :
**         ---             - error code
** ===================================================================
*/
uint8_t McuDS18B20_SetResolution(uint8_t sensor_index, DS18B20_ResolutionBits resolution)
{
  uint8_t config;

  if(Device.Busy) {
    return ERR_BUSY;
  }
#if McuDS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=McuDS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#else
  (void)sensor_index; /* not used */
#endif
  Sensor[sensor_index].resolution = resolution;
  config = (((uint8_t)(resolution<<DS18B20_CONFIG_SHIFT_R0))|DS18B20_CONFIG_ONE_MASK); /* build proper configuration register mask */
  Device.Busy = TRUE;
  McuOneWire_SendReset();
#if McuDS18B20_CONFIG_MULTIPLE_BUS_DEVICES
  Device.WorkSensor = sensor_index;
  McuOneWire_SendByte(RC_MATCH_ROM);
  McuOneWire_SendBytes(Sensor[sensor_index].Rom, DS18B20_ROM_CODE_SIZE);
  McuOneWire_SendByte(FC_WRITE_SCRATCHPAD);
  McuOneWire_SendByte(0x01);
  McuOneWire_SendByte(0x10);
  McuOneWire_SendByte(config);
#else /* only single device on the bus, can skip ROM code */
  McuOneWire_SendByte(RC_SKIP_ROM);
  McuOneWire_SendByte(FC_WRITE_SCRATCHPAD);
  McuOneWire_SendByte(0x01);
  McuOneWire_SendByte(0x10);
  McuOneWire_SendByte(config);
#endif
  Device.Busy = FALSE;
  return ERR_OK;
}


/*
** ===================================================================
**     Method      :  ReadTemperature (component DS18B20)
**
**     Description :
**         Read the temperature value from the sensor and stores it in
**         memory
**     Parameters  :
**         NAME            - DESCRIPTION
**         sensor_index    - Sensor index, use zero
**                           if only using one sensor
**     Returns     :
**         ---             - error code
** ===================================================================
*/
uint8_t McuDS18B20_ReadTemperature(uint8_t sensor_index)
{
  uint8_t data[DS18B20_NOF_SCRATCHPAD_BYTES]; /* scratchpad data */
  uint8_t res;

  res = McuDS18B20_ReadScratchpad(sensor_index, &data[0]);
  if (res!=ERR_OK) {
    return res;
  }
  /* temperature is in data[0] (LSB) and data[1] (MSB) */
#if McuDS18B20_CONFIG_NUMBER_OF_SENSORS>1
  Sensor[sensor_index].Temperature = (data[1]<<8)+data[0];
#else
  Sensor[0].Temperature = (data[1]<<8)+data[0];
#endif
  return ERR_OK;
}

/*
** ===================================================================
**     Method      :  ReadResolution (component DS18B20)
**
**     Description :
**         Read the sensor resolution sensor and stores it in memory
**     Parameters  :
**         NAME            - DESCRIPTION
**         sensor_index    - Sensor index, use zero
**                           if only using one sensor
**     Returns     :
**         ---             - error code
** ===================================================================
*/
uint8_t McuDS18B20_ReadResolution(uint8_t sensor_index)
{
  uint8_t data[DS18B20_NOF_SCRATCHPAD_BYTES]; /* scratchpad data */
  uint8_t res;

#if McuDS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=McuDS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#endif
  res = McuDS18B20_ReadScratchpad(sensor_index, &data[0]);
  if (res!=ERR_OK) {
    return res;
  }
  /* configuration is in data[4] */
  Sensor[sensor_index].resolution = ((data[4])>>DS18B20_CONFIG_SHIFT_R0)&0x3; /* only two bits */
  return ERR_OK;
}

/*
** ===================================================================
**     Method      :  GetTemperature (component DS18B20)
**
**     Description :
**         Gets the temperature from memory.
**     Parameters  :
**         NAME            - DESCRIPTION
**       * sensor_index    - Index of the sensor to
**                           get the temperature value.
**     Returns     :
**         ---             - 
** ===================================================================
*/
uint8_t McuDS18B20_GetTemperatureRaw(uint16_t sensor_index, uint32_t *raw)
{
#if McuDS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=McuDS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#else
  (void)sensor_index; /* not used */
#endif
#if McuDS18B20_CONFIG_NUMBER_OF_SENSORS>1
  *raw = Sensor[sensor_index].Temperature;
#else
  *raw = Sensor[0].Temperature;
#endif
  return ERR_OK;
}

/*
** ===================================================================
**     Method      :  GetTemperatureFloat (component DS18B20)
**
**     Description :
**         Returns the temperature from memory in floating point format.
**     Parameters  :
**         NAME            - DESCRIPTION
**         sensor_index    - Sensor index, use zero
**                           if only using one sensor
**       * temperature     - Pointer to where to store
**                           the value
**     Returns     :
**         ---             - Error code
** ===================================================================
*/
uint8_t McuDS18B20_GetTemperatureFloat(uint8_t sensor_index, float *temperature)
{
  int16_t raw, intPart, fracPart;

#if McuDS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=McuDS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#else
  (void)sensor_index; /* not used */
#endif
#if McuDS18B20_CONFIG_NUMBER_OF_SENSORS>1
  raw = Sensor[sensor_index].Temperature;
#else
  raw = Sensor[0].Temperature;
#endif
  intPart = raw>>4; /* integral part */
  fracPart = raw&0xf; /* fractional part */
  *temperature = ((float)intPart) + (((float)fracPart)*0.0625);
  return ERR_OK;
}


/*
** ===================================================================
**     Method      :  isBusy (component DS18B20)
**
**     Description :
**         Returns TRUE if there are a operation in progress.
**     Parameters  : None
**     Returns     :
**         ---             - Returns TRUE if the device is busy, and
**                           FALSE if its ready to operate.
** ===================================================================
*/
bool McuDS18B20_isBusy(void)
{
  return Device.Busy;
}

/*
** ===================================================================
**     Method      :  GetRomCode (component DS18B20)
**
**     Description :
**         Gets the rom code from the memory.
**     Parameters  :
**         NAME            - DESCRIPTION
**         sensor_index    - Sensor index, use zero
**                           if only using one sensor
**       * romCodePtr      - Pointer to a pointer for the
**                           return value
**     Returns     :
**         ---             - error code
** ===================================================================
*/
uint8_t McuDS18B20_GetRomCode(uint8_t sensor_index, uint8_t **romCodePtr)
{
#if McuDS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=McuDS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#else
  (void)sensor_index; /* not used */
#endif
#if McuDS18B20_CONFIG_NUMBER_OF_SENSORS>1
  *romCodePtr = &Sensor[sensor_index].Rom[0];
#else
  *romCodePtr = &Sensor[0].Rom[0];
#endif
  return ERR_OK; /* ok */
}

/*
** ===================================================================
**     Method      :  ReadRom (component DS18B20)
**
**     Description :
**         Starts to read the rom code and saves it in memory.
**     Parameters  :
**         NAME            - DESCRIPTION
**         sensor_index    - Sensor index, use zero
**                           if only using one sensor
**     Returns     :
**         ---             - error code
** ===================================================================
*/
uint8_t McuDS18B20_ReadRom(uint8_t sensor_index)
{
  uint8_t res;

  if(Device.Busy) {
    return ERR_BUSY;
  }
#if McuDS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=McuDS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#else
  (void)sensor_index; /* not used */
#endif
  Device.Busy = TRUE;
#if McuDS18B20_CONFIG_NUMBER_OF_SENSORS>1
  Device.WorkSensor = sensor_index;
#endif
#if McuDS18B20_CONFIG_NUMBER_OF_SENSORS>1
  res = McuOneWire_ReadRomCode(Sensor[sensor_index].Rom);
#else
  res = McuOneWire_ReadRomCode(Sensor[0].Rom); /* 8 bytes */
#endif
  Device.Busy = FALSE;
  /* index 0  : family code
     index 1-6: 48bit serial number
     index 7  : CRC
  */
  if (res!=ERR_OK) {
    return res; /* error */
  }
#if McuDS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if (Sensor[sensor_index].Rom[0]!=DS18B20_FAMILY_CODE) {
#else
  if (Sensor[0].Rom[0]!=DS18B20_FAMILY_CODE) {
#endif
    return ERR_FAILED; /* not a DS18B20? */
  }
  return ERR_OK; /* ok */
}

/*
** ===================================================================
**     Method      :  Deinit (component DS18B20)
**
**     Description :
**         Driver deinitialization routine
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/
void McuDS18B20_Deinit(void)
{
  /* nothing special needed here */
}


/*
** ===================================================================
**     Method      :  Init (component DS18B20)
**
**     Description :
**         Initializes the device.
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/
void McuDS18B20_Init(void)
{
#if McuDS18B20_CONFIG_MULTIPLE_BUS_DEVICES
  int i;

  for(i=0;i<McuDS18B20_CONFIG_NUMBER_OF_SENSORS;i++) {
    Sensor[i].resolution = DS18B20_RESOLUTION_BITS_12;
  }
  memcpy(Sensor[0].Rom, "\x28\x00\x00\x00\x00\x00\x00\x00", 8);
#else
  memset(&Sensor[0], 0, sizeof(Sensor[0])); /* initialize all fields with zero */
  Sensor[0].resolution = DS18B20_RESOLUTION_BITS_12; /* default resolution */
#endif
  Device.Value = 8500000; /* dummy value or 85�C */
}

/*
** ===================================================================
**     Method      :  SearchAndAssignROMCodes (component DS18B20)
**
**     Description :
**         Scans the devices on the bus and assigns the ROM codes to
**         the list of available sensors
**     Parameters  : None
**     Returns     :
**         ---             - Error code
** ===================================================================
*/
uint8_t McuDS18B20_SearchAndAssignROMCodes(void)
{
  uint8_t rom[McuOneWire_ROM_CODE_SIZE];
  bool found;
  int i, nofFound = 0;

  McuOneWire_ResetSearch(); /* reset search fields */
  McuOneWire_TargetSearch(DS18B20_FAMILY_CODE); /* only search for DS18B20 */
  do {
    found = McuOneWire_Search(&rom[0], TRUE);
    if (found) {
      /* store ROM code in device list */
      for(i=0;i<McuOneWire_ROM_CODE_SIZE;i++) {
        Sensor[nofFound].Rom[i] = rom[i];
      }
      nofFound++;
    }
  } while(found && nofFound<McuDS18B20_CONFIG_NUMBER_OF_SENSORS);
  if (nofFound==0) {
    return ERR_FAILED; /* nothing found */
  }
  return ERR_OK;
}

/* END McuDS18B20. */

/*!
** @}
*/


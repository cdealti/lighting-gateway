/**************************************************************************************************
 * Filename:       interface_srpcserver.h
 * Description:    Socket Remote Procedure Call Interface - sample device application.
 *
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
#ifndef ZCL_INTERFACE_SERVER_H
#define ZCL_INTERFACE_SERVER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "zllSocCmd.h"
#include "interface_devicelist.h"

//define the outgoing RPSC command ID's
#define RPCS_NEW_ZLL_DEVICE     0x0001
#define RPCS_DEV_ANNCE		      0x0002
#define RPCS_SIMPLE_DESC	      0x0003
#define RPCS_TEMP_READING       0x0004
#define RPCS_POWER_READING      0x0005
#define RPCS_PING               0x0006
#define RPCS_GET_DEV_STATE_RSP  0x0007	
#define RPCS_GET_DEV_LEVEL_RSP  0x0008
#define RPCS_GET_DEV_HUE_RSP    0x0009
#define RPCS_GET_DEV_SAT_RSP    0x000a
#define RPCS_ADD_GROUP_RSP      0x000b
#define RPCS_GET_GROUP_RSP      0x000c
#define RPCS_ADD_SCENE_RSP      0x000d
#define RPCS_GET_SCENE_RSP      0x000e


//define incoming RPCS command ID's
#define RPCS_CLOSE              0x80;
#define RPCS_GET_DEVICES        0x81;
#define RPCS_SET_DEV_STATE      0x82;	
#define RPCS_SET_DEV_LEVEL      0x83;	
#define RPCS_SET_DEV_COLOR      0x84;
#define RPCS_GET_DEV_STATE      0x85;	
#define RPCS_GET_DEV_LEVEL      0x86;	
#define RPCS_GET_DEV_HUE        0x87;
#define RPCS_GET_DEV_SAT        0x88;
#define RPCS_BIND_DEVICES       0x89;
#define RPCS_GET_THERM_READING  0x8a;
#define RPCS_GET_POWER_READING  0x8b;
#define RPCS_DISCOVER_DEVICES   0x8c;
#define RPCS_SEND_ZCL           0x8d;
#define RPCS_GET_GROUPS         0x8e;	
#define RPCS_ADD_GROUP          0x8f;	
#define RPCS_GET_SCENES         0x90;	
#define RPCS_STORE_SCENE        0x91;	
#define RPCS_RECALL_SCENE       0x92;	
#define RPCS_IDENTIFY_DEVICE    0x93;	
#define RPCS_CHANGE_DEVICE_NAME 0x94;
#define RPCS_REMOVE_DEVICE      0x95;

#define SRPC_FUNC_ID 0
#define SRPC_MSG_LEN 1

#define SRPC_TCP_PORT 0x2be3

#define CLOSE_AUTH_NUM 0x2536

typedef struct
{
  union
  {
    uint16_t      shortAddr;
    uint8_t       extAddr[Z_EXTADDR_LEN];
  } addr;
  afAddrMode_t addrMode;
  uint8_t endPoint;
  uint16_t panId;  // used for the INTER_PAN feature
} afAddrType_t;

//RPSC ZLL Interface functions
void SRPC_Init(void);
uint8_t RSPC_SendEpInfo(epInfo_t *epInfo);

void RPCS_ZLL_CallBack_getStateRsp(uint8_t state, uint16_t srcAddr, uint8_t endpoint, uint32_t clientFd);
void RPCS_ZLL_CallBack_getLevelRsp(uint8_t level, uint16_t srcAddr, uint8_t endpoint, uint32_t clientFd);
void RPCS_ZLL_CallBack_getHueRsp(uint8_t hue, uint16_t srcAddr, uint8_t endpoint, uint32_t clientFd);
void RPCS_ZLL_CallBack_getSatRsp(uint8_t sat, uint16_t srcAddr, uint8_t endpoint, uint32_t clientFd);
void RPCS_ZLL_CallBack_getTempRsp(uint16_t temp, uint16_t srcAddr, uint8_t endpoint, uint32_t clientFd);
void RPCS_ZLL_CallBack_getPowerRsp(uint32_t power, uint16_t srcAddr, uint8_t endpoint, uint32_t clientFd);

#ifdef __cplusplus
}
#endif

#endif //ZCL_INTERFACE_SERVER_H
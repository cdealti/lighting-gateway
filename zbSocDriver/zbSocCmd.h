/*
 * zbSocCmd.h
 *
 * This module contains the API for the zll SoC Host Interface.
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

#ifndef ZLLSOCCMD_H
#define ZLLSOCCMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/********************************************************************/
// ZLL Soc Types
typedef enum
{
  afAddrNotPresent = 0,
  afAddrGroup      = 1,
  afAddr16Bit      = 2,
  afAddr64Bit      = 3,
  afAddrBroadcast  = 15
} afAddrMode_t;

#define Z_EXTADDR_LEN 8

// Endpoint information record entry
typedef struct {
	uint8_t IEEEAddr[8];
	uint16_t nwkAddr; // Network address
	uint8_t endpoint; // Endpoint identifier
	uint16_t profileID; // Profile identifier
	uint16_t deviceID; // Device identifier
	uint8_t version; // Version
	char* deviceName;
	uint8_t status;
  uint8_t flags;
} epInfo_t;

typedef struct
{
	epInfo_t * epInfo;
	uint16_t prevNwkAddr;	// Precious network address
	uint8_t type;	// new / updated / old
} epInfoExtended_t;

#define EP_INFO_TYPE_EXISTING 0
#define EP_INFO_TYPE_NEW 1
#define EP_INFO_TYPE_UPDATED 2
#define EP_INFO_TYPE_REMOVED 4

typedef uint8_t (*zbSocTlIndicationCb_t)(epInfo_t *epInfo);
typedef uint8_t (*zllNewDevIndicationCb_t)(epInfo_t *epInfo);
typedef uint8_t (*zbSocZclGetStateCb_t)(uint8_t state, uint16_t nwkAddr,
		uint8_t endpoint);
typedef uint8_t (*zbSocZclGetLevelCb_t)(uint8_t level, uint16_t nwkAddr,
		uint8_t endpoint);
typedef uint8_t (*zbSocZclGetHueCb_t)(uint8_t hue, uint16_t nwkAddr,
		uint8_t endpoint);
typedef uint8_t (*zbSocZclGetSatCb_t)(uint8_t sat, uint16_t nwkAddr,
		uint8_t endpoint);
typedef uint8_t (*zdoSimpleDescRspCb_t)(epInfo_t *epInfo);
typedef uint8_t (*zdoLeaveIndCb_t)(uint16_t nwkAddr);
typedef uint8_t (*utilGetDevInfoRspcb_t) (uint8_t status, uint16_t nwkAddr, uint8_t ieeeAddr[8], uint8_t devType, uint8_t devState);
typedef uint8_t (*zbSocZclGetModelCb_t)(uint8_t *ModelId);

typedef struct {
	zbSocTlIndicationCb_t pfnTlIndicationCb; // TouchLink Indication callback
	zllNewDevIndicationCb_t pfnNewDevIndicationCb; // New device Indication callback
	zbSocZclGetStateCb_t pfnZclGetStateCb; // ZCL response callback for get State
	zbSocZclGetLevelCb_t pfnZclGetLevelCb; // ZCL response callback for get Level
	zbSocZclGetHueCb_t pfnZclGetHueCb; // ZCL response callback for get Hue
	zbSocZclGetSatCb_t pfnZclGetSatCb; // ZCL response callback for get Sat
	zdoSimpleDescRspCb_t pfnZdoSimpleDescRspCb; // ZCL callback for simple desc response
	zdoLeaveIndCb_t pfnZdoLeaveIndCb; // ZCL callback for simple desc response
	utilGetDevInfoRspcb_t pfnUtilGetDevInfoRsp;
	zbSocZclGetModelCb_t pfnZclGetModelCb; //
} zbSocCallbacks_t;


/********************************************************************/
// ZLL Soc API
//configuration API's
int32_t zbSocOpen(char *devicePath);
void zbSocRegisterCallbacks(zbSocCallbacks_t zbSocCallbacks);
void zbSocClose(void);
void zbSocProcessRpc(void);

//ZLL API's
void zbSocTouchLink(void);
void zbSocResetToFn(void);
void zbSocSendResetToFn(void);
void zbSocOpenNwk(uint8_t duration);
//ZCL Set API's
void zbSocSetState(uint8_t state, uint16_t dstAddr, uint8_t endpoint,
		uint8_t addrMode);
void zbSocSetLevel(uint8_t level, uint16_t time, uint16_t dstAddr,
		uint8_t endpoint, uint8_t addrMode);
void zbSocSetHue(uint8_t hue, uint16_t time, uint16_t dstAddr,
		uint8_t endpoint, uint8_t addrMode);
void zbSocSetSat(uint8_t sat, uint16_t time, uint16_t dstAddr,
		uint8_t endpoint, uint8_t addrMode);
void zbSocSetHueSat(uint8_t hue, uint8_t sat, uint16_t time, uint16_t dstAddr,
		uint8_t endpoint, uint8_t addrMode);
void zbSocAddGroup(uint16_t groupId, uint16_t dstAddr, uint8_t endpoint,
		uint8_t addrMode);
void zbSocStoreScene(uint16_t groupId, uint8_t sceneId, uint16_t dstAddr,
		uint8_t endpoint, uint8_t addrMode);
void zbSocRecallScene(uint16_t groupId, uint8_t sceneId, uint16_t dstAddr,
		uint8_t endpoint, uint8_t addrMode);
void zbSocBind(uint16_t srcNwkAddr, uint8_t srcEndpoint, uint8_t srcIEEE[8],
		uint8_t dstEndpoint, uint8_t dstIEEE[8], uint16_t clusterID);
void zbSocSendIdentify(uint16_t identifyTime, uint16_t dstAddr,
		uint8_t endpoint, uint8_t addrMode);
void zbSocSendIdentifyEffect(uint8_t effect, uint8_t effectVarient, uint16_t dstAddr,
		uint8_t endpoint, uint8_t addrMode);
void zbSocGetInfo(void);
void zbSocGetModel(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);

//ZCL Get API's
void zbSocGetState(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocGetLevel(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocGetHue(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocGetSat(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
#ifdef __cplusplus
}
#endif

#endif /* ZLLSOCCMD_H */

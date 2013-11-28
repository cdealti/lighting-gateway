/**************************************************************************************************
 * Filename:       interface_srpcserver.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <poll.h>

#include "interface_srpcserver.h"
#include "socket_server.h"
#include "interface_devicelist.h"
#include "interface_grouplist.h"
#include "interface_scenelist.h"

#include "hal_defs.h"

#include "zbSocCmd.h"

void SRPC_RxCB(int clientFd);
void SRPC_ConnectCB(int status);

static uint8_t SRPC_setDeviceState(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_setDeviceLevel(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_setDeviceColor(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDeviceState(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDeviceLevel(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDeviceHue(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDeviceSat(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_bindDevices(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getGroups(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_addGroup(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getScenes(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_storeScene(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_recallScene(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_identifyDevice(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_close(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDevices(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_permitJoin(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_changeDeviceName(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_notSupported(uint8_t *pBuf, uint32_t clientFd);

//RPSC ZLL Interface call back functions
static void SRPC_CallBack_addGroupRsp(uint16_t groupId, char *nameStr,
		uint32_t clientFd);
static void SRPC_CallBack_addSceneRsp(uint16_t groupId, uint8_t sceneId,
		char *nameStr, uint32_t clientFd);

static uint8_t* srpcParseEpInfo(epInfoExtended_t* epInfoEx);

typedef uint8_t (*srpcProcessMsg_t)(uint8_t *pBuf, uint32_t clientFd);

srpcProcessMsg_t rpcsProcessIncoming[] =
{ SRPC_close, //SRPC_CLOSE
		SRPC_getDevices, //SRPC_GET_DEVICES
		SRPC_setDeviceState, //SRPC_SET_DEV_STATE
		SRPC_setDeviceLevel, //SRPC_SET_DEV_LEVEL
		SRPC_setDeviceColor, //SRPC_SET_DEV_COLOR
		SRPC_getDeviceState, //SRPC_GET_DEV_STATE
		SRPC_getDeviceLevel, //SRPC_GET_DEV_LEVEL
		SRPC_getDeviceHue, //SRPC_GET_DEV_HUE
		SRPC_getDeviceSat, //SRPC_GET_DEV_HUE
		SRPC_bindDevices, //SRPC_BIND_DEVICES
		SRPC_notSupported, //reserved
		SRPC_notSupported, //reserved
		SRPC_notSupported, //reserved
		SRPC_notSupported, //reserved
		SRPC_getGroups, //SRPC_GET_GROUPS
		SRPC_addGroup, //SRPC_ADD_GROUP
		SRPC_getScenes, //SRPC_GET_SCENES
		SRPC_storeScene, //SRPC_STORE_SCENE
		SRPC_recallScene, //SRPC_RECALL_SCENE
		SRPC_identifyDevice, //SRPC_IDENTIFY_DEVICE
		SRPC_changeDeviceName, //SRPC_CHANGE_DEVICE_NAME
		SRPC_notSupported, //SRPC_REMOVE_DEVICE
		SRPC_notSupported, //SRPC_RESERVED_10
		SRPC_notSupported, //SRPC_RESERVED_11
		SRPC_notSupported, //SRPC_RESERVED_12
		SRPC_notSupported, //SRPC_RESERVED_13
		SRPC_notSupported, //SRPC_RESERVED_14
		SRPC_notSupported, //SRPC_RESERVED_15
		SRPC_permitJoin, //SRPC_PERMIT_JOIN
		};

static void srpcSend(uint8_t* srpcMsg, int fdClient);
static void srpcSendAll(uint8_t* srpcMsg);

/***************************************************************************************************
 * @fn      srpcParseEpInfp - Parse epInfo and prepare the SRPC message.
 *
 * @brief   Parse epInfo and prepare the SRPC message.
 * @param   epInfo_t* epInfo
 *
 * @return  pSrpcMessage
 ***************************************************************************************************/
static uint8_t* srpcParseEpInfo(epInfoExtended_t* epInfoEx)
{
	uint8_t i;
	uint8_t *pSrpcMessage, *pTmp, devNameLen = 0, pSrpcMessageLen;

	//printf("srpcParseEpInfo++\n");

	//RpcMessage contains function ID param Data Len and param data
	if (epInfoEx->epInfo->deviceName)
	{
		devNameLen = strlen(epInfoEx->epInfo->deviceName);
	}

	//sizre of EP infor - the name char* + num bytes of device name
	//pSrpcMessageLen = sizeof(epInfo_t) - sizeof(char*) + devNameLen;
	pSrpcMessageLen = 2 /* network address */
	+ 1 /* endpoint */
	+ 2 /* profile ID */
	+ 2 /* device ID */
	+ 1 /* version */
	+ 1 /* device name length */
	+ devNameLen /* device name */
	+ 1 /* status */
	+ 8 /* IEEE address */
	+ 1 /* type */
	+ 2 /* previous network address */
	+ 1; /* flags */
	pSrpcMessage = malloc(pSrpcMessageLen + 2);

	pTmp = pSrpcMessage;

	if (pSrpcMessage)
	{
		//Set func ID in SRPC buffer
		*pTmp++ = SRPC_NEW_DEVICE;
		//param size
		*pTmp++ = pSrpcMessageLen;

		*pTmp++ = LO_UINT16(epInfoEx->epInfo->nwkAddr);
		*pTmp++ = HI_UINT16(epInfoEx->epInfo->nwkAddr);
		*pTmp++ = epInfoEx->epInfo->endpoint;
		*pTmp++ = LO_UINT16(epInfoEx->epInfo->profileID);
		*pTmp++ = HI_UINT16(epInfoEx->epInfo->profileID);
		*pTmp++ = LO_UINT16(epInfoEx->epInfo->deviceID);
		*pTmp++ = HI_UINT16(epInfoEx->epInfo->deviceID);
		*pTmp++ = epInfoEx->epInfo->version;

		if (devNameLen > 0)
		{
			*pTmp++ = devNameLen;
			for (i = 0; i < devNameLen; i++)
			{
				*pTmp++ = epInfoEx->epInfo->deviceName[i];
			}
			printf("srpcParseEpInfo: name:%s\n", epInfoEx->epInfo->deviceName);
		}
		else
		{
			*pTmp++ = 0;
		}
		*pTmp++ = epInfoEx->epInfo->status;

		for (i = 0; i < 8; i++)
		{
			//printf("srpcParseEpInfp: IEEEAddr[%d] = %x\n", i, epInfo->IEEEAddr[i]);
			*pTmp++ = epInfoEx->epInfo->IEEEAddr[i];
		}

		*pTmp++ = epInfoEx->type;
		*pTmp++ = LO_UINT16(epInfoEx->prevNwkAddr);
		*pTmp++ = HI_UINT16(epInfoEx->prevNwkAddr);
		*pTmp++ = epInfoEx->epInfo->flags; //bit 0 : start, bit 1: end

	}
	//printf("srpcParseEpInfp--\n");

//  printf("srpcParseEpInfo %0x:%0x\n", epInfo->nwkAddr, epInfo->endpoint);   

	printf(
			"srpcParseEpInfo: %s device, nwkAddr=0x%04X, endpoint=0x%X, profileID=0x%04X, deviceID=0x%04X, flags=0x%02X",
			epInfoEx->type == EP_INFO_TYPE_EXISTING ? "EXISTING" :
			epInfoEx->type == EP_INFO_TYPE_NEW ? "NEW" : "UPDATED",
			epInfoEx->epInfo->nwkAddr, epInfoEx->epInfo->endpoint,
			epInfoEx->epInfo->profileID, epInfoEx->epInfo->deviceID,
			epInfoEx->epInfo->flags);

	if (epInfoEx->type == EP_INFO_TYPE_UPDATED)
	{
		printf(", prevNwkAddr=0x%04X\n", epInfoEx->prevNwkAddr);
	}

	printf("\n");

	return pSrpcMessage;
}

/***************************************************************************************************
 * @fn      srpcSend
 *
 * @brief   Send a message over SRPC to a clients.
 * @param   uint8_t* srpcMsg - message to be sent
 *
 * @return  Status
 ***************************************************************************************************/
static void srpcSend(uint8_t* srpcMsg, int fdClient)
{
	int rtn;

	rtn = socketSeverSend(srpcMsg, (srpcMsg[SRPC_MSG_LEN] + 2), fdClient);
	if (rtn < 0)
	{
		printf("ERROR writing to socket\n");
	}

	return;
}

/***************************************************************************************************
 * @fn      srpcSendAll
 *
 * @brief   Send a message over SRPC to all clients.
 * @param   uint8_t* srpcMsg - message to be sent
 *
 * @return  Status
 ***************************************************************************************************/
static void srpcSendAll(uint8_t* srpcMsg)
{
	int rtn;

	rtn = socketSeverSendAllclients(srpcMsg, (srpcMsg[SRPC_MSG_LEN] + 2));
	if (rtn < 0)
	{
		printf("ERROR writing to socket\n");
	}

	return;
}

/*********************************************************************
 * @fn          SRPC_ProcessIncoming
 *
 * @brief       This function processes incoming messages.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
void SRPC_ProcessIncoming(uint8_t *pBuf, uint32_t clientFd)
{
	srpcProcessMsg_t func;

//  printf("SRPC_ProcessIncoming++[%x]\n", pBuf[SRPC_FUNC_ID]);
	/* look up and call processing function */
	func = rpcsProcessIncoming[(pBuf[SRPC_FUNC_ID] & ~(0x80))];
	if (func)
	{
		(*func)(pBuf, clientFd);
	}
	else
	{
		//printf("Error: no processing function for CMD 0x%x\n", pBuf[SRPC_FUNC_ID]);
	}

	//printf("SRPC_ProcessIncoming--\n");
}

/*********************************************************************
 * @fn          SRPC_addGroup
 *
 * @brief       This function exposes an interface to add a devices to a group.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_addGroup(uint8_t *pBuf, uint32_t clientFd)
{
	uint16_t dstAddr;
	uint8_t addrMode;
	uint8_t endpoint;
	char *nameStr;
	uint8_t nameLen;
	uint16_t groupId;

	//printf("SRPC_addGroup++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	nameLen = *pBuf++;
	nameStr = malloc(nameLen + 1);
	int i;
	for (i = 0; i < nameLen; i++)
	{
		nameStr[i] = *pBuf++;
	}
	nameStr[nameLen] = '\0';

	printf("SRPC_addGroup++: %x:%x:%x name[%d] %s \n", dstAddr, addrMode,
			endpoint, nameLen, nameStr);

	groupId = groupListAddDeviceToGroup(nameStr, dstAddr, endpoint);
	zbSocAddGroup(groupId, dstAddr, endpoint, addrMode);

	SRPC_CallBack_addGroupRsp(groupId, nameStr, clientFd);

	free(nameStr);

	//printf("SRPC_addGroup--\n");

	return 0;
}

/*********************************************************************
 * @fn          uint8_t SRPC_storeScene(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to store a scene.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_storeScene(uint8_t *pBuf, uint32_t clientFd)
{
	uint16_t dstAddr;
	uint8_t addrMode;
	uint8_t endpoint;
	char *nameStr;
	uint8_t nameLen;
	uint16_t groupId;
	uint8_t sceneId;

	//printf("SRPC_storeScene++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	groupId = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += 2;

	nameLen = *pBuf++;
	nameStr = malloc(nameLen + 1);
	int i;
	for (i = 0; i < nameLen; i++)
	{
		nameStr[i] = *pBuf++;
	}
	nameStr[nameLen] = '\0';

	printf("SRPC_storeScene++: name[%d] %s, group %d \n", nameLen, nameStr, groupId);

	sceneId = sceneListAddScene(nameStr, groupId);
	zbSocStoreScene(groupId, sceneId, dstAddr, endpoint, addrMode);
	SRPC_CallBack_addSceneRsp(groupId, sceneId, nameStr, clientFd);

	free(nameStr);

	//printf("SRPC_storeScene--: id:%x\n", sceneId);

	return 0;
}

/*********************************************************************
 * @fn          uint8_t SRPC_recallScene(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to recall a scene.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_recallScene(uint8_t *pBuf, uint32_t clientFd)
{
	uint16_t dstAddr;
	uint8_t addrMode;
	uint8_t endpoint;
	char *nameStr;
	uint8_t nameLen;
	uint16_t groupId;
	uint8_t sceneId;

	//printf("SRPC_recallScene++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	groupId = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += 2;

	nameLen = *pBuf++;
	nameStr = malloc(nameLen + 1);
	int i;
	for (i = 0; i < nameLen; i++)
	{
		nameStr[i] = *pBuf++;
	}
	nameStr[nameLen] = '\0';

	printf("SRPC_recallScene++: name[%d] %s, group %d \n", nameLen, nameStr, groupId);

	sceneId = sceneListGetSceneId(nameStr, groupId);
	zbSocRecallScene(groupId, sceneId, dstAddr, endpoint, addrMode);

	free(nameStr);

	//printf("SRPC_recallScene--: id:%x\n", sceneId);

	return 0;
}

/*********************************************************************
 * @fn          uint8_t SRPC_identifyDevice(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to make a device identify.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_identifyDevice(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;
	uint16_t identifyTime;

	//printf("SRPC_identifyDevice++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	identifyTime = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += 2;

	zbSocSendIdentify(identifyTime, dstAddr, endpoint, addrMode);

	return 0;
}

/*********************************************************************
 * @fn          SRPC_bindDevices
 *
 * @brief       This function exposes an interface to set a bind devices.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
uint8_t SRPC_bindDevices(uint8_t *pBuf, uint32_t clientFd)
{
	uint16_t srcNwkAddr;
	uint8_t srcEndpoint;
	uint8 srcIEEE[8];
	uint8_t dstEndpoint;
	uint8 dstIEEE[8];
	uint16 clusterId;

	//printf("SRPC_bindDevices++\n");

	//increment past SRPC header
	pBuf += 2;

	/* Src Address */
	srcNwkAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
	pBuf += 2;

	srcEndpoint = *pBuf++;

	memcpy(srcIEEE, pBuf, Z_EXTADDR_LEN);
	pBuf += Z_EXTADDR_LEN;

	dstEndpoint = *pBuf++;

	memcpy(dstIEEE, pBuf, Z_EXTADDR_LEN);
	pBuf += Z_EXTADDR_LEN;

	clusterId = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += 2;

	zbSocBind(srcNwkAddr, srcEndpoint, srcIEEE, dstEndpoint, dstIEEE, clusterId);

	return 0;
}

/*********************************************************************
 * @fn          SRPC_setDeviceState
 *
 * @brief       This function exposes an interface to set a devices on/off attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_setDeviceState(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;
	bool state;

	//printf("SRPC_setDeviceState++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	state = (bool) *pBuf;

	//printf("SRPC_setDeviceState: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x, state=%x\n", dstAddr, endpoint, addrMode, state);

	// Set light state on/off
	zbSocSetState(state, dstAddr, endpoint, addrMode);

	//printf("SRPC_setDeviceState--\n");

	return 0;
}

/*********************************************************************
 * @fn          SRPC_setDeviceLevel
 *
 * @brief       This function exposes an interface to set a devices level attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_setDeviceLevel(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;
	uint8_t level;
	uint16_t transitionTime;

	//printf("SRPC_setDeviceLevel++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	level = *pBuf++;

	transitionTime = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += 2;

	//printf("SRPC_setDeviceLevel: dstAddr.addr.shortAddr=%x ,level=%x, tr=%x \n", dstAddr, level, transitionTime);

	zbSocSetLevel(level, transitionTime, dstAddr, endpoint, addrMode);

	//printf("SRPC_setDeviceLevel--\n");

	return 0;
}

/*********************************************************************
 * @fn          SRPC_setDeviceColor
 *
 * @brief       This function exposes an interface to set a devices hue and saturation attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_setDeviceColor(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;
	uint8_t hue, saturation;
	uint16_t transitionTime;

	//printf("SRPC_setDeviceColor++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	hue = *pBuf++;

	saturation = *pBuf++;

	transitionTime = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += 2;

//  printf("SRPC_setDeviceColor: dstAddr=%x ,hue=%x, saturation=%x, tr=%x \n", dstAddr, hue, saturation, transitionTime); 

	zbSocSetHueSat(hue, saturation, transitionTime, dstAddr, endpoint, addrMode);

	//printf("SRPC_setDeviceColor--\n");

	return 0;
}

/*********************************************************************
 * @fn          SRPC_getDeviceState
 *
 * @brief       This function exposes an interface to get a devices on/off attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDeviceState(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;

	//printf("SRPC_getDeviceState++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	//printf("SRPC_getDeviceState: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x", dstAddr, endpoint, addrMode);

	// Get light state on/off
	zbSocGetState(dstAddr, endpoint, addrMode);

	//printf("SRPC_getDeviceState--\n");

	return 0;
}

/*********************************************************************
 * @fn          SRPC_getDeviceLevel
 *
 * @brief       This function exposes an interface to get a devices level attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDeviceLevel(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;

	//printf("SRPC_getDeviceLevel++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	//printf("SRPC_getDeviceLevel: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x\n", dstAddr, endpoint, addrMode);

	// Get light level
	zbSocGetLevel(dstAddr, endpoint, addrMode);

	//printf("SRPC_getDeviceLevel--\n");

	return 0;
}

/*********************************************************************
 * @fn          SRPC_getDeviceHue
 *
 * @brief       This function exposes an interface to get a devices hue attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDeviceHue(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;

	//printf("SRPC_getDeviceHue++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	//printf("SRPC_getDeviceHue: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x\n", dstAddr, endpoint, addrMode);

	// Get light hue
	zbSocGetHue(dstAddr, endpoint, addrMode);

	//printf("SRPC_getDeviceHue--\n");

	return 0;
}

/*********************************************************************
 * @fn          SRPC_getDeviceSat
 *
 * @brief       This function exposes an interface to get a devices sat attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDeviceSat(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;

	//printf("SRPC_getDeviceSat++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	//printf("SRPC_getDeviceSat: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x\n", dstAddr, endpoint, addrMode);

	// Get light sat
	zbSocGetSat(dstAddr, endpoint, addrMode);

	//printf("SRPC_getDeviceSat--\n");

	return 0;
}

/*********************************************************************
 * @fn          SRPC_changeDeviceName
 *
 * @brief       This function exposes an interface to set a bind devices.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_changeDeviceName(uint8_t *pBuf, uint32_t clientFd)
{
	uint16_t devNwkAddr;
	uint8_t devEndpoint;
	uint8_t nameLen;
	epInfo_t * epInfo;

	printf("RSPC_ZLL_changeDeviceName++\n");

	//increment past SRPC header
	pBuf += 2;

	// Src Address
	devNwkAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
	pBuf += 2;

	devEndpoint = *pBuf++;

	nameLen = *pBuf++;
	if (nameLen > MAX_SUPPORTED_DEVICE_NAME_LENGTH)
	{
		nameLen = MAX_SUPPORTED_DEVICE_NAME_LENGTH;
	}

	printf("RSPC_ZLL_changeDeviceName: renaming device %s, len=%d\n", pBuf,
			nameLen);
	printf("RSPC_ZLL_changeDeviceName: devNwkAddr=%x, devEndpoint=%x\n",
			devNwkAddr, devEndpoint);

	epInfo = devListRemoveDeviceByNaEp(devNwkAddr, devEndpoint);
	if (epInfo != NULL)
	{
		/*
		 if(epInfo->deviceName != NULL)
		 {
		 free(epInfo->deviceName);
		 }*/
		epInfo->deviceName = malloc(nameLen + 1);
		printf("RSPC_ZLL_changeDeviceName: renamed device %s, len=%d\n", pBuf,
				nameLen);
		strncpy(epInfo->deviceName, (char *) pBuf, nameLen);
		epInfo->deviceName[nameLen] = '\0';
		devListAddDevice(epInfo);
	}

	return 0;
}

/*********************************************************************
 * @fn          SRPC_close
 *
 * @brief       This function exposes an interface to allow an upper layer to close the interface.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
uint8_t SRPC_close(uint8_t *pBuf, uint32_t clientFd)
{
	uint16_t magicNumber;

	//printf("SRPC_close++\n");

	//increment past SRPC header
	pBuf += 2;

	magicNumber = BUILD_UINT16(pBuf[0], pBuf[1]);

	if (magicNumber == CLOSE_AUTH_NUM)
	{
		//Close the application
		socketSeverClose();
		//TODO: Need to create API's and close other fd's

		exit(EXIT_SUCCESS);
	}

	return 0;
}

/*********************************************************************
 * @fn          uint8_t SRPC_permitJoin(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to permit other devices to join the network.
 *
 * @param       pBuf - incomin messages
 *
 * @return      none
 */
static uint8_t SRPC_permitJoin(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t duration;
	uint16_t magicNumber;

	//increment past SRPC header
	pBuf += 2;

	duration = *pBuf++;
	magicNumber = BUILD_UINT16(pBuf[0], pBuf[1]);

	if (magicNumber == JOIN_AUTH_NUM)
	{
		//Open the network for joining
		zbSocOpenNwk(duration);
	}

	return 0;
}

/*********************************************************************
 * @fn          uint8_t SRPC_getGroups(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to get the group list.
 *
 * @param       pBuf - incomin messages
 *
 * @return      none
 */
static uint8_t SRPC_getGroups(uint8_t *pBuf, uint32_t clientFd)
{
	uint32_t context = 0;
	groupRecord_t *group = groupListGetNextGroup(&context);
	uint8_t nameLen, nameIdx;

	//printf("SRPC_getGroups++\n");

	while (group != NULL)
	{
		uint8_t *pSrpcMessage, *pBuf;
		//printf("SRPC_getGroups: group != null\n");

		//printf("SRPC_getGroups: malloc'ing %d bytes\n", (2 + (sizeof(uint16_t)) + ((uint8_t) (group->groupNameStr[0]))));

		//RpcMessage contains function ID param Data Len and param data
		//2 (SRPC header) + sizeof(uint16_t) (GroupId) + sizeof(uint8_t) (GroupName Len) + group->groupNameStr[0] (string)
		pSrpcMessage = malloc(
				2 + (sizeof(uint16_t)) + sizeof(uint8_t) + ((uint8_t)(group->name[0])));

		pBuf = pSrpcMessage;

		//Set func ID in RPCS buffer
		*pBuf++ = SRPC_GET_GROUP_RSP;
		//param size
		//sizeof(uint16_t) (GroupId) + sizeof(uint8_t) (SceneId) + sizeof(uint8_t) (GroupName Len) + group->groupNameStr[0] (string)
		*pBuf++ = (sizeof(uint16_t) + sizeof(uint8_t) + group->name[0]);

		*pBuf++ = group->id & 0xFF;
		*pBuf++ = (group->id & 0xFF00) >> 8;

		nameLen = strlen(group->name);
		if (nameLen > 0)
		{
			*pBuf++ = nameLen;
			for (nameIdx = 0; nameIdx < nameLen; nameIdx++)
			{
				*pBuf++ = group->name[nameIdx];
			}
			printf("SRPC_getGroups: name:%s\n", group->name);
		}
		else
		{
			*pBuf++ = 0;
		}

		//printf("SRPC_CallBack_addGroupRsp: groupName[%d] %s\n", group->groupNameStr[0], &(group->groupNameStr[1]));
		//Send SRPC
		srpcSend(pSrpcMessage, clientFd);
		free(pSrpcMessage);
		//get next group (NULL if all done)
		group = groupListGetNextGroup(&context);
	}
	//printf("SRPC_getGroups--\n");

	return 0;
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_addGroupRsp
 *
 * @brief   Sends the groupId to the client after a groupAdd
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_addGroupRsp(uint16_t groupId, char *nameStr,
		uint32_t clientFd)
{
	uint8_t *pSrpcMessage, *pBuf;

	//printf("SRPC_CallBack_addGroupRsp++\n");

	//printf("SRPC_CallBack_addGroupRsp: malloc'ing %d bytes\n", 2 + 3 + nameStr[0]);
	//RpcMessage contains function ID param Data Len and param data
	pSrpcMessage = malloc(2 + 3 + strlen(nameStr));

	pBuf = pSrpcMessage;

	//Set func ID in SRPC buffer
	*pBuf++ = SRPC_ADD_GROUP_RSP;
	//param size
	*pBuf++ = 3 + strlen(nameStr);

	*pBuf++ = groupId & 0xFF;
	*pBuf++ = (groupId & 0xFF00) >> 8;

	*pBuf++ = strlen(nameStr);
	int i;
	for (i = 0; i < strlen(nameStr); i++)
	{
		*pBuf++ = nameStr[i];
	}

	printf("SRPC_CallBack_addGroupRsp: groupName[%d] %s\n", strlen(nameStr),
			nameStr);
	//Send SRPC
	srpcSend(pSrpcMessage, clientFd);

	//printf("SRPC_CallBack_addGroupRsp--\n");

	return;
}

/*********************************************************************
 * @fn          uint8_t SRPC_getScenes(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to get the scenes defined for a group.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getScenes(uint8_t *pBuf, uint32_t clientFd)
{
	uint32_t context = 0;
	sceneRecord_t *scene = sceneListGetNextScene(&context);
	uint8_t nameLen, nameIdx;

	//printf("SRPC_getScenes++\n");

	while (scene != NULL)
	{
		uint8_t *pSrpcMessage, *pBuf;
		//printf("SRPC_getScenes: scene != null\n");

		//printf("SRPC_getScenes: malloc'ing %d bytes\n", (2 + (sizeof(uint16_t)) + (sizeof(uint8_t)) + (sizeof(uint8_t)) + ((uint8_t) (scene->sceneNameStr[0]))));

		//RpcMessage contains function ID param Data Len and param data
		//2 (SRPC header) + sizeof(uint16_t) (GroupId) + sizeof(uint8_t) (SceneId) + sizeof(uint8_t) (SceneName Len) + scene->sceneNameStr[0] (string)
		pSrpcMessage = malloc(
				2 + (sizeof(uint16_t)) + (sizeof(uint8_t)) + (sizeof(uint8_t))
						+ ((uint8_t)(scene->name[0])));

		pBuf = pSrpcMessage;

		//Set func ID in RPCS buffer
		*pBuf++ = SRPC_GET_SCENE_RSP;
		//param size
		//sizeof(uint16_t) (GroupId) + sizeof(uint8_t) (SceneId) + sizeof(uint8_t) (SceneName Len) + scene->sceneNameStr[0] (string)
		*pBuf++ = (sizeof(uint16_t) + (sizeof(uint8_t)) + (sizeof(uint8_t))
				+ scene->name[0]);

		*pBuf++ = scene->groupId & 0xFF;
		*pBuf++ = (scene->groupId & 0xFF00) >> 8;

		*pBuf++ = scene->sceneId;

		nameLen = strlen(scene->name);
		if (nameLen > 0)
		{
			*pBuf++ = nameLen;
			for (nameIdx = 0; nameIdx < nameLen; nameIdx++)
			{
				*pBuf++ = scene->name[nameIdx];
			}
			printf("SRPC_getScenes: name:%s\n", scene->name);
		}
		else
		{
			*pBuf++ = 0;
		}

		//printf("SRPC_getScenes: sceneName[%d] %s, groupId %x\n", scene->sceneNameStr[0], &(scene->sceneNameStr[1]), scene->groupId);
		//Send SRPC
		srpcSend(pSrpcMessage, clientFd);
		free(pSrpcMessage);
		//get next scene (NULL if all done)
		scene = sceneListGetNextScene(&context);
	}
	//printf("SRPC_getScenes--\n");

	return 0;
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_addSceneRsp
 *
 * @brief   Sends the sceneId to the client after a storeScene
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_addSceneRsp(uint16_t groupId, uint8_t sceneId, char *nameStr,
		uint32_t clientFd)
{
	uint8_t *pSrpcMessage, *pBuf;

	//printf("SRPC_CallBack_addSceneRsp++\n");

	//printf("SRPC_CallBack_addSceneRsp: malloc'ing %d bytes\n", 2 + 4 + nameStr[0]);
	//RpcMessage contains function ID param Data Len and param data
	pSrpcMessage = malloc(2 + 4 + nameStr[0]);

	pBuf = pSrpcMessage;

	//Set func ID in SRPC buffer
	*pBuf++ = SRPC_ADD_SCENE_RSP;
	//param size
	*pBuf++ = 4 + nameStr[0];

	*pBuf++ = groupId & 0xFF;
	*pBuf++ = (groupId & 0xFF00) >> 8;

	*pBuf++ = sceneId & 0xFF;

	*pBuf++ = nameStr[0];
	int i;
	for (i = 0; i < nameStr[0]; i++)
	{
		*pBuf++ = nameStr[i + 1];
	}

	//printf("SRPC_CallBack_addSceneRsp: groupName[%d] %s\n", nameStr[0], nameStr);
	//Send SRPC
	srpcSend(pSrpcMessage, clientFd);

	//printf("SRPC_CallBack_addSceneRsp--\n");

	return;
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_getStateRsp
 *
 * @brief   Sends the get State Rsp to the client that sent a get state
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_getStateRsp(uint8_t state, uint16_t srcAddr,
		uint8_t endpoint, uint32_t clientFd)
{
	uint8_t * pBuf;
	uint8_t pSrpcMessage[2 + 4];

	//printf("SRPC_CallBack_getStateRsp++\n");

	//RpcMessage contains function ID param Data Len and param data

	pBuf = pSrpcMessage;

	//Set func ID in RPCS buffer
	*pBuf++ = SRPC_GET_DEV_STATE_RSP;
	//param size
	*pBuf++ = 4;

	*pBuf++ = srcAddr & 0xFF;
	*pBuf++ = (srcAddr & 0xFF00) >> 8;
	*pBuf++ = endpoint;
	*pBuf++ = state & 0xFF;

	//printf("SRPC_CallBack_getStateRsp: state=%x\n", state);

	//Store the device that sent the request, for now send to all clients
	srpcSendAll(pSrpcMessage);

	//printf("SRPC_CallBack_addSceneRsp--\n");

	return;
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_getLevelRsp
 *
 * @brief   Sends the get Level Rsp to the client that sent a get level
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_getLevelRsp(uint8_t level, uint16_t srcAddr,
		uint8_t endpoint, uint32_t clientFd)
{
	uint8_t * pBuf;
	uint8_t pSrpcMessage[2 + 4];

	//printf("SRPC_CallBack_getLevelRsp++\n");

	//RpcMessage contains function ID param Data Len and param data

	pBuf = pSrpcMessage;

	//Set func ID in RPCS buffer
	*pBuf++ = SRPC_GET_DEV_LEVEL_RSP;
	//param size
	*pBuf++ = 4;

	*pBuf++ = srcAddr & 0xFF;
	*pBuf++ = (srcAddr & 0xFF00) >> 8;
	*pBuf++ = endpoint;
	*pBuf++ = level & 0xFF;

	//printf("SRPC_CallBack_getLevelRsp: level=%x\n", level);

	//Store the device that sent the request, for now send to all clients
	srpcSendAll(pSrpcMessage);

	//printf("SRPC_CallBack_getLevelRsp--\n");

	return;
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_getHueRsp
 *
 * @brief   Sends the get Hue Rsp to the client that sent a get hue
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_getHueRsp(uint8_t hue, uint16_t srcAddr, uint8_t endpoint,
		uint32_t clientFd)
{
	uint8_t * pBuf;
	uint8_t pSrpcMessage[2 + 4];

	//printf("SRPC_CallBack_getLevelRsp++\n");

	//RpcMessage contains function ID param Data Len and param data

	pBuf = pSrpcMessage;

	//Set func ID in RPCS buffer
	*pBuf++ = SRPC_GET_DEV_HUE_RSP;
	//param size
	*pBuf++ = 4;

	*pBuf++ = srcAddr & 0xFF;
	*pBuf++ = (srcAddr & 0xFF00) >> 8;
	*pBuf++ = endpoint;
	*pBuf++ = hue & 0xFF;

	//printf("SRPC_CallBack_getHueRsp: hue=%x\n", hue);

	//Store the device that sent the request, for now send to all clients
	srpcSendAll(pSrpcMessage);

	//printf("SRPC_CallBack_getHueRsp--\n");

	return;
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_getSatRsp
 *
 * @brief   Sends the get Sat Rsp to the client that sent a get sat
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_getSatRsp(uint8_t sat, uint16_t srcAddr, uint8_t endpoint,
		uint32_t clientFd)
{
	uint8_t * pBuf;
	uint8_t pSrpcMessage[2 + 4];

	//printf("SRPC_CallBack_getSatRsp++\n");

	//RpcMessage contains function ID param Data Len and param data

	pBuf = pSrpcMessage;

	//Set func ID in RPCS buffer
	*pBuf++ = SRPC_GET_DEV_SAT_RSP;
	//param size
	*pBuf++ = 4;

	*pBuf++ = srcAddr & 0xFF;
	*pBuf++ = (srcAddr & 0xFF00) >> 8;
	*pBuf++ = endpoint;
	*pBuf++ = sat & 0xFF;

	//printf("SRPC_CallBack_getSatRsp: sat=%x\n", sat);

	//Store the device that sent the request, for now send to all clients
	srpcSendAll(pSrpcMessage);

	//printf("SRPC_CallBack_getSatRsp--\n");

	return;
}

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

/*********************************************************************
 * @fn          SRPC_getDevices
 *
 * @brief       This function exposes an interface to allow an upper layer to start device discovery.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDevices(uint8_t *pBuf, uint32_t clientFd)
{
	epInfoExtended_t epInfoEx;
	uint32_t context = 0;
	uint8_t *pSrpcMessage;

	//printf("SRPC_getDevices++ \n");

	while ((epInfoEx.epInfo = devListGetNextDev(&context)) != NULL)
	{
		epInfoEx.type = EP_INFO_TYPE_EXISTING;
		epInfoEx.prevNwkAddr = 0xFFFF;
		epInfoEx.epInfo->flags = MT_NEW_DEVICE_FLAGS_NONE;

		//Send epInfo
		pSrpcMessage = srpcParseEpInfo(&epInfoEx);
		printf("SRPC_getDevices: %x:%x:%x:%x:%x\n", epInfoEx.epInfo->nwkAddr, epInfoEx.epInfo->endpoint,
				epInfoEx.epInfo->profileID, epInfoEx.epInfo->deviceID, epInfoEx.type);
		//Send SRPC
		srpcSend(pSrpcMessage, clientFd);
		free(pSrpcMessage);

		usleep(1000);

		//get next device (NULL if all done)
//printf("--?--> 0x%04X, 0x%02X", (uint32_t)epInfoEx.epInfo->nwkAddr, (uint32_t)epInfoEx.epInfo->endpoint);
//printf(" --!--> 0x%08X\n", (uint32_t)epInfoEx.epInfo);
	}

	return 0;
}

/*********************************************************************
 * @fn          SRPC_notSupported
 *
 * @brief       This function is called for unsupported commands.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_notSupported(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}

/***************************************************************************************************
 * @fn      SRPC_Init
 *
 * @brief   initialises the RPC interface and waitsfor a client to connect.
 * @param   
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_Init(void)
{
	if (socketSeverInit(SRPC_TCP_PORT) == -1)
	{
		//exit if the server does not start
		exit(-1);
	}

	serverSocketConfig(SRPC_RxCB, SRPC_ConnectCB);
}

/*********************************************************************
 * @fn          SRPC_SendEpInfo
 *
 * @brief       This function exposes an interface to allow an upper layer to start send an ep indo to all devices.
 *
 * @param       epInfo - pointer to the epInfo to be sent
 *
 * @return      afStatus_t
 */
uint8_t SRPC_SendEpInfo(epInfoExtended_t *epInfoEx)
{ 
  uint8_t *pSrpcMessage = srpcParseEpInfo(epInfoEx);  
  
  printf("SRPC_SendEpInfo++ %x:%x:%x:%x\n", epInfoEx->epInfo->nwkAddr, epInfoEx->epInfo->endpoint, epInfoEx->epInfo->profileID, epInfoEx->epInfo->deviceID);
    
  //Send SRPC
  srpcSendAll(pSrpcMessage);  
  free(pSrpcMessage); 
  printf("RSPC_SendEpInfo--\n");
  
  return 0;  
}

/***************************************************************************************************
 * @fn      SRPC_ConnectCB
 *
 * @brief   Callback for connecting SRPC clients.
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_ConnectCB(int clientFd)
{
	//printf("SRPC_ConnectCB++ \n");
	/*
	 epInfo = devListGetNextDev(0xFFFF, 0);

	 while(epInfo)
	 {
	 printf("SRPC_ConnectCB: send epInfo\n");
	 //Send device Annce
	 uint8_t *pSrpcMessage = srpcParseEpInfo(epInfo);
	 //Send SRPC
	 srpcSend(pSrpcMessage, clientFd);
	 free(pSrpcMessage);

	 usleep(1000);

	 //get next device (NULL if all done)
	 epInfo = devListGetNextDev(epInfo->nwkAddr, epInfo->endpoint);
	 }
	 */
	//printf("SRPC_ConnectCB--\n");
}

/***************************************************************************************************
 * @fn      SRPC_RxCB
 *
 * @brief   Callback for Rx'ing SRPC messages.
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_RxCB(int clientFd)
{
	char buffer[256];
	int byteToRead;
	int byteRead;
	int rtn;

	//printf("SRPC_RxCB++[%x]\n", clientFd);

	rtn = ioctl(clientFd, FIONREAD, &byteToRead);

	if (rtn != 0)
	{
		printf("SRPC_RxCB: Socket error\n");
	}

	while (byteToRead)
	{
		bzero(buffer, 256);
		byteRead = 0;
		//Get the CMD-ID and Len
		byteRead += read(clientFd, buffer, 2);
		//Get the rest of the message
		//printf("SRPC: reading %x byte message\n",buffer[SRPC_MSG_LEN]);
		byteRead += read(clientFd, &buffer[2], buffer[SRPC_MSG_LEN]);
		byteToRead -= byteRead;
		if (byteRead < 0) error("SRPC ERROR: error reading from socket\n");
		if (byteRead < buffer[SRPC_MSG_LEN])
			error("SRPC ERROR: full message not read\n");
		//printf("Read the message[%x]\n",byteRead);
		SRPC_ProcessIncoming((uint8_t*) buffer, clientFd);
	}

	//printf("SRPC_RxCB--\n");

	return;
}

/***************************************************************************************************
 * @fn      Closes the TCP port
 *
 * @brief   Send a message over SRPC.
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_Close(void)
{
	socketSeverClose();
}


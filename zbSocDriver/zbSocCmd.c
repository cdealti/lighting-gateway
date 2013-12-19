/*
 * zbSocCmd.c
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

/*********************************************************************
 * INCLUDES
 */
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <errno.h>
#include <string.h>

#include "zbSocCmd.h"

/*********************************************************************
 * MACROS
 */

#define APPCMDHEADER(len) \
0xFE,                                                                             \
len,   /*RPC payload Len                                      */     \
0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP        */     \
0x00, /*MT_APP_MSG                                                   */     \
0x0B, /*Application Endpoint                                  */     \
0x02, /*short Addr 0x0002                                     */     \
0x00, /*short Addr 0x0002                                     */     \
0x0B, /*Dst EP                                                             */     \
0xFF, /*Cluster ID 0xFFFF invalid, used for key */     \
0xFF, /*Cluster ID 0xFFFF invalid, used for key */     \

#define BUILD_UINT16(loByte, hiByte) \
          ((uint16_t)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((uint32_t)((uint32_t)((Byte0) & 0x00FF) \
          + ((uint32_t)((Byte1) & 0x00FF) << 8) \
          + ((uint32_t)((Byte2) & 0x00FF) << 16) \
          + ((uint32_t)((Byte3) & 0x00FF) << 24)))

/*********************************************************************
 * CONSTANTS
 */
#define ZLL_MT_APP_RPC_CMD_TOUCHLINK          0x01
#define ZLL_MT_APP_RPC_CMD_RESET_TO_FN        0x02
#define ZLL_MT_APP_RPC_CMD_CH_CHANNEL         0x03
#define ZLL_MT_APP_RPC_CMD_JOIN_HA            0x04
#define ZLL_MT_APP_RPC_CMD_PERMIT_JOIN        0x05
#define ZLL_MT_APP_RPC_CMD_SEND_RESET_TO_FN   0x06
#define ZLL_MT_APP_RPC_CMD_START_DISTRIB_NWK  0x07

#define MT_APP_RSP                           0x80
#define MT_APP_ZLL_TL_IND                    0x81
#define MT_APP_ZLL_NEW_DEV_IND               0x82

#define MT_DEBUG_MSG                         0x80

#define COMMAND_LIGHTING_MOVE_TO_HUE  			0x00
#define COMMAND_LIGHTING_MOVE_TO_SATURATION 0x03
#define COMMAND_LEVEL_MOVE_TO_LEVEL 				0x00
#define COMMAND_IDENTIFY                    0x00
#define COMMAND_IDENTIFY_TRIGGER_EFFECT     0x40

/*** Foundation Command IDs ***/
#define ZCL_CMD_READ                                    0x00
#define ZCL_CMD_READ_RSP                                0x01
#define ZCL_CMD_WRITE                                   0x02
#define ZCL_CMD_WRITE_UNDIVIDED                         0x03
#define ZCL_CMD_WRITE_RSP                               0x04
//ZDO
#define MT_ZDO_SIMPLE_DESC_RSP             0x84 
#define MT_ZDO_ACTIVE_EP_RSP               0x85 
#define MT_ZDO_END_DEVICE_ANNCE_IND          0xC1
#define MT_ZDO_LEAVE_IND                     0xC9

//UTIL
#define MT_UTIL_GET_DEVICE_INFO              0x00

// General Clusters
#define ZCL_CLUSTER_ID_GEN_BASIC                             0x0000
#define ZCL_CLUSTER_ID_GEN_IDENTIFY                          0x0003
#define ZCL_CLUSTER_ID_GEN_GROUPS                            0x0004
#define ZCL_CLUSTER_ID_GEN_SCENES                            0x0005
#define ZCL_CLUSTER_ID_GEN_ON_OFF                            0x0006
#define ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL                     0x0008
// Lighting Clusters
#define ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL                0x0300

// Data Types
#define ZCL_DATATYPE_BOOLEAN                            0x10
#define ZCL_DATATYPE_UINT8                              0x20
#define ZCL_DATATYPE_INT16                              0x29
#define ZCL_DATATYPE_INT24                              0x2a
#define ZCL_DATATYPE_CHAR_STR                           0x42

/*******************************/
/*** Generic Cluster ATTR's  ***/
/*******************************/
#define ATTRID_BASIC_MODEL_ID                             0x0005
#define ATTRID_ON_OFF                                     0x0000
#define ATTRID_LEVEL_CURRENT_LEVEL                        0x0000

/*******************************/
/*** Lighting Cluster ATTR's  ***/
/*******************************/
#define ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE         0x0000
#define ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION  0x0001

/*******************************/
/*** Scenes Cluster Commands ***/
/*******************************/
#define COMMAND_SCENE_STORE                               0x04
#define COMMAND_SCENE_RECALL                              0x05

/*******************************/
/*** Groups Cluster Commands ***/
/*******************************/
#define COMMAND_GROUP_ADD                                 0x00

/* The 3 MSB's of the 1st command field byte are for command type. */
#define MT_RPC_CMD_TYPE_MASK  0xE0

/* The 5 LSB's of the 1st command field byte are for the subsystem. */
#define MT_RPC_SUBSYSTEM_MASK 0x1F

#define MT_RPC_SOF         0xFE

/*******************************/
/*** Bootloader Commands ***/
/*******************************/
#define SB_FORCE_BOOT               0xF8
#define SB_FORCE_RUN               (SB_FORCE_BOOT ^ 0xFF)
#define SB_FORCE_BOOT_1             0x10
#define SB_FORCE_RUN_1             (SB_FORCE_BOOT_1 ^ 0xFF)


typedef enum {
	MT_RPC_CMD_POLL = 0x00,
	MT_RPC_CMD_SREQ = 0x20,
	MT_RPC_CMD_AREQ = 0x40,
	MT_RPC_CMD_SRSP = 0x60,
	MT_RPC_CMD_RES4 = 0x80,
	MT_RPC_CMD_RES5 = 0xA0,
	MT_RPC_CMD_RES6 = 0xC0,
	MT_RPC_CMD_RES7 = 0xE0
} mtRpcCmdType_t;

typedef enum {
	MT_RPC_SYS_RES0, /* Reserved. */
	MT_RPC_SYS_SYS,
	MT_RPC_SYS_MAC,
	MT_RPC_SYS_NWK,
	MT_RPC_SYS_AF,
	MT_RPC_SYS_ZDO,
	MT_RPC_SYS_SAPI, /* Simple API. */
	MT_RPC_SYS_UTIL,
	MT_RPC_SYS_DBG,
	MT_RPC_SYS_APP,
	MT_RPC_SYS_OTA,
	MT_RPC_SYS_ZNP,
	MT_RPC_SYS_SPARE_12,
	MT_RPC_SYS_UBL = 13, // 13 to be compatible with existing RemoTI.
	MT_RPC_SYS_MAX // Maximum value, must be last (so 14-32 available, not yet assigned).
} mtRpcSysType_t;

/************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
int serialPortFd = 0;
uint8_t transSeqNumber = 0;

zbSocCallbacks_t zbSocCb;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void calcFcs(uint8_t *msg, int size);
static void processRpcSysAppTlInd(uint8_t *TlIndBuff);
static void processRpcSysAppZcl(uint8_t *zclRspBuff);
static void processRpcSysAppZclFoundation(uint8_t *zclRspBuff,
		uint8_t zclFrameLen, uint16_t clusterID, uint16_t nwkAddr, uint8_t endpoint);
static void processRpcSysApp(uint8_t *rpcBuff);
static void processRpcSysDbg(uint8_t *rpcBuff);
static void zbSocTransportWrite(uint8_t* buf, uint8_t len);

/*********************************************************************
 * @fn      calcFcs
 *
 * @brief   populates the Frame Check Sequence of the RPC payload.
 *
 * @param   msg - pointer to the RPC message
 *
 * @return  none
 */
void calcFcs(uint8_t *msg, int size)
{
	uint8_t result = 0;
	int idx = 1; //skip SOF
	int len = (size - 1); // skip FCS

	while ((len--) != 0)
	{
		result ^= msg[idx++];
	}

	msg[(size - 1)] = result;
}

/*********************************************************************
 * API FUNCTIONS
 */

/*********************************************************************
 * @fn      zbSocOpen
 *
 * @brief   opens the serial port to the CC253x.
 *
 * @param   devicePath - path to the UART device
 *
 * @return  status
 */
int32_t zbSocOpen(char *devicePath)
{
	struct termios tio;

	/* open the device to be non-blocking (read will return immediatly) */
	serialPortFd = open(devicePath, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (serialPortFd < 0)
	{
		perror(devicePath);
		printf("%s open failed\n", devicePath);
		return (-1);
	}

	//make the access exclusive so other instances will return -1 and exit
	ioctl(serialPortFd, TIOCEXCL);

	/* c-iflags
	 B115200 : set board rate to 115200
	 CRTSCTS : HW flow control (disabled below)
	 CS8     : 8n1 (8bit,no parity,1 stopbit)
	 CLOCAL  : local connection, no modem contol
	 CREAD   : enable receiving characters*/
	tio.c_cflag = B38400 | CRTSCTS | CS8 | CLOCAL | CREAD;
	/* c-iflags
	 ICRNL   : maps 0xD (CR) to 0x10 (LR), we do not want this.
	 IGNPAR  : ignore bits with parity erros, I guess it is
	 better to ignStateore an erronious bit then interprit it incorrectly. */
	tio.c_iflag = IGNPAR & ~ICRNL;
	tio.c_oflag = 0;
	tio.c_lflag = 0;

	tcflush(serialPortFd, TCIFLUSH);
	tcsetattr(serialPortFd, TCSANOW, &tio);

	//Send the bootloader force boot incase we have a bootloader that waits
	uint8_t forceBoot[] = {SB_FORCE_RUN, SB_FORCE_RUN_1};
	zbSocTransportWrite(forceBoot, 2);

	return serialPortFd;
}

void zbSocClose(void)
{
	tcflush(serialPortFd, TCOFLUSH);
	close(serialPortFd);

	return;
}

/*********************************************************************
 * @fn      zbSocTransportWrite
 *
 * @brief   Write to the the serial port to the CC253x.
 *
 * @param   fd - file descriptor of the UART device
 *
 * @return  status
 */
static void zbSocTransportWrite(uint8_t* buf, uint8_t len)
{
	int remain = len;
	int offset = 0;
#if 1
	//printf("zbSocTransportWrite : len = %d\n", len);

	while (remain > 0)
	{
		int sub = (remain >= 8 ? 8 : remain);
		//printf("writing %d bytes (offset = %d, remain = %d)\n", sub, offset,
		//		remain);
		write(serialPortFd, buf + offset, sub);

		tcflush(serialPortFd, TCOFLUSH);
		usleep(5000);
		remain -= 8;
		offset += 8;
	}
#else
	write (serialPortFd, buf, len);
	tcflush(serialPortFd, TCOFLUSH);

#endif
	return;
}

/*********************************************************************
 * @fn      zbSocRegisterCallbacks
 *
 * @brief   opens the serial port to the CC253x.
 *
 * @param   devicePath - path to the UART device
 *
 * @return  status
 */
void zbSocRegisterCallbacks(zbSocCallbacks_t zbSocCallbacks)
{
	//copy the callback function pointers
	memcpy(&zbSocCb, &zbSocCallbacks, sizeof(zbSocCallbacks_t));
	return;
}

/*********************************************************************
 * @fn      zbSocTouchLink
 *
 * @brief   Send the touchLink command to the CC253x.
 *
 * @param   none
 *
 * @return  none
 */
void zbSocTouchLink(void)
{
	uint8_t cmd[] =
	{ APPCMDHEADER(13) 0x06, //Data Len
			0x02, //Address Mode
			0x00,//2dummy bytes
			0x00, ZLL_MT_APP_RPC_CMD_TOUCHLINK, 0x00, //
			0x00, //
			0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocBridgeStartNwk
 *
 * @brief   Send the start network command to the CC253x.
 *
 * @param   none
 *
 * @return  none
 */
void zbSocBridgeStartNwk(void)
{
	uint8_t cmd[] =
	{ APPCMDHEADER(13) 0x06, //Data Len
			0x02, //Address Mode
			0x00,//2dummy bytes
			0x00, ZLL_MT_APP_RPC_CMD_START_DISTRIB_NWK, 0x00, //
			0x00, //
			0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocResetToFn
 *
 * @brief   Send the reset to factory new command to the CC253x.
 *
 * @param   none
 *
 * @return  none
 */
void zbSocResetToFn(void)
{
	uint8_t cmd[] =
	{ APPCMDHEADER(13) 0x06, //Data Len
			0x02, //Address Mode
			0x00,//2dummy bytes
			0x00, ZLL_MT_APP_RPC_CMD_RESET_TO_FN, 0x00, //
			0x00, //
			0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocSendResetToFn
 *
 * @brief   Send the reset to factory new command to a ZLL device.
 *
 * @param   none
 *
 * @return  none
 */
void zbSocSendResetToFn(void)
{
	uint8_t cmd[] =
	{ APPCMDHEADER(13) 0x06, //Data Len
			0x02, //Address Mode
			0x00,//2dummy bytes
			0x00, ZLL_MT_APP_RPC_CMD_SEND_RESET_TO_FN, 0x00, //
			0x00, //
			0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocOpenNwk
 *
 * @brief   Send the open network command to a ZLL device.
 *
 * @param   none
 *
 * @return  none
 */
void zbSocOpenNwk(uint8_t duration)
{
	uint16_t srcNwkAddr = 0xFFFD; //Everyone with RxOnWhenIdle == TRUE

	uint8_t mgmtPermit[] =
	{ 0xFE, 5, /*RPC payload Len */
	MT_RPC_CMD_SREQ | MT_RPC_SYS_ZDO,
	0x36, /*MT_ZDO_MGMT_PERMIT_JOIN_REQ*/
	afAddrBroadcast, //addr mode
	(srcNwkAddr & 0x00ff), /*Src Nwk Addr - To send the bind message to*/
	(srcNwkAddr & 0xff00) >> 8, /*Src Nwk Addr - To send the bind message to*/
	duration, /*Dst endpoint for the binding*/
	1, /*trust center significance set*/
	0x00 //FCS - fill in later
	};

	uint8_t localPermit[] =
	{ APPCMDHEADER(13) 0x06, //Data Len
			0x02, //Address Mode
			0x00,//2dummy bytes
			0x00, ZLL_MT_APP_RPC_CMD_PERMIT_JOIN,
			duration, //
			0x00, //
			0x00 //FCS - fill in later
			};

	printf("zbSocOpenNwk: duration %ds\n", duration);

	calcFcs(localPermit, sizeof(localPermit));
	zbSocTransportWrite(localPermit, sizeof(localPermit));

	//wait for message to be consumed
	usleep(30);

	calcFcs(mgmtPermit, sizeof(mgmtPermit));
	zbSocTransportWrite(mgmtPermit, sizeof(mgmtPermit));
}

/*********************************************************************
 * @fn      zbSocSendIdentify
 *
 * @brief   Send identify command to a ZLL light.
 *
 * @param   identifyTime - in s.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocSendIdentify(uint16_t identifyTime, uint16_t dstAddr,
		uint8_t endpoint, uint8_t addrMode)
{
	uint8_t cmd[] =
	{ 0xFE,
			13, //RPC payload Len
			0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP
			0x00, //MT_APP_MSG
			0x0B, //Application Endpoint
			(dstAddr & 0x00ff), (dstAddr & 0xff00) >> 8,
			endpoint, //Dst EP
			(ZCL_CLUSTER_ID_GEN_IDENTIFY & 0x00ff),
			(ZCL_CLUSTER_ID_GEN_IDENTIFY & 0xff00) >> 8, 0x06, //Data Len
			addrMode, 0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
			transSeqNumber++, COMMAND_IDENTIFY, (identifyTime
					& 0xff), (identifyTime & 0xff00) >> 8, 0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));

	zbSocTransportWrite(cmd, sizeof(cmd));

	printf("zbSocSendIdentify: dstAddr=%x, endpoint=%x, addrMode=%x, identifyTime=%x\n",
			dstAddr, endpoint, addrMode, identifyTime);
}

/*********************************************************************
 * @fn      COMMAND_IDENTIFY_TRIGGER_EFFECT
 *
 * @brief   Send identify command to a ZLL light.
 *
 * @param   effect - effect.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocSendIdentifyEffect(uint8_t effect, uint8_t effectVarient, uint16_t dstAddr,
		uint8_t endpoint, uint8_t addrMode)
{
	uint8_t cmd[] =
	{ 0xFE,
			13, //RPC payload Len
			0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP
			0x00, //MT_APP_MSG
			0x0B, //Application Endpoint
			(dstAddr & 0x00ff), (dstAddr & 0xff00) >> 8,
			endpoint, //Dst EP
			(ZCL_CLUSTER_ID_GEN_IDENTIFY & 0x00ff),
			(ZCL_CLUSTER_ID_GEN_IDENTIFY & 0xff00) >> 8, 0x06, //Data Len
			addrMode, 0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
			transSeqNumber++, COMMAND_IDENTIFY_TRIGGER_EFFECT, effect,
			effectVarient, 0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));

	zbSocTransportWrite(cmd, sizeof(cmd));

	printf("zbSocSendIdentify: dstAddr=%x, endpoint=%x, addrMode=%x, effect=%x\n",
			dstAddr, endpoint, addrMode, effect);
}

/*********************************************************************
 * @fn      zbSocSetState
 *
 * @brief   Send the on/off command to a ZLL light.
 *
 * @param   state - 0: Off, 1: On.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocSetState(uint8_t state, uint16_t dstAddr, uint8_t endpoint,
		uint8_t addrMode)
{
	uint8_t cmd[] =
	{ 0xFE, 11, /*RPC payload Len */
	0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
	0x00, /*MT_APP_MSG  */
	0x0B, /*Application Endpoint */
	(dstAddr & 0x00ff), (dstAddr & 0xff00) >> 8, endpoint, /*Dst EP */
	(ZCL_CLUSTER_ID_GEN_ON_OFF & 0x00ff), (ZCL_CLUSTER_ID_GEN_ON_OFF & 0xff00)
			>> 8, 0x04, //Data Len
			addrMode, 0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
			transSeqNumber++, (state ? 1 : 0), 0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocSetLevel
 *
 * @brief   Send the level command to a ZLL light.
 *
 * @param   level - 0-128 = 0-100%
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocSetLevel(uint8_t level, uint16_t time, uint16_t dstAddr,
		uint8_t endpoint, uint8_t addrMode)
{
	uint8_t cmd[] =
	{ 0xFE,
			14, //RPC payload Len
			0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP
			0x00, //MT_APP_MSG
			0x0B, //Application Endpoint
			(dstAddr & 0x00ff), (dstAddr & 0xff00) >> 8,
			endpoint, //Dst EP
			(ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL & 0x00ff),
			(ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL & 0xff00) >> 8, 0x07, //Data Len
			addrMode, 0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
			transSeqNumber++, COMMAND_LEVEL_MOVE_TO_LEVEL, (level & 0xff), (time
					& 0xff), (time & 0xff00) >> 8, 0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));

	zbSocTransportWrite(cmd, sizeof(cmd));

	printf("zbSocSetLevel: dstAddr=%x, endpoint=%x, addrMode=%x, level=%x\n",
			dstAddr, endpoint, addrMode, level);
}

/*********************************************************************
 * @fn      zbSocSetHue
 *
 * @brief   Send the hue command to a ZLL light.
 *
 * @param   hue - 0-128 represent the 360Deg hue color wheel : 0=red, 42=blue, 85=green  
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocSetHue(uint8_t hue, uint16_t time, uint16_t dstAddr, uint8_t endpoint,
		uint8_t addrMode)
{
	uint8_t cmd[] =
	{ 0xFE,
			15, //RPC payload Len
			0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP
			0x00, //MT_APP_MSG
			0x0B, //Application Endpoint
			(dstAddr & 0x00ff), (dstAddr & 0xff00) >> 8,
			endpoint, //Dst EP
			(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
			(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8, 0x08, //Data Len
			addrMode, 0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
			transSeqNumber++, COMMAND_LIGHTING_MOVE_TO_HUE, (hue & 0xff), 0x00, //Move with shortest distance
			(time & 0xff), (time & 0xff00) >> 8, 0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocSetSat
 *
 * @brief   Send the satuartion command to a ZLL light.
 *
 * @param   sat - 0-128 : 0=white, 128: fully saturated color  
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocSetSat(uint8_t sat, uint16_t time, uint16_t dstAddr, uint8_t endpoint,
		uint8_t addrMode)
{
	uint8_t cmd[] =
	{ 0xFE,
			14, //RPC payload Len
			0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP
			0x00, //MT_APP_MSG
			0x0B, //Application Endpoint
			(dstAddr & 0x00ff), (dstAddr & 0xff00) >> 8,
			endpoint, //Dst EP
			(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
			(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8, 0x07, //Data Len
			addrMode, 0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
			transSeqNumber++, COMMAND_LIGHTING_MOVE_TO_SATURATION, (sat & 0xff), (time
					& 0xff), (time & 0xff00) >> 8, 0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocSetHueSat
 *
 * @brief   Send the hue and satuartion command to a ZLL light.
 *
 * @param   hue - 0-128 represent the 360Deg hue color wheel : 0=red, 42=blue, 85=green  
 * @param   sat - 0-128 : 0=white, 128: fully saturated color  
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocSetHueSat(uint8_t hue, uint8_t sat, uint16_t time, uint16_t dstAddr,
		uint8_t endpoint, uint8_t addrMode)
{
	uint8_t cmd[] =
	{ 0xFE,
			15, //RPC payload Len
			0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP
			0x00, //MT_APP_MSG
			0x0B, //Application Endpoint
			(dstAddr & 0x00ff), (dstAddr & 0xff00) >> 8,
			endpoint, //Dst EP
			(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
			(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8, 0x08, //Data Len
			addrMode, 0x01, //ZCL Header Frame Control
			transSeqNumber++, 0x06, //ZCL Header Frame Command (COMMAND_LEVEL_MOVE_TO_HUE_AND_SAT)
			hue, //HUE - fill it in later
			sat, //SAT - fill it in later
			(time & 0xff), (time & 0xff00) >> 8, 0x00 //fcs
			};

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocAddGroup
 *
 * @brief   Add Group.
 *
 * @param   groupId - Group ID of the Scene.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast. 
 *
 * @return  none
 */
void zbSocAddGroup(uint16_t groupId, uint16_t dstAddr, uint8_t endpoint,
		uint8_t addrMode)
{
	uint8_t cmd[] =
	{ 0xFE, 14, /*RPC payload Len */
	0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
	0x00, /*MT_APP_MSG  */
	0x0B, /*Application Endpoint */
	(dstAddr & 0x00ff), (dstAddr & 0xff00) >> 8, endpoint, /*Dst EP */
	(ZCL_CLUSTER_ID_GEN_GROUPS & 0x00ff), (ZCL_CLUSTER_ID_GEN_GROUPS & 0xff00)
			>> 8,
			0x07, //Data Len
			addrMode,
			0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
			transSeqNumber++, COMMAND_GROUP_ADD, (groupId & 0x00ff),
			(groupId & 0xff00) >> 8, 0, //Null group name - Group Name not pushed to the devices
			0x00 //FCS - fill in later
			};

	printf("zbSocAddGroup: dstAddr 0x%x\n", dstAddr);

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocStoreScene
 *
 * @brief   Store Scene.
 * 
 * @param   groupId - Group ID of the Scene.
 * @param   sceneId - Scene ID of the Scene.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast. 
 *
 * @return  none
 */
void zbSocStoreScene(uint16_t groupId, uint8_t sceneId, uint16_t dstAddr,
		uint8_t endpoint, uint8_t addrMode)
{
	uint8_t cmd[] =
	{ 0xFE, 14, /*RPC payload Len */
	0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
	0x00, /*MT_APP_MSG  */
	0x0B, /*Application Endpoint */
	(dstAddr & 0x00ff), (dstAddr & 0xff00) >> 8, endpoint, /*Dst EP */
	(ZCL_CLUSTER_ID_GEN_SCENES & 0x00ff), (ZCL_CLUSTER_ID_GEN_SCENES & 0xff00)
			>> 8, 0x07, //Data Len
			addrMode, 0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
			transSeqNumber++, COMMAND_SCENE_STORE, (groupId & 0x00ff), (groupId
					& 0xff00) >> 8, sceneId++, 0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocRecallScene
 *
 * @brief   Recall Scene.
 *
 * @param   groupId - Group ID of the Scene.
 * @param   sceneId - Scene ID of the Scene.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled. 
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast. 
 
 * @return  none
 */
void zbSocRecallScene(uint16_t groupId, uint8_t sceneId, uint16_t dstAddr,
		uint8_t endpoint, uint8_t addrMode)
{
	uint8_t cmd[] =
	{ 0xFE, 14, /*RPC payload Len */
	0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
	0x00, /*MT_APP_MSG  */
	0x0B, /*Application Endpoint */
	(dstAddr & 0x00ff), (dstAddr & 0xff00) >> 8, endpoint, /*Dst EP */
	(ZCL_CLUSTER_ID_GEN_SCENES & 0x00ff), (ZCL_CLUSTER_ID_GEN_SCENES & 0xff00)
			>> 8, 0x07, //Data Len
			addrMode, 0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
			transSeqNumber++, COMMAND_SCENE_RECALL, (groupId & 0x00ff), (groupId
					& 0xff00) >> 8, sceneId++, 0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocBind
 *
 * @brief   Recall Scene.
 *
 * @param   
 *
 * @return  none
 */
void zbSocBind(uint16_t srcNwkAddr, uint8_t srcEndpoint, uint8_t srcIEEE[8],
		uint8_t dstEndpoint, uint8_t dstIEEE[8], uint16_t clusterID)
{
	uint8_t cmd[] =
	{ 0xFE, 23, /*RPC payload Len */
	MT_RPC_CMD_SREQ | MT_RPC_SYS_ZDO,
	0x21, /*MT_ZDO_BIND_REQ*/
	(srcNwkAddr & 0x00ff), /*Src Nwk Addr - To send the bind message to*/
	(srcNwkAddr & 0xff00) >> 8, /*Src Nwk Addr - To send the bind message to*/
	srcIEEE[0], /*Src IEEE Addr for the binding*/
	srcIEEE[1], /*Src IEEE Addr for the binding*/
	srcIEEE[2], /*Src IEEE Addr for the binding*/
	srcIEEE[3], /*Src IEEE Addr for the binding*/
	srcIEEE[4], /*Src IEEE Addr for the binding*/
	srcIEEE[5], /*Src IEEE Addr for the binding*/
	srcIEEE[6], /*Src IEEE Addr for the binding*/
	srcIEEE[7], /*Src IEEE Addr for the binding*/
	srcEndpoint, /*Src endpoint for the binding*/
	(clusterID & 0x00ff), /*cluster ID to bind*/
	(clusterID & 0xff00) >> 8, /*cluster ID to bind*/
	afAddr64Bit, /*Addr mode of the dst to bind*/
	dstIEEE[0], /*Dst IEEE Addr for the binding*/
	dstIEEE[1], /*Dst IEEE Addr for the binding*/
	dstIEEE[2], /*Dst IEEE Addr for the binding*/
	dstIEEE[3], /*Dst IEEE Addr for the binding*/
	dstIEEE[4], /*Dst IEEE Addr for the binding*/
	dstIEEE[5], /*Dst IEEE Addr for the binding*/
	dstIEEE[6], /*Dst IEEE Addr for the binding*/
	dstIEEE[7], /*Dst IEEE Addr for the binding*/
	dstEndpoint, /*Dst endpoint for the binding*/
	0x00 //FCS - fill in later
			};

	/*printf("zbSocBind: srcNwkAddr=0x%x, srcEndpoint=0x%x, srcIEEE=0x%x:%x:%x:%x:%x:%x:%x:%x, dstEndpoint=0x%x, dstIEEE=0x%x:%x:%x:%x:%x:%x:%x:%x, clusterID:%x\n",
	 srcNwkAddr, srcEndpoint, srcIEEE[0], srcIEEE[1], srcIEEE[2], srcIEEE[3], srcIEEE[4], srcIEEE[5], srcIEEE[6], srcIEEE[7],
	 srcEndpoint, dstIEEE[0], dstIEEE[1], dstIEEE[2], dstIEEE[3], dstIEEE[4], dstIEEE[5], dstIEEE[6], dstIEEE[7], clusterID);*/

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

void zbSocGetInfo(void)
{
	uint8_t cmd[] =
	{ 0xFE, 0, /*RPC payload Len */
	MT_RPC_CMD_SREQ | MT_RPC_SYS_UTIL,
	MT_UTIL_GET_DEVICE_INFO,
	0x00 //FCS - fill in later
	};

	//printf("zbSocGetInfo++ \n");

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocGetState
 *
 * @brief   Send the get state command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocGetState(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
	uint8_t cmd[] =
	{ 0xFE, 13, /*RPC payload Len */
	0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
	0x00, /*MT_APP_MSG  */
	0x0B, /*Application Endpoint */
	(dstAddr & 0x00ff), (dstAddr & 0xff00) >> 8, endpoint, /*Dst EP */
	(ZCL_CLUSTER_ID_GEN_ON_OFF & 0x00ff), (ZCL_CLUSTER_ID_GEN_ON_OFF & 0xff00)
			>> 8, 0x06, //Data Len
			addrMode, 0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
			transSeqNumber++, ZCL_CMD_READ, (ATTRID_ON_OFF & 0x00ff), (ATTRID_ON_OFF
					& 0xff00) >> 8, 0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocGetLevel
 *
 * @brief   Send the get level command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocGetLevel(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
	uint8_t cmd[] =
	{ 0xFE, 13, /*RPC payload Len */
	0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
	0x00, /*MT_APP_MSG  */
	0x0B, /*Application Endpoint */
	(dstAddr & 0x00ff), (dstAddr & 0xff00) >> 8, endpoint, /*Dst EP */
	(ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL & 0x00ff), (ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL
			& 0xff00) >> 8,
			0x06, //Data Len
			addrMode,
			0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
			transSeqNumber++, ZCL_CMD_READ, (ATTRID_LEVEL_CURRENT_LEVEL & 0x00ff),
			(ATTRID_LEVEL_CURRENT_LEVEL & 0xff00) >> 8, 0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocGetHue
 *
 * @brief   Send the get hue command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocGetHue(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
	uint8_t cmd[] =
	{ 0xFE, 13, /*RPC payload Len */
	0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
	0x00, /*MT_APP_MSG  */
	0x0B, /*Application Endpoint */
	(dstAddr & 0x00ff), (dstAddr & 0xff00) >> 8, endpoint, /*Dst EP */
	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
			(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,
			0x06, //Data Len
			addrMode,
			0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
			transSeqNumber++, ZCL_CMD_READ, (ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE
					& 0x00ff), (ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE & 0xff00) >> 8,
			0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocGetSat
 *
 * @brief   Send the get saturation command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocGetSat(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
	uint8_t cmd[] =
	{ 0xFE, 13, /*RPC payload Len */
	0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
	0x00, /*MT_APP_MSG  */
	0x0B, /*Application Endpoint */
	(dstAddr & 0x00ff), (dstAddr & 0xff00) >> 8, endpoint, /*Dst EP */
	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
			(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,
			0x06, //Data Len
			addrMode,
			0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
			transSeqNumber++, ZCL_CMD_READ,
			(ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION & 0x00ff),
			(ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION & 0xff00) >> 8, 0x00 //FCS - fill in later
			};

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocGetModel
 *
 * @brief   Send the get saturation command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocGetModel(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
	uint8_t cmd[] =
	{ 0xFE, 13, /*RPC payload Len */
	0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
	0x00, /*MT_APP_MSG  */
	0x0B, /*Application Endpoint */
	(dstAddr & 0x00ff), (dstAddr & 0xff00) >> 8, endpoint, /*Dst EP */
	(ZCL_CLUSTER_ID_GEN_BASIC & 0x00ff),
			(ZCL_CLUSTER_ID_GEN_BASIC & 0xff00) >> 8,
			0x06, //Data Len
			addrMode,
			0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL foundation command)
			transSeqNumber++, ZCL_CMD_READ,
			(ATTRID_BASIC_MODEL_ID & 0x00ff),
			(ATTRID_BASIC_MODEL_ID & 0xff00) >> 8,
			0x00 //FCS - fill in later
			};

	//printf("zbSocGetModel: dstAddr:%x, endpoint:%x, addrMode:%x\n", dstAddr, endpoint, addrMode);

	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd, sizeof(cmd));
}

/*************************************************************************************************
 * @fn      processRpcSysAppTlInd()
 *
 * @brief  process the TL Indication from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysAppTlInd(uint8_t *TlIndBuff)
{
	epInfo_t epInfo;

	epInfo.nwkAddr = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
	TlIndBuff += 2;
	epInfo.endpoint = *TlIndBuff++;
	epInfo.profileID = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
	TlIndBuff += 2;
	epInfo.deviceID = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
	TlIndBuff += 2;
	epInfo.version = *TlIndBuff++;
	epInfo.status = *TlIndBuff++;

	if (zbSocCb.pfnTlIndicationCb)
	{
		zbSocCb.pfnTlIndicationCb(&epInfo);
	}
}

/*************************************************************************************************
 * @fn      processRpcSysAppZcl()
 *
 * @brief  process the ZCL Rsp from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysAppZcl(uint8_t *zclRspBuff)
{
	uint8_t zclHdrLen = 3;
	uint16_t nwkAddr, clusterID;
	uint8_t endpoint, zclFrameLen, zclFrameFrameControl;

	//printf("processRpcSysAppZcl++\n");

	//This is a ZCL response

	//Index past app EP
	zclRspBuff++;
	nwkAddr = BUILD_UINT16(zclRspBuff[0], zclRspBuff[1]);
	zclRspBuff += 2;

	endpoint = *zclRspBuff++;
	clusterID = BUILD_UINT16(zclRspBuff[0], zclRspBuff[1]);
	zclRspBuff += 2;

	zclFrameLen = *zclRspBuff++;
	zclFrameFrameControl = *zclRspBuff++;
	//is it manufacturer specific
	if (zclFrameFrameControl & (1 << 2))
	{
		//currently not supported shown for reference
		uint16_t ManSpecCode;
		//manu spec code
		ManSpecCode = BUILD_UINT16(zclRspBuff[0], zclRspBuff[1]);
		zclRspBuff += 2;
		//Manufacturer specif commands have 2 extra byte in te header
		zclHdrLen += 2;

		//supress warning
		(void)ManSpecCode;
	}

	//is this a foundation command
	if ((zclFrameFrameControl & 0x3) == 0)
	{
		//printf("processRpcSysAppZcl: Foundation messagex\n");
		processRpcSysAppZclFoundation(zclRspBuff, zclFrameLen, clusterID, nwkAddr,
				endpoint);
	}
}

/*************************************************************************************************
 * @fn      processRpcSysAppZclFoundation()
 *
 * @brief  process the ZCL Rsp from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysAppZclFoundation(uint8_t *zclRspBuff,
		uint8_t zclFrameLen, uint16_t clusterID, uint16_t nwkAddr, uint8_t endpoint)
{
	uint8_t transSeqNum, commandID;

	transSeqNum = *zclRspBuff++;
	commandID = *zclRspBuff++;

	if (commandID == ZCL_CMD_READ_RSP)
	{
		uint16_t attrID;
		uint8_t status;
		uint8_t dataType;

		attrID = BUILD_UINT16(zclRspBuff[0], zclRspBuff[1]);
		zclRspBuff += 2;
		status = *zclRspBuff++;
		//get data type;
		dataType = *zclRspBuff++;

		//printf("processRpcSysAppZclFoundation: clusterID:%x, attrID:%x, dataType=%x\n", clusterID, attrID, dataType);
		if ((clusterID == ZCL_CLUSTER_ID_GEN_BASIC) && (attrID == ATTRID_BASIC_MODEL_ID)
				&& (dataType == ZCL_DATATYPE_CHAR_STR))
		{
			if (zbSocCb.pfnZclGetModelCb)
			{
				zbSocCb.pfnZclGetModelCb(&zclRspBuff[0]);
			}
		}
		else if ((clusterID == ZCL_CLUSTER_ID_GEN_ON_OFF) && (attrID == ATTRID_ON_OFF)
				&& (dataType == ZCL_DATATYPE_BOOLEAN))
		{
			if (zbSocCb.pfnZclGetStateCb)
			{
				uint8_t state = zclRspBuff[0];
				zbSocCb.pfnZclGetStateCb(state, nwkAddr, endpoint);
			}
		}
		else if ((clusterID == ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL)
				&& (attrID == ATTRID_LEVEL_CURRENT_LEVEL)
				&& (dataType == ZCL_DATATYPE_UINT8))
		{
			if (zbSocCb.pfnZclGetLevelCb)
			{
				uint8_t level = zclRspBuff[0];
				zbSocCb.pfnZclGetLevelCb(level, nwkAddr, endpoint);
			}
		}
		else if ((clusterID == ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL)
				&& (attrID == ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE)
				&& (dataType == ZCL_DATATYPE_UINT8))
		{
			if (zbSocCb.pfnZclGetHueCb)
			{
				uint8_t hue = zclRspBuff[0];
				zbSocCb.pfnZclGetHueCb(hue, nwkAddr, endpoint);
			}
		}
		else if ((clusterID == ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL)
				&& (attrID == ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION)
				&& (dataType == ZCL_DATATYPE_UINT8))
		{
			if (zbSocCb.pfnZclGetSatCb)
			{
				uint8_t sat = zclRspBuff[0];
				zbSocCb.pfnZclGetSatCb(sat, nwkAddr, endpoint);
			}
		}
		else
		{
			//unsupported ZCL Read Rsp
			printf("processRpcSysAppZclFoundation: Unsupported ZCL Rsp\n");
		}
	}
	else
	{
		//unsupported ZCL Rsp
		printf("processRpcSysAppZclFoundation: Unsupported ZCL Rsp");
		;
	}

	return;
}

/*************************************************************************************************
 * @fn      processRpcSysZdoEndDeviceAnnceInd
 *
 * @brief   read and process the RPC ZDO message from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
void processRpcSysZdoEndDeviceAnnceInd(uint8_t *EndDeviceAnnceIndBuff)
{
	epInfo_t epInfo =	{ 0 };
	uint8_t i;

	EndDeviceAnnceIndBuff += 2;
	epInfo.nwkAddr =
			BUILD_UINT16(EndDeviceAnnceIndBuff[0], EndDeviceAnnceIndBuff[1]);
	EndDeviceAnnceIndBuff += 2;

	printf("processRpcSysZdoEndDeviceAnnceInd nwkAddr: %x, IEEE Addr: ",
			epInfo.nwkAddr);

	for (i = 0; i < 8; i++)
	{
		epInfo.IEEEAddr[i] = *EndDeviceAnnceIndBuff++;
		printf("%x", epInfo.IEEEAddr[i]);
		if (i < 7) printf(":");
	}
	printf("\n");

	if (zbSocCb.pfnNewDevIndicationCb)
	{
		zbSocCb.pfnNewDevIndicationCb(&epInfo);
	}

}

/*************************************************************************************************
 * @fn      processRpcSysZdoActiveEPRsp
 *
 * @brief   read and process the RPC ZDO message from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
void processRpcSysZdoActiveEPRsp(uint8_t *ActiveEPRspBuff)
{
	uint16_t nwkAddr;
	uint8_t epCount;

	nwkAddr = BUILD_UINT16(ActiveEPRspBuff[0], ActiveEPRspBuff[1]);
	ActiveEPRspBuff += 2;
	epCount = *ActiveEPRspBuff;

	printf("processRpcSysZdoActiveEPRsp nwkAddr: %x, Num Ep's: %d\n", nwkAddr,
			epCount);

	//Can be used to check all Ep's have associated ZdoSimpleDesc.
}

/*************************************************************************************************
 * @fn      processRpcSysZdoSimpleDescRsp
 *
 * @brief   read and process the RPC ZDO message from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
void processRpcSysZdoSimpleDescRsp(uint8_t *SimpleDescRspBuff)
{
	epInfo_t epInfo;
	uint8_t numInputClusters, numOutputClusters, clusterIdx;
	uint16_t  *inClusters, *outClusters;

	SimpleDescRspBuff += 2; //src address
	epInfo.status = *SimpleDescRspBuff++; //status..offset 2
	epInfo.nwkAddr = BUILD_UINT16(SimpleDescRspBuff[0], SimpleDescRspBuff[1]); //network address
	SimpleDescRspBuff += 2; //increment
	//printf("processRpcSysZdoSimpleDescRsp: Length:%x \n",SimpleDescRspBuff[0]  );
	SimpleDescRspBuff += 1;
	epInfo.endpoint = *SimpleDescRspBuff++; //end point
	epInfo.profileID = BUILD_UINT16(SimpleDescRspBuff[0], SimpleDescRspBuff[1]); //profile id
	SimpleDescRspBuff += 2;
	epInfo.deviceID = BUILD_UINT16(SimpleDescRspBuff[0], SimpleDescRspBuff[1]); //device id
	SimpleDescRspBuff += 2;
	epInfo.version = *SimpleDescRspBuff++;
	// epInfo.status = *TlIndBuff++;
	epInfo.deviceName = NULL;

	printf("processRpcSysZdoSimpleDescRsp: nwkAddr:%x endpoint:%x\n",  epInfo.nwkAddr, epInfo.endpoint);

	numInputClusters = *SimpleDescRspBuff++;
	inClusters = (uint16_t*) malloc(2*numInputClusters);
	if(inClusters)
	{
		printf("processRpcSysZdoSimpleDescRsp: inClusters[%d]:\n",  numInputClusters);
		for(clusterIdx = 0; clusterIdx < numInputClusters; clusterIdx++)
		{
			inClusters[clusterIdx] = BUILD_UINT16(SimpleDescRspBuff[0], SimpleDescRspBuff[1]); //profile id
			SimpleDescRspBuff += 2;
			printf("0x%x ", inClusters[clusterIdx]);
		}
	}
	printf("\n");

	numOutputClusters = *SimpleDescRspBuff++;
	outClusters = (uint16_t*) malloc(2*numOutputClusters);
	if(outClusters)
	{
		printf("processRpcSysZdoSimpleDescRsp: outClusters[%d]:\n",  numOutputClusters);
		for(clusterIdx = 0; clusterIdx < numOutputClusters; clusterIdx++)
		{
			outClusters[clusterIdx] = BUILD_UINT16(SimpleDescRspBuff[0], SimpleDescRspBuff[1]); //profile id
			SimpleDescRspBuff += 2;
			printf("0x%x ", outClusters[clusterIdx]);
		}
	}
	printf("\n");

	//if this is the TL endpoint then ignore it
	if( !((numInputClusters == 1) && (numOutputClusters == 1) && (epInfo.profileID) == (0xC05E)) )
	{
		if (zbSocCb.pfnZdoSimpleDescRspCb)
		{
			zbSocCb.pfnZdoSimpleDescRspCb(&epInfo);
		}
	}

	if(inClusters)
	{
		free(inClusters);
	}
	if(outClusters)
	{
		free(outClusters);
	}
}

void processRpcSysZdoLeaveInd(uint8_t *LeaveIndRspBuff)
{
	uint16_t nwkAddr;

	nwkAddr = BUILD_UINT16(LeaveIndRspBuff[0], LeaveIndRspBuff[1]);
	LeaveIndRspBuff += 2;

	printf("processRpcSysZdoLeaveInd nwkAddr: %x\n", nwkAddr);

	if (zbSocCb.pfnZdoLeaveIndCb)
	{
		zbSocCb.pfnZdoLeaveIndCb(nwkAddr);
	}
}

void processRpcUtilGetDevInfoRsp(uint8_t *GetDevInfoRsp)
{
	uint8_t ieeeAddr[8], ieeeIdx, devType, devState, status;
	uint16_t nwkAddr;

	status = *GetDevInfoRsp++;

	if(status == 0)
	{
		for(ieeeIdx = 0; ieeeIdx < 8; ieeeIdx++)
		{
			ieeeAddr[ieeeIdx] = *GetDevInfoRsp++;
		}

		nwkAddr = BUILD_UINT16(GetDevInfoRsp[0], GetDevInfoRsp[1]);
		GetDevInfoRsp += 2;

		devType = *GetDevInfoRsp++;

		devState = *GetDevInfoRsp++;

		//printf("processRpcUtilGetDevInfoRsp: status:%x devState:%x, nwkAddr:%x ieeeIdx:%x:%x:%x:%x:%x:%x:%x:%x\n", status, devState, nwkAddr,
		//		ieeeAddr[7], ieeeAddr[6], ieeeAddr[5], ieeeAddr[4], ieeeAddr[3], ieeeAddr[2], ieeeAddr[1], ieeeAddr[0]);
	}

	if (zbSocCb.pfnUtilGetDevInfoRsp)
	{
		zbSocCb.pfnUtilGetDevInfoRsp(status, nwkAddr, ieeeAddr, devType, devState);
	}
}

/*************************************************************************************************
 * @fn      processRpcSysApp()
 *
 * @brief   read and process the RPC App message from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysApp(uint8_t *rpcBuff)
{
	if (rpcBuff[1] == MT_APP_ZLL_TL_IND)
	{
		processRpcSysAppTlInd(&rpcBuff[2]);
	}

	else if (rpcBuff[1] == MT_APP_RSP)
	{
		processRpcSysAppZcl(&rpcBuff[2]);
	}
	else if (rpcBuff[1] == 0)
	{
		if (rpcBuff[2] == 0)
		{
			//printf("processRpcSysApp: Command Received Successfully\n\n");
		}
		else
		{
			printf("processRpcSysApp: Command Error\n\n");
		}
	}
	else
	{
		printf("processRpcSysApp: Unsupported MT App Msg\n");
	}

	return;
}

/*************************************************************************************************
 * @fn      processRpcSysZdo()
 *
 * @brief   read and process the RPC ZDO messages from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
void processRpcSysZdo(uint8_t *rpcBuff)
{
	if (rpcBuff[1] == MT_ZDO_END_DEVICE_ANNCE_IND)
	{
		processRpcSysZdoEndDeviceAnnceInd(&rpcBuff[2]);
	}
	else if (rpcBuff[1] == MT_ZDO_ACTIVE_EP_RSP)
	{
		processRpcSysZdoActiveEPRsp(&rpcBuff[2]);
	}
	else if (rpcBuff[1] == MT_ZDO_SIMPLE_DESC_RSP)
	{
		processRpcSysZdoSimpleDescRsp(&rpcBuff[2]);
	}
	else if (rpcBuff[1] == MT_ZDO_LEAVE_IND)
	{
		processRpcSysZdoLeaveInd(&rpcBuff[2]);
	}
	else
	{
		//printf("processRpcSysZdo: Unsupported MT ZDO Msg\n");
	}

	return;

}

/*************************************************************************************************
 * @fn      processRpcSysUtil()
 *
 * @brief   read and process the RPC UTIL messages from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
void processRpcSysUtil(uint8_t *rpcBuff)
{
	if (rpcBuff[1] == MT_UTIL_GET_DEVICE_INFO)
	{
		processRpcUtilGetDevInfoRsp(&rpcBuff[2]);
	}
	else
	{
		printf("processRpcSysUtil: Unsupported MT UTIL Msg\n");
	}

	return;

}

/* @fn      processRpcSysDbg()
 *
 * @brief   read and process the RPC debug message from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysDbg(uint8_t *rpcBuff)
{
	if (rpcBuff[1] == MT_DEBUG_MSG)
	{
		//we got a debug string
		printf("lcd_debug message from zll controller: %s\n",
				(char*) &(rpcBuff[2]));
	}
	else if (rpcBuff[1] == 0)
	{
		if (rpcBuff[2] == 0)
		{
			//printf("processRpcSysDbg: Command Received Successfully\n\n");
		}
		else
		{
			printf("processRpcSysDbg: Command Error\n\n");
		}
	}
	else
	{
		printf("processRpcSysDbg: Unsupported MT App Msg\n");
	}
}

/*************************************************************************************************
 * @fn      zbSocProcessRpc()
 *
 * @brief   read and process the RPC from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
void zbSocProcessRpc(void)
{
	uint8_t rpcLen, bytesRead, sofByte, *rpcBuff, rpcBuffIdx;
	static uint8_t retryAttempts = 0;

	//read first byte and check it is a SOF
	read(serialPortFd, &sofByte, 1);
	if (sofByte == MT_RPC_SOF)
	{
		retryAttempts = 0;

		//read len
		bytesRead = read(serialPortFd, &rpcLen, 1);

		if (bytesRead == 1)
		{
			//allocating RPC payload (+ cmd0, cmd1 and fcs)
			rpcLen += 3;

			rpcBuff = malloc(rpcLen);

			//non blocking read, so we need to wait for the rpc to be read
			rpcBuffIdx = 0;
			while (rpcLen > 0)
			{
				//read rpc
				bytesRead = read(serialPortFd, &(rpcBuff[rpcBuffIdx]), rpcLen);

				//check for error
				if (bytesRead > rpcLen)
				{
					//there was an error
					printf("zbSocProcessRpc: read of %d bytes failed - %s\n", rpcLen,
							strerror(errno));

					if (retryAttempts++ < 5)
					{
						//sleep for 10ms
						usleep(10000);
						//try again
						bytesRead = 0;
					}
					else
					{
						//something went wrong.
						printf("zbSocProcessRpc: failed\n");
						free(rpcBuff);
						return;
					}
				}

				rpcLen -= bytesRead;
				rpcBuffIdx += bytesRead;
			}

			//printf("zbSocProcessRpc: Processing CMD0:%x, CMD1:%x\n", rpcBuff[0],
			//		rpcBuff[1]);
			//Read CMD0
			switch (rpcBuff[0] & MT_RPC_SUBSYSTEM_MASK)
			{
				case MT_RPC_SYS_DBG:
				{
					processRpcSysDbg(rpcBuff);
					break;
				}
				case MT_RPC_SYS_APP:
				{
					processRpcSysApp(rpcBuff);
					break;
				}

				case MT_RPC_SYS_ZDO:
				{
					processRpcSysZdo(rpcBuff);
					break;
				}

				case MT_RPC_SYS_UTIL:
				{
					processRpcSysUtil(rpcBuff);
					break;
				}

				default:
				{
					//printf("zbSocProcessRpc: CMD0:%x, CMD1:%x, not handled\n", rpcBuff[0] , rpcBuff[1])
					break;
				}
			}

			free(rpcBuff);
		}
		else
		{
			printf("zbSocProcessRpc: No valid Start Of Frame found\n");
		}
	}

	return;
}


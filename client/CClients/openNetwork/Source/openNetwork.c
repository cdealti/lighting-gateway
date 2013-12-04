/**************************************************************************************************
 Filename:       addGroup.c
 Revised:        $Date: 2012-03-21 17:37:33 -0700 (Wed, 21 Mar 2012) $
 Revision:       $Revision: 246 $

 Description:    This file contains an example client for the zbGateway sever

 Copyright (C) {2012} Texas Instruments Incorporated - http://www.ti.com/


 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.

 Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the
 distribution.

 Neither the name of Texas Instruments Incorporated nor the names of
 its contributors may be used to endorse or promote products derived
 from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 **************************************************************************************************/
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <poll.h>

#include "socket_client.h"
#include "interface_srpcserver.h"
#include "hal_defs.h"

int keyFd;
uint8 waitRsp = 1;

#define CONSOLEDEVICE "/dev/console"

void sendOpenNetwork(uint8_t duration);
void socketClientCb(msgData_t *msg);

typedef uint8_t (*srpcProcessMsg_t)(uint8_t *msg);

srpcProcessMsg_t srpcProcessIncoming[] =
{
		NULL,							//0
		NULL,							//SRPC_NEW_ZLL_DEVICE     0x0001
		NULL, 						//SRPC_RESERVED_1		      0x0002
		NULL, 						//SRPC_RESERVED_2	        0x0003
		NULL, 						//SRPC_RESERVED_3         0x0004
		NULL, 						//SRPC_RESERVED_4         0x0005
		NULL, 						//SRPC_RESERVED_5         0x0006
		NULL, 						//SRPC_GET_DEV_STATE_RSP  0x0007
		NULL, 						//SRPC_GET_DEV_LEVEL_RSP  0x0008
		NULL, 						//SRPC_GET_DEV_HUE_RSP    0x0009
		NULL, 						//SRPC_GET_DEV_SAT_RSP    0x000a
		NULL, //SRPC_ADD_GROUP_RSP      0x000b
		NULL, 						//SRPC_GET_GROUP_RSP      0x000c
		NULL, 						//SRPC_ADD_SCENE_RSP      0x000d
		NULL, 						//SRPC_GET_SCENE_RSP      0x000e
};

void keyInit(void)
{
	keyFd = open(CONSOLEDEVICE, O_RDONLY | O_NOCTTY | O_NONBLOCK);
	tcflush(keyFd, TCIFLUSH);
}

int main(int argc, char *argv[])
{
	uint8_t duration;

	if (argc < 2)
	{
		printf(
				"Expected 1 and got %d params Usage: %s <duration> <addr mode> <ep> <group name>\n",
				argc, argv[0]);
		printf(
				"Example - open network for 60s: %s 60\n",
				argv[0]);
		exit(0);
	}
	else
	{
		uint32_t tmpInt;

		sscanf(argv[1], "%d", &tmpInt);
		duration = (uint8_t) tmpInt;
	}

	socketClientInit("127.0.0.1:11235", socketClientCb);

	sendOpenNetwork(duration);

	sleep(1);

	socketClientClose();

	return 0;
}

/*********************************************************************
 * @fn          socketClientCb
 *
 * @brief
 *
 * @param
 *
 * @return
 */
void socketClientCb(msgData_t *msg)
{
	srpcProcessMsg_t func;

	printf("socketClientCb: cmdId:%x\n",(msg->cmdId));

	func = srpcProcessIncoming[(msg->cmdId)];
	if (func)
	{
		(*func)(msg->pData);
	}
}

void sendOpenNetwork(uint8_t duration)
{
	msgData_t msg;
	uint8_t* pRpcCmd = msg.pData;

	printf("sendOpenNetwork++: duration=%d\n", duration);

	msg.cmdId = SRPC_PERMIT_JOIN;
	msg.len = 5;

	//duration
	*pRpcCmd++ = duration;
  *pRpcCmd++ = JOIN_AUTH_NUM & 0xFF;
  *pRpcCmd++ = (JOIN_AUTH_NUM & 0xFF00) >> 8;

	socketClientSendData(&msg);
}

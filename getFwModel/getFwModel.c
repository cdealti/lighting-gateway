/**************************************************************************************************
 * Filename:       fwGetModel.c
 * Description:    This file contains the interface to the UART.
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>

#include "zbSocCmd.h"

uint8_t tlIndicationCb(epInfo_t *epInfo);
uint8_t utilGetDevInfoRspCb(uint8_t status, uint16_t nwkAddr,
		uint8_t ieeeAddr[8], uint8_t devType, uint8_t devState);
uint8_t zclGetModelCb(uint8_t *ModelId);

static zbSocCallbacks_t zbSocCbs =
{ tlIndicationCb, // pfnTlIndicationCb - TouchLink Indication callback
		NULL, // pfnNewDevIndicationCb - New Device Indication callback
		NULL, //pfnZclGetStateCb - ZCL response callback for get State
		NULL, //pfnZclGetLevelCb_t - ZCL response callback for get Level
		NULL, // pfnZclGetHueCb - ZCL response callback for get Hue
		NULL, //pfnZclGetSatCb - ZCL response callback for get Sat
		NULL, //pfnzdoSimpleDescRspCb - ZDO simple desc rsp
		NULL, // pfnZdoLeaveIndCb - ZDO Leave indication
		utilGetDevInfoRspCb, //pfnUtilGetDevInfoRspCb
		zclGetModelCb //pfnZclGetModelCb
		};

void usage(char* exeName)
{
	printf("Usage: ./%s <port>\n", exeName);
	printf("Eample: ./%s /dev/ttyACM0\n", exeName);
}

int main(int argc, char* argv[])
{	
	int retval = 0;
	int zbSoc_fd;

	//printf("%s -- %s %s\n", argv[0], __DATE__, __TIME__);


	// accept only 1
	if (argc != 2)
	{
		usage(argv[0]);
		//printf("attempting to use /dev/ttyACM0\n");
		zbSoc_fd = zbSocOpen("/dev/ttyO4");
	}
	else
	{
		zbSoc_fd = zbSocOpen(argv[1]);
	}

	if (zbSoc_fd == -1)
	{
		exit(-1);
	}

	zbSocRegisterCallbacks(zbSocCbs);

	zbSocGetInfo();

	while (1)
	{
		struct pollfd pollFd;

		//set the zllSoC serial port FD in the poll file descriptors
		pollFd.fd = zbSoc_fd;
		pollFd.events = POLLIN;

		//printf("%s: waiting for poll()\n", argv[1]);

		poll(&pollFd, 1, 500);

		//printf("%s: got poll()\n", argv[1]);

		//did the poll unblock because of the zllSoC serial?
		if (pollFd.revents)
		{
			//printf("%s: Message from ZB SoC\n", argv[1]);
			zbSocProcessRpc();
		}
                else
                {
                        //printf("timeout\n");
			static int timeoutCnt = 0;
			timeoutCnt++;
			if(timeoutCnt > 5)
			{
				printf("Unknown model\n");
				zbSocClose();
				exit(0);
			}
			else if(timeoutCnt > 3)
			{
				//maybe MT_UTIL is not define, assume coord (nwkAddr=0x0) and try to get model ID
				zbSocGetModel(0x0000, 0xff, afAddr16Bit);
			}
			else
			{
				zbSocGetInfo();
			}
                }
	}

	return retval;
}

uint8_t tlIndicationCb(epInfo_t *epInfo)
{

return 0;
}

uint8_t utilGetDevInfoRspCb(uint8_t status, uint16_t nwkAddr,
	uint8_t ieeeAddr[8], uint8_t devType, uint8_t devState)
{
	//printf("processRpcUtilGetDevInfoRsp: status:%x devState:%x, nwkAddr:%x ieeeIdx:%x:%x:%x:%x:%x:%x:%x:%x\n", status, devState, nwkAddr,
//ieeeAddr[7], ieeeAddr[6], ieeeAddr[5], ieeeAddr[4], ieeeAddr[3], ieeeAddr[2], ieeeAddr[1], ieeeAddr[0]);

	zbSocGetModel(nwkAddr, 0xff, afAddr16Bit);

	return 0;
}

uint8_t zclGetModelCb(uint8_t *ModelId)
{
	uint8_t strIdx;
	printf("Model ID:");

	for(strIdx = 0; strIdx < ModelId[0]; strIdx++)
	{
		printf("%c", ModelId[strIdx+1]);
	}
	printf("\n");

	zbSocClose();
	exit(0);

	return 0;
}


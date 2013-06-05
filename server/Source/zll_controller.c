/**************************************************************************************************
 * Filename:       zll_controller.c
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

#include "zllSocCmd.h"
#include "interface_devicelist.h"
#include "interface_grouplist.h"
#include "interface_scenelist.h"

uint8_t pfnTlIndicationCb(epInfo_t *epInfo);
uint8_t pfnNewDevIndicationCb(epInfo_t *epInfo);
uint8_t zclGetStateCb(uint8_t state, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclGetLevelCb(uint8_t level, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclGetHueCb(uint8_t hue, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclGetSatCb(uint8_t sat, uint16_t nwkAddr, uint8_t endpoint);

static zllSocCallbacks_t zllSocCbs =
{
  pfnTlIndicationCb,      // pfnTlIndicationCb - TouchLink Indication callback  
  pfnNewDevIndicationCb,  // pfnNewDevIndicationCb - New Device Indication callback
  zclGetStateCb,          //pfnZclGetStateCb - ZCL response callback for get State
  zclGetLevelCb,          //pfnZclGetLevelCb_t - ZCL response callback for get Level
  zclGetHueCb,            // pfnZclGetHueCb - ZCL response callback for get Hue
  zclGetSatCb,            //pfnZclGetSatCb - ZCL response callback for get Sat
};

#include "interface_srpcserver.h"
#include "socket_server.h"

void usage( char* exeName )
{
    printf("Usage: ./%s <port>\n", exeName);
    printf("Eample: ./%s /dev/ttyACM0\n", exeName);
}

int main(int argc, char* argv[])
{
  int retval = 0;
  int zllSoc_fd;    
 
  printf("%s -- %s %s\n", argv[0], __DATE__, __TIME__ );
 
  // accept only 1
  if( argc != 2 )
  {
    usage(argv[0]);
    printf("attempting to use /dev/ttyACM0\n");
    zllSoc_fd = zllSocOpen( "/dev/ttyACM0" );        
  }
  else
  {
    zllSoc_fd = zllSocOpen( argv[1] );       
  }
  
  if( zllSoc_fd == -1 )
  {
    exit(-1);
  }

  //printf("zllMain: restoring device, group and scene lists\n");
  devListRestorDevices();
  groupListRestorGroups();  
  sceneListRestorScenes();
  
  zllSocRegisterCallbacks( zllSocCbs );    
  SRPC_Init();
  
  while(1)
  {          
    int numClientFds = socketSeverGetNumClients(); 
    
	  //poll on client socket fd's and the ZllSoC serial port for any activity
		if(numClientFds)
		{
		  int pollFdIdx;  		   
		  int *client_fds = malloc(  numClientFds * sizeof( int ) );
		  //socket client FD's + zllSoC serial port FD
		  struct pollfd *pollFds = malloc(  ((numClientFds + 1) * sizeof( struct pollfd )) );
		  
		  if(client_fds && pollFds)	
		  {
		    //set the zllSoC serial port FD in the poll file descriptors
		    pollFds[0].fd = zllSoc_fd;
  			pollFds[0].events = POLLIN;
  			
		    //Set the socket file descriptors  		    
	  	  socketSeverGetClientFds(client_fds, numClientFds);  			    	  	    	  	 
		  	for(pollFdIdx=0; pollFdIdx < numClientFds; pollFdIdx++)
  	  	{
  			  pollFds[pollFdIdx+1].fd = client_fds[pollFdIdx];
  			  pollFds[pollFdIdx+1].events = POLLIN | POLLRDHUP;
  			  //printf("zllMain: adding fd %d to poll()\n", pollFds[pollFdIdx].fd); 	  				
  		  }	

        printf("zllMain: waiting for poll()\n");

        poll(pollFds, (numClientFds+1), -1); 
        
        //printf("zllMain: got poll()\n");
        
        //did the poll unblock because of the zllSoC serial?
        if(pollFds[0].revents)
        {
          printf("Message from ZLL SoC\n");
          zllSocProcessRpc();
        }
        //did the poll unblock because of activity on the socket interface?
        for(pollFdIdx=1; pollFdIdx < (numClientFds+1); pollFdIdx++)
        {
          if ( (pollFds[pollFdIdx].revents) )
          {
            printf("Message from Socket Sever\n");
            socketSeverPoll(pollFds[pollFdIdx].fd, pollFds[pollFdIdx].revents);
          }
        }          
    		  	  
  		  free( client_fds );	  
  		  free( pollFds );	  		
		  }
		}  		           
  }    

  return retval;
}

uint8_t pfnTlIndicationCb(epInfo_t *epInfo)
{
  devListAddDevice(epInfo);
  RSPC_SendEpInfo(epInfo);
  return 0;  
}

uint8_t pfnNewDevIndicationCb(epInfo_t *epInfo)
{
  devListAddDevice(epInfo);
  RSPC_SendEpInfo(epInfo);
  return 0;  
}

uint8_t zclGetStateCb(uint8_t state, uint16_t nwkAddr, uint8_t endpoint)
{
  RPCS_ZLL_CallBack_getStateRsp(state, nwkAddr, endpoint, 0);
  return 0;  
}

uint8_t zclGetLevelCb(uint8_t level, uint16_t nwkAddr, uint8_t endpoint)
{
  RPCS_ZLL_CallBack_getLevelRsp(level, nwkAddr, endpoint, 0);
  return 0;  
}

uint8_t zclGetHueCb(uint8_t hue, uint16_t nwkAddr, uint8_t endpoint)
{
  RPCS_ZLL_CallBack_getHueRsp(hue, nwkAddr, endpoint, 0);
  return 0;  
}

uint8_t zclGetSatCb(uint8_t sat, uint16_t nwkAddr, uint8_t endpoint)
{
  RPCS_ZLL_CallBack_getSatRsp(sat, nwkAddr, endpoint, 0);
  return 0;  
}


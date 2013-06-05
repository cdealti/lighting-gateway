/**************************************************************************************************
 * Filename:       interface_grouplist.h
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

#ifndef INTERFACE_GROUPLIST_H
#define INTERFACE_GROUPLIST_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include <stdint.h>

typedef struct
{
  uint16_t groupId;
  char *groupNameStr;
}groupListItem_t;

/*
 * groupListAddGroup - create a group and add a rec fto the list.
 */
uint16_t groupListAddGroup( char *groupNameStr );

/*
 * groupListAddDeviceToGroup - Add a device to a group.
 */
void groupListAddDeviceToGroup( char *groupNameStr, uint16_t nwkAddr );

/*
 * groupListGetNextGroup - Return the next group in the list..
 */
groupListItem_t* groupListGetNextGroup( char *groupNameStr );

/*
 * groupListRestorGroups - Restore Group List from file.
 */
void groupListRestorGroups( void );

#ifdef __cplusplus
}
#endif

#endif /* INTERFACE_DEVICELIST_H */

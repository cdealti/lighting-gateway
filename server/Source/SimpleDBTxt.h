/**************************************************************************************************
 * Filename:       SimpleDBTxt.h
 * Description:    This is a specific implemntation of a text-based db system, based on the SimpleDB module.
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
#ifndef SIMPLE_DB_TXT_H
#define SIMPLE_DB_TXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "SimpleDB.h"

#define SDBT_BAD_FORMAT_CHARACTER '@'
#define SDBT_PENDING_COMMENT_FORMAT_CHARACTER '?'
#define SDBT_DELETED_LINE_CHARACTER ';'

uint32_t sdbtGetRecordSize(void * record);
bool sdbtCheckDeleted(void * record);
bool sdbtCheckIgnored(void * record);
void sdbtMarkDeleted(void * record);
uint32_t sdbtGetRecordCount(db_descriptor * db);
bool sdbtErrorComment(db_descriptor * db, char * record);
void sdbtMarkError(db_descriptor * db, char * record, parsingResult_t * parsingResult);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_DB_TXT_H */

